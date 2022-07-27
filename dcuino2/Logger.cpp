#include "Arduino.h"
#include "Logger.h"

namespace logger{
  static struct ALOG buf[1000];
  struct ALOG *data=buf;
  struct ALOG stage;
  static int32_t tzero;
  int size=0;
  void clear(){
    memset(&stage,0,sizeof(stage));
  }
  int length(){
    return size;
  }
  void start(){
	  clear();
	  size=0;
    tzero=micros();
  }
  void latch(){
	  if(stage.stamp){
	    buf[size++]=stage;
  	  clear();
	  }
    stage.stamp=micros()-tzero;
  }
  void dump(){
    if(!Serial) return;
    for(int i=0;i<size;i++){
      ALOG *logs=data+i;
      Serial.print(logs->stamp);
      Serial.print(" ");
      Serial.print(logs->interval);
      Serial.print(" ");
      Serial.print(logs->mode);
      Serial.print(" ");
      Serial.print(logs->latency);
      Serial.print(" ");
      Serial.print(logs->exetime);
      Serial.print(" ");
      Serial.print(logs->omega);
      Serial.print(" ");
      Serial.print(logs->beta);
      Serial.print(" ");
      Serial.print(logs->duty);
      Serial.print(" ");
      Serial.print(logs->cmd);
      Serial.print(" ");
      Serial.print(logs->eval);
      Serial.print(" ");
      Serial.println(logs->icoef);
    }
  }
}
