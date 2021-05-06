#include <Timeout.h>
#include <StreamCallback.h>
#include <SoftwareSerial.h>
#include "IcmDriver.h"
#include "HapDriver.h"

//IMU module definition
#include "ICM_20948.h"  // Click here to get the library: http://librarymanager/All#SparkFun_ICM_20948_IMU
#define IMU_ADDS  1
ICM_20948_I2C IMU;  // using ICM_20948 as IMU

//Haptics module definition
#define HAP_RX 9
#define HAP_TX 11
SoftwareSerial Serial3(HAP_RX,HAP_TX);

//Drivers
HapDriver *HAPD;
IcmDriver  *ICMD;
StreamCallback *BLED;

void setup() {
//Initializing Serial ports
  pinMode(HAP_RX,INPUT);
  pinMode(HAP_TX,OUTPUT);
  Serial.begin(115200);
  Serial2.begin(115200);
  Serial3.begin(115200);
  Serial.println("Spresense+Haptics+IMU proto");
//Setting callback for Haptics
  HAPD=new HapDriver(&Serial3,[](char *s){
    Serial.print("HAP->");
    Serial.println(s);
    BLED->serial->print(s);
  });
//Setting callback for BLE module
  BLED=new StreamCallback(&Serial2,[](char *s){
    Serial.print("BLE->");
    Serial.println(s);
    if(s[0]=='S') HAPD->parse(s);
  });
//Setting callback for IMU module on I2C
  ICMD=new IcmDriver(&IMU,&Wire,IMU_ADDS,[](double *d){
/*    Serial.print("IMU->[");
    Serial.print(d[4]);Serial.print(",");
    Serial.print(d[5]);Serial.print(",");
    Serial.print(d[6]);Serial.print("]  [");
    Serial.print(d[24]);Serial.print(",");
    Serial.print(d[25]);Serial.print(",");
    Serial.print(d[26]);Serial.println("]");*/
    HAPD->setQuat(d);
  });
  HAPD->serout("F0011500255255");
  HAPD->parse("SF I255 C90");
  HAPD->parse("SF C-45");
  HAPD->parse("SF B30");
}
void loop(){
  Timeout.spinOnce();
}
