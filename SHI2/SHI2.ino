#include <Timeout.h>
#include <StreamCallback.h>
#include <SoftwareSerial.h>
#include "IcmDriver.h"

//IMU module definition
#include "ICM_20948.h"
#define IMU_BUS Wire
#define IMU_ADDS  1
ICM_20948_I2C IMU;

//Drivers
IcmDriver *IMUD;

//globals
long tmnow;

void setup() {
  tmnow=millis();
//Initializing dios
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
//Initializing Serial ports
  Serial.begin(115200);
//Setting callback for Serial
  new StreamCallback(&Serial,[](char *s){
    Serial.print("SER->");
    Serial.println(s);
    int mk=s[0];
    if(mk=='M') IMUD->parse(s); //M: for IMU module
  });
//Setting callback for IMU module
  IMUD=new IcmDriver(&IMU,&IMU_BUS,IMU_ADDS,[](double *d){
    long t=millis();
    if(t-tmnow<100) return;
    Serial.print("[");
    Serial.print(d[0]);
    Serial.print(",");
    Serial.print(d[1]);
    Serial.print(",");
    Serial.print(d[2]);
    Serial.print(",");
    Serial.print(d[3]);
    Serial.println("]");
    tmnow=t;
  });
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
