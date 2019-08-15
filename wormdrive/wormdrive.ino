#define INVERT true
#define RATIO 128

#define STEPWIDTH 500
#define O_ENABLE 8
#define O_SLEEP 9
#define O_STEP 10
#define O_DIR 11
#define O_RESET 12
#define O_LED 13
#define I_A1 0
#define I_A2 1
#define I_B1 2
#define I_B2 3
#define INT_A1 2
#define INT_A2 3
#define INT_B1 1
#define INT_B2 0

//Stepper driver
bool step_off_req=false;
short step_on_t=0;
void step_on(){
  digitalWrite(O_STEP,HIGH);
  digitalWrite(O_LED,HIGH);
  step_on_t=micros();
  step_off_req=true;
}
void step_off(){
  digitalWrite(O_STEP,LOW);
  digitalWrite(O_LED,LOW);
}
void step_test(){
  for(int x = 0; x < 200; x++){
    digitalWrite(O_STEP,HIGH);
    delayMicroseconds(500);
    digitalWrite(O_STEP,LOW);
    delayMicroseconds(5000);
  }
}

//Encoder driver
byte count=0;
bool stat_A=false,stat_B=false,dir=false;
void count_up(){
#if !INVERT
  byte c=count;
  count+=RATIO;
//  Serial.println(count);
  if(count<=c) step_on();
#endif
}
void count_down(){
#if INVERT
  byte c=count;
  count+=RATIO;
//  Serial.println(count);
  if(count<=c) step_on();
#endif
}
void isr_A1(){//A up
  if(stat_A) return;
  stat_A=true;
  if(dir){
    if(!stat_B) count_up();
    else dir=false;
  }
  else{
    if(stat_B) count_down();
    else dir=true;
  }
}
void isr_A2(){//A down
  if(!stat_A) return;
  stat_A=false;
  if(dir){
    if(stat_B) count_up();
     else dir=false;
  }
  else{
     if(!stat_B) count_down();
     else dir=true;
  }
}
void isr_B1(){// B up
  if(stat_B) return;
  stat_B=true;
  if(dir){
    if(stat_A) count_up();
    else dir=false;
  }
  else{
    if(!stat_A) count_down();
    else dir=true;
  }
}
void isr_B2(){// B down
  if(!stat_B) return;
  stat_B=false;
  if(dir){
    if(!stat_A) count_up();
    else dir=false;
  }
  else{
    if(stat_A) count_down();
    else dir=true;
  }
}

void setup() {
  pinMode(O_STEP,OUTPUT);
  pinMode(O_SLEEP,OUTPUT);
  pinMode(O_ENABLE,OUTPUT);
  pinMode(O_DIR,OUTPUT);
  pinMode(O_RESET,OUTPUT);
  pinMode(O_LED,OUTPUT);
  digitalWrite(O_ENABLE,LOW);
  digitalWrite(O_DIR,LOW);
  digitalWrite(O_SLEEP,HIGH);
  digitalWrite(O_RESET,HIGH);

  pinMode(I_A1,INPUT_PULLUP);
  pinMode(I_A2,INPUT_PULLUP);
  pinMode(I_B1,INPUT_PULLUP);
  pinMode(I_B2,INPUT_PULLUP);
}

void loop() {
  while(true){
    bool a=digitalRead(I_A1);
    bool b=digitalRead(I_B1);
    if(a) isr_A1();
    else isr_A2();
    if(b) isr_B1();
    else isr_B2();
    if(step_off_req){
      short t=micros();
      short dt=t-step_on_t;
      if(dt>STEPWIDTH){
        step_off();
        step_off_req=false;
      }
    }
  }
}
