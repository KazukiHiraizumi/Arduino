#include <Timeout.h>
#include <StreamCallback.h>
#include <SoftwareSerial.h>
#include "IcmCallback.h"
#include "ICM_20948.h"  // Click here to get the library: http://librarymanager/All#SparkFun_ICM_20948_IMU
ICM_20948_I2C IMU;  // using ICM_20948 as IMU
#define RX3 9
#define TX3 11
SoftwareSerial Serial3(RX3,TX3);

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
  Serial.println(F("Spresense+Haptics+IMU proto"));
  while(Serial.available()) Serial.read();// Make RX buffer empty    
  Serial.println(F("Press any key to continue..."));
  while(!Serial.available()) ;
//Init peripherals
  new StreamCallback(&Serial2,[](char *s){
    Serial.print(F("Msg:"));
    Serial.println(s);
  });
  new IcmCallback(&IMU,[](double *d){
    Serial.print(F("Pose:"));
    Serial.println(d[0]);
  });
}

void loop(){
  Timeout.spinOnce();
}
