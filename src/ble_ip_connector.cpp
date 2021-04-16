#include "ble_ip_connector.h"

BLECharacteristic *pCharacteristic;

void setup_ble_ip_connector(char *name)
{
  Serial.println("Starting BLE IP Connector");
  BLEDevice::init("ESP Radio");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(BLEUUID(SERVICE_CONNECTOR_UUID));
  pCharacteristic = pService->createCharacteristic(
      BLEUUID(CHARACTERISTIC_IP_UUID),
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_CONNECTOR_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  Serial.println("Starting BLE IP advertising");
  BLEDevice::startAdvertising();
  Serial.println("BLE IP Connector started");
}

void update_ble_ip_broadcast()
{
  Serial.println("Update BLE IP Connector");
  const char* str = WiFi.localIP().toString().c_str();
  Serial.printf("Test %s \n", str);
  pCharacteristic->setValue(str);
}