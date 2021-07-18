#include <Timeout.h>
#include <StreamCallback.h>
#include "ESP32_BLE.h"
#include "CastumCallback.h"

#define DEVICE_NAME "Shimano"
#define SERVICE_UUID "10014246-f5b0-e881-09ab-42000ba24f83"
#define CH_ADDS_UUID "20024246-f5b0-e881-09ab-42000ba24f83"
#define CH_WOUT_UUID "20054246-f5b0-e881-09ab-42000ba24f83"

byte param[120];
word param_idx;
long tusec[2000]; //time in usec
word velocity[2000];
short tension[2000];

byte cmd[]={2,0,0,0,0,2,1,0,0xFB};

//Callbacks
BleSvCallback *cb0;
BleCReCallback *cb1;
void *cb2,*cb3,*cb4;
//Timeouts
long tm_notif=0;

static void cb_notif(){
  static uint32_t seq=0;
  static uint8_t buf[4];
  seq++;
  Serial.print("setVal ");
  Serial.print(seq);
  Serial.print(" ");
  Serial.println(tm_notif);
  buf[0]=seq&0xff;
  buf[1]=(seq>>8)&0xff;
  buf[2]=(seq>>16)&0xff;
  buf[3]=(seq>>24)&0xff;
  cb1->pCharacteristic->setValue(buf,4);
  cb1->pCharacteristic->notify();
//  tm_notif=Timeout.set(cb_notif,1000);
}
void setup(){
  memset(param,0,sizeof(param));
  Serial.begin(115200);
  Serial2.begin(230400);
  Serial.println("Qi start");

  cb0=new BleSvCallback(DEVICE_NAME,SERVICE_UUID,[](BleSvCallback *,bool connect){
    if(connect){//connect event
      Serial.println("Connected");
      tm_notif=Timeout.set(cb_notif,10000);
    }
    else{//disconnect event
      Serial.println("Disconnected");
      Timeout.clear(tm_notif);
      Timeout.set([](){
        cb0->aon();
      },3000);
    }
  });
  cb1=new BleCReCallback(cb0->pService,CH_WOUT_UUID,20,[](BleCReCallback *){
    Serial.println("Read callback");
  });
  cb2=new StreamCallback(&Serial,100,[](char *s,int){
    Serial.println("Rec"); //echo
    Serial2.write(cmd,9);
  },5);
  cb3=new CastumCallback(&Serial2,[](char *s,int l){ //receive callback
    if(l>=9){//minimum bytes in a packet
      int adds;
      Serial.print("cmd ");
      Serial.print(s[6]);
      Serial.print("(");
      Serial.print(l);
      Serial.print(") ");
      switch(s[6]){
      case 2:
        adds=s[7];
        param[adds]=s[8];
        Serial.print(adds);
        Serial.print(" ");
        Serial.println(param[adds]);
        break;
      default:
        Serial.println("");
      }
    }
    else{
      Serial.print("Checksum error ");
      Serial.println(l);
    }
  },[](char *,int l){ //result callback
    if(l==0){
      Serial.println("Multi-packets received");
    }
    else{
      Serial.println("Packet receive timeout");
    }
  });
  cb4=new BleCWrCallback(cb0->pService,CH_ADDS_UUID,20,[](BleCWrCallback *p){
    Serial.print("Write callback:");
    std::string s=p->pCharacteristic->getValue();
    for(int i=0;i<s.size();i++){
      Serial.print(int(s.at(i)));
      Serial.print(" ");
    }
    Serial.println("");
/*    int a,b,i;
    std::string s=p->pCharacteristic->getValue();
    unsigned long dat=0;
    Serial.print("CWr ");
    for(int i=0;i<s.size();i++){
      dat+=s.at(i);
      dat<<=8;
      Serial.print(s.at(i));
      Serial.print(" ");
    }
    Serial.println("");*/
  });

  Timeout.set([](){
    Serial.println("Qi adv");
    cb0->aon();
  },1000);
}
void loop(){
  Timeout.spinOnce();
}

/*
        VSO_puts("ADS=");VSO_putlx(dat);VSO_cr();
        if(dat==0xFC01){//Request to send wave data
            CT_send();
        }
        else if(dat==0xFA01){//Request to send ROM data
            unsigned char dat=0;
            power_on();
            Timer[3]=10;//Power-off timer
            USART2_cmd(1,&dat,1);
        }
        else if((dat&0xF00000)==0xA00000){
            unsigned char buf[2];
            buf[1]=dat&0xFF;
            buf[0]=(dat>>8)&0xFF;
            power_on();
            Timer[3]=5;
            USART2_cmd(0x11,buf,2);
        }
    }
*/
