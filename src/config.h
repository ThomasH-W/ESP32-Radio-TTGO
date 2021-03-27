#ifndef _CONFIG_H
#define _CONFIG_H

#define WIFI_SETUP_FILE "/setup.ini"

struct SetupGPIO
{
    int P_I2S_LRCK = 255;
    int P_I2S_BCLK = 255;
    int P_I2S_DATA = 255;

    int P_ENC0_A = 255;
    int P_ENC0_B = 255;
    int P_ENC0_BTN = 255;
    int P_ENC0_PWR = 255;

    int P_ADC_BAT = 255;
    int P_ADC_EN = 255;
};

struct SetupRadio
{
    char RadioName[20] = "";
    char RadioURL[200] = "";
    char RadioTitleSeperator[2] = ":";
    bool RadioArtistFirst = true;
};

// --------------------- wifi config
#define WIFI_AP_SSID "ESP32-AP"
#define WIFI_AP_PASS "password"
//#define WIFI_CONFIG_FILE "/config.json"

#define MQTT_CLIENTID "ESP32_6666"
#define MQTT_BROKER "192.168.178.20"
#define MQTT_PORT "1883"

#endif
