#include "Timeout.h"
//#include <StreamCallback.h>
#include <esp_wifi.h>
#include "ESP32_BLE.h"
#include "CastumCallback.h"

#define DEVICE_NAME "Shimano"
#define SERVICE_UUID "10014246-f5b0-e881-09ab-42000ba24f83"
#define CH_ADDS_UUID "20024246-f5b0-e881-09ab-42000ba24f83"
#define CH_WOUT_UUID "20054246-f5b0-e881-09ab-42000ba24f83"

//Task & Job controls
//Callbacks
TimeoutClass *Timeout;
BleSvCallback *cb0;
BleCReCallback *cb1;
//StreamCallback *cb2;
CastumCallback *cb3;
BleCWrCallback *cb4;
//Jobs
uint32_t tm_romdump=0;
uint32_t tm_powon=0;
uint32_t tm_romwrite=0;
uint32_t tm_wavdump=0;

//copy of rom
uint8_t rom[256];
uint16_t rom_range=0;
uint16_t rom_idx=0;
//wave data
uint16_t wav_tmsec[1000]; //elapsed time
uint16_t wav_dt[1000];
int16_t wav_tens[1000];
uint16_t wav_pwm[1000];
uint16_t wav_idx=0;
uint16_t wav_range=0;

//declararion of utility functions
static uint16_t chendian(uint16_t b);
static void cb_powon();
static void cb_powoff();
static void mkwav();

static void cb_romdump(){
  uint8_t buf[10];
  Serial.printf("romdump %d\n",rom_idx);
  buf[0]=0xB0;
  buf[1]=rom_idx;
  int i=2;
  for(;i<sizeof(buf) && rom_idx<rom_range;i++,rom_idx++) buf[i]=rom[rom_idx];
  cb1->pCharacteristic->setValue(buf,i);
  cb1->pCharacteristic->notify();
}
static void cb_wavdump(){
  uint8_t buf[10];
  Serial.printf("wavdump %d\n",wav_idx);
  memset(buf,0,10);
  long srev=0,stm=0,sdt=0,spwm=0,stens=0;
  for(int i=0;;){
    srev=wav_idx;
    stm=wav_tmsec[wav_idx-1];
    sdt+=wav_dt[wav_idx];
    spwm+=wav_pwm[wav_idx];
    stens+=wav_tens[wav_idx];
    wav_idx++;
    i++;
    if(wav_idx>=wav_range || i>=3){
      uint16_t *p=(uint16_t *)buf;
      p[0]=chendian(srev);
      p[1]=chendian(stm);
      p[2]=chendian(sdt/i);
      p[3]=chendian(spwm/i);
      p[4]=chendian(stens/i);
      break;
    }
  }
  cb1->pCharacteristic->setValue(buf,10);
  cb1->pCharacteristic->notify();
}

void setup(){
  esp_wifi_stop();
  setCpuFrequencyMhz(50);
  Serial.begin(115200);
  Serial.println("setup");
//  Serial2.begin(230400);
//  Serial2.setRxBufferSize(2048);

  Timeout=new TimeoutClass;

  cb0=new BleSvCallback(DEVICE_NAME,SERVICE_UUID,[](BleSvCallback *,bool connect){
    if(connect){//connect event
      Serial.println("Connected");
    }
    else{//disconnect event
      Serial.println("Disconnected");
      Timeout->set([](){
        cb0->aon();
      },3000);
    }
  });
  cb1=new BleCReCallback(cb0->pService,CH_WOUT_UUID,[](BleCReCallback *){
    Serial.println("Read/Notif callback");
    if(tm_romdump){
      if(rom_idx<rom_range) tm_romdump=Timeout->set(cb_romdump,20);
      else tm_romdump=0;
    }
    else if(tm_wavdump){
      if(wav_idx<wav_range) tm_wavdump=Timeout->set(cb_wavdump,20);
      else tm_wavdump=0;
    }
  });
/*  cb2=new StreamCallback(&Serial,100,[](char *s,int){
    Serial.println("Serial recv"); //ech
  },5)*/
  cb3=new CastumCallback(&Serial2,[](uint8_t *s,int l){ //receive callback
    if(l<=0){
      Serial.printf("Castum protocol error %d\n",l);
    }
    else if(l>=9){//minimum bytes in a packet
      int adds;
      uint8_t buf[10];
      switch(s[6]){
      case 2://upload rom data
        Timeout->clear(tm_romdump);
        adds=s[7];
        rom[adds]=s[8];
//        Serial.printf("rom load %d %d\n",adds,rom[adds]);
        if(rom_range<adds+1) rom_range=adds+1;
        tm_romdump=Timeout->set(cb_romdump,500);
        break;
      case 3://rom dump end
        Serial.println("Rom dump end");
        break;
      case 0x12: //rom write end
        Timeout->clear(tm_romwrite);
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
        if(tm_wavdump==0) wav_range=wav_idx=0;
        else Timeout->clear(tm_wavdump);
        wav_tmsec[wav_range]=(((uint32_t)s[2]<<16)+((uint16_t)s[3]<<8)+(uint8_t)s[4])>>8;
        wav_dt[wav_range]=((uint16_t)s[7]<<8)+s[8];
        wav_pwm[wav_range]=(uint16_t)s[10];
        wav_tens[wav_range]=((int16_t)(int8_t)s[13]<<8)+(uint8_t)s[14];
//        Serial.printf("wrec %d %d %d %d\n",wav_range,wav_dt[wav_range],wav_pwm[wav_range],wav_tens[wav_range]);
        wav_range++;
        tm_wavdump=Timeout->set(cb_wavdump,300);
        break;
      default:
        Serial.printf("Castum receive other %x\n",s[6]);
      }
    }
  });
  cb4=new BleCWrCallback(cb0->pService,CH_ADDS_UUID,[](BleCWrCallback *p){
    std::string s=p->pCharacteristic->getValue();
    if(s.size()==3 && s.at(0)==0xA0){//Request Rom Write
      uint8_t buf[3];
      if(tm_powon) Timeout->clear(tm_powon);
      else cb_powon();
      buf[0]=s.at(1);
      buf[1]=s.at(2);
      Timeout->set(buf,2,[](uint8_t *s,int){
        Serial.printf("Send rom write %d %d\n",s[0],s[1]);
        cb3->write3(0x11,s[0],s[1]);//Castum command,param1(adds),param2(value)
        tm_romwrite=Timeout->set([](){Serial.println("Rom write failed"); },100);
      },50);
      tm_powon=Timeout->set(cb_powoff,500);
    }
    else if(s.size()==2 && s.at(0)==0xFA && s.at(1)==0x01){//Request Rom Dump
      Serial.println("Req rom dump");
      if(tm_powon) Timeout->clear(tm_powon);
      else cb_powon();
      Timeout->set([](){
        rom_range=rom_idx=0;
        cb3->write2(1,0);//Castum command,param
        tm_romdump=Timeout->set(cb_romdump,100);
      },50);
      tm_powon=Timeout->set(cb_powoff,500);
    }
    else if(s.size()==2 && s.at(0)==0xFC && s.at(1)==0x01){//Request Wave Dump
      Serial.println("Req wav dump");
//      mkwav();
      wav_idx=0;
      tm_wavdump=Timeout->set(cb_wavdump,0);
    }
  });
  Timeout->set([](){
    cb0->aon(); //Advertise
  },1000);
}
void loop(){
  Timeout->spinOnce();
}

//Bodies of functions
static void mkwav(){
  long CT_dt=3000;
  long CT_time=CT_dt;
  long CT_pwm=100;
  long CT_tens=200;
  for(int i=0;i<500;i++){
    wav_tmsec[i]=CT_time>>10;
    wav_dt[i]=CT_dt;
    wav_pwm[i]=CT_pwm;
    wav_tens[i]=CT_tens;
    CT_time+=CT_dt;
    CT_dt=CT_dt*257>>8;
    CT_tens=CT_tens-1;
  }
  wav_range=500;
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
static uint16_t chendian(uint16_t b){
  uint16_t a;
  uint8_t *p=(byte *)&a;
  p[0]=b>>8;
  p[1]=b&0xff;
  return a;
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
