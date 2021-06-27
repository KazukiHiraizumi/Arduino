#include "StreamCallback.h"
#include "Timeout.h"
#include "Arduino.h"

static void cb_trigger(void *ref){
	StreamCallback *tag=(StreamCallback *)ref;
	tag->blen=tag->bptr;
	Timeout.set(tag,[](void *t){
		StreamCallback *tag=(StreamCallback *)t;
		(*tag->callback)(tag->buf,tag->blen);
	},0);
	tag->bptr=tag->ev_trigger=0;
}
static void cb_loop(void *ref){
	StreamCallback *tag=(StreamCallback *)ref;
	if(tag->serial->available()){
		if(tag->ev_trigger!=0) Timeout.clear(tag->ev_trigger);
		tag->buf[tag->bptr++]=tag->serial->read();
		tag->buf[tag->bptr]=0;
		tag->ev_trigger=Timeout.set(tag,cb_trigger,tag->timeout);
	}
	tag->ev_loop=Timeout.set(tag,cb_loop,0);
}
StreamCallback::StreamCallback(Stream *se,int sz,void (*cb)(char *,int),int t){
	callback=cb;
	serial=se;
	ev_trigger=0;
	ev_loop=Timeout.set(this,cb_loop,0);
	buf=new char[sz];
	timeout=t;
	bptr=blen=0;
}
