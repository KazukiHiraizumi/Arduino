#include "StreamCallback.h"
#include "Timeout.h"
#include "Arduino.h"

#define BUFLEN 20
#define LATENCY 5  //ms

static void cb_trigger(void *ref){
	StreamCallback *tag=(StreamCallback *)ref;
	Timeout.set(tag,[](void *t){
		StreamCallback *tag=(StreamCallback *)t;
		(*tag->callback)(tag->buf);
	},0);
	tag->bptr=tag->ev_trigger=0;
}
static void cb_loop(void *ref){
	StreamCallback *tag=(StreamCallback *)ref;
	if(tag->serial->available()){
		if(tag->ev_trigger!=0) Timeout.clear(tag->ev_trigger);
		tag->buf[tag->bptr++]=tag->serial->read();
	    	tag->buf[tag->bptr]=0;
		tag->ev_trigger=Timeout.set(tag,cb_trigger,LATENCY);
	}
	tag->ev_loop=Timeout.set(tag,cb_loop,0);
}
StreamCallback::StreamCallback(Stream *se,CallbackCharPtr cb){
	callback=cb;
	serial=se;
	ev_trigger=0;
	ev_loop=Timeout.set(this,cb_loop,0);
	buf=new char[BUFLEN];
	bptr=0;
}
