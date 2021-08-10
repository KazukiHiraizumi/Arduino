#include "ESP32_BLE.h"
#include <Arduino.h>
#include <BLE2902.h>

BleSvCallback::BleSvCallback(char *device_name,char *service_uuid,void (*cb)(BleSvCallback *,bool f)){
  BLEDevice::init(device_name);
  pServer=BLEDevice::createServer();
  pServer->setCallbacks(this);
  pService=pServer->createService(uuid=service_uuid);
  pAdvertising=NULL;
  callback=cb;
}
void BleSvCallback::aon(void){
  if(pAdvertising==NULL){
    pService->start();
    pAdvertising=BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(uuid);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
  }
  BLEDevice::startAdvertising();
  Serial.println("Start Advertising");
}
void BleSvCallback::aoff(void){
  BLEDevice::stopAdvertising();
  Serial.println("Stop Advertising");
}
void BleSvCallback::onConnect(BLEServer*){
  callback(this,connect=true);
}
void BleSvCallback::onDisconnect(BLEServer*){
  callback(this,connect=false);
}

BleCReCallback::BleCReCallback(BLEService *sv,char *ch_uuid,void (*cb)(BleCReCallback*)){
  blesv = sv;
  pCharacteristic = blesv->createCharacteristic(ch_uuid,BLECharacteristic::PROPERTY_READ|BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->setCallbacks(this);
  pCharacteristic->addDescriptor(new BLE2902());
  callback=cb;
}
void BleCReCallback::onRead(BLECharacteristic*){
  callback(this);
}
void BleCReCallback::onNotify(BLECharacteristic *){
  callback(this);
}
void BleCReCallback::onStatus(BLECharacteristic*, Status s, uint32_t code){
  Serial.print("status cb ");
  Serial.println(s);
}

BleCWrCallback::BleCWrCallback(BLEService *sv,char *ch_uuid,void (*cb)(BleCWrCallback*)){
  blesv = sv;
  pCharacteristic = blesv->createCharacteristic(ch_uuid,BLECharacteristic::PROPERTY_WRITE);
  pCharacteristic->setCallbacks(this);
  callback=cb;
}
void BleCWrCallback::onWrite(BLECharacteristic*){
  callback(this);
}
