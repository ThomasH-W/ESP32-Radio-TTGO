/*
  File : audio.cpp
  Date : 26.03.2021 

  Based on https://github.com/schreibfaul1/ESP32-audioI2S

lib_deps = 
    https://github.com/schreibfaul1/ESP32-audioI2S

*/

#include <Arduino.h>

#define DEBUG true
#include "_SerialPrintf.h"
#include "config.h"
const char *setupFileName2 = WIFI_SETUP_FILE;
#include "wifiMgr.h"

#include "Preferences.h"
Preferences preferences; // create an instance of Preferences library
void setup_audio_preferences();
void save_audio_preferences();

#include "FS.h"
#include "LITTLEFS.h" //this needs to be first, or it all crashes and burns...

SetupGPIO setupGPIO;
#define MAX_RADIO_STATIONS 10
SetupRadio setupRadio[MAX_RADIO_STATIONS];
int used_radio_stations = 0;

#include "Audio.h"
Audio audio;

#include "esp_adc_cal.h"
float battery_voltage = 0;

#include "main.h"
audio_data_struct au; // audio data structure

unsigned long currentMillisAudioLoop = 0, previousMillisRotary = 0, intervalRotaryLoop = 3000;
unsigned long previousPreSelectMillis = 0, intervalPreSelectLoop = 3000;
//-----------------------------------------------------------------------------------------
// when rotary encoder button is pressed, toggle mode
// a) change volume
// b) change station / preset
int audio_rotary_button(void)
{
    int rotaryPos = 0;
    if (!au.radioRotaryVolume)
    { // was mode station
        au.radioRotaryVolume = true;
        rotaryPos = au.radioCurrentVolume;
        serial_d_printf("audio::audio_rotary_button> Volume: %o - %d\n", au.radioRotaryVolume, rotaryPos);
    }
    else
    { // was mode volume
        au.radioRotaryVolume = false;
        rotaryPos = au.radioCurrentStation;
        serial_d_printf("audio::audio_rotary_button> Radio: %o - %d\n", au.radioRotaryVolume, rotaryPos);
        previousMillisRotary = millis(); // otherwise mode will be reset in loop_audio()
    }
    return rotaryPos;
} // end of function

//-----------------------------------------------------------------------------------------
// depending on mode, change volume or station/preset
void audio_rotary_rotation(bool dirUp)
{

    if (au.radioRotaryVolume)
    { // mode volume
        serial_d_printf("audio::audio_rotary_rotation> Volume %o ; turn up %o\n", au.radioRotaryVolume, dirUp);
        if (dirUp)
            audio_mode(AUDIO_VOLUME_UP);
        else
            audio_mode(AUDIO_VOLUME_DOWN);
    }
    else
    { // mode station
        serial_d_printf("audio::audio_rotary_rotation> Preset %o ; turn up %o\n", !au.radioRotaryVolume, dirUp);
        if (dirUp)
            station_pre_select(au.radioNextStation + 1);
        else
            station_pre_select(au.radioNextStation - 1);
        previousMillisRotary = millis(); // otherwise mode will be reset in loop_audio()
    }

} // end of function

//-----------------------------------------------------------------------------------------
// overlay if not value is being provided
void audio_mode(int mode)
{
    audio_mode(mode, 0);
} // end of function

//-----------------------------------------------------------------------------------------
// used for mqtt messages to request certain actions
void audio_mode(int mode, int value)
{
    serial_d_printf("audio::audio_mode> %d\n", mode);
    char buf[10];
    switch (mode)
    {
    case AUDIO_MUTE:
        if (au.radioMute) // true (1) or false (0)
        {
            audio.setVolume(au.radioCurrentVolume);
            au.radioMute = false;
            mqtt_pub_tele("Mute", "play");
        }
        else
        {
            audio.setVolume(0);
            au.radioMute = true;
            mqtt_pub_tele("Mute", "muted");
        }
        au.update = UP_VOLUME;
        break;
    case AUDIO_VOLUME:
        au.radioCurrentVolume = value;
        serial_d_printf("audio::audio_mode> volume %d\n", au.radioCurrentVolume);
        audio.setVolume(au.radioCurrentVolume);
        au.update = UP_VOLUME;
        break;
    case AUDIO_VOLUME_UP:
        audio.setVolume(++au.radioCurrentVolume);
        au.update = UP_VOLUME;
        break;
    case AUDIO_VOLUME_DOWN:
        audio.setVolume(--au.radioCurrentVolume);
        au.update = UP_VOLUME;
        break;
    case AUDIO_PRESET_UP:
        serial_d_printf("audio::audio_mode> preset: current %d -> requested %d\n", au.radioCurrentStation, au.radioCurrentStation + 1);
        station_select(au.radioCurrentStation + 1);
        au.update = UP_PRESET;
        break;
    case AUDIO_PRESET_DOWN:
        serial_d_printf("audio::audio_mode> preset: current %d -> requested %d\n", au.radioCurrentStation, au.radioCurrentStation - 1);
        station_select(au.radioCurrentStation - 1);
        au.update = UP_PRESET;
        break;
    case AUDIO_STOP:
        audio.setVolume(0);
        break;
    case AUDIO_START:
        audio.setVolume(au.radioCurrentVolume);
        break;
    default:
        // if nothing else matches, do the default
        // default is optional
        break;
    }

    serial_d_printf("audio Station %d\n", au.radioCurrentStation);
    serial_d_printf("audio Volume %d\n", au.radioCurrentVolume);
    sprintf(buf, "%d", au.radioCurrentVolume);
    mqtt_pub_tele("Volume", buf);
    main_displayUpdate(false);
    wsSendTuner(au.radioCurrentStation + 1, au.radioCurrentVolume);
} // end of function

//-----------------------------------------------------------------------------------------
// async selection of radio station
// in order to scroll through list of station before committing and tuning
void station_pre_select(int stationID)
{
    char buf[10];
    int presetNo;

    presetNo = stationID + 1; // array index 0...9; preset 1...10

    serial_d_printf("audio::station_select> requested preset %d of %d\n", presetNo, used_radio_stations);

    if (stationID < 0)
        stationID = used_radio_stations - 1;
    if (stationID >= used_radio_stations)
        stationID = 0;

    if (strncmp(setupRadio[stationID].RadioURL, "http", 4))
        serial_d_printf("audio::station_select> Preset %d invalid URL: %s\n", presetNo, setupRadio[stationID].RadioURL);
    else
    {
        serial_d_printf("audio::station_select> Preset %d : %s\n", presetNo, setupRadio[stationID].RadioURL);
        au.radioNextStation = stationID;

        sprintf(buf, "%d", presetNo);
        mqtt_pub_tele("PresetNext", buf);
        strncpy(au.radioNextName, setupRadio[stationID].RadioName, sizeof(au.radioNextName));
        au.update = UP_PRESET;
        au.preSelect = true;

    } // check url

    main_displayUpdate(true);
} // end of function

//-----------------------------------------------------------------------------------------
// once timer expires, tune to new station and remove update flag
void station_apply_preselect(void)
{
    if (au.radioNextStation != au.radioCurrentStation)
    {
        serial_d_printf("audio::station_apply_preselect> requested preset %d != %d\n", au.radioNextStation, au.radioCurrentStation);
        station_select(au.radioNextStation);
    }
    else
        serial_d_printf("audio::station_apply_preselect> no change for preset %d = %d = current\n", au.radioNextStation, au.radioCurrentStation);

    au.preSelect = false;
} // end of function

//-----------------------------------------------------------------------------------------
// tune to new station and populate info structure
// in paralle push info to mqtt
void station_select(int stationID)
{
    char buf[20];
    int presetNo;

    presetNo = stationID + 1; // array index 0...9; preset 1...10

    serial_d_printf("\n-------------------------------- Staion Select ------------------------------------------\n");
    serial_d_printf("audio::station_select> requested preset %d of %d\n", presetNo, used_radio_stations);

    if (stationID < 0)
        stationID = used_radio_stations - 1;
    if (stationID >= used_radio_stations)
        stationID = 0;

    if (strncmp(setupRadio[stationID].RadioURL, "http", 4))
        serial_d_printf("audio::station_select> Preset %d invalid URL: %s\n", presetNo, setupRadio[stationID].RadioURL);
    else
    {
        serial_d_printf("audio::station_select> Preset %d : %s\n", presetNo, setupRadio[stationID].RadioURL);
        au.radioCurrentStation = stationID;
        au.radioNextStation = stationID;
        save_audio_preferences();

        mqtt_pub_tele("Station", "");
        mqtt_pub_tele("Title", "");
        mqtt_pub_tele("SongTitle", "");
        mqtt_pub_tele("Artist", "");

        strncpy(au.radioSongTitle, "", 2);
        strncpy(au.radioArtist, "", 2);

        serial_d_printf("audio::station_select> au.radioTitleSeperator %s\n", setupRadio[stationID].RadioTitleSeperator);
        au.radioTitleSeperator = setupRadio[stationID].RadioTitleSeperator[0];
        serial_d_printf("audio::station_select> au.radioArtistFirst %d\n", setupRadio[stationID].RadioArtistFirst);
        au.radioArtistFirst = setupRadio[stationID].RadioArtistFirst;

        serial_d_printf("audio::station_select> presetNo %d\n", presetNo);
        sprintf(buf, "%d", presetNo);
        serial_d_printf("audio::station_select> mqtt publish >%s<\n", buf);
        mqtt_pub_tele("PresetNo", buf);

        strncpy(au.radioArtist, setupRadio[stationID].RadioName, sizeof(au.radioName)); // to prevent empty screen
        strncpy(au.radioNextName, setupRadio[stationID].RadioName, sizeof(au.radioNextName));

        uint32_t freeHeap = memoryInfo();
        sprintf(buf, "%u", freeHeap);
        mqtt_pub_tele("MemoryHeap", buf);

        serial_d_printf("audio::station_select> set volume (0)\n");
        audio.setVolume(0); // 0...21
        delay(10);
        serial_d_printf("audio::station_select> URL >%s<\n", setupRadio[stationID].RadioURL);
        audio.connecttohost(setupRadio[stationID].RadioURL); //  start streaming
        serial_d_printf("audio::station_select> conected<\n");
        au.update = UP_PRESET;
        delay(20);
        serial_d_printf("audio::station_select> mqtt publish >%s<\n", setupRadio[stationID].RadioName);
        mqtt_pub_tele("Preset", setupRadio[stationID].RadioName);
        strncpy(au.radioName, setupRadio[stationID].RadioName, sizeof(au.radioName));
        serial_d_printf("audio::station_select> mqtt wsSendArtistTitle %s / %s \n", au.radioArtist, au.radioSongTitle);
        wsSendArtistTitle(au.radioArtist, au.radioSongTitle);
        wsSendTuner(au.radioCurrentStation + 1, au.radioCurrentVolume);

        if (au.radioCurrentVolume == 0)
            au.radioCurrentVolume = AUDIO_DEFAULT_VOLUME;
        if (au.radioMute == true)
            au.radioMute = false;
        audio.setVolume(au.radioCurrentVolume); // 0...21
        sprintf(buf, "%d", au.radioCurrentVolume);
        mqtt_pub_tele("Volume", buf);
    } // check url

    showVoltage();
    main_displayUpdate(true);
} // end of function

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

//-----------------------------------------------------------------------------------------
void audio_ws_tuner()
{
    wsSendTuner(au.radioCurrentStation + 1, au.radioCurrentVolume);
}

//-----------------------------------------------------------------------------------------
void audio_ws_meta()
{
    wsSendArtistTitle(au.radioArtist, au.radioSongTitle);
}
//-----------------------------------------------------------------------------------------
// optional - THX
void audio_info(const char *info)
{
    char buf[20];
    Serial.print("info        ");
    Serial.println(info);
    if (strncmp(info, "BitRate", 6) == 0)
    {
        sprintf(buf, "%d", atoi(strstr(info, ": ") + 2) / 1000);
        strncpy(au.radioBitRate, buf, sizeof(au.radioBitRate));
        // Serial.println("BitRate found");
        // Serial.println(au.radioBitRate);
        mqtt_pub_tele("Bitrate", au.radioBitRate);
    }
}
void audio_id3data(const char *info)
{ //id3 metadata
    Serial.print("id3data     ");
    Serial.println(info);
}
void audio_eof_mp3(const char *info)
{ //end of file
    Serial.print("eof_mp3     ");
    Serial.println(info);
}
void audio_showstation(const char *info)
{
    Serial.print("station     ");
    Serial.println(info);
    mqtt_pub_tele("Station", info);
}
void audio_showstreamtitle(const char *info)
{
    Serial.print("streamtitle ");
    Serial.println(info);

    strncpy(au.radioTitle, info, sizeof(au.radioTitle));
    // Serial.println(au.radioTitle);
    mqtt_pub_tele("Title", info);

    char buf[20] = ";";
    buf[0] = au.radioTitleSeperator;
    mqtt_pub_tele("TitleSep", buf);
    char *posSlash = strchr(info, au.radioTitleSeperator);
    if (posSlash) // found / so the title contains SongTitle/Artist
    {
        Serial.println("streamtitle: seperator found");
        *posSlash = 0; // set end of string as position of /

        if (au.radioArtistFirst)
        {
            strncpy(au.radioArtist, info, sizeof(au.radioSongTitle));
            strncpy(au.radioSongTitle, posSlash + 2, sizeof(au.radioArtist));
        }
        else
        {
            strncpy(au.radioSongTitle, info, sizeof(au.radioSongTitle));
            strncpy(au.radioArtist, posSlash + 2, sizeof(au.radioArtist));
        }

        Serial.print("Artist      ");
        Serial.println(au.radioArtist);
        displayDebugPrintln(au.radioArtist);
        mqtt_pub_tele("Artist", au.radioArtist);

        Serial.print("SongTitle   ");
        Serial.println(au.radioSongTitle);
        displayDebugPrintln(au.radioSongTitle);
        mqtt_pub_tele("SongTitle", au.radioSongTitle);

        audio_ws_meta();
    }
    // show memory usage and publish free heap
    uint32_t freeHeap = memoryInfo();
    sprintf(buf, "%u", freeHeap);
    // sprintf(buf, "%u", ESP.getFreeHeap());
    // Serial.printf("ESP free Heap() %s\n",buf);
    mqtt_pub_tele("MemoryHeap", buf);

    main_displayUpdate(false);
}
void audio_bitrate(const char *info)
{
    char buf[10];
    Serial.print("bitrate     ");
    Serial.println(info);
    sprintf(buf, "%d", atoi(info) / 1000);
    strncpy(au.radioBitRate, buf, sizeof(au.radioBitRate));
    mqtt_pub_tele("Bitrate", buf);
}
void audio_commercial(const char *info)
{ //duration in sec
    Serial.print("commercial  ");
    Serial.println(info);
}
void audio_icyurl(const char *info)
{ //homepage
    Serial.print("icyurl      ");
    Serial.println(info);
}
void audio_lasthost(const char *info)
{ //stream URL played
    Serial.print("lasthost    ");
    Serial.println(info);
}
void audio_eof_speech(const char *info)
{
    Serial.print("eof_speech  ");
    Serial.println(info);
}

// -----------------------------------------------------------------------------------------
// debug: print GPIO config
void setup_show_data()
{
    serial_d_printf("P_I2S_LRCK %d\n", setupGPIO.P_I2S_LRCK);
    serial_d_printf("P_I2S_BCLK %d\n", setupGPIO.P_I2S_BCLK);
    serial_d_printf("P_I2S_DATA %d\n", setupGPIO.P_I2S_DATA);

    serial_d_printf("P_ENC0_A %d\n", setupGPIO.P_ENC0_A);
    serial_d_printf("P_ENC0_B %d\n", setupGPIO.P_ENC0_B);
    serial_d_printf("P_ENC0_BTN %d\n", setupGPIO.P_ENC0_BTN);
    serial_d_printf("P_ENC0_PWR %d\n", setupGPIO.P_ENC0_PWR);

    serial_d_printf("P_ADC_BAT %d\n", setupGPIO.P_ADC_BAT);

    serial_d_printf("P_ADC_BAT %d\n", setupGPIO.P_ADC_BAT);

    serial_d_printf("Volume %d\n", au.radioCurrentVolume);

    for (int i = 0; i < 4; i++)
        serial_d_printf("Radio %d: %s - %s\n", i, setupRadio[i].RadioName, setupRadio[i].RadioURL);

} // end of function

// -----------------------------------------------------------------------------------------
// read name and asign value
void setup_use_data(int lineNo, String setupName, String setupValue)
{
    if (setupName.indexOf("P_I2S_LRCK") >= 0)
        setupGPIO.P_I2S_LRCK = setupValue.toInt();
    if (setupName.indexOf("P_I2S_BCLK") >= 0)
        setupGPIO.P_I2S_BCLK = setupValue.toInt();
    if (setupName.indexOf("P_I2S_DATA") >= 0)
        setupGPIO.P_I2S_DATA = setupValue.toInt();

    if (setupName.indexOf("P_ENC0_A") >= 0)
        setupGPIO.P_ENC0_A = setupValue.toInt();
    if (setupName.indexOf("P_ENC0_B") >= 0)
        setupGPIO.P_ENC0_B = setupValue.toInt();
    if (setupName.indexOf("P_ENC0_BTN") >= 0)
        setupGPIO.P_ENC0_BTN = setupValue.toInt();
    if (setupName.indexOf("P_ENC0_PWR") >= 0)
        setupGPIO.P_ENC0_BTN = setupValue.toInt();

    if (setupName.indexOf("P_ADC_BAT") >= 0)
        setupGPIO.P_ADC_BAT = setupValue.toInt();
    if (setupName.indexOf("P_ADC_EN") >= 0)
        setupGPIO.P_ADC_EN = setupValue.toInt();

    if (setupName.indexOf("Volume") >= 0)
        au.radioCurrentVolume = setupValue.toInt();

    if (used_radio_stations <= MAX_RADIO_STATIONS)
    {
        if (setupName.indexOf("RadioName") >= 0)
        {
            setupValue.toCharArray(setupRadio[used_radio_stations].RadioName, sizeof(setupRadio[used_radio_stations].RadioName));
            used_radio_stations++;
        }
        if (setupName.indexOf("RadioURL") >= 0)
        {
            setupValue.toCharArray(setupRadio[used_radio_stations - 1].RadioURL, sizeof(setupRadio[used_radio_stations - 1].RadioURL));
        }
        if (setupName.indexOf("RadioTitleSeperator") >= 0)
            setupValue.toCharArray(setupRadio[used_radio_stations - 1].RadioTitleSeperator, sizeof(setupRadio[used_radio_stations - 1].RadioTitleSeperator));
        if (setupName.indexOf("RadioTitleFirst") >= 0)
        {
            if (setupValue.indexOf("Artist") >= 0)
                setupRadio[used_radio_stations - 1].RadioArtistFirst = true;
            else
                setupRadio[used_radio_stations - 1].RadioArtistFirst = false;
        }
    }

} // end of function

// -----------------------------------------------------------------------------------------------
// parse single line form config file setup.ini
void setup_read_line(int lineNo, const char *str)
{
    char *value; // Points to value after equal sign in command

    value = strstr(str, "="); // See if command contains a "="
    if (value)
    {
        *value = '\0'; // Separate command from value
        value++;       // Points to value after "="
    }
    else
    {
        value = (char *)"0"; // No value, assume zero
    }

    String setupName;  // Argument as string
    String setupValue; // Value of an setupName as a string
    int inx;           // Index in string

    setupName = str;
    if ((inx = setupName.indexOf("#")) >= 0) // Comment line or partial comment?
    {
        setupName.remove(inx); // Yes, remove
    }
    setupName.trim();            // Remove spaces and CR
    if (setupName.length() == 0) // Lege commandline (comment)?
    {
        return;
    }
    // setupName.toLowerCase(); // Force to lower case
    //value = chomp ( val ) ;                             // Get the specified value
    setupValue = value;
    if ((inx = setupValue.indexOf("#")) >= 0) // Comment line or partial comment?
    {
        setupValue.remove(inx); // Yes, remove
    }
    setupValue.trim(); // Remove spaces and CR

    if (setupValue.length())
    {
        serial_d_printf("[%3d]> ", lineNo);
        serial_d_print(setupName);
        serial_d_printF(" = ");
        serial_d_println(setupValue);
    }
    else
    {
        serial_d_printf("[%3d]> %s (no value)\n", lineNo, setupName);
    }
    setup_use_data(lineNo, setupName, setupValue);
} // end of function

// -------------------------------------------------------------------------
// List files in ESP8266 or ESP32 SPIFFS memory
// -------------------------------------------------------------------------
void listDir2(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");

#ifdef CONFIG_LITTLEFS_FOR_IDF_3_2
            Serial.println(file.name());
#else
            Serial.print(file.name());
            time_t t = file.getLastWrite();
            struct tm *tmstruct = localtime(&t);
            Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
#endif

            if (levels)
            {
                listDir2(fs, file.name(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");

#ifdef CONFIG_LITTLEFS_FOR_IDF_3_2
            Serial.println(file.size());
#else
            Serial.print(file.size());
            time_t t = file.getLastWrite();
            struct tm *tmstruct = localtime(&t);
            Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
#endif
        }
        file = root.openNextFile();
    }
}

// -----------------------------------------------------------------------------------------------
// read config file setup.ini from littleFS and pass every line to parser
bool setup_read_file()
{
    String line;  // Zeile von .ini Datei
    File inifile; // Datei

    if (!LITTLEFS.begin(true)) // LITTLEFS Starten
    {
        serial_d_printF("ERR setup_read_file> Filesystem could not be mounted\n");
        return false; // Partition missing
    }

    listDir2(LITTLEFS, "/", 0); // Lists the files so you can see what is in the SPIFFS

    if (!LITTLEFS.exists(setupFileName2)) // setup.ini  vorchanden?
    {
        serial_d_printf("setup_read_file> %s not found, create one\n", setupFileName2);
        File DataFile = LITTLEFS.open(setupFileName2, "w"); // wenn nicht dann Anlegen
        DataFile.println("#INI neu angelegt bitte editieren");
        DataFile.close();
    }
    // LITTLEFS.open(setupFileName, "r");
    inifile = LITTLEFS.open(setupFileName2, "r"); // WiFiManager INI-Datei offnen
    serial_d_printf("setup_read_file> reading %s\n", setupFileName2);
    int i = 0;
    while (inifile.available())
    {
        line = inifile.readStringUntil('\n'); // Zeilenweise lesen
        setup_read_line(i++, line.c_str());   // und in der Funktion analyzeZeile bearbeiten
    }
    inifile.close();
    serial_d_printf("setup_read_file> %s processed\n", setupFileName2);
    setup_show_data();
    return true; // WiFiManager INI -Datei erfolgreih gelesen
} // end of function

//-------------------------------------------------------------------------------------------------------------------------------------------
// analog read of voltage
// Note: does only work if wifi is not active
void readVoltage()
{
    int vref = 1100;
    uint16_t v = analogRead(setupGPIO.P_ADC_EN);
    battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
    serial_d_printf("readVoltage> Voltage %2f\n", battery_voltage);
} // end of function

//-------------------------------------------------------------------------------------------------------------------------------------------
// print and publish curretn voltage
void showVoltage()
{
    char buf[10];
    serial_d_printf("showVoltage> Voltage %2f\n", battery_voltage);
    sprintf(buf, "%2f", battery_voltage);
    mqtt_pub_tele("Voltage", buf);
} // end of function

//-------------------------------------------------------------------------------------------------------------------------------------------
// save current radio station so radio will tune to this statin after reboot
void save_audio_preferences()
{
    preferences.begin("iotsharing", false);                       /* Start a namespace "iotsharing"in Read-Write mode */
    preferences.putUInt("radioPresetNo", au.radioCurrentStation); /* Store preset to the Preferences */
    preferences.putUInt("radioVolume", au.radioCurrentVolume);    /* Store preset to the Preferences */
    preferences.end();                                            /* Close the Preferences */
} // end of function

//-------------------------------------------------------------------------------------------------------------------------------------------
// retrieve last radio station before reboot
void setup_audio_preferences()
{
    /* Start a namespace "iotsharing"in Read-Write mode: set second parameter to false Note: Namespace name is limited to 15 chars */
    preferences.begin("iotsharing", false);

    /* get value of key "reset_times", if key not exist return default value 0 in second argument Note: Key name is limited to 15 chars too */
    unsigned int reset_times = preferences.getUInt("reset_times", 0);
    au.radioCurrentStation = preferences.getUInt("radioPresetNo", 0);
    au.radioCurrentVolume = preferences.getUInt("radioVolume", 10);
    Serial.printf("audio::setup_preferences> Preset %d\n", au.radioCurrentStation);
    Serial.printf("audio::setup_preferences> Volume %d\n", au.radioCurrentVolume);

    /* we have just reset ESP then increase reset_times */
    reset_times++;
    Serial.printf("audio::setup_preferences> Number of restart times: %d\n", reset_times);

    /* Store reset_times to the Preferences */
    preferences.putUInt("reset_times", reset_times);

    /* Close the Preferences */
    preferences.end();
} // end of function

//-----------------------------------------------------------------------------------------
// set I2S aduio interface and start streaming
audio_data_struct *setup_audio()
{
    setup_audio_preferences();

    Serial.println("setup_audio> begin");
    mqtt_pub_tele("Station", "");
    mqtt_pub_tele("Title", "");
    audio.setPinout(setupGPIO.P_I2S_BCLK, setupGPIO.P_I2S_LRCK, setupGPIO.P_I2S_DATA);
    station_select(au.radioCurrentStation);

    return &au;
} // end of function

//-----------------------------------------------------------------------------------------
// standard audio loop
// timer is used when new preset is selected by rotaray encoder
void loop_audio()
{
    audio.loop();
    if (Serial.available())
    { // put streamURL in serial monitor
        audio.stopSong();
        String r = Serial.readString();
        r.trim();
        if (r.length() > 5)
            audio.connecttohost(r.c_str());
        log_i("free heap=%i", ESP.getFreeHeap());
    }

    currentMillisAudioLoop = millis();
    if (currentMillisAudioLoop - previousMillisRotary > intervalRotaryLoop)
    { // after a few seconds change mode of rotary back to volume
        if (!au.radioRotaryVolume)
        {
            Serial.printf("audio::loop_audio> au.radioRotaryVolume %o (req:%o)\n", au.radioRotaryVolume, true);
            //au.radioRotaryVolume = true;
            //Serial.printf(" -> %o", au.radioRotaryVolume);
            rotary_onButtonClick();
            //Serial.printf(" -> %o\n", au.radioRotaryVolume);
            station_apply_preselect();
        }
        previousMillisRotary = millis();
    }
} // end of function
