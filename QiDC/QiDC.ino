#include "TimeoutMacro.h"
//#include <StreamCallback.h>
#include <esp_wifi.h>
#include "ESP32_BLE.h"
#include "CastumCallback.h"

#define DEVICE_NAME "Shimano"
#define SERVICE_UUID "10014246-f5b0-e881-09ab-42000ba24f83"
#define CH_ADDS_UUID "20024246-f5b0-e881-09ab-42000ba24f83"
#define CH_WOUT_UUID "20054246-f5b0-e881-09ab-42000ba24f83"

TimeoutMacro Timeout;

//copy of rom
byte rom[256];
word rom_range=0;
word rom_idx=0;
//wave data
word wav_tmsec[1000]; //elapsed time
word wav_dt[1000];
short wav_tens[1000];
word wav_pwm[1000];
word wav_idx=0;
word wav_range=0;

//Callbacks
BleSvCallback *cb0;
BleCReCallback *cb1;
void *cb2;
CastumCallback *cb3;
void *cb4;
//Timeouts
long tm_romdump=0;
long tm_powon=0;
long tm_romwrite=0;
long tm_wavdump=0;

static void cb_romdump(){
  uint8_t buf[10];
  Serial.print("romdump ");
  Serial.println(rom_idx);
  buf[0]=0xB0;
  buf[1]=rom_idx;
  int i=2;
  for(;i<sizeof(buf) && rom_idx<=rom_range;i++,rom_idx++) buf[i]=rom[rom_idx];
  cb1->pCharacteristic->setValue(buf,i);
  cb1->pCharacteristic->notify();
}
static void cb_wavdump(){
  uint8_t buf[10];
  Serial.print("wavdump ");
  *(word *)(buf)=wav_idx; //rev
  *(word *)(buf+2)=wav_tmsec[wav_idx];
  *(word *)(buf+4)=wav_dt[wav_idx];
  *(word *)(buf+6)=wav_pwm[wav_idx];
  *(short *)(buf+8)=wav_tens[wav_idx];
  wav_idx++;
  cb1->pCharacteristic->setValue(buf,10);
  cb1->pCharacteristic->notify();
}
static void cb_powon(){
  Serial.println("power on");
  pinMode(18,OUTPUT);pinMode(19,OUTPUT);pinMode(21,OUTPUT);pinMode(23,OUTPUT);
  digitalWrite(18,HIGH);digitalWrite(19,HIGH);digitalWrite(21,HIGH);digitalWrite(23,HIGH);
}
static void cb_powoff(){
  tm_powon=0;
  Serial.println("power off");
  digitalWrite(18,LOW);digitalWrite(19,LOW);digitalWrite(21,LOW);digitalWrite(23,LOW);
//  pinMode(18,INPUT);pinMode(19,INPUT);pinMode(21,INPUT);pinMode(23,INPUT);
}
////For testing
static void mkwav(){
  long CT_dt=3000;
  long CT_time=CT_dt;
  long CT_pwm=100;
  long CT_tens=1000;
  for(int i=0;i<500;i++){
    wav_tmsec[i]=CT_time>>10;
    wav_dt[i]=CT_dt;
    wav_pwm[i]=CT_pwm;
    wav_tens[i]=CT_tens;
    CT_time+=CT_dt;
    CT_dt=CT_dt*260>>8;
    CT_tens=CT_tens-5;
  }
  wav_range=100;
}

void setup(){
  esp_wifi_stop();
  setCpuFrequencyMhz(50);
  Serial.begin(115200);
  Serial.println("setup");
  Serial2.begin(230400);
  Serial2.setRxBufferSize(2048);

  cb0=new BleSvCallback(DEVICE_NAME,SERVICE_UUID,[](BleSvCallback *,bool connect){
    if(connect){//connect event
      Serial.println("Connected");
    }
    else{//disconnect event
      Serial.println("Disconnected");
      Timeout.set([cb0](){
        cb0->aon();
      },3000,"aon");
    }
  });
  cb1=new BleCReCallback(cb0->pService,CH_WOUT_UUID,[](BleCReCallback *){
    Serial.println("Read or Notif callback");
    if(tm_romdump){
      if(rom_idx<=rom_range) tm_romdump=Timeout.set(cb_romdump,0);
      else tm_romdump=0;
    }
    else if(tm_wavdump){
      if(wav_idx<=wav_range) tm_wavdump=Timeout.set(cb_wavdump,0);
      else tm_wavdump=0;
    }
  });
/*  cb2=new StreamCallback(&Serial,100,[](char *s,int){
    Serial.println("Serial recv"); //ech
  },5)*/
  cb3=new CastumCallback(&Serial2,[](char *s,int l){ //receive callback
    if(l<=0){
      Serial.print("Castum protocol error");
      Serial.println(l);
    }
    else if(l>=9){//minimum bytes in a packet
      int adds;
      uint8_t buf[10];
      switch(s[6]){
      case 2://rom dump
        Timeout.clear(tm_romdump);
        adds=s[7];
        rom[adds]=s[8];
        if(rom_range<adds) rom_range=adds;
        tm_romdump=Timeout.set(cb_romdump,100);
        break;
      case 3://rom dump end
        Serial.println("Rom dump end");
        break;
      case 0x12: //rom write end
        Timeout.clear(tm_romwrite);
        if(s[8]){
          buf[0]=0xA0;
          buf[1]=s[7];
        }
        else{
          buf[0]=0xAF;
          buf[1]=0xFF;
        }
        cb1->pCharacteristic->setValue(buf,2);
        cb1->pCharacteristic->notify();
        break;
      case 0x22: //wave record
        break;
      default:
        Serial.print("Castum receive other ");
        Serial.println(int(s[6]),HEX);
      }
    }
  });
  cb4=new BleCWrCallback(cb0->pService,CH_ADDS_UUID,[](BleCWrCallback *p){
    static byte buf[3];
    std::string s=p->pCharacteristic->getValue();
    if(s.size()==3 && s.at(0)==0xA0){//Request Rom Write
      if(tm_powon) Timeout.clear(tm_powon);
      else cb_powon();
      buf[0]=s.at(1);
      buf[1]=s.at(2);
      Timeout.set([buf](){
        Serial.print("Send rom write ");
        Serial.print(buf[0]);
        Serial.print(" ");
        Serial.println(buf[1]);
        cb3->write3(0x11,buf[0],buf[1]);
        tm_romwrite=Timeout.set([](){Serial.println("Rom write failed"); },100);
      },50,"Rwrt");
      char s[8];
      static int seq;
      sprintf(s,"p%d",seq);
      seq++;
      tm_powon=Timeout.set(cb_powoff,500,s);
    }
    else if(s.size()==2 && s.at(0)==0xFA && s.at(1)==0x01){//Request Rom Dump
      Serial.println("Req rom dump");
      if(tm_powon) Timeout.clear(tm_powon);
      else cb_powon();
      Timeout.set([](){
        Serial.println("Send rom dump");
        rom_range=rom_idx=0;
        cb3->write2(1,0);
        tm_romdump=Timeout.set(cb_romdump,100);
      },50);
      tm_powon=Timeout.set(cb_powoff,500);
    }
    else if(s.size()==2 && s.at(0)==0xFC && s.at(1)==0x01){//Request Wave Dump
      Serial.println("Req wav dump");
      mkwav();
      wav_idx=0;
      tm_wavdump=Timeout.set(cb_wavdump,0);
    }
  });
  Timeout.set([](){
    cb0->aon(); //Advertise
  },1000);
}
void loop(){
  Timeout.spinOnce();
  cb3->update();
}
/*  int wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason){
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }*/

/*
        VSO_puts("ADS=");VSO_putlx(dat);VSO_cr();
        if(dat==0xFC01){//Request to send wave data
            CT_send();
        }
    }
*/
