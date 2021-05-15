/*
  GPIO_PINS.h - Library for mqtt client
*/
#ifndef GPIO_PINS_h
#define GPIO_PINS_h

// I2S
#define PIN_I2S_LRCK 25
#define PIN_I2S_BCLK 26
#define PIN_I2S_DATA 22

// Rotary Encoder: swap A / B to change direction
#define PIN_ENC0_A 12   // #pin A clk
#define PIN_ENC0_B 13   // #pin B Data
#define PIN_ENC0_BTN 15 // #pin SW
#define PIN_ENC0_PWR 2  // #pin SW

// pin 34...39 input only, no PWM
// ADC ranging from 0 to 4095
// ADC1 pin 32..39 - ADC2 pins cannot be used when Wi-Fi is used
// ADC2 pin 0..26

// Battery - analog digital converter
#define PIN_ADC_BAT 39 // #gpio32 to 39 or 255 if not used
#define PIN_ADC_EN 14  // #ADC_EN is the ADC detection enable port

// Buttons
#define PIN_BTN_4 27 // preset+
#define PIN_BTN_1 0  // preset+ - onboard button
#define PIN_BTN_2 35 // mode - onboard button
#define PIN_BTN_3 36 // mode
#define PIN_BTN_5 37 // reset
#define PIN_BTN_6 32 // volume +
#define PIN_BTN_7 33 // volume -
#define PIN_BTN_8 38 // preset -

#endif
