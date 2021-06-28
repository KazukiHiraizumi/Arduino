#include "StreamCallback.h"
#include "Timeout.h"
#include "Arduino.h"

static void cb_timeout(void *ref){
	StreamCallback *tag=(StreamCallback *)ref;
	tag->blen=tag->bptr;
	Timeout.set(tag,[](void *t){
		StreamCallback *tag=(StreamCallback *)t;
		(*tag->callback)(tag->buf,tag->blen);
	},0);
	tag->bptr=tag->ev_timeout=0;
}
static void cb_loop(void *ref){
	StreamCallback *tag=(StreamCallback *)ref;
	if(tag->serial->available()){
		if(tag->ev_timeout!=0) Timeout.clear(tag->ev_timeout);
		tag->buf[tag->bptr++]=tag->serial->read();
		tag->buf[tag->bptr]=0;
		tag->ev_timeout=Timeout.set(tag,cb_timeout,tag->timer);
	}
	tag->ev_loop=Timeout.set(tag,cb_loop,0);
}
StreamCallback::StreamCallback(Stream *se,int sz,void (*cb)(char *,int),int t){
	callback=cb;
	serial=se;
	ev_timeout=0;
	ev_loop=Timeout.set(this,cb_loop,0);
	buf=new char[sz];
	timer=t;
	bptr=blen=0;
}
