/*
*  File : wiFiMgr.cpp
*  Date : 26.03.2021 
*
*   Wifi Modul based on WifiManager by tzapu
*   MQTT added
* Battery
* https://github.com/0015/ThatProject/blob/master/ESP32_TTGO/TTGO_Battery_Indicator/TTGO_Battery_Indicator.ino
*
* https://github.com/alanswx/ESPAsyncWiFiManager
* lib_deps =
*    alanswx/ESPAsyncWiFiManager
*/

// https://www.rustimation.eu/index.php/esp8266-wifi-parameter-speichern/

#if !defined(ESP32)
#error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#endif

#include "FS.h"
#include "LITTLEFS.h" //this needs to be first, or it all crashes and burns...
#define FORMAT_LITTLEFS_IF_FAILED true

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
AsyncWebServer webServer(80);
AsyncWebSocket ws("/");
AsyncWebSocketClient *globalClient = NULL;

// #include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson

#include <ESPAsyncWiFiManager.h> //https://github.com/tzapu/WiFiManager
DNSServer dns;
// AsyncWiFiManager wifiManager(&webServer, &dns);

// tzapu WiFimanager
// #include <WiFiManager.h>           // https://github.com/tzapu/WiFiManager
// WiFiManager wm;                    // global wm instance
// WiFiManagerParameter custom_field; // global param ( for non blocking w params )

#include "Preferences.h"
Preferences wifiPreferences; // create an instance of Preferences library
void setup_wifi_preferences();
void save_wifi_preferences();

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

//#include "INI_Setup_html.h"
//#include <WebServer.h>
//WebServer webServer(80);
const char *setupFileName = WIFI_SETUP_FILE;

const char *WifiAP_SSID = WIFI_AP_SSID;
const char *WifiAP_PASS = WIFI_AP_PASS;
// const char *configFileName = WIFI_CONFIG_FILE;

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

// wsSendTuner(au.radioCurrentStation + 1, au.radioCurrentVolume);
// --------------------------------------------------------------------------
void wsSendTuner(int presetNo, int volume)
{
    Serial.printf("wifi::wsSendTuner> Preset %d , Volume %d\n", presetNo, volume);
    ws.printfAll_P("station_select=%d", presetNo);
    ws.printfAll_P("volume=%d", volume);
}

// --------------------------------------------------------------------------
void wsSendArtistTitle(char *Artist, char *SongTitle)
{
    int strLen = strlen(SongTitle);
    Serial.printf("wifi::wsSendArtistTitle> SongTitle >%s< (len: %d) Artist >%s<\n", SongTitle, strLen, Artist);

    if (strLen)
    {
        ws.printfAll_P("meta_playing=%s@%s", Artist, SongTitle);
        getCoverBMID(Artist, SongTitle);
    }
    else
        ws.printfAll_P("meta_playing=@");
}

// --------------------------------------------------------------------------
void wsBroadcast()
{
    audio_ws_tuner();
    audio_ws_meta();
    //    ws.textAll("meta_playing=Lauv & Troye Sivan@I'm So Tired...");
} // end of function

// --------------------------------------------------------------------------
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{

    if (type == WS_EVT_CONNECT)
    {

        Serial.println("onWsEvent> Websocket client connection received");
        client->text("Hello from ESP32 Server");
        wsBroadcast();
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        Serial.println("onWsEvent> Client disconnected");
    }
    else if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->opcode == WS_TEXT)
        {
            data[len] = 0;
            char *command = (char *)data;
            Serial.printf("onWsEvent> command: >%s< len: %d\n", command, strlen(command));

            // onWsEvent> command: >playstate=pause< len: 15
            if (strncmp(command, "playstate", strlen("playstate")) == 0)
            {
                char *value = command + strlen("playstate=");
                Serial.printf("onWsEvent> playstate: >%s< len: %d\n", value, strlen(value));
                // if (strncmp(value, "play", 4) || strncmp(value, "unmute", 6))
                if (strncmp(value, "play", strlen("play")) == 0)
                    audio_mode(AUDIO_PLAY);
                if (strncmp(value, "unmute", strlen("unmute")) == 0)
                    audio_mode(AUDIO_PLAY);
                if (strncmp(value, "mute", strlen("mute")) == 0)
                    audio_mode(AUDIO_MUTE);
                if (strncmp(value, "pause", strlen("pause")) == 0)
                    audio_mode(AUDIO_MUTE);
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
    Serial.print("DUMMY handleConfig> start wifiManager.startWebPortal()");
    AsyncWiFiManager wifiManager(&webServer, &dns);
    wifiManager.startConfigPortal("OnDemandAP");
}

void handleRadioConfig(AsyncWebServerRequest *request)
{
    if (request->hasParam("config", true))
    {
        AsyncWebParameter *p = request->getParam("config");
        // use p->value() to read the new config
        Serial.println(p->value().c_str());
    }

    // After handling the post we redirect to the config page
    request->redirect("/radio.html");
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

    webServer.on("/radio", handleRadioConfig);

    webServer.on("/config", handleConfig);

    webServer.on("/inline", [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "this works as well");
    });

    webServer.serveStatic("/", LITTLEFS, "/").setDefaultFile("index.html");
    webServer.onNotFound(handleNotFound);

    ws.onEvent(onWsEvent);
    webServer.addHandler(&ws);

    webServer.begin();
    Serial.println("HTTP server started");

} // end of function

// ----------------------------------------------------------------------------------------
uint32_t memoryInfo()
{
    Serial.printf("\n-------------------------------- Get System Info------------------------------------------\n");
    //Get IDF version
    // Serial.printf("     SDK version:%s\n", esp_get_idf_version());
    //Get chip available memory
    // Serial.printf("     esp_get_free_heap_size : %d  \n", esp_get_free_heap_size());
    //Get the smallest memory that has never been used
    // Serial.printf("     esp_get_minimum_free_heap_size : %d  \n", esp_get_minimum_free_heap_size());

    char temp[200];
    sprintf(temp, "Heap: Free:%i, Min:%i, Size:%i, Alloc:%i\n", ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap());
    Serial.print(temp);

    //Get the memory distribution of the chip, see the structure flash_size_map for the return value
    // printf("     system_get_flash_size_map(): %d \n", system_get_flash_size_map());

    // uint32_t freeHeap = esp_get_free_heap_size();
    uint32_t freeHeap = ESP.getFreeHeap();
    return freeHeap;
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
    char buf[20];
    wifi_data.rssiLevel = dBmtoPercentage(WiFi.RSSI());
    dtostrf(wifi_data.rssiLevel, 1, 0, wifi_data.rssiChar); // 5 digits, no decimal
    Serial.printf("wifi> signal    : %s = %d dBm\n", wifi_data.rssiChar, WiFi.RSSI());
    sprintf(buf, "%s (%d dBm)", wifi_data.rssiChar, WiFi.RSSI());

    mqtt_pub_tele("IP", wifi_data.IPChar);
    mqtt_pub_tele("RSSI", buf);
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

/*
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
*/

//-------------------------------------------------------------------------------------------------------------------------------------------
// save current radio station so radio will tune to this statin after reboot
void save_wifi_preferences()
{
    wifiPreferences.begin("iotsharing", false); /* Start a namespace "iotsharing"in Read-Write mode */

    wifiPreferences.putString("mqtt_clientID", config.mqtt_clientID); /* Store preset to the Preferences */
    wifiPreferences.putString("mqtt_server", config.mqtt_server);     /* Store preset to the Preferences */
    wifiPreferences.putString("mqtt_port", config.mqtt_port);         /* Store preset to the Preferences */
    wifiPreferences.putString("mqtt_prefix", config.mqtt_prefix);     /* Store preset to the Preferences */

    Serial.printf("wifi::setup_wifi_preferences> mqtt_clientID %s\n", config.mqtt_clientID);
    Serial.printf("wifi::setup_wifi_preferences> mqtt_server   %s\n", config.mqtt_server);
    Serial.printf("wifi::setup_wifi_preferences> mqtt_port     %s\n", config.mqtt_port);
    Serial.printf("wifi::setup_wifi_preferences> mqtt_prefix   %s\n", config.mqtt_prefix);

    wifiPreferences.end(); /* Close the Preferences */
} // end of function

//-------------------------------------------------------------------------------------------------------------------------------------------
// retrieve last radio station before reboot
void setup_wifi_preferences()
{
    /* Start a namespace "iotsharing"in Read-Write mode: set second parameter to false Note: Namespace name is limited to 15 chars */
    wifiPreferences.begin("iotsharing", false);

    /* get value of key "mqtt_clientID", if key not exist return default value  in second argument Note: Key name is limited to 15 chars too */

    strncpy(config.mqtt_clientID, wifiPreferences.getString("mqtt_clientID", MQTT_CLIENTID).c_str(), sizeof(config.mqtt_clientID));
    strncpy(config.mqtt_server, wifiPreferences.getString("mqtt_server", MQTT_BROKER).c_str(), sizeof(config.mqtt_server));
    strncpy(config.mqtt_port, wifiPreferences.getString("mqtt_port", MQTT_PORT).c_str(), sizeof(config.mqtt_port));
    strncpy(config.mqtt_prefix, wifiPreferences.getString("mqtt_prefix", "house/").c_str(), sizeof(config.mqtt_prefix));

    Serial.printf("wifi::setup_wifi_preferences> mqtt_clientID %s\n", config.mqtt_clientID);
    Serial.printf("wifi::setup_wifi_preferences> mqtt_server   %s\n", config.mqtt_server);
    Serial.printf("wifi::setup_wifi_preferences> mqtt_port     %s\n", config.mqtt_port);
    Serial.printf("wifi::setup_wifi_preferences> mqtt_prefix   %s\n", config.mqtt_prefix);

    /* Close the Preferences */
    wifiPreferences.end();
} // end of function

// --------------------------------------------------------------------------
void setup_wifi()
{
    snprintf(config.mqtt_clientID, sizeof(config.mqtt_clientID), "ESP32_%d", get_chipID());
    serial_d_printf("wifi::setup_wifi> default clientID: %s\n", config.mqtt_clientID);

    // setup_read_fs_values();
    setup_wifi_preferences();

    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

    // for testing
    // wifiManager.resetSettings(); // wipe settings

    AsyncWiFiManager wifiManager(&webServer, &dns);

    //set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    AsyncWiFiManagerParameter custom_mqtt_server("server", "mqtt server", config.mqtt_server, sizeof(config.mqtt_server));
    AsyncWiFiManagerParameter custom_mqtt_port("port", "mqtt port", config.mqtt_port, sizeof(config.mqtt_port));
    // WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, sizeof(mqtt_user));
    // WiFiManagerParameter custom_mqtt_password("password", "mqtt password", mqtt_password, sizeof(mqtt_password));
    AsyncWiFiManagerParameter custom_mqtt_clientID("clientID", "mqtt client ID", config.mqtt_clientID, sizeof(config.mqtt_clientID));
    AsyncWiFiManagerParameter custom_mqtt_prefix("prefix", "mqtt topic prefix", config.mqtt_prefix, sizeof(config.mqtt_prefix));
    // config.mqtt_topic_prefix

    //add all your parameters here
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);
    // wifiManager.addParameter(&custom_mqtt_user);
    // wifiManager.addParameter(&custom_mqtt_password);
    wifiManager.addParameter(&custom_mqtt_clientID);
    wifiManager.addParameter(&custom_mqtt_prefix);

    /*
    // int customFieldLength = 40;
    // test custom html(radio)
    const char *custom_radio_str = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> Three";
    new (&custom_field) WiFiManagerParameter(custom_radio_str); // custom html input
    wifiManager.addParameter(&custom_field);
    wifiManager.setSaveParamsCallback(saveParamCallback);
    */

    // set dark theme
    // wifiManager.setClass("invert");

    //set static ip
    // wifiManager.setSTAStaticIPConfig(IPAddress(192,168,178,254), IPAddress(192,168,178,1), IPAddress(255,255,255,0)); // set static ip,gw,sn
    // wifiManager.setShowStaticFields(true); // force show static ip fields

    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wifiManager.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result

    wifiManager.setConnectTimeout(5); // in seconds
    // wifiManager.setConnectRetries(3); // default 1

    bool res;
    // res = wifiManager.autoConnect(); // auto generated AP name from chipid
    // res = wifiManager.autoConnect("AutoConnectAP"); // anonymous ap
    res = wifiManager.autoConnect(WifiAP_SSID, WifiAP_PASS); // password protected ap

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
        save_wifi_preferences();
        shouldSaveConfig = false;
    }

    mqtt_init();
    setup_webServer();
    memoryInfo();
    mqtt_pub_tele("Firmware", FIRMWARE_VERSION);
} // end of function

// --------------------------------------------------------------------------
void loop_wifi()
{
    mqtt.loop();
} // end of function

// unsigned char specials[] = "$&+,/:;=?@ <>#%{}|\^~[]`"; /* String containing chars you want encoded */
char specials[] = "$+,;@ <>#%{}|~[]`";

static char hex_digit(char c)
{
    return "0123456789ABCDEF"[c & 0x0F];
}

char *urlencode(char *dst, char *src)
{
    char c, *d = dst;
    while ((c = *src++))
    {
        if (strchr(specials, c))
        {
            *d++ = '%';
            *d++ = hex_digit(c >> 4);
            c = hex_digit(c);
        }
        *d++ = c;
    }
    *d = '\0';
    return dst;
}

#include "HttpClient.h"
String serverName = "http://musicbrainz.org/ws/2/release/?fmt=json&limit=1&query=release:";
//                   http://musicbrainz.org/ws/2/release/?limit=1&query=release:Welshly%20Arms%20%20/%20Legendary
//                   http://musicbrainz.org/ws/2/release/?fmt=json%26limit=1%26query=artist:Michael%20Patrick%20Kelly%20AND%20release:Beautiful%20Madness%20
//                  `http://musicbrainz.org/ws/2/release/?fmt=json&limit=${limit}&query=artist:${safeArtist} AND release:${safeTitle}`
// --------------------------------------------------------------------------
void getCoverBMID(char *Artist, char *SongTitle)
{
    return;

    HTTPClient http;

    char buf[200];
    char serverPath[200] = "http://musicbrainz.org/ws/2/release/?fmt=json&limit=1&query=artist:";
    strcat(serverPath, Artist);
    strcat(serverPath, " AND release:");
    strcat(serverPath, SongTitle);

    urlencode(buf, serverPath);
    Serial.printf("wifi::getCoverBMID> http.begin(%s)\n", buf);

    http.begin(buf);
    // http.begin("http://jsonplaceholder.typicode.com/comments?id=10");
    // http.begin("http://musicbrainz.org/ws/2/release/?fmt=json&limit=1&query=release:Welshly%20Arms%20%20/%20Legendary");

    // Send HTTP GET request
    int httpResponseCode = http.GET();
    Serial.printf("wifi::getCoverBMID> HTTP Response code: %d\n", httpResponseCode);

    if (httpResponseCode == 200)
    {
        String payload = http.getString();
        Serial.println(payload);
        // mqtt_pub_tele("BMID",payload.c_str());
    }

    // Free resources
    http.end();

} // end of function

// --------------------------------------------------------------------------
void getCoverJPG(char *coverBMID)
{
    // http://coverartarchive.org/release/${bmid}/front-250.jpg`
    // {"created":"2021-03-27T14:15:29.797Z","count":3,"offset":0,"releases":[{"id":"bcf13f43-1c36-4e67-a5f8-2c3d216112fc","score":100,
    // "status-id":"4e304316-386d-3409-af2e-78857eec5cfe",
    // "packaging-id":"119eba76-b343-3e02-a292-f0f00644bb9b",
    // http://coverartarchive.org/release/119eba76-b343-3e02-a292-f0f00644bb9b/front-250.jpg
    HTTPClient http;

    char buf[200];
    char serverPath[200] = "http: //coverartarchive.org/release/";
    strcat(serverPath, coverBMID);
    strcat(serverPath, "/front-250.jpg");

    urlencode(buf, serverPath);
    Serial.printf("wifi::getCoverJPG> http.begin(%s)\n", buf);

    http.begin(buf);

    // Send HTTP GET request
    int httpResponseCode = http.GET();
    Serial.printf("wifi::getCoverJPG> HTTP Response code: %d\n", httpResponseCode);

    if (httpResponseCode == 200)
    {
        // mqtt_pub_tele("BMID",payload.c_str());
    }

    // Free resources
    http.end();

} // end of function

/*

https://musicbrainz.org/doc/Cover_Art_Archive/API

const getBMID = async(title, artist) = >
{
    const argument = encodeURIComponent(`${artist} $ { title }`);
    const resp = await fetch(
    `http
        : //musicbrainz.org/ws/2/release/?fmt=json&limit=1&query=release:${argument}`
    );
    const release = await resp.json();
    const bmid = release.releases[0].id;
    return bmid;
};

const getTrackCoverURL = async(title, artist) = >
{
    const bmid = await getBMID(title, artist);
    return `http: //coverartarchive.org/release/${bmid}/front-250.jpg`;
};
*/
