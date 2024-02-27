/*
  File : myDisplay.cpp
  Date : 26.02.2024 - update for LittleFS

  handler for display

https://github.com/Bodmer/TFT_eSPI

*/
#include "Arduino.h"
#include "myDisplay.h"
#include "main.h"

// ------------------------------------------------------------------------------------------------------------------------
// Class members
// constructor, which is used to create an instance of the class
myDisplay::myDisplay(void)
{

} // end of function

// ------------------------------------------------------------------------------------------------------------------------
bool myDisplay::begin(void)
{
    Serial.print("Display> begin() ... ");
    myTFT.init();
    myTFT.setRotation(3); // 0-portarit, 1-landscape, 2-flip_portrait, 3-flip_landscape
    getScreenSize();
    myTFT.fillScreen(TFT_BLACK);

    myTFT.setCursor(0, 0, 2);
    myTFT.setTextColor(TFT_WHITE, TFT_BLACK);
    myTFT.setTextSize(1);
    Serial.println(" complete");
    return true;
} // end of function

// ------------------------------------------------------------------------------------------------------------------------
void myDisplay::clear(void)
{
    Serial.println("myDisplay::clear");
    myTFT.fillScreen(TFT_BLACK);
    myTFT.setCursor(0, 0, 2);
    printlines = 1;
} // end of function

// ------------------------------------------------------------------------------------------------------------------------
void myDisplay::getScreenSize(void)
{
    tft_w = myTFT.width();
    tft_h = myTFT.height();
    Serial.printf("Screen size %u * %u", tft_w, tft_h);
} // end of function

// ------------------------------------------------------------------------------------------------------------------------
void myDisplay::println(const char *msg)
{
    bool errorFlag = strncmp(msg, "ERR", 3);
    if (errorFlag)
        myTFT.setTextColor(TFT_WHITE, TFT_BLACK); // TFT_WHITE
    else
        myTFT.setTextColor(TFT_RED, TFT_BLACK);

    if (++printlines > 8)
    {
        myTFT.fillScreen(TFT_BLACK);
        myTFT.setCursor(0, 0, 2);
        printlines = 1;
    }
    myTFT.println(msg);
} // end of function

// ------------------------------------------------------------------------------------------------------------------------
void myDisplay::print(const char *msg)
{
    bool errorFlag = strncmp(msg, "ERR", 3);
    if (errorFlag)
        myTFT.setTextColor(TFT_WHITE, TFT_BLACK); // TFT_WHITE
    else
        myTFT.setTextColor(TFT_RED, TFT_BLACK);
    myTFT.print(msg);
} // end of function

// ------------------------------------------------------------------------------------------------------------------------
/*
  version() returns the version of the library:
*/
int myDisplay::value(void)
{
    return 1; // temp_ds
} // end of function

// ------------------------------------------------------------------------------------------------------------------------
/*
  version() returns the version of the library:
*/
int myDisplay::version(void)
{
    return 1;
} // end of function

// ------------------------------------------------------------------------------------------------------------------------
void myDisplay::Text(const char *co2Char, const char *tempChar)
{
    myTFT.setTextColor(TFT_WHITE, TFT_BLACK);
} // end of function

// ------------------------------------------------------------------------------------------------------------------------
void myDisplay::wfiSignal(int my_x, int my_y, int my_max, int level) // 100,100,22
{
    // int my_x = 100;
    // int my_y = 100;
    int my_w = 3;
    int my_step = my_w + 5;
    // int my_i = 4; // 3 * 4 + 10 = 12+10 // my_i = (max -10) / 3
    int my_i = (my_max - 10) / 3;

    if (level > 0)
        myTFT.fillRect(my_x + 0 * my_step, my_y - 10, my_w, 10, TFT_WHITE); // width / height of rectangle in pixels
    if (level > 1)
        myTFT.fillRect(my_x + 1 * my_step, my_y - 10 - 1 * my_i, my_w, 10 + 1 * my_i, TFT_WHITE); // width / height of rectangle in pixels
    if (level > 2)
        myTFT.fillRect(my_x + 2 * my_step, my_y - 10 - 2 * my_i, my_w, 10 + 2 * my_i, TFT_WHITE); // width / height of rectangle in pixels
    if (level > 3)
        myTFT.fillRect(my_x + 3 * my_step, my_y - 10 - 3 * my_i, my_w, 10 + 3 * my_i, TFT_WHITE); // width / height of rectangle in pixels

} // end of function

// ------------------------------------------------------------------------------------------------------------------------
void myDisplay::myFont1(String fontname)
{
    if (LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED))
    {
        Serial.print("file system mounted\n");
        myTFT.fillScreen(TFT_BLACK);
        myTFT.loadFont(fontname, LittleFS);
        for (size_t i = 1; i <= 6; i++)
        {
            myTFT.setTextSize(i); // change la taille
            myTFT.setCursor(0, 0);
            String txt = "Size ";
            txt += i;
            txt += " ";
            txt += fontname;
            Serial.print("Display");
            Serial.println(txt);
            myTFT.println(txt);
            delay(1000);
            myTFT.fillScreen(TFT_BLACK);
        }
        myTFT.unloadFont();
        delay(2000);
    }
}

// ------------------------------------------------------------------------------------------------------------------------
// 0 dummy / testing
void myDisplay::Gui0()
{
    /*
    Listing directory: /
  FILE: /Final-Frontier-28.vlw  SIZE: 25287  LAST WRITE: 1970-01-01 00:00:00
  FILE: /NotoSansBold15.vlw  SIZE: 10766  LAST WRITE: 1970-01-01 00:00:00
  FILE: /NotoSansBold36.vlw  SIZE: 44169  LAST WRITE: 1970-01-01 00:00:00
  FILE: /NotoSansMonoSCB20.vlw  SIZE: 15382  LAST WRITE: 1970-01-01 00:00:00
    */
    myFont1("Final-Frontier-28");
    myFont1("NotoSansBold15");
    myFont1("NotoSansBold36");
} // end of function

// ------------------------------------------------------------------------------------------------------------------------
// 1 Default - Song/Artist
void myDisplay::Gui1(audio_data_struct *aData, wifi_data_struct *wData)
{
    char buf[100];
    // 240 * 135
    myTFT.fillScreen(TFT_BLACK);
    myTFT.setTextColor(TFT_WHITE, TFT_BLACK);

    wfiSignal(200, 133, 18, wData->rssiLevel); // x=100, y=100, max=22

    myTFT.drawString(aData->radioArtist, 2, 30, 4);    // string, x,y, font
    myTFT.drawString(aData->radioSongTitle, 2, 75, 4); // string, x,y, font

    myTFT.drawLine(1, 100, tft_w, 100, TFT_WHITE); // xs,ys,xe,ye,col);

    int yInfo = 122;
    sprintf(buf, "Vol %d", aData->radioCurrentVolume);
    if (aData->radioMute)
    {
        myTFT.setTextColor(TFT_RED, TFT_BLACK);
        myTFT.drawString("mute   ", 2, yInfo, 2); // string, x,y, font
        // https://github.com/Bodmer/TFT_eSPI/blob/master/examples/Generic/ESP32_SDcard_jpeg/ESP32_SDcard_jpeg.ino
        myTFT.setTextColor(TFT_WHITE, TFT_BLACK);
    }
    else
    {
        myTFT.drawString(buf, 2, yInfo, 2); // string, x,y, font
    }

    // myTFT.drawString(aData->radioBitRate, 60, yInfo, 2); // string, x,y, font

    // show time from ntp
    Serial.printf("myDisplay::Gui1 drawstring >%s<\n", wData->timeOfDayChar);
    myTFT.drawString(wData->timeOfDayChar, 60, yInfo, 2); // string, x,y, font

    // show current station
    // sprintf(buf, "%d - %s", aData->radioCurrentStation + 1, aData->radioName);
    // myTFT.drawString(buf, 95, yInfo, 2); // string, x,y, font
    myTFT.drawString(aData->radioName, 110, yInfo, 2); // string, x,y, font

    printlines = 0;
} // end of function

// ------------------------------------------------------------------------------------------------------------------------
// 2 Volume
void myDisplay::Gui2(audio_data_struct *aData)
{
    char buf[100];
    // 240 * 135
    // Find centre of screen
    uint16_t x = tft_w / 2;
    uint16_t y = tft_h / 2;

    myTFT.fillScreen(TFT_WHITE);
    myTFT.setTextColor(TFT_BLACK, TFT_WHITE);
    myTFT.setTextSize(1);

    if (aData->radioMute)
    {
        sprintf(buf, "mute");
        Serial.printf("myDisplay::Gui2 drawstring >%s<\n", buf);
        myTFT.setTextDatum(MC_DATUM);   // Set datum to Middle Right
        myTFT.setTextSize(2);           // size 1; font 4
        myTFT.drawString(buf, x, y, 4); // string, x,y, font
    }
    else
    {
        sprintf(buf, "%d", aData->radioCurrentVolume);
        Serial.printf("myDisplay::Gui2 drawstring >%s<\n", buf);
        myTFT.setTextDatum(MR_DATUM);     // Set datum to Middle Right
        myTFT.drawString(buf, 170, y, 8); // string, x,y, font

        // empty box
        myTFT.drawLine(40, 110, 200, 110, TFT_BLUE);  // xs, ys, xe,ye, color
        myTFT.drawLine(40, 130, 200, 130, TFT_BLUE);  // xs, ys, xe,ye, color
        myTFT.drawLine(40, 110, 40, 130, TFT_BLUE);   // xs, ys, xe,ye, color
        myTFT.drawLine(200, 110, 200, 130, TFT_BLUE); // xs, ys, xe,ye, color

        // fill box
        // map(value, fromLow, fromHigh, toLow, toHigh)
        int val = map(aData->radioCurrentVolume, 0, 20, 1, 158);
        myTFT.fillRect(41, 111, val, 18, TFT_BLUE); // x,y ,w,h, color
    }

    myTFT.setTextDatum(ML_DATUM);
    myTFT.setTextSize(1);
} // end of function

// ------------------------------------------------------------------------------------------------------------------------
// 3 Preset
void myDisplay::Gui3(audio_data_struct *aData)
{
    char buf[100];
    // 240 * 135
    // uint16_t x = tft_w / 2;
    uint16_t y = tft_h / 2;

    myTFT.fillScreen(TFT_BLUE);
    myTFT.setTextColor(TFT_WHITE, TFT_BLUE);
    myTFT.setTextSize(1);

    sprintf(buf, "%d", aData->radioNextStation + 1);
    Serial.printf("myDisplay::Gui3 drawstring PRESET>%s<\n", buf);
    // myTFT.setTextDatum(TR_DATUM); // Set datum to Middle Right
    // myTFT.drawString(buf, tft_w - 10, 20, 6); // string, x,y, font
    myTFT.setTextDatum(ML_DATUM);     // Set datum to Middle Right
    myTFT.drawString(buf, 5, 27, 6); // string, x,y, font

    myTFT.setTextDatum(ML_DATUM); // Set datum to Middle Right
    sprintf(buf, "%s", aData->radioNextName);
    Serial.printf("myDisplay::Gui3 drawstring NAME  >%s<\n", buf);
    myTFT.setTextSize(2);                // size 2; font 4
    myTFT.drawString(buf, 20, y + 12, 4); // string, x,y, font

    myTFT.setTextDatum(ML_DATUM);
    myTFT.setTextSize(1);
} // end of function

// ------------------------------------------------------------------------------------------------------------------------
// 4 show wifi info
void myDisplay::Gui5(wifi_data_struct *wData)
{
    // 240 * 135
    // myTFT.fillScreen(TFT_BLACK);
    myTFT.setTextColor(TFT_WHITE, TFT_BLACK);

    //wfiSignal(aData.rssiLevel);
    wfiSignal(200, 30, 18, wData->rssiLevel); // x=100, y=100, max=22

    myTFT.drawString(wData->ssidChar, 3, 30, 4); // string, x,y, font

    myTFT.drawString("IP", 3, 65, 4);          // string, x,y, font
    myTFT.drawString(wData->IPChar, 40, 65, 4); // string, x,y, font

    printlines = 0;
} // end of function

// ------------------------------------------------------------------------------------------------------------------------
// 6 clock
void myDisplay::Gui4(wifi_data_struct *wData)
{
    // 240 * 135
    uint16_t yInfo = tft_h / 2;
    
    myTFT.fillScreen(TFT_BLACK);
    myTFT.setTextColor(TFT_WHITE, TFT_BLACK);

    // show time from ntp
    Serial.printf("myDisplay::Gui4 drawstring >%s<\n", wData->timeOfDayChar);
    myTFT.drawString(wData->timeOfDayChar, 0, yInfo, 8); // string, x,y, font

    myTFT.drawString("IP", 3, 130, 2);                  // string, x,y, font
    myTFT.drawString(wData->IPChar, 20, 130, 2);        // string, x,y, font

    printlines = 0;
} // end of function
