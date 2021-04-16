/*
  myXmodule.h - Library for mqtt client

  inject two lines in main.cpp
    #include <myXmodule.h>
    myXmodule myXmodule1;

*/
#ifndef MYBLEIPCONNECTOR_H
#define MYBLEIPCONNECTOR_H

#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include <WiFi.h>

// Configure BLE IDs
#define SERVICE_CONNECTOR_UUID "e7a4645e-8475-4193-8929-b45bcd60bfec"
#define CHARACTERISTIC_IP_UUID "bd976edf-a32a-4689-9ddb-2e25c19f9f8a"

void setup_ble_ip_connector(char* name);
void update_ble_ip_broadcast();

#endif
