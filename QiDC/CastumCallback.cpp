#include "CastumCallback.h"
#include "Timeout.h"
#include "Arduino.h"

static void cb_do(void *ref){
	CastumCallback *tag=(CastumCallback *)ref;
	(*tag->callback)(tag->buf,tag->blen);
}
static void cb_chksum(void *ref){
  CastumCallback *tag=(CastumCallback *)ref;
  (*tag->callback)(tag->buf,-tag->blen);
}
static void cb_timeout(void *ref){
	CastumCallback *tag=(CastumCallback *)ref;
  tag->blen=tag->bptr;
	tag->bptr=tag->ev_timeout=0;
	Serial.println("Castum packet break detected");
  (*tag->callback2)(tag->buf,tag->blen);
}
static void cb_loop(void *ref){
	CastumCallback *tag=(CastumCallback *)ref;
	if(tag->serial->available()){
		if(tag->ev_timeout!=0) Timeout.clear(tag->ev_timeout);
		unsigned char a=tag->serial->read();
		int p=tag->bptr;
		if(tag->bptr<tag->bsize) tag->buf[tag->bptr++]=a;
		if(p==0){ //waiting STX
			if(a!=2){
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
        tag->blen=p+1;
				if(a==256-tag->csum){//sum check ok
					Timeout.set(tag,cb_do,0);
				}
        else{
          Timeout.set(tag,cb_chksum,0);
        }
				tag->bptr=tag->ev_timeout=0;
        tag->ev_timeout=Timeout.set(tag,cb_timeout,tag->timer);
			}
      else{
        tag->csum+=a;
        tag->ev_timeout=Timeout.set(tag,cb_timeout,tag->timer);
      }
    }
	}
	tag->ev_loop=Timeout.set(tag,cb_loop,0);
}
CastumCallback::CastumCallback(Stream *se,void (*cb)(char *,int),void (*cb2)(char *,int)):StreamCallback(se,32,cb,50){
	csum=0;//receive packet pointer
  callback2=cb2;
  Timeout.clear(ev_loop);
  ev_loop=Timeout.set(this,cb_loop,0);
}
