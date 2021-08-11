#include "CastumCallback.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include "esp_task_wdt.h"

static TaskHandle_t taskHandle = NULL;

static void cb_loop(void *ref){
  CastumCallback *tag=(CastumCallback *)ref;
LOOP:
 	int a=tag->serial->read();
  if(a<0){
    esp_task_wdt_reset();
    delay(1);
    goto LOOP;
  }
  long dt=micros()-tag->timestamp;
  if(dt>tag->timeout) tag->bptr=0;
	int p=tag->bptr;
  if(tag->bptr<tag->bsize) tag->buf[tag->bptr++]=a;
	if(p==0){ //waiting STX
	  if(a!=2){
		  tag->bptr=tag->csum=0;
	  }
    else{
      tag->csum=a;
//      Serial.print("STX");
    }
	}
  else if(p<6){
    tag->csum+=a;
//    if(p==5) Serial.printf("LEN=%d",a);
  }
	else{
//    if(p==6) Serial.printf("CMD=%d",a);
		int len=tag->buf[5];
  	if(p==6+len){//end of packet
      tag->blen=p+1;
	 		if(a==256-tag->csum){//sum check ok
        (*tag->callback)(tag->buf,tag->blen); //check sum ok
  		}
      else{
        (*tag->callback)(tag->buf,-tag->blen); //check sum error
      }
      tag->bptr=tag->csum=0;
    }
    else{
      tag->csum+=a;
    }
  }
  tag->timestamp=micros();
  goto LOOP;
}
CastumCallback::CastumCallback(HardwareSerial *se,void (*cb)(uint8_t *,int)){
  serial=se;
  callback=cb;
  csum=0;//receive packet pointer
  bsize=sizeof(buf);
  bptr=blen=csum=0;
  timeout=10000;
  timestamp=0;
  serial->begin(230400);
  serial->setTimeout(1000);
  xTaskCreate(cb_loop,"CSTM",2048,this,1,&taskHandle);
}
void CastumCallback::update(){ cb_loop(this);}
void CastumCallback::write2(int b1,int b2){
  byte hdr[10];
  byte cs=0;
  cs+=hdr[0]=2; //STX
  cs+=hdr[1]=0; //stamp
  cs+=hdr[2]=0; //stamp 
  cs+=hdr[3]=0; //stamp
  cs+=hdr[4]=0; //stamp
  cs+=hdr[5]=2; //length
  cs+=hdr[6]=b1;//command
  cs+=hdr[7]=b2;//param
  hdr[8]=256-cs;//checksum
  serial->begin(230400);
  serial->setTimeout(1000);
  serial->write(hdr,9);
}
void CastumCallback::write3(int b1,int b2,int b3){
  byte hdr[10];
  byte cs=0;
  cs+=hdr[0]=2; //STX
  cs+=hdr[1]=0; //stamp
  cs+=hdr[2]=0; //stamp 
  cs+=hdr[3]=0; //stamp
  cs+=hdr[4]=0; //stamp
  cs+=hdr[5]=3; //length
  cs+=hdr[6]=b1;//command
  cs+=hdr[7]=b2;//param
  cs+=hdr[8]=b3;//param
  hdr[9]=256-cs;//checksum
  serial->begin(230400);
  serial->setTimeout(1000);
  serial->write(hdr,10);
}
