#include <Timeout.h>
#include <StreamCallback.h>
#include "CastumCallback.h"

byte cmd[]={2,0,0,0,0,2,1,0,0xFB};

StreamCallback cb1(&Serial,100,[](char *s,int){
  Serial.println("Rec"); //echo
//  Serial2.print(s);
  Serial2.write(cmd,9);
},5);

CastumCallback cb2(&Serial2,[](char *s,int l){
//StreamCallback cb2(&Serial2,2000,[](char *s,int l){
  Serial.print("len ");
  Serial.print(l);
  Serial.print(" ");
//  Serial.println(s);
  for(int i=0;i<l;i++){
    Serial.print(int(s[i]));
    Serial.print(",");
  }
});

void setup() {
  Serial.begin(115200);
  Serial2.begin(230400);
}
void loop(){
  Timeout.spinOnce();
}
