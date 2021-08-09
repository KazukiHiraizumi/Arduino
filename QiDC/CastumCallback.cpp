#include "CastumCallback.h"
#include "TimeoutMacro.h"
#include <Arduino.h>

static void cb_do(void *ref){
	CastumCallback *tag=(CastumCallback *)ref;
	(*tag->callback)(tag->buf,tag->blen);
}
static void cb_chksum(void *ref){
  CastumCallback *tag=(CastumCallback *)ref;
  (*tag->callback)(tag->buf,-1);
}
static void cb_loop(void *ref){
  CastumCallback *tag=(CastumCallback *)ref;
  if(tag->serial->available()){
    long dt=micros()-tag->timestamp;
    if(dt>tag->timeout) tag->bptr=0;
  	do{
  		unsigned char a=tag->serial->read();
  		int p=tag->bptr;
	  	if(tag->bptr<tag->bsize) tag->buf[tag->bptr++]=a;
		  if(p==0){ //waiting STX
			  if(a!=2){
				  tag->bptr=tag->csum=0;
			  }
        else{
          tag->csum=a;
        }
  		}
      else if(p<6){
        tag->csum+=a;
      }
  		else{
  			int len=tag->buf[5];
	  		if(p==6+len){//end of packet
          tag->blen=p+1;
		  		if(a==256-tag->csum){//sum check ok
  					Timeout.set(tag,cb_do,-1); //force calling in the next loop
//          cb_do(tag);
  				}
          else{
            Timeout.set(tag,cb_chksum,-1); //force calling in the next loop
//          cb_chksum(tag);
          }
          tag->bptr=tag->csum=0;
          break;
			  }
        else{
          tag->csum+=a;
        }
      }
	  } while(tag->serial->available());
    tag->timestamp=micros();
  }
//	tag->ev_loop=Timeout.set(tag,cb_loop,0);
}
CastumCallback::CastumCallback(HardwareSerial *se,void (*cb)(char *,int)){
  serial=se;
  callback=cb;
  csum=0;//receive packet pointer
  bsize=sizeof(buf);
  bptr=blen=csum=0;
  timeout=10000;
  timestamp=0;
//  ev_loop=Timeout.set(this,cb_loop,0);
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
  ((HardwareSerial *)serial)->begin(230400);
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
  ((HardwareSerial *)serial)->begin(230400);
  serial->write(hdr,10);
}
