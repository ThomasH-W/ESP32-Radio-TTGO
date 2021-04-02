/*
  File : main.cpp
  Date : 26.03.2021 

  ESP32 Internet Radio with Display based on TTGO

  ToDo:
    ESP deepsleep, wakeup
    Battery
    show picture - loudspeaker
*/

#include <Arduino.h>

// Wrapper for Serial.print
#define DEBUG true
#include "_SerialPrintf.h"
#include "_GPIO.h"

#if !(defined(ESP32))
#error This code is intended to run on the ESP8266 or ESP32 platform! Please check your Tools->Board setting.
#endif

#define LED_ON HIGH
#define LED_OFF LOW

#include "wifiMgr.h"
void setup_read_file();
void loop_audio();
void setup_rotary();
void loop_rotary();

#include "FS_tools.h"

#include "myDisplay.h"
myDisplay myDisplay1;
char statusChar[50];
wifi_data_struct wifiData;

#include "myNTP.h"
myNTP myNtp;
unsigned long lastNtp = 0;

audio_data_struct *audio_data_ptr;
wifi_data_struct *wifi_data_ptr;

bool displayUpdate = false;
int mode = 0, oldMode = 0; // based on state
unsigned long currentMillisLoop = 0, previousMillisDisplay = 0, intervalDisplayLoop = 3000;

// --------------------------------------------------------------------------
// set flag for updating display, will be recognized next time in displayLoop
void main_displayUpdate(bool clearScreen)
{
  if (audio_data_ptr)                                                       // pointer is initialized after display setup
    serial_printf("main::main_displayUpdate %d\n", audio_data_ptr->update); //audio_data_ptr->update
  else
    serial_printf("main::main_displayUpdate\n"); //audio_data_ptr->update
  displayUpdate = true;
  if (clearScreen)
    displayReset();
} // end of function

// ----------------------------------------------------------------------------------------
// if not in boot mode, print message on TFT display
void displayDebugPrint(const char *message)
{
  if (1 > mode)
    myDisplay1.print(message);
  Serial.print(message);
} // end of function

// ----------------------------------------------------------------------------------------
// if not in boot mode, print message on TFT display
void displayDebugPrintln(const char *message)
{
  if (1 > mode)
    myDisplay1.println(message);
  Serial.println(message);
} // end of function

// ---------------------------------------------------------------------------------------
// clear display, request update of current UI
void displayReset(void)
{
  serial_println("main::displayReset");
  myDisplay1.clear();
  displayUpdate = true;
} // end of function

// ---------------------------------------------------------------------------------------
// after interval, fallback to GUI1 showing mani screen
// if audio_data changed, flag update will be set
void displayLoop(void)
{
  currentMillisLoop = millis();
  if (currentMillisLoop - previousMillisDisplay > intervalDisplayLoop)
  {
    audio_data_ptr->update = UP_INFO;
    mode = ST_GUI_1;
    previousMillisDisplay = millis();
  }

  switch (audio_data_ptr->update)
  {
  case UP_INFO: // 3
    // mode = ST_GUI_1;
    break;
  case UP_VOLUME: // 3
    mode = ST_GUI_2;
    break;
  case UP_PRESET: // 4
    mode = ST_GUI_3;
    break;
  }

  if (mode != oldMode)
  {
    myDisplay1.clear();
    oldMode = mode;
    displayUpdate = true;
  }

  if (displayUpdate == true)
  {
    switch (mode)
    {
    case ST_GUI_1:                                    // 3
      pub_wifi_info();                                // get latest wifi signal strength
      myNtp.value(wifi_data_ptr->timeOfDayChar, wifi_data_ptr->dateChar);
      myDisplay1.Gui1(audio_data_ptr, wifi_data_ptr); // text
      break;
    case ST_GUI_2:                     // 4
      myDisplay1.Gui2(audio_data_ptr); // gauge left/right
      break;
    case ST_GUI_3:                     // 4
      myDisplay1.Gui3(audio_data_ptr); // gauge left/right
      break;
    }
    previousMillisDisplay = millis();
    displayUpdate = false;
  }

} // end of function

// --------------------------------------------------------------------------
void setup()
{
  serial_begin(115200);
  while (!Serial)
    ;
  delay(50);
  serial_println("-------------------------------------------------------- wifi init th");

  myDisplay1.begin(); // start display
  delay(10);
  myDisplay1.println("> Boot ...");

  setup_read_file(); // read setup
  readVoltage();     // must be done before wifi is established - conflict using ADC

  myDisplay1.print("> Firmware ");
  myDisplay1.println(FIRMWARE_VERSION);
  // myDisplay1.Gui0(); test gui using differrent fonts, files to be loaded into SPIFFS

  myDisplay1.println("> setup WiFi ...");
  setup_wifi(); // call wifimanager and establish mqtt connection
  wifi_data_ptr = setup_wifi_info();

  myDisplay1.println("> setup NTP ...");
  myNtp.begin();

  myDisplay1.println("> setup audio ...");
  audio_data_ptr = setup_audio(); // start audio

  mqtt_pub_tele("INFO", "setup complete");
  list_FS();      // debug: show files in littlefs

  setup_rotary(); // initialize rotary encoder
  myDisplay1.println("> setup complete");

  mode = ST_GUI_1; // switch to default GUI 7 main screen
} // end of function

// --------------------------------------------------------------------------
void loop()
{
  loop_wifi();
  loop_rotary();
  loop_audio();
  myNtp.loop(); 

  displayLoop();
  yield();
} // end of function
