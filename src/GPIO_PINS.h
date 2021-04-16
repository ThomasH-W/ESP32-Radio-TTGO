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

// Battery - analog digital converter
#define PIN_ADC_BAT 34 // #gpio32 to 39 or 255 if not used
#define PIN_ADC_EN 14  // #ADC_EN is the ADC detection enable port

// Buttons
#define PIN_BTN_1 0
#define PIN_BTN_2 35
#define PIN_BTN_3 36

#endif
