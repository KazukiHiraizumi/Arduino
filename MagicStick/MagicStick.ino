#include <Timeout.h>
#include <StreamCallback.h>

void lighter(void *){
  if(digitalRead(4)==LOW){
    digitalWrite(8,HIGH);
  }
  else{
    digitalWrite(8,LOW);
  }
  Timeout.set(NULL,lighter,100);
}
void logger(void *){
  Serial.println(!digitalRead(4));
  Timeout.set(NULL,logger,100);  
}
void setup() {
//Initializing IOs
  pinMode(4, INPUT_PULLUP);
  pinMode(8, OUTPUT);
//Initializing Serial ports
  Serial.begin(115200);
//Setting callback for Serial
  new StreamCallback(&Serial,[](char *s){
    Serial.print("SER->");
    Serial.println(s);
  });
//Setting callback for Constant scan with Timeout
  lighter(NULL);
  logger(NULL);
}
void loop(){
  Timeout.spinOnce();
}
