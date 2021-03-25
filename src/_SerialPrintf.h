/*
 * Simple printf for writing to an Arduino serial port.  Allows specifying Serial..Serial3.
 * 
 * const HardwareSerial&, the serial port to use (Serial..Serial3)
 * const char* fmt, the formatting string followed by the data to be formatted
 * 
 * int d = 65;
 * float f = 123.4567;
 * char* str = "Hello";
 * serial_printf(Serial, "<fmt>", d);
 * 
 * Example:
 *   serial_printf(Serial, "Sensor %d is %o and reads %1f\n", d, d, f) will
 *   output "Sensor 65 is on and reads 123.5" to the serial port.
 * 
 * Formatting strings <fmt>
 * %B    - binary (d = 0b1000001)
 * %b    - binary (d = 1000001)  
 * %c    - character (s = H)
 * %d/%i - integer (d = 65)\
 * %f    - float (f = 123.45)
 * %3f   - float (f = 123.346) three decimal places specified by %3.
 * %o    - boolean on/off (d = On)
 * %s    - char* string (s = Hello)
 * %X    - hexidecimal (d = 0x41)
 * %x    - hexidecimal (d = 41)
 * %%    - escaped percent ("%")
 * Thanks goes to @alw1746 for his %.4f precision enhancement 
 */

#ifndef _SERIAL_PRINTF_H
#define _SERIAL_PRINTF_H

#include <HardwareSerial.h>

void serial_printf(const char *fmt, ...);
void serialx_printf(HardwareSerial &serial, const char *fmt, ...);

#define serial_begin(...) Serial.begin(__VA_ARGS__)
#define serial_print(...) Serial.print(__VA_ARGS__)
#define serial_println(...) Serial.println(__VA_ARGS__)
#define serial_printF(...) Serial.print(F(__VA_ARGS__))
#define serial_printFln(...) Serial.println(F(__VA_ARGS__)) //printing text using the F macro

#ifdef DEBUG
#define serial_d_begin(...) Serial.begin(__VA_ARGS__)
#define serial_d_print(...) Serial.print(__VA_ARGS__)
#define serial_d_println(...) Serial.println(__VA_ARGS__)
#define serial_d_printF(...) Serial.print(F(__VA_ARGS__))
#define serial_d_printFln(...) Serial.println(F(__VA_ARGS__)) //printing text using the F macro
#define serial_d_printf(...) serial_printf(__VA_ARGS__)
#define dDelay(...) delay(__VA_ARGS__)
//***************************************************************
#else
#define serial_d_begin(...)
#define serial_d_print(...)
#define serial_d_println(...)
#define serial_d_printF(...)
#define serial_d_printFln(...)
#define serial_d_printf(...)
#define serial_d_Delay(...)
#endif

#endif
