#ifndef _Timeout_h
#define _Timeout_h

#include <Arduino.h>

typedef void(*TimeoutCallback)();
typedef void(*TimeoutCallbackP)(void *);

struct TimeoutTab{
  long timeout;
  void *object;
  char ID[8];
  TimeoutCallbackP func;
  TimeoutTab(){
    timeout=0;
    object=NULL;
    func=NULL;
    ID[0]=0;
  }
  TimeoutTab& operator =(TimeoutTab& t){
    timeout=t.timeout;
    object=t.object;
    func=t.func;
    strcpy(ID,t.ID);
    return *this;
  }
};

class TimeoutMacro{
public:
  struct TimeoutTab tbl[10];
  TimeoutMacro(void){}
  long set(TimeoutCallback f,int ms){ return set(NULL,(TimeoutCallbackP)f,ms,"");}
  long set(TimeoutCallback f,int ms,char *s){ return set(NULL,(TimeoutCallbackP)f,ms,s);}
  long set(void *obj,TimeoutCallbackP f,int ms){ return set(obj,f,ms,"");}
  long set(void *obj,TimeoutCallbackP f,int ms,char *s){
    long now=micros();
    long tout=now+(((long)ms)<<10);
    if(f==NULL){
      Serial.println("Timeout set error ");
      return 0;
    }
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
    strcpy(tbl[n].ID,s);
    tbl[n].object=obj;
    tbl[n].func=f;
    tbl[n].timeout=tout;
    if(tbl[n].ID[0]){
      Serial.print("Timeout set OK ");
      Serial.print(n);
      Serial.print(" ");
      Serial.print(tbl[n].ID);
      Serial.print(" ");
      Serial.println(tbl[n].timeout);
    }
    return tbl[n].timeout;
  }
  int clear(long d){
    int n=this->lookup(d);
    if(tbl[n].func==NULL){
      Serial.print("Timeout clear error ");
      Serial.println(tbl[n].ID);
    }
    if(tbl[n].ID[0]){
      Serial.print("Timeout clear OK ");
      Serial.print(tbl[n].ID);
      Serial.print(" ");
      Serial.println(tbl[n].timeout);
    }
    if(n>=0){
      for(int i=n;;i++){
        tbl[i]=tbl[i+1];
        if(tbl[i].func==NULL) break;
      }
      return n;
    }
    else return -1;
  }
  int lookup(long d){
    TimeoutTab *et=tbl;
    int n=0;
    for(;n<sizeof(tbl);n++,et++){
      if(et->timeout==d){
    	  return n;
      }
    }
    return -1; //not found
  }
  void spinOnce(void){
    TimeoutTab et=tbl[0];
    if(et.func==NULL) return;
    long now=micros();
    long diff=et.timeout-now;
    if(diff<0){
      if(et.ID[0]){
        Serial.print("Timeout spin ");
        Serial.print(et.ID);
        Serial.print(" ");
        Serial.println(et.timeout);
      }
      for(int i=0;;i++){
        tbl[i]=tbl[i+1];
        if(tbl[i].func==NULL) break;
      }
      if(et.func==NULL || et.timeout==0){
        Serial.println("Timeout spin error ");
        return;
      }
  	  if(et.object==NULL) (*(TimeoutCallback)et.func)();
      else (*(TimeoutCallbackP)et.func)(et.object);
    }
  }
};

extern TimeoutMacro Timeout;

#endif
