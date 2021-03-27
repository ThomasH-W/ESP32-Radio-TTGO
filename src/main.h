/*
  main.h - Library for mqtt client
*/
#ifndef MAIN_h
#define MAIN_h

// https://github.com/Locoduino/RingBuffer

#include "Arduino.h"
#include "FS.h"
#include "LITTLEFS.h" //this needs to be first, or it all crashes and burns...
#define FORMAT_LITTLEFS_IF_FAILED true
#define FlashFS LittleFS

#define AUDIO_DEFAULT_VOLUME 12

enum States
{
    ST_BOOT,  // 0 pure text
    ST_GUI_1, // 1 Time/WLAN -- CO2 -- Temp/Humi
    ST_GUI_2, // 2 Volume
    ST_GUI_3  // 2 Preset
};

enum Update
{
    UP_INFO,   // 0 stream info
    UP_VOLUME, // 1 volume changed
    UP_PRESET  // 2 new station
};

enum audio_modes
{
    AUDIO_STOP,
    AUDIO_START,
    AUDIO_MUTE,
    AUDIO_VOLUME,
    AUDIO_VOLUME_UP,
    AUDIO_VOLUME_DOWN,
    AUDIO_PRESET_UP,
    AUDIO_PRESET_DOWN
};

struct audio_data_struct
{
    int radioCurrentVolume = 0;
    int radioCurrentStation = 0;
    int radioNextStation = 0;
    bool radioMute = false;
    char radioBitRate[10];
    char radioName[50];
    char radioNextName[50];
    char radioTitle[100];
    char radioSongTitle[100];
    char radioArtist[100];
    char radioTitleSeperator = ':';
    bool radioArtistFirst = true;
    bool radioRotaryVolume = true;
    bool preSelect = false;
    int update = UP_INFO;
};

struct wifi_data_struct
{
    char rssiChar[20]; // wifi signal strength
    int rssiLevel = 0; // wifi signal strength
    char ssidChar[20]; // SSID - wlan name
    char IPChar[20];   // SSID - wlan name
};

audio_data_struct *setup_audio(void);
wifi_data_struct *setup_wifi_info(void);
void pub_wifi_info(void);
void main_displayUpdate(bool clearScreen);
void displayReset(void);
void displayDebugPrint(const char *message);
void displayDebugPrintln(const char *message);
void rotary_onButtonClick(void);
void audio_ws_meta(void);
void audio_ws_tuner(void);
void wsSendTuner(int presetNo, int volume);
void wsSendArtistTitle(char *Artist, char *SongTitle);
uint32_t memoryInfo();

#endif
