const configTextarea = document.querySelector("#config");

const loadConfig = async () => {
  const resp = await fetch("./setup.ini");
  const ini = await resp.text();
  configTextarea.value = ini;
};

loadConfig();
