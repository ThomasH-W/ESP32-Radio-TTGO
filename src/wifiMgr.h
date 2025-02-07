#ifndef _WIFI_MGR_FUNC_H
#define _WIFI_MGR_FUNC_H

// url will be [ip-address]/update
#define OTA_USER "radio"
#define OTA_PASS "update"

void setup_wifi(void);
void loop_wifi(void);
void showVoltage(void);
void readVoltage(void);

void mqtt_pub_tele(const char *topic, const char *message);
void mqtt_pub_stat(const char *topic, const char *message);
void mqtt_pub_cmnd(const char *topic, const char *message);

#endif
