/*
  myDisplay.h - Library for mqtt client
*/
#ifndef MYDISPLAY_h
#define MYDISPLAY_h

#include "Arduino.h"

#include <User_Setups/Setup25_TTGO_T_Display.h>
// #define USER_SETUP_LOADED

#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>

#include "main.h"

class myDisplay // define class
{
public:
    myDisplay(void); // constructor, which is used to create an instance of the class
    bool begin(void);
    void clear(void);
    void loop(void);
    int value(void);
    void print(const char *msg);
    void println(const char *msg);
    void getScreenSize(void);
    int version(void);
    void Text(const char *co2Char, const char *tempChar);
    void Gui1(audio_data_struct *sData, wifi_data_struct *wData);
    void Gui2(audio_data_struct *sData);
    void Gui0();
    void Gui3(audio_data_struct *sData);
    void Gui4(wifi_data_struct *wData);
    void Gui5(wifi_data_struct *wData);

  private:
    TFT_eSPI myTFT = TFT_eSPI(); // Invoke custom library
    unsigned long timer_output_0 = 0,
                  timer_output_5 = 5000;
    int printlines = 0;
    int16_t tft_h = 1;
    int16_t tft_w = 1;
    void wfiSignal(int my_x, int my_y, int my_max, int level);
    void myFont1(String fontname);
};

#endif
