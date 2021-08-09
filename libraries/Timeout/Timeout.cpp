#include "Timeout.h"
#include "Arduino.h"
#define TBLEN 10

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

static TimeoutTab ttab_[TBLEN];
TimeoutClass::TimeoutClass(void){
  tbl=ttab_;
}
long TimeoutClass::set(TimeoutCallback f,int ms){
  return set(NULL,(TimeoutCallbackP)f,ms,"");
}
long TimeoutClass::set(TimeoutCallback f,int ms,char *s){
  return set(NULL,(TimeoutCallbackP)f,ms,s);
}
long TimeoutClass::set(void *obj,TimeoutCallbackP f,int ms){
	set(obj,f,ms,"");
}
long TimeoutClass::set(void *obj,TimeoutCallbackP f,int ms,char *s){
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
  return tbl[n].timeout;
}
int TimeoutClass::clear(long d){
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
int TimeoutClass::lookup(long d){
  TimeoutTab *et=tbl;
  int n=0;
  for(;n<TBLEN;n++,et++){
    if(et->timeout==d){
    	return n;
    }
  }
  return -1; //not found
}
void TimeoutClass::spinOnce(void){
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
    if(et.func==NULL){
      Serial.println("Timeout spin error ");
      return;
    }
  	if(et.object==NULL) (*(TimeoutCallback)et.func)();
    else (*(TimeoutCallbackP)et.func)(et.object);
  }
}


/*
long tm=0;

void cb1(){
	printf("cb1\n");
}

void cb2(){
	printf("cb2\n");
	Timeout.clear(tm);
	tm=Timeout.set(cb1,1000,"cb1");
	Timeout.set(cb2,500);
}

main(){
	tm=Timeout.set(cb1,1000,"cb1");
	Timeout.set(cb2,500,"cb2");
	while(1) Timeout.spinOnce();
}
*/