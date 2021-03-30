#include "AiEsp32RotaryEncoder.h"
#include "Arduino.h"

#include "main.h"

/*
connecting Rotary encoder

Rotary encoder side    MICROCONTROLLER side  
-------------------    ---------------------------------------------------------------------
CLK (A pin)            any microcontroler intput pin with interrupt -> in this example pin 32
DT (B pin)             any microcontroler intput pin with interrupt -> in this example pin 21
SW (button pin)        any microcontroler intput pin with interrupt -> in this example pin 25
GND - to microcontroler GND
VCC                    microcontroler VCC (then set ROTARY_ENCODER_VCC_PIN -1) 

***OR in case VCC pin is not free you can cheat and connect:***
VCC                    any microcontroler output pin - but set also ROTARY_ENCODER_VCC_PIN 25 
                        in this example pin 25

lip_deps =
 igorantolic/Ai Esp32 Rotary Encoder 


*/
#define ROTARY_ENCODER_A_PIN 12
#define ROTARY_ENCODER_B_PIN 13
#define ROTARY_ENCODER_BUTTON_PIN 15
#define ROTARY_ENCODER_VCC_PIN 2 /* 27 put -1 of Rotary encoder Vcc is connected directly to 3,3V; else you can use declared output pin for powering rotary encoder */

//depending on your encoder - try 1,2 or 4 to get expected behaviour
//#define ROTARY_ENCODER_STEPS 1
//#define ROTARY_ENCODER_STEPS 2
#define ROTARY_ENCODER_STEPS 4

//instead of changing here, rather change numbers above
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

bool ignoreNextChange = false;

int audio_rotary_button(void);
void audio_rotary_rotation(bool dirUp);

//-------------------------------------------------------------------------------------------------------
void rotary_onButtonClick()
{
    int rotaryPos = 0;
    static unsigned long lastTimePressed = 0;
    //ignore multiple press in that time milliseconds
    if (millis() - lastTimePressed > 500)
    {
        lastTimePressed = millis();
        Serial.println("rotary_onButtonClick> button pressed");
        rotaryPos = audio_rotary_button();
        rotaryEncoder.reset(rotaryPos); // this will trigger rotaryEncoder.encoderChanged()
        Serial.printf("rotary_onButtonClick> rotaryPos: %d\n", rotaryPos);
        ignoreNextChange = true;
    }
} // end of function

//-------------------------------------------------------------------------------------------------------
void rotary_loop()
{
    static int readOldVal = 0;
    static int readVal = 0;

    //dont print anything unless value changed
    if (rotaryEncoder.encoderChanged())
    {
        if (!ignoreNextChange)
        {
            readOldVal = readVal;
            readVal = rotaryEncoder.readEncoder();
            Serial.print("rotary_onButtonClick> Value: ");
            Serial.println(readVal);
            if (readVal > readOldVal)
                audio_rotary_rotation(true);
            else
                audio_rotary_rotation(false);
        }
        else
            ignoreNextChange = false;
    }
} // end of function

//-------------------------------------------------------------------------------------------------------
void setup_rotary()
{
    //we must initialize rotary encoder
    rotaryEncoder.begin();

    rotaryEncoder.setup(
        [] { rotaryEncoder.readEncoder_ISR(); },
        [] { rotary_onButtonClick(); });

    //set boundaries and if values should cycle or not
    //in this example we will set possible values between 0 and 1000;
    bool circleValues = false;
    rotaryEncoder.setBoundaries(0, 20, circleValues); //minValue, maxValue, circleValues true|false (when max go to min and vice versa)
    rotaryEncoder.reset(AUDIO_DEFAULT_VOLUME);

    /*Rotary acceleration introduced 25.2.2021.
   * in case range to select is huge, for example - select a value between 0 and 1000 and we want 785
   * without accelerateion you need long time to get to that number
   * Using acceleration, faster you turn, faster will the value raise.
   * For fine tuning slow down.
   */
    rotaryEncoder.disableAcceleration(); //acceleration is now enabled by default - disable if you dont need it
    // rotaryEncoder.setAcceleration(250); //or set the value - larger number = more accelearation; 0 or 1 means disabled acceleration
} // end of function

//-------------------------------------------------------------------------------------------------------
void loop_rotary()
{
    rotary_loop();
} // end of function
