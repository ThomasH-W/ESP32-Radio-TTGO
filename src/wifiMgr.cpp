/*
*  File : wiFiMgr.cpp
*  Date : 26.03.2021 
*
*   Wifi Modul based on WifiManager by tzapu
*   MQTT added
*
*/

// https://www.rustimation.eu/index.php/esp8266-wifi-parameter-speichern/
// lib_deps =
//    https://github.com/tzapu/WiFiManager.git

#if !defined(ESP32)
#error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#endif

#include "FS.h"
#include "LITTLEFS.h" //this needs to be first, or it all crashes and burns...
#define FORMAT_LITTLEFS_IF_FAILED true

#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson

#include <WiFiManager.h>           // https://github.com/tzapu/WiFiManager
WiFiManager wm;                    // global wm instance
WiFiManagerParameter custom_field; // global param ( for non blocking w params )

#include <MQTT.h>
#define DEBUG true
#include "_SerialPrintf.h"
#include "wifiMgr.h"
#include "main.h"
#include "config.h"
// Set web server port number to 80
WiFiServer server(80);
WiFiClient net;
MQTTClient mqtt;

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
AsyncWebServer webServer(80);
AsyncWebSocket ws("/");
AsyncWebSocketClient *globalClient = NULL;

//#include "INI_Setup_html.h"
//#include <WebServer.h>
//WebServer webServer(80);
const char *setupFileName = WIFI_SETUP_FILE;

const char *WifiAP_SSID = WIFI_AP_SSID;
const char *WifiAP_PASS = WIFI_AP_PASS;
const char *configFileName = WIFI_CONFIG_FILE;

struct Config
{
    char mqtt_clientID[25] = MQTT_CLIENTID;
    char mqtt_server[15] = MQTT_BROKER;
    char mqtt_port[8] = MQTT_PORT;
    char mqtt_user[20] = "";
    char mqtt_password[20] = "";
    char mqtt_prefix[20] = "house/";
};
Config config;

char mqtt_topic_tele[50] = "";
char mqtt_topic_stat[50] = "";
char mqtt_topic_cmnd[50] = "";
char mqtt_topic_subscribe[50] = "";

//flag for saving data
bool shouldSaveConfig = false;

#include "audioConfig.h"
//audio_mode_t audioMode2;

wifi_data_struct wifi_data;

// --------------------------------------------------------------------------
// dummy function uased a template
void wifi_foo()
{
} // end of function

// --------------------------------------------------------------------------
// publish mqtt message to tele-topic
void mqtt_pub_tele(const char *topic, const char *message)
{
    mqtt.publish(String(mqtt_topic_tele) + "/" + topic, message);
    // serial_d_printf("wifi::mqtt_pub_tele> %s : %s\n", topic, message);
} // end of function

// --------------------------------------------------------------------------
// publish mqtt message to stat-topic
void mqtt_pub_stat(const char *topic, const char *message)
{
    mqtt.publish(String(mqtt_topic_stat) + "/" + topic, message);
    serial_d_printf("wifi::mqtt_topic_stat> %s : %s\n", topic, message);
} // end of function

// --------------------------------------------------------------------------
// publish mqtt message to cmnd-topic
void mqtt_pub_cmnd(const char *topic, const char *message)
{
    mqtt.publish(String(mqtt_topic_cmnd) + "/" + topic, message);
    serial_d_printf("wifi::mqtt_pub_cmnd> %s : %s\n", topic, message);
} // end of function

// --------------------------------------------------------------------------
// use mac address to generate a "unique" name for e.g. mqtt client name
int32_t get_chipID()
{
#ifdef ESP8266
    chipID = ESP.getChipId();
#endif

#ifdef ESP32
    uint64_t macAddress = ESP.getEfuseMac();
    uint64_t macAddressTrunc = macAddress << 40;
    int32_t chipID = macAddressTrunc >> 40;
#endif

    return chipID;
} // end of function

// --------------------------------------------------------------------------
// create full topic for standard topics: tele/cmnd/stat and subscribe
void mqtt_set_topics() //mqtt_topic_subscribe
{
    snprintf(mqtt_topic_tele, sizeof(mqtt_topic_tele), "%stele/%s", config.mqtt_prefix, config.mqtt_clientID);
    snprintf(mqtt_topic_stat, sizeof(mqtt_topic_stat), "%sstat/%s", config.mqtt_prefix, config.mqtt_clientID);
    snprintf(mqtt_topic_cmnd, sizeof(mqtt_topic_cmnd), "%scmnd/%s", config.mqtt_prefix, config.mqtt_clientID);
    snprintf(mqtt_topic_subscribe, sizeof(mqtt_topic_subscribe), "%scmnd/%s/#", config.mqtt_prefix, config.mqtt_clientID);

    serial_d_printf("wifi::mqtt_set_topics\ntele> %s\n", mqtt_topic_tele);
    serial_d_printf("stat> %s\n", mqtt_topic_stat);
    serial_d_printf("cmnd> %s\n", mqtt_topic_cmnd);
    serial_d_printf("subscribe> %s\n", mqtt_topic_subscribe);
} // end of function

// --------------------------------------------------------------------------
// wm callback notifying us of the need to save config
void saveConfigCallback()
{
    serial_d_printF("wifi::saveConfigCallback> saving\n");
    shouldSaveConfig = true;
} // end of function

// --------------------------------------------------------------------------
// wm custom parameter
String getParam(String name)
{
    //read parameter from server, for customhmtl input
    String value;
    if (wm.server->hasArg(name))
    {
        value = wm.server->arg(name);
    }
    return value;
} // end of function

// --------------------------------------------------------------------------
// wm call back for custom parameter
void saveParamCallback()
{
    serial_d_printF("wifi::saveParamCallback> saveParamCallback fired\n");
    serial_d_printf("wifi::saveParamCallback> PARAM customfieldid = %s\n", getParam("customfieldid"));
} // end of function

// --------------------------------------------------------------------------
// read config.json from littleFS
void setup_read_fs_values()
{
    //read configuration from FS json
    serial_d_printF("wifi::setup_read_fs_values> mounting FS...\n");

    if (LITTLEFS.begin(FORMAT_LITTLEFS_IF_FAILED))
    {
        serial_d_printF("file system mounted\n");
        if (LITTLEFS.exists(configFileName))
        {
            //file exists, reading and loading
            serial_d_printf("reading config file %s\n", configFileName);
            File configFile = LITTLEFS.open(configFileName, "r");
            if (configFile)
            {
                size_t size = configFile.size();
                serial_d_printf("config file opened (%d bytes)\n", size);

                // Allocate a temporary JsonDocument
                // Don't forget to change the capacity to match your requirements.
                // Use arduinojson.org/v6/assistant to compute the capacity.
                StaticJsonDocument<512> doc;

                // Deserialize the JSON document
                DeserializationError error = deserializeJson(doc, configFile);
                if (error)
                    serial_d_printF("ERR wifi::setup_read_fs_values> Failed to read file, using default configuration\n");
                else
                {
                    serializeJsonPretty(doc, Serial); // print json
                    serial_d_printF("\nJSON parsed\n");

                    // Copy values from the JsonDocument to the Config
                    // config.port = doc["port"] | 2731;
                    strlcpy(config.mqtt_server,               // <- destination
                            doc["mqtt_server"] | MQTT_BROKER, // <- source
                            sizeof(config.mqtt_server));      // <- destination's capacity
                    strlcpy(config.mqtt_port,                 // <- destination
                            doc["mqtt_port"] | MQTT_PORT,     // <- source
                            sizeof(config.mqtt_port));        // <- destination's capacity                    config.mqtt_port = doc["mqtt_port"] | MQTT_PORT;

                    strlcpy(config.mqtt_clientID,                        // <- destination
                            doc["mqtt_clientID"] | config.mqtt_clientID, // <- source
                            sizeof(config.mqtt_clientID));               // <- destination's capacity

                    strlcpy(config.mqtt_prefix,            // <- destination
                            doc["mqtt_prefix"] | "house/", // <- source
                            sizeof(config.mqtt_prefix));   // <- destination's capacity
                }                                          // DeserializationError
            }                                              // configFile
        }                                                  // LITTLEFS.exists
    }                                                      // LITTLEFS.begin
    else
    {
        serial_d_printF("ERR wifi::setup_read_fs_values> failed to mount filesystem\n");
    }
    //end read
} // end of function

// --------------------------------------------------------------------------
// mqtt callback handler
void mqtt_callback(String &topic, String &payload)
{
    Serial.println("incoming: " + topic + " - " + payload);
    int lastPos = topic.lastIndexOf('/');
    String lastElement = topic.substring(lastPos + 1);
    serial_d_printf("mqtt callback: %s\n", lastElement);

    if (lastElement.indexOf("PresetNo") >= 0)
        station_select(payload.toInt() - 1); // array id [0...9]= preset [1...10] -1

    if (lastElement.indexOf("PresetUp") >= 0)
        audio_mode(AUDIO_PRESET_UP);
    if (lastElement.indexOf("PresetDown") >= 0)
        audio_mode(AUDIO_PRESET_DOWN);

    if (lastElement.indexOf("Volume") >= 0)
        audio_mode(AUDIO_VOLUME, payload.toInt());
    if (lastElement.indexOf("VolUp") >= 0)
        audio_mode(AUDIO_VOLUME_UP);
    if (lastElement.indexOf("VolDown") >= 0)
        audio_mode(AUDIO_VOLUME_DOWN);
    if (lastElement.indexOf("Mute") >= 0)
        audio_mode(AUDIO_MUTE);
    if (lastElement.indexOf("Stop") >= 0)
        audio_mode(AUDIO_STOP);
    if (lastElement.indexOf("Start") >= 0)
        audio_mode(AUDIO_START);

    if (lastElement.indexOf("Voltage") >= 0)
        showVoltage();

} // end of function

// --------------------------------------------------------------------------
//  connect to mqtt broker
void mqtt_init()
{
    bool res;

    mqtt_set_topics();
    mqtt.onMessage(mqtt_callback);

    serial_d_printF("wifi::mqtt_init> > MQTT: mqtt.begin\n");
    mqtt.begin(config.mqtt_server, atoi(config.mqtt_port), net);
    // test if module works if mqtt is down
    // mqtt.begin("dummy", atoi(config.mqtt_port), net);
    res = mqtt.connect(config.mqtt_server, config.mqtt_user, config.mqtt_password);
    if (res)
    {
        serial_d_printF("MQTT: connected\n");
        mqtt_pub_tele("INFO", "MQTT Connected");
        mqtt_pub_tele("CMND", mqtt_topic_cmnd);
        mqtt.subscribe(mqtt_topic_subscribe);
    }
    else
        serial_d_printF("ERR wifi::mqtt_init> > MQTT: connection failed.\n");

} // end of function

// --------------------------------------------------------------------------
void handleNotFound(AsyncWebServerRequest *request)
{
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += request->url();
    message += "\nMethod: ";
    message += (request->method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += request->args();
    message += "\n";

    for (uint8_t i = 0; i < request->args(); i++)
    {
        message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
    }

    request->send(404, "text/plain", message);
} // end of function

// --------------------------------------------------------------------------
bool loadFromSPIFFS(AsyncWebServerRequest *request, String path)
{
    String dataType = "text/html";

    Serial.print("loadFromSPIFFS> Requested page -> ");
    Serial.println(path);
    if (LITTLEFS.exists(path))
    {
        File dataFile = LITTLEFS.open(path, "r");
        if (!dataFile)
        {
            handleNotFound(request);
            return false;
        }

        AsyncWebServerResponse *response = request->beginResponse(LITTLEFS, path, dataType);
        Serial.print("Real file path: ");
        Serial.println(path);

        request->send(response);

        dataFile.close();
    }
    else
    {
        handleNotFound(request);
        return false;
    }
    return true;
} // end of function

// --------------------------------------------------------------------------
void handleRoot(AsyncWebServerRequest *request)
{
    loadFromSPIFFS(request, "/index.html");
} // end of function

/*
https://techtutorialsx.com/2018/08/14/esp32-async-http-web-server-websockets-introduction/

https://techtutorialsx.com/2018/09/11/esp32-arduino-web-server-sending-data-to-javascript-client-via-websocket/
https://techtutorialsx.com/2018/09/13/esp32-arduino-web-server-receiving-data-from-javascript-websocket-client/

*/

// --------------------------------------------------------------------------
void wsSendArtistTitle(char *Artist, char *SongTitle)
{
    char buf[200];
    sprintf(buf, "meta_playing=%s@%s", Artist, SongTitle);

    Serial.printf("wsSendArtistTitle> >%s<\n", buf);
    if (globalClient) // if not null re client is connected
    {
        globalClient->text("sending ESP32 Stream meta data ....");
        globalClient->text(buf);
    }
}

// --------------------------------------------------------------------------
void wsBroadcast()
{
    audio_ws_meta();
    //    ws.textAll("meta_playing=Thomas  + Raphael Hoeser@Heavy Cross");
} // end of function

// --------------------------------------------------------------------------
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{

    if (type == WS_EVT_CONNECT)
    {

        Serial.println("onWsEvent> Websocket client connection received");
        globalClient = client;

        client->text("Hello from ESP32 Server");
        wsBroadcast();
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        Serial.println("onWsEvent> Client disconnected");
        globalClient = NULL;
    }
    else if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->opcode == WS_TEXT)
        {
            data[len] = 0;
            char *command = (char *)data;
            Serial.printf("onWsEvent> command: >%s< len: %d\n", command, strlen(command));
            if (strncmp(command, "playstate", strlen("playstate")) == 0)
            {
                char *value = command + strlen("playstate=");
                if (strncmp(value, "play", 4) || strncmp(value, "unmute", 6))
                {
                    audio_mode(AUDIO_MUTE);
                }
                else
                {
                    audio_mode(AUDIO_MUTE);
                }
            }
            if (strncmp(command, "volume", strlen("volume")) == 0)
            {
                // okay since command is \0 terminated
                uint8_t volume = atoi(command + strlen("volume="));
                Serial.printf("onWsEvent> set volume to %d\n", volume);
                audio_mode(AUDIO_VOLUME, volume);
            }
            if (strncmp(command, "station_select", strlen("station_select")) == 0)
            {
                // okay since command is \0 terminated
                uint8_t presetNo = atoi(command + strlen("station_select="));
                int stationID = (int)presetNo - 1;
                Serial.printf("onWsEvent> tune to station %d [%d]\n", presetNo, stationID);
                station_select(stationID); // tune to new station; index 0...9
            }
        }
    }
} // end of function

// --------------------------------------------------------------------------
void handleConfig(AsyncWebServerRequest *request)
{
    Serial.print("handleConfig> start wm.startWebPortal()");
    wm.startWebPortal();
}

// --------------------------------------------------------------------------
void setup_webServer()
{
    Serial.print(F("setup_webServer> Inizializing FS..."));
    if (LITTLEFS.begin())
    {
        Serial.println(F("done."));
    }
    else
    {
        Serial.println(F("fail."));
    }

    // webServer.on("/", handleRoot);
    webServer.serveStatic("/", LITTLEFS, "/").setDefaultFile("index.html");

    webServer.on("/config", handleConfig);

    webServer.on("/inline", [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "this works as well");
    });

    webServer.onNotFound(handleNotFound);

    ws.onEvent(onWsEvent);
    webServer.addHandler(&ws);

    webServer.begin();
    Serial.println("HTTP server started");

} // end of function

// ----------------------------------------------------------------------------------------
int dBmtoPercentage(int dBm)
{
    Serial.print(" (");
    Serial.print(dBm);
    Serial.print(" db) ");
    int quality = 0;
    if (dBm > -90) // s/b be done differetnly with else / if
        quality = 1;
    if (dBm > -80)
        quality = 2;
    if (dBm > -70)
        quality = 3;
    if (dBm > -67)
        quality = 4;
    if (dBm > -30)
        quality = 5;

    return quality;
} // end of fuction dBmtoPercentage

// --------------------------------------------------------------------------
void pub_wifi_info()
{
    wifi_data.rssiLevel = dBmtoPercentage(WiFi.RSSI());
    dtostrf(wifi_data.rssiLevel, 1, 0, wifi_data.rssiChar); // 5 digits, no decimal
    Serial.printf("wifi> signal    : %s = %d dBm\n", wifi_data.rssiChar, WiFi.RSSI());

    mqtt_pub_tele("IP", wifi_data.IPChar);
    mqtt_pub_tele("RSSI", wifi_data.rssiChar);
} // end of function

// --------------------------------------------------------------------------
wifi_data_struct *setup_wifi_info()
{
    strncpy(wifi_data.ssidChar, WiFi.SSID().c_str(), 15);
    Serial.printf("wifi> WiFi SSID: %s\n", wifi_data.ssidChar);

    strncpy(wifi_data.IPChar, WiFi.localIP().toString().c_str(), 15);
    Serial.printf("wifi> IP address: %s\n", wifi_data.IPChar);

    pub_wifi_info();
    return &wifi_data;
} // end of function

// --------------------------------------------------------------------------
void setup_wifi()
{
    snprintf(config.mqtt_clientID, sizeof(config.mqtt_clientID), "ESP32_%d", get_chipID());
    serial_d_printf("wifi::setup_wifi> default clientID: %s\n", config.mqtt_clientID);

    setup_read_fs_values();

    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

    // for testing
    // wm.resetSettings(); // wipe settings

    //set config save notify callback
    wm.setSaveConfigCallback(saveConfigCallback);

    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", config.mqtt_server, sizeof(config.mqtt_server));
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", config.mqtt_port, sizeof(config.mqtt_port));
    // WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, sizeof(mqtt_user));
    // WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password, sizeof(mqtt_password));
    WiFiManagerParameter custom_mqtt_clientID("clientID", "mqtt client ID", config.mqtt_clientID, sizeof(config.mqtt_clientID));
    WiFiManagerParameter custom_mqtt_prefix("prefix", "mqtt topic prefix", config.mqtt_prefix, sizeof(config.mqtt_prefix));
    // config.mqtt_topic_prefix

    //add all your parameters here
    wm.addParameter(&custom_mqtt_server);
    wm.addParameter(&custom_mqtt_port);
    // wm.addParameter(&custom_mqtt_user);
    // wm.addParameter(&custom_mqtt_password);
    wm.addParameter(&custom_mqtt_clientID);
    wm.addParameter(&custom_mqtt_prefix);

    /*
    // int customFieldLength = 40;
    // test custom html(radio)
    const char *custom_radio_str = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> Three";
    new (&custom_field) WiFiManagerParameter(custom_radio_str); // custom html input
    wm.addParameter(&custom_field);
    wm.setSaveParamsCallback(saveParamCallback);
    */

    // set dark theme
    wm.setClass("invert");

    //set static ip
    // wm.setSTAStaticIPConfig(IPAddress(192,168,178,254), IPAddress(192,168,178,1), IPAddress(255,255,255,0)); // set static ip,gw,sn
    // wm.setShowStaticFields(true); // force show static ip fields

    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result

    wm.setConnectTimeout(5); // in seconds
    wm.setConnectRetries(3); // default 1

    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
    res = wm.autoConnect(WifiAP_SSID, WifiAP_PASS); // password protected ap

    if (!res)
    {
        Serial.println("ERR setup_wifi> Failed to connect");
        // ESP.restart();
    }
    else
    {
        //if you get here you have connected to the WiFi
        serial_d_printF("wifi::setup_wifi> connected...yeey :)\n");
        serial_d_printF("local IP: ");
        serial_d_println(WiFi.localIP());
        serial_d_printF("gateway : ");
        serial_d_println(WiFi.gatewayIP());
        serial_d_printF("subnet  : ");
        serial_d_println(WiFi.subnetMask());
    }

    serial_d_printf("wifi::setup_wifi> mqtt_clientID  : %s (wm custom)\n", custom_mqtt_clientID.getValue());
    strlcpy(config.mqtt_clientID,            // <- destination
            custom_mqtt_clientID.getValue(), // <- source
            sizeof(config.mqtt_clientID));   // <- destination's capacity

    serial_d_printf("wifi::setup_wifi> mqtt_clientID  : %s (config)\n", config.mqtt_clientID);

    //save the custom parameters to FS
    if (shouldSaveConfig)
    {
        serial_d_printF("wifi::setup_wifi> >  saving config\n");

        LITTLEFS.format(); // weiss nicht, ob das nötig ist, schadet aber nicht.
        File configFile = LITTLEFS.open(configFileName, "w");
        if (!configFile)
        {
            serial_d_printf("wifi::setup_wifi> failed to open config file %s for writing\n", configFileName);
        }
        else
        {
            // Allocate a temporary JsonDocument
            // Don't forget to change the capacity to match your requirements.
            // Use arduinojson.org/assistant to compute the capacity.
            StaticJsonDocument<256> doc;

            // Set the values in the document
            doc["mqtt_server"] = custom_mqtt_server.getValue();
            doc["mqtt_port"] = custom_mqtt_port.getValue();
            doc["mqtt_clientID"] = custom_mqtt_clientID.getValue();
            doc["mqtt_prefix"] = custom_mqtt_prefix.getValue();

            // Serialize JSON to file
            if (serializeJson(doc, configFile) == 0)
            {
                serial_d_printF("wifi::setup_wifi> >  Failed to write to file\n");
            }
            configFile.close();
            serial_d_printf("wifi::setup_wifi> mqtt_clientID  : %s (saved to config file)\n", config.mqtt_clientID);
        }

        //end save

        shouldSaveConfig = false;
    }

    mqtt_init();
    setup_webServer();
} // end of function

// --------------------------------------------------------------------------
void loop_wifi()
{
    mqtt.loop();
} // end of function
