#include "StreamCallback.h"
#include "Timeout.h"
#include "Arduino.h"

#define BUFLEN 20
#define LATENCY 5  //ms

void cb_rec(void *ref){
	StreamCallback *tag=(StreamCallback *)ref;
	Timeout.set(tag,[](void *t){
		StreamCallback *tag=(StreamCallback *)t;
		(*tag->callback)(tag->buf);
	},0);
	tag->bptr=tag->evrec=0;
}
void cb_tmr(void *ref){
	StreamCallback *tag=(StreamCallback *)ref;
	if(tag->serial->available()){
		if(tag->evrec!=0) Timeout.clear(tag->evrec);
		tag->buf[tag->bptr++]=tag->serial->read();
	    	tag->buf[tag->bptr]=0;
		tag->evrec=Timeout.set(tag,cb_rec,LATENCY);
	}
	tag->evtmr=Timeout.set(tag,cb_tmr,0);
}
StreamCallback::StreamCallback(Stream *se,StreamCallbackFunc cb){
	callback=cb;
	serial=se;
	evrec=0;
	evtmr=Timeout.set(this,cb_tmr,0);
	buf=new char[BUFLEN];
	bptr=0;
}
