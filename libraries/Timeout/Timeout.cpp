#include "Timeout.h"
#include "Arduino.h"

#define TBLEN 20

struct TimeoutTab{
  int key;
  long timeout;
  void *object;
  void *func;
};

TimeoutClass::TimeoutClass(void){
  tbl=new TimeoutTab[TBLEN];
  for(int i=0;i<TBLEN;i++) tbl[i].func=tbl[i].object=NULL;
}
long TimeoutClass::set(TimeoutCallback f,int ms){
	set(NULL,(TimeoutCallbackP)f,ms);
}
long TimeoutClass::set(void *obj,TimeoutCallbackP f,int ms){
  long now=micros();
  long tout=now+((long)ms<<10);
  int n=TBLEN;
  TimeoutTab *et=tbl;
  for(;;n--,et++){
    if(et->func==NULL) break;
    long diff=tout-et->timeout;
    if(diff<0){
      memmove(et+1,et,sizeof(TimeoutTab)*(n-1));
      break;
    }
  }
  et->object=obj;
  et->func=f;
  et->key=(int)now;
  et->timeout=tout;
  return now;
}
void TimeoutClass::clear(long d){
  int n=this->lookup(d);
  if(n<0) return;
  TimeoutTab *et=tbl+n;
  memmove(et,et+1,sizeof(TimeoutTab)*(TBLEN-n-1));
}
int TimeoutClass::lookup(long d){
  TimeoutTab *et=tbl;
  int n=0;
  for(;n<TBLEN;n++,et++){
    if(et->key==d) return n;
  }
  return -1; //not found
}
void TimeoutClass::spinOnce(void){
  TimeoutTab *et=tbl;
  if(et->func==NULL) return;
  long now=micros();
  long diff=et->timeout-now;
  if(diff<0){
  	void *cb=et->func;
  	void *obj=et->object;
	memmove(et,et+1,sizeof(TimeoutTab)*(TBLEN-1));
  	if(obj==NULL) (*(TimeoutCallback)cb)();
    else (*(TimeoutCallbackP)cb)(obj);
  }
}

TimeoutClass Timeout;
