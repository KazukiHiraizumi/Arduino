#include "CastumCallback.h"
#include "Timeout.h"
#include "Arduino.h"

static void cb_do(void *ref){
	CastumCallback *tag=(CastumCallback *)ref;
	(*tag->callback)(tag->buf,tag->blen);
}
static void cb_timeout(void *ref){
	CastumCallback *tag=(CastumCallback *)ref;
	tag->bptr=tag->ev_timeout=0;
	Serial.println("Castum timeout");
}
static void cb_loop(void *ref){
	CastumCallback *tag=(CastumCallback *)ref;
	if(tag->serial->available()){
		if(tag->ev_timeout!=0) Timeout.clear(tag->ev_timeout);
		unsigned char a=tag->serial->read();
		int p=tag->bptr;
		tag->buf[tag->bptr++]=a;
		if(p==0){
			if(a!=2){ //STX error
				tag->bptr=tag->ev_timeout=tag->csum=0;
			}
      else{
        tag->csum=a;
        tag->ev_timeout=Timeout.set(tag,cb_timeout,tag->timer);
      }
		}
    else if(p<6){
      tag->csum+=a;
      tag->ev_timeout=Timeout.set(tag,cb_timeout,tag->timer);
    }
		else{
			int len=tag->buf[5];
			if(p==6+len){//end of packet
				if(a==256-tag->csum){//sum check ok
					tag->blen=p+1;
					Timeout.set(tag,cb_do,0);
				}
        else{
          Serial.println("Castum chksum error");
        }
				tag->bptr=tag->ev_timeout=0;
			}
      else{
        tag->csum+=a;
        tag->ev_timeout=Timeout.set(tag,cb_timeout,tag->timer);
      }
    }
	}
	tag->ev_loop=Timeout.set(tag,cb_loop,0);
}
CastumCallback::CastumCallback(Stream *se,void (*cb)(char *,int)):StreamCallback(se,20,cb,10){
	csum=0;//receive packet pointer
  Timeout.clear(ev_loop);
  ev_loop=Timeout.set(this,cb_loop,0);
}
