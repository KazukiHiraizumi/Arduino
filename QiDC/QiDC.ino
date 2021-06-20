#include <Timeout.h>
#include <StreamCallback.h>

void *cb1=new StreamCallback(&Serial,[](char *s){
  Serial2.print(s);
});

void *cb2=new StreamCallback(&Serial2,[](char *s){
  Serial.print(s);
});

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
}
void loop(){
  Timeout.spinOnce();
}
