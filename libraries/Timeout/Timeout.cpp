#include <freertos/FreeRTOS.h>
#include "Timeout.h"
#include "Arduino.h"

SemaphoreHandle_t xMutex = NULL;

struct TimeoutTab{
  long timeout;
  uint8_t msg[8];
  int msglen;
  TimeoutCallback func;
  TimeoutTab(){
  	func=NULL;
    timeout=0;
    memset(msg,0,sizeof(msg));
    msglen=0;
  }
  TimeoutTab& operator =(TimeoutTab& t){
  	func=t.func;
    timeout=t.timeout;
    memcpy(msg,t.msg,sizeof(msg));
    msglen=t.msglen;
  }
};

TimeoutClass::TimeoutClass(void){
  tbl=new TimeoutTab[20];
  xMutex = xSemaphoreCreateMutex();
}
long TimeoutClass::set(TimeoutCallback f,int ms){
  return set(NULL,-1,(TimeoutCallbackPN)f,ms);
}
long TimeoutClass::set(char *s,TimeoutCallbackP f,int ms){
  return set((uint8_t *)s,0,(TimeoutCallbackPN)f,ms);
}
long TimeoutClass::set(uint8_t *s,int l,TimeoutCallbackPN f,int ms){
  const TickType_t xTicksToWait=1000UL;
  BaseType_t xStatus = xSemaphoreTake(xMutex, xTicksToWait);
  long now=micros();
  long tout=now+(((long)ms)<<10);
  int n=0;
  for(;;n++){
    if(tbl[n].func==NULL) break;
    long diff=tout-tbl[n].timeout;
    if(diff<0){
      TimeoutTab tt1=tbl[n],tt2=tbl[n+1];
      for(int i=n+1;;i++){
        tt2=tbl[i];
        tbl[i]=tt1;
      	if(tbl[i].func==NULL) break;
        tt1=tt2;
      }
      break;
    }
  }
  tbl[n].func=(TimeoutCallback)f;
  tbl[n].timeout=tout;
  tbl[n].msglen=l;
  if(s!=NULL){
  	if(l==0) strcpy((char *)tbl[n].msg,(char *)s);
  	else memcpy(tbl[n].msg,s,l);
  }
  xSemaphoreGive(xMutex);
  return tout;
}
int TimeoutClass::clear(long d){
  const TickType_t xTicksToWait=1000UL;
  BaseType_t xStatus = xSemaphoreTake(xMutex, xTicksToWait);
  int n=this->lookup(d);
  if(n>=0){
    for(int i=n;;i++){
      tbl[i]=tbl[i+1];
      if(tbl[i].func==NULL) break;
    }
  }
  xSemaphoreGive(xMutex);
  return n;
}
int TimeoutClass::lookup(long d){
  TimeoutTab *et=tbl;
  for(int n=0;;n++,et++){
    if(et->func==NULL) return -n-1;
    else if(et->timeout==d) return n;
  }
}
void TimeoutClass::spinOnce(void){
  const TickType_t xTicksToWait=1000UL;
  BaseType_t xStatus = xSemaphoreTake(xMutex, xTicksToWait);
  TimeoutTab et=tbl[0];
  long now=micros();
  long diff=et.timeout-now;
  if(diff<0){
    for(int i=0;;i++){
      tbl[i]=tbl[i+1];
      if(tbl[i].func==NULL) break;
    }
  }
  else et.func=NULL;
  xSemaphoreGive(xMutex);
  if(et.func!=NULL){
    if(et.msglen>0) (*(TimeoutCallbackPN)et.func)(et.msg,et.msglen);
    else if(et.msglen==0) (*(TimeoutCallbackP)et.func)((char *)et.msg);
    else (*(TimeoutCallback)et.func)();
  }
}
