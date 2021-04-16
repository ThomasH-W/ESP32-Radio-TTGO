const configTextarea = document.querySelector("#config");

const stations = [];

const loadConfig = async () => {
  const resp = await fetch("./setup.ini");
  const ini = await resp.text();
  parseStations(ini);
  updateStationsTable();
};

const resetStations = () => {
  stations.length = 0;
};

const parseStations = (text) => {
  resetStations();
  for (const line of text.split("\n")) {
    const [key, value] = line.split("=").map((s) => s.trim());
    const lastStation = stations[stations.length - 1];
    switch (key) {
      case "RadioName":
        stations.push({
          RadioName: value,
          RadioURL: "",
          RadioTitleSeperator: ":",
          RadioTitleFirst: "Artist",
        });
        break;
      case "RadioURL":
        lastStation.RadioURL = value;
        break;
      case "RadioTitleSeperator":
        lastStation.RadioTitleSeperator = value;
        break;
      case "RadioTitleFirst":
        lastStation.RadioTitleFirst = value;
        break;
    }
  }
};

const serializeStations = () => {
  return (
    `# ESP32 Radio / Thomas Hoeser

# Radio Station - define max 10 stations
` +
    stations.reduce(
      (prev, curr, id) =>
        prev +
        `
# ${id + 1}
RadioName = ${curr.RadioName}
RadioURL = ${curr.RadioURL}
RadioTitleSeperator = ${curr.RadioTitleSeperator}
RadioTitleFirst = ${curr.RadioTitleFirst}
`,
      ""
    )
  );
};

const switchStations = (i, j) => {
  const tmp = stations[i];
  stations[i] = stations[j];
  stations[j] = tmp;
  updateStationsTable();
};

const deleteStation = (i) => {
  stations.splice(i, 1);
  updateStationsTable();
};

const addEmptyStation = (i) => {
  stations.push({
    RadioName: "",
    RadioURL: "",
    RadioTitleSeperator: ":",
    RadioTitleFirst: "Artist",
  });
  updateStationsTable();
};

const stations_settings = document.querySelector("#stations_settings");

const updateStationsTable = () => {
  stations_settings.innerHTML = "";
  for (let i = 0; i < stations.length; i++) {
    const station = stations[i];
    const newStationSettings = document.createElement("div");
    newStationSettings.classList.add("station_settings");
    newStationSettings.innerHTML = `
  <button class="material-icons id_up" ${
    i == 0 ? "disabled" : ""
  }>arrow_upward</button>
  <button class="material-icons id_down" ${
    i == stations.length - 1 ? "disabled" : ""
  }>arrow_downward</button>
  <button class="material-icons delete">delete</button>
  <input type="text" placeholder="Station Name" class="RadioName" size="1" value="${station.RadioName}" />
  <input type="text" placeholder="URL" class="RadioURL" value="${station.RadioURL}" />
  <input type="text" placeholder="Title Seperator" class="RadioTitleSeperator" size="1" value="${
    station.RadioTitleSeperator
  }" />
  <select class="RadioTitleFirst" value="${station.RadioTitleFirst}">
    <option>Artist</option>
    <option>Song</option>
  </select>`;
    newStationSettings
      .querySelector(".id_up")
      .addEventListener("click", () => switchStations(i, i - 1));
    newStationSettings
      .querySelector(".id_down")
      .addEventListener("click", () => switchStations(i, i + 1));
    newStationSettings
      .querySelector(".delete")
      .addEventListener("click", () => deleteStation(i));
    stations_settings.appendChild(newStationSettings);
  }
};

document
  .querySelector("#stations_settings__add_station")
  .addEventListener("click", () => addEmptyStation());

  
document.querySelector("#save_station_settings").addEventListener('click', async () => {
  const formData = new FormData();
  formData.append("config", serializeStations())
  const resp = await fetch("radio", {
    method: "POST",
    body: formData
  });
  alert(resp.ok ? "Saved Successfully" : "Failed to save!");
  loadConfig();
});

loadConfig();