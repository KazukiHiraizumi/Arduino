#include "IcmCallback.h"
#include "Timeout.h"
#include "Arduino.h"
#include <ICM_20948.h>

#define BUFLEN 20

static void cb_trigger(void *ref){
	IcmCallback *tag=(IcmCallback *)ref;
  tag->ev_trigger=0;
  (*tag->callback)(tag->data);
}
static void cb_loop(void *ref){
	IcmCallback *tag=(IcmCallback *)ref;
}
IcmCallback::IcmCallback(ICM_20948 *dev,CallbackDoublePtr cb){
	callback=cb;
	icm=dev;
	ev_trigger=0;
	ev_loop=Timeout.set(this,cb_loop,0);
	data=new double[BUFLEN];
}
