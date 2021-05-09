#include <Timeout.h>
#include <StreamCallback.h>
#include <SoftwareSerial.h>
#include "IcmDriver.h"
#include "HapDriver.h"

//IMU module definition
#include "ICM_20948.h"  // Click here to get the library: http://librarymanager/All#SparkFun_ICM_20948_IMU
#define IMU_BUS Wire
#define IMU_ADDS  1
ICM_20948_I2C IMU;

//Haptics module definition
#define HAP_RX 9
#define HAP_TX 11
SoftwareSerial HAP_BUS(HAP_RX,HAP_TX);

//BLE module definition
#define BLE_BUS Serial2

//Drivers
HapDriver *HAPD;
IcmDriver *IMUD;
StreamCallback *BLED;

void setup() {
//Initializing dios
  pinMode(HAP_RX,INPUT);
  pinMode(HAP_TX,OUTPUT);
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
//Initializing Serial ports
  Serial.begin(115200);
  BLE_BUS.begin(115200);
  HAP_BUS.begin(115200);
  Serial.println("Spresense+Haptics+IMU proto");
//Setting callback for Haptics
  HAPD=new HapDriver(&HAP_BUS,[](char *s){
    Serial.print("HAP->");
    Serial.println(s);
    BLE_BUS.print(s);
  });
//Setting callback for BLE module
  BLED=new StreamCallback(&BLE_BUS,[](char *s){
    Serial.print("BLE->");
    Serial.println(s);
    int mk=s[0];
    if(mk=='S') HAPD->parse(s);  //S:Sense command for Haptics module
    else if(mk=='M') IMUD->parse(s); //M: for IMU module
//    else if(mk=='G') GPSD->parse(s); //G: for GPS module
  });
//Setting callback for IMU module
  IMUD=new IcmDriver(&IMU,&IMU_BUS,IMU_ADDS,[](double *d){
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
static int n_loop=0;
void loop(){
  int m=(n_loop>>10)%6;
  switch(m){
    case 0:
      digitalWrite(LED0,HIGH);
      digitalWrite(LED1,LOW);
      break;
    case 1:
      digitalWrite(LED1,HIGH);
      digitalWrite(LED0,LOW);
      break;
    case 2:
      digitalWrite(LED2,HIGH);
      digitalWrite(LED1,LOW);
      break;
    case 3:
      digitalWrite(LED3,HIGH);
      digitalWrite(LED2,LOW);
      break;
    case 4:
      digitalWrite(LED2,HIGH);
      digitalWrite(LED3,LOW);
      break;
    case 5:
      digitalWrite(LED1,HIGH);
      digitalWrite(LED2,LOW);
      break;
  }
  n_loop++;
  Timeout.spinOnce();
}
