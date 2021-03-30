const sleep = (n) => new Promise((resolve) => setTimeout(n, resolve));

/*******************/
/* ESP Message Bus */
/*******************/

class ESPMessageBus {
  #connection;
  #wsURL;
  /**
   * @type {WebSocket}
   */
  #ws;
  constructor({ wsURL = `ws://${window.location.host}`, protocols } = {}) {
    this.#wsURL = wsURL;
    this.connect();
  }
  connect() {
    if (!this.#ws || this.#ws.readyState >= 2) {
      // WS is Closing or Closed
      console.log("EMB: Starting Conenction");
      this.#ws = new WebSocket(this.#wsURL);
      this.#ws.addEventListener("message", (e) => this.message(e.data));

      this.#connection = new Promise((resolve, reject) => {
        const connectionRetry = async (event) => {
          console.log("EMB: Unable to open Connection");
          console.debug(event);
          resolve(await this.connect());
        };
        this.#ws.addEventListener("error", connectionRetry);
        this.#ws.addEventListener("close", connectionRetry);
        this.#ws.addEventListener("open", () => {
          console.log("EMB: Connection Opened");
          this.#ws.removeEventListener("error", connectionRetry);
          this.#ws.addEventListener("error", console.log);
          // this.#ws.addEventListener("error", (e) => this.connect(e.data));
          resolve(this.#ws);
        });
      });
    }
  }
  async send(msg) {
    console.log("Sending:", msg);
    await this.#connection;
    this.#ws.send(msg);
  }
  async sendJSON(msg) {
    console.log("Sending:", msg);
    await this.#connection;
    this.#ws.send(JSON.stringify(msg));
  }

  message(msg) {
    msg = msg.trim().replaceAll("\0", "");
    try {
      const parsedMsg = JSON.parse(msg);
      this.handleCommand(parsedMsg);
    } catch {
      this.old_message(msg);
    }
  }

  /**
   *
   * @param {{
   *   playstate: "mute" | "pause" | "unmute" | "play",
   *   volume: number,
   *   station_select: number,
   *   station_update: boolean,
   *   meta: {
   *     artist: string,
   *     title: string
   *   }
   * }} msg
   */
  handleCommand(msg) {
    switch (msg.playstate) {
      case "mute":
      case "pause":
        playstate(false);
        break;
      case "unmute":
      case "play":
        playstate(true);
        break;
    }
    if ("volume" in msg) {
      volume_range.value = msg.volume;
    }
    if ("station_select" in msg) {
      station_select.selectedIndex = msg.station_select - 1;
    }
    if ("station_update" in msg) {
      loadStations();
    }
    if ("meta" in msg) {
      setMetadata(msg.meta);
    }
  }

  /**
   * Takes commands via Websocket in the form of <command>[=<argument>]:
   *    x playstate=
   *        x mute | pause -> playback paused
   *        x unmute | play -> playback playing
   *    x volume=<integer_value> -> Loudness to play
   *    x station_select=<station_id> -> 1 indexed station to select from ini
   *    < station_update -> Refresh Station List
   *    < meta_playing=<artist>@<title> -> Metadata for currently playing track
   *
   * Legend:
   *    < -> Incoming ESP to Website
   *    > -> Outgoing Website to ESP
   *    x -> Bidirectional ESP to Website and Website to ESP
   */
  old_message(msg) {
    console.log("Recieved:", msg);
    const commands = msg.split(";");
    for (const command of commands) {
      const [cmdName, ...argParts] = command.split("=");
      const cmdArgs = argParts.join("=");
      this.old_handleCommand(cmdName, cmdArgs);
    }
  }

  old_handleCommand(cmd, args) {
    console.log("Hadnling Command", {
      cmd,
      args,
    });
    switch (cmd) {
      case "playstate":
        console.log("CMD Playstate");
        switch (args) {
          case "mute":
          case "pause":
            playstate(false);
            break;
          case "unmute":
          case "play":
            playstate(true);
            break;
        }
        break;
      case "volume":
        console.log("CMD Volume");
        volume_range.value = args;
        break;
      case "station_select":
        console.log("CMD Station Select");
        station_select.selectedIndex = args - 1;
        break;
      case "station_update":
        console.log("CMD Station Update");
        loadStations();
        break;
      case "meta_playing":
        console.log("CMD Meta Playing");
        const [artist, title] = args.split("@");
        setMetadata({
          artist,
          title,
        });
        break;
    }
  }
}

const emb = new ESPMessageBus();

// for frontend testing
// const emb = new ESPMessageBus({ wsURL: "ws://192.168.178.148" });
window.emb = emb; // do this for easy debugging

/****************/
/* Album Art    */
/* and Theeming */
/****************/
const imageToConicalColors = (img, steps = 4) => {
  const CANVAS_SIZE = 64;
  const canvas = document.createElement("canvas");
  canvas.width = CANVAS_SIZE;
  canvas.height = CANVAS_SIZE;
  const ctx = canvas.getContext("2d");
  ctx.drawImage(img, 0, 0, CANVAS_SIZE, CANVAS_SIZE);
  const colors = [];
  for (let i = 0; i < steps; i++) {
    const angle = (360 / steps) * i;
    const angleRad = (angle * Math.PI) / 180;
    let dx = 0;
    let dy = 0;
    if (angle == 0) {
      dx = 0;
      dy = -CANVAS_SIZE / 2;
    } else if (angle == 90) {
      dx = CANVAS_SIZE / 2;
      dy = 0;
    } else if (angle == 180) {
      dx = 0;
      dy = CANVAS_SIZE / 2;
    } else if (angle == 270) {
      dx = -CANVAS_SIZE / 2;
      dy = 0;
    } else if (angle <= 45) {
      // collission top right
      dy = -CANVAS_SIZE / 2;
      dx = -dy * Math.tan(angleRad);
    } else if (angle <= 135) {
      // collission right
      dx = CANVAS_SIZE / 2;
      dy = -dx / Math.tan(angleRad);
    } else if (angle <= 225) {
      // collission bottom
      dy = CANVAS_SIZE / 2;
      dx = -dy * Math.tan(angleRad);
    } else if (angle <= 315) {
      // collission left
      dx = -CANVAS_SIZE / 2;
      dy = -dx / Math.tan(angleRad);
    } else {
      // collision top left
      dy = -CANVAS_SIZE / 2;
      dx = -dy * Math.tan(angleRad);
    }
    const x = Math.max(0, Math.min(CANVAS_SIZE - 1, CANVAS_SIZE / 2 + dx));
    const y = Math.max(0, Math.min(CANVAS_SIZE - 1, CANVAS_SIZE / 2 + dy));
    const pixels = ctx.getImageData(Math.floor(x), Math.floor(y), 1, 1);
    colors.push(pixels.data);
  }
  colors.push(colors[0]);
  return colors;
};

const getConicGradientString = (img, colorStops) => {
  const colors = imageToConicalColors(img, colorStops);
  const colorStopStrings = colors.map((c) => `rgba(${c.join(", ")})`);
  return `conic-gradient(${colorStopStrings.join(", ")})`;
};

const rgbToHsl = (r, g, b) => {
  (r /= 255), (g /= 255), (b /= 255);

  var max = Math.max(r, g, b),
    min = Math.min(r, g, b);
  var h,
    s,
    l = (max + min) / 2;

  if (max == min) {
    h = s = 0; // achromatic
  } else {
    var d = max - min;
    s = l > 0.5 ? d / (2 - max - min) : d / (max + min);

    switch (max) {
      case r:
        h = (g - b) / d + (g < b ? 6 : 0);
        break;
      case g:
        h = (b - r) / d + 2;
        break;
      case b:
        h = (r - g) / d + 4;
        break;
    }

    h /= 6;
  }

  return [h, s, l];
};

const getMainColor = (img) => {
  const SAMPLE_RATIO = 0.33;
  const canvas = document.createElement("canvas");
  canvas.width = 100;
  canvas.height = 100;
  const ctx = canvas.getContext("2d");
  const sx = img.naturalWidth * (0.5 - SAMPLE_RATIO / 2);
  const sy = img.naturalHeight * (0.5 - SAMPLE_RATIO / 2);
  const sw = img.naturalWidth * SAMPLE_RATIO;
  const sh = img.naturalHeight * SAMPLE_RATIO;
  ctx.drawImage(img, sx, sy, sw, sh, 0, 0, 100, 100);
  const color = ctx.getImageData(0, 0, 1, 1).data;
  const hsl = rgbToHsl(...color);
  hsl[1] = 1;
  hsl[2] = 0.4;
  return hsl;
};

const img = document.querySelector("#meta_cover");
img.addEventListener("load", () => {
  img.style.background = getConicGradientString(img, 8);
  document.documentElement.style.setProperty(
    "--primary-color-hue",
    getMainColor(img)[0] * 360
  );
});

img.addEventListener("error", () => {
  if (img.getAttribute("src")) {
    img.src = "fallbackCover.jpg";
  }
});

/*******************/
/* Station Buttons */
/*******************/
const onStationChange = () => {
  emb.send(`station_select=${station_select.value}`);
  // emb.sendJSON({ station_select: parseInt(station_select.value) });
};

const station_select = document.getElementById("station_select");
station_select.addEventListener("input", onStationChange);

document.getElementById("next").addEventListener("click", () => {
  station_select.selectedIndex =
    (station_select.selectedIndex + station_select.length + 1) %
    station_select.length;
  onStationChange();
});
document.getElementById("back").addEventListener("click", () => {
  station_select.selectedIndex =
    (station_select.selectedIndex + station_select.length - 1) %
    station_select.length;
  onStationChange();
});

/******************/
/* Volume Buttons */
/******************/
const onVolumeChange = () => {
  emb.send(`volume=${volume_range.value}`);
  // emb.sendJSON({ volume: parseInt(volume_range.value) });
};
const volume_range = document.getElementById("volume_range");
volume_range.addEventListener("input", onVolumeChange);
document.getElementById("volume_up").addEventListener("click", () => {
  volume_range.value = parseInt(volume_range.value) + 1;
  onVolumeChange();
});
document.getElementById("volume_down").addEventListener("click", () => {
  volume_range.value = parseInt(volume_range.value) - 1;
  onVolumeChange();
});

const pause_play = document.getElementById("pause_play");
pause_play.addEventListener("click", () => {
  const play = pause_play.innerText != "pause";
  playstate(play);
  emb.send(`playstate=${play ? "play" : "pause"}`);
  // emb.sendJSON({ playstate: play ? "play" : "pause" });
});
const playstate = (playing) => {
  pause_play.innerText = playing ? "pause" : "play_arrow";
};

/****************/
/* Load Artwork */
/****************/
const getBMIDs = async (title, artist, limit = 5) => {
  const safeArtist = encodeURIComponent(artist);
  const safeTitle = encodeURIComponent(title);
  const resp = await fetch(
    `http://musicbrainz.org/ws/2/release/?fmt=json&limit=${limit}&query=artist:${safeArtist} AND release:${safeTitle}`
  );
  const releases = await resp.json();
  const bmids = releases.releases.map((r) => r.id);
  return bmids;
};

const getBMID = async (title, artist) => {
  const bmid = await getBMIDs(title, artist, 1)[0];
};

const getTrackCoverURL = async (title, artist) => {
  if (title === "" || artist == "") {
    return "";
  }
  const bmids = await getBMIDs(title, artist, 5);
  for (const bmid of bmids) {
    const res = await fetch(
      `http://coverartarchive.org/release/${bmid}/front-250.jpg`
    );
    if (res.ok) {
      return `http://coverartarchive.org/release/${bmid}/front-250.jpg`;
    }
  }
  return "fallbackCover.jpg";
};

const metaCover = document.getElementById("meta_cover");

const updateTrackCover = async (title, artist) => {
  metaCover.src = "";
  metaCover.src = await getTrackCoverURL(title, artist);
  metaCover.alt = `Coverart of ${title} from ${artist}`;
};

/*******************/
/* Outside Control */
/*******************/
const meta_artist = document.getElementById("meta_artist");
const meta_title = document.getElementById("meta_title");

const setMetadata = ({ artist, title }) => {
  meta_artist.innerText = artist;
  meta_title.innerText = title;
  updateTrackCover(title, artist);
};

const loadStations = async () => {
  const resp = await fetch("./setup.ini");
  const ini = await resp.text();
  const stations = ini.match(/RadioName = .*/g).map((s) => s.substr(12));
  station_select.options.length = 0;
  for (let i = 0; i < stations.length; i++) {
    station_select.options[station_select.options.length] = new Option(
      stations[i],
      i + 1
    );
  }
};

const new_loadStations = async () => {
  const resp = await fetch("./setup.json");
  const config = await resp.json();
  station_select.options.length = 0;
  for (let i = 0; i < config.stations.length; i++) {
    station_select.options[station_select.options.length] = new Option(
      config.stations[i].name,
      i + 1
    );
  }
};

loadStations();
