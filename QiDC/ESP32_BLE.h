#ifndef _ESP32_BLE_h
#define _ESP32_BLE_h

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

class BleSvCallback:public BLEServerCallbacks{
public:
  BLEServer *pServer;
  BLEService *pService;
  BLEAdvertising *pAdvertising;
  char *uuid;
  void (*callback)(BleSvCallback *,bool);
  BleSvCallback(char *device_name,char *service_uuid,void (*cb)(BleSvCallback*,bool));
  void aon(void);  //advertising on
  void aoff(void);  //advertising off
  void onConnect(BLEServer* pServer);
  void onDisconnect(BLEServer* pServer);
};

class BleCReCallback:public BLECharacteristicCallbacks {
public:
  uint8_t *buf;
  int length;
  BLEService *blesv;
  void (*callback)(BleCReCallback *);
  BLECharacteristic *pCharacteristic;
  BleCReCallback(BLEService *,char *characteristic_uuid,int len,void (*cb)(BleCReCallback *));
  void onRead(BLECharacteristic* pCharacteristic);
};

class BleCWrCallback:public BLECharacteristicCallbacks {
public:
  uint8_t *buf;
  int length;
  BLEService *blesv;
  void (*callback)(BleCWrCallback *);
  BLECharacteristic *pCharacteristic;
  BleCWrCallback(BLEService *,char *characteristic_uuid,int len,void (*cb)(BleCWrCallback *));
  void onWrite(BLECharacteristic* pCharacteristic);
};

#endif
