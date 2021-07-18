#include "ESP32_BLE.h"
#include "Timeout.h"
#include "Arduino.h"

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
  callback(this,true);
}
void BleSvCallback::onDisconnect(BLEServer*){
  callback(this,false);
}

BleCReCallback::BleCReCallback(BLEService *sv,char *ch_uuid,int l,void (*cb)(BleCReCallback*)){
  blesv = sv;
  buf=new uint8_t[length=l];
  memset(buf,'R',length);
  pCharacteristic = blesv->createCharacteristic(ch_uuid,BLECharacteristic::PROPERTY_READ|BLECharacteristic::PROPERTY_NOTIFY);
//  pCharacteristic = blesv->createCharacteristic(ch_uuid,BLECharacteristic::PROPERTY_READ);
  pCharacteristic->setValue(buf,length);
  pCharacteristic->setCallbacks(this);
  callback=cb;
}
void BleCReCallback::onRead(BLECharacteristic*){
  callback(this);
}

BleCWrCallback::BleCWrCallback(BLEService *sv,char *ch_uuid,int l,void (*cb)(BleCWrCallback*)){
  blesv = sv;
  buf=new uint8_t[length=l];
  memset(buf,'W',length);
  pCharacteristic = blesv->createCharacteristic(ch_uuid,BLECharacteristic::PROPERTY_WRITE);
  pCharacteristic->setValue(buf,length);
  pCharacteristic->setCallbacks(this);
  callback=cb;
}
void BleCWrCallback::onWrite(BLECharacteristic*){
  callback(this);
}
