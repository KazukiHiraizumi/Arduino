#include <Timeout.h>
#include <StreamCallback.h>

byte cmd[]={2,0,0,0,0,2,1,0,0xFB};

StreamCallback cb1(&Serial,[](char *s){
//  Serial2.print(s);
  Serial.print("$>");
  Serial.println(s); //echo
  Serial2.print(s);
//  Serial2.write(cmd,9);
});

StreamCallback cb2(&Serial2,[](char *s,int l){
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
  Serial2.begin(115200);
}
void loop(){
  Timeout.spinOnce();
}
