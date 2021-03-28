# ESP32-Radio-TTGO
ESP32 Internet Radio with Display 

## Introduction
Existing radios or loudspeakers will be used in order to get decent sound quality.
The display should show relevant information and the font size should not be too small.

Most of the time you just want to switch the device on and listen to the music.
There are only two things you want to change: volume and radio station.
Nevertheless there is only one dial to cover both functions.

In addition there is a webserver including song covers as well as mqtt to allow integration into home automation.

## Setup
### WiFi Manager
Upon first boot, the wifi  manager will start and ask for wifi credntials as well as mqtt setting.
### Radio Stations
10 stations can be configured using the four parameters below.
- RadioName = 1Live
- RadioURL = http://wdr-1live-live.icecast.wdr.de/wdr/1live/live/mp3/128/stream.mp3
- RadioTitleSeperator = -
- RadioTitleFirst = Artist

| Token     | Comment    |
| :---------- | :----- |
| RadioName  | A short name to be shown onthe display |
| RadioURL  | url of the mp3 stream |
| RadioTitleSeperator  | Some stations are using '/' or '-' to split artist and title |
| RadioTitleFirst  | Indicate if order is Title/Artist or Artist/Title |


## Hardware

![TTGO-1](README/images/ESP_Radio_1.jpg)

Despite of the TTGO you need the audio decoder and a rotary encoder.

### Digital / Analog Converter – DAC
The PCM5102 is using the I2S interface.

| PCM5102     | ESP    |
| :---------- | :----- |
| P_I2S_LRCK  | Pin 25 |
| P_I2S_BCLK  | Pin 26 |
| P_I2S_DATA  | Pin 22 |


In addition you need to connect the following pins:

| PCM5102 | Wiring      | Comment |
| :------ | :---------- |:----- |
| FLT     | GND         | Filter Select. GND for normal latency|
| DEMP    | GND        | De-Emphasis control. Low = off|
| SMT     | GND        | Audio format. Low = I2S|
| XMT     | 10K -> 3V3:| Mute: pulled via 10k Resistor to 3,3V to un-mute|

### Rotary Encoder

| Encoder     | ESP    |
| :---------- | :----- |
| P_ENC0_A  | Pin 12 |
| P_ENC0_B  | Pin 13 |
| P_ENC0_BTN  | Pin 15 |
| P_ENC0_PWR  | Pin 2 |


## Software 

### WiFi Manager



# PlatformIO

### LittleFS

### TFT_eSPI
