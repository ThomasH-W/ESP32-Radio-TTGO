/*
https://fipsok.de/Esp32-Webserver/ntp-zeit-Esp32.tab

lib_deps =

*/
#include "Arduino.h"
#include "myNTP.h"

// Instance of external ibrary  object
const char *const PROGMEM ntpServer[] = {"fritz.box", "de.pool.ntp.org", "at.pool.ntp.org", "ch.pool.ntp.org", "ptbtime1.ptb.de", "europe.pool.ntp.org"};
const char *const PROGMEM dayNames[] = {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"};
const char *const PROGMEM dayShortNames[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
const char *const PROGMEM monthNames[] = {"Januar", "Februar", "März", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember"};
const char *const PROGMEM monthShortNames[] = {"Jan", "Feb", "Mrz", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez"};

// ------------------------------------------------------------------------------------------------------------------------
// Class members
// constructor, which is used to create an instance of the class
myNTP::myNTP(void)
{
    // set PIN
} // end of function

// ------------------------------------------------------------------------------------------------------------------------
bool myNTP::begin(void)
{
    bool error;

    configTzTime("CET-1CEST,M3.5.0/02,M10.5.0/03", ntpServer[1]); // deinen NTP Server einstellen (von 0 - 5 aus obiger Liste)
    if (!getLocalTime(&tm))
    {
        Serial.println("NTP> Zeit konnte nicht geholt werden\n");
        error = true;
    }
    else
    {
        getLocalTime(&tm);
        Serial.println(&tm, "NTP> %A, %B %d %Y %H:%M:%S");
        sensorFound = true;
        error = false;
    }
    Serial.println(localTime());
    return error;
} // end of function

// ------------------------------------------------------------------------------------------------------------------------
char *myNTP::localTime()
{
    static char buf[20]; // je nach Format von "strftime" eventuell die Größe anpassen
    static time_t lastsec{0};
    getLocalTime(&tm, 50);
    if (tm.tm_sec != lastsec)
    {
        lastsec = tm.tm_sec;
        strftime(buf, sizeof(buf), "%d.%m.%Y %T", &tm); // http://www.cplusplus.com/reference/ctime/strftime/
        time_t now;
        if (!(time(&now) % 86400))
            begin(); // einmal am Tag die Zeit vom NTP Server holen o. jede Stunde "% 3600" aller zwei "% 7200"
    }
    return buf;
}

// ------------------------------------------------------------------------------------------------------------------------
void myNTP::loop()
{
    char tmpChar[20];

    currentMillisLoop = millis();
    if (currentMillisLoop - previousMillis > intervalLoop)
    {
        // Serial.print("t");
        if (sensorFound == true)
        {
            Serial.print("NTP-loop> ");
            // Serial.println(localTime());

            getLocalTime(&tm, 50);

            strftime(tmpChar, sizeof(tmpChar), "%R", &tm);
            Serial.print(" Time: ");
            Serial.print(ntp_data.timeOfDayChar);
            strncpy(ntp_data.timeOfDayChar, tmpChar, sizeof(tmpChar)); // dest, src, size

            strftime(tmpChar, sizeof(tmpChar), "%d.%m.", &tm);
            Serial.print(" / Date: ");
            Serial.println(tmpChar);
            strncpy(ntp_data.dateChar, tmpChar, sizeof(tmpChar)); // dest, src, size
        }
        previousMillis = millis();
    } // end of timer

    if (currentMillisLoop - previousMillisScan > intervalScanDevice)
    {
        // Serial.print("T");
        if (sensorFound == false)
        {
            begin();
        }
        previousMillisScan = millis();
    } // end of timer

    yield();
} // end of function

// ------------------------------------------------------------------------------------------------------------------------
void myNTP::value(char *bufTime, char *bufDate)
{
    char tmpChar[20];

    getLocalTime(&tm, 50);
    strftime(tmpChar, sizeof(tmpChar), "%R", &tm);
    Serial.print("NTP-Value Time: ");
    Serial.print(tmpChar);
    strncpy(bufTime, tmpChar, sizeof(tmpChar)); // dest, src, size
    // strftime(bufTime, sizeof(bufTime), "%R", &tm); // http://www.cplusplus.com/reference/ctime/strftime/

    strftime(tmpChar, sizeof(tmpChar), "%d.%m.", &tm);
    Serial.print(" / Date: ");
    Serial.println(tmpChar);
    strncpy(bufDate, tmpChar, sizeof(tmpChar)); // dest, src, size
                                                // strftime(bufDate, sizeof(bufDate), "%d.%m.", &tm); // http://www.cplusplus.com/reference/ctime/strftime/
} // end of function

// ------------------------------------------------------------------------------------------------------------------------
void myNTP::status(char *value)
{
    strcpy(value, statusChar);
} // end of function

// ------------------------------------------------------------------------------------------------------------------------
/*
  version() returns the version of the library:
*/
int myNTP::version(void)
{
    return 1;
} // end of function
