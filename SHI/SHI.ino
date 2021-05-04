#include <Timeout.h>
#include <StreamCallback.h>
#include <SoftwareSerial.h>
//from ICM20948.ino
#include "ICM_20948.h"  // Click here to get the library: http://librarymanager/All#SparkFun_ICM_20948_IMU
extern void imu_setup(ICM_20948_I2C&);
extern double imu_data[];
extern byte imu_update(ICM_20948_I2C&);

//Static objects
ICM_20948_I2C IMU;  // Otherwise create an ICM_20948_I2C object

//events
int ev_ser_rec=0;
int ev_ser_remsg=0;
int ev_imu_enable=0;

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
//Openning message
  Serial.println(F("Spresense+Haptics+IMU proto"));
  while(Serial.available()) Serial.read();// Make RX buffer empty    
  Serial.println(F("Press any key to continue..."));
  while(!Serial.available()) ;
//Init peripherals
  imu_setup(IMU);
  new StreamCallback(&Serial2,[](char *s){
    Serial.print(F("Msg:"));
    Serial.println(s);
  });
}

unsigned char ser2msg[20];
void check_event(){
//  if (IMU.status==ICM_20948_Stat_FIFOMoreDataAvail) event_push(1);
  byte imu=imu_update(IMU);
  if(imu&1) event_push(11);  //Pose data(imu_data[:3]) available
  if(imu&2) event_push(12);  //Mag data(imu_data[4:6]) available
  if(imu&4) event_push(13);
  if(imu&8) event_push(14);
  if(Serial2.available()){


  ev_ser_rec=Timeout.
    event_push(20);
  }
}
void loop(){
  Timeout.spinOnce();
  check_event();
  int ev=event_pop();
  switch(ev){
  case 11:
    POSErecv();
    break;
  case 12:
//    Serial.print(F("Mag:"));
//    Serial.println(imu_data[4]);
    break;
  case 13:
//    Serial.print(F("Com:"));
//    Serial.println(imu_data[7]);
    break;
  case 14:
//    Serial.print(F("Acc:"));
//    Serial.println(imu_data[10]);
    break;
  case 20:
    BLErecv();
  }
}
void POSErecv(){
//  Serial.print(F("Pose:"));
//  Serial.println(imu_data[0]);
}
void BLErecv(){
  delay(10);
  Serial.print(F("BLE:"));
  while(Serial2.available()){
    int incomingByte = Serial2.read();
    Serial.print(incomingByte,HEX);
    Serial.print(F(","));
  }
  Serial.println(F(""));
}
