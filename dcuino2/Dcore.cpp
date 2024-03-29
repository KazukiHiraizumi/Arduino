#include "Arduino.h"
#include <mbed.h>
#include <rtos.h>
#include <SetTimeout.h>
#include "Dcore.h"
#include "Logger.h"
#include "NRF52_MBED_TimerInterrupt.h"

static NRF52_MBED_Timer ITimer0(NRF_TIMER_3);   //PWM
static NRF52_MBED_Timer ITimer1(NRF_TIMER_4);   //Debouncer and PWM blocker

#define T_PWM 833 //usec
#define T_PWM_MIN (T_PWM/10) //usec
#define T_WAIT 200 //usec

#define LENGTH(x) (sizeof(x))/(sizeof(x[0]))

namespace dcore{//DC core
  uint8_t Mode;
  static StartCB_t start_callback;
  static ContCB_t cont_callback;
  static EndCB_t end_callback;
  static void init();
}

namespace pwm{//methods for gate on(turn pwm pulse high)
  static uint8_t DO;//digital out pin
  static uint32_t Count; //Switch count
  static uint16_t Interval; //Switch interval
  static uint16_t Tact; //actual turn on time
  static uint16_t Ton; //latch for Tact
  static uint8_t Max;
  static uint8_t Duty; //duty command
  static bool Block;
  void on(); //to turn gate on
  void off(); //to turn gate off
  static void intr_on(); //interrupt callback to turn pulse high
  static void intr_off(); //interrupt callback to turn pulse low
  void stop(); //stop pwm sequence
  void start(uint32_t duration); //start pwm sequence
}

namespace sens{
  static rtos::Thread thread(osPriorityHigh);  //rotation sensor
  static rtos::Semaphore sema;
  static long wdt;
  static volatile int32_t Tm;
  static volatile uint32_t Interval;
  static uint8_t Nint;
  static mbed::InterruptIn* irq;
  static uint8_t DI;
//methods for sensing(pin interrupt)
  static void intr();
  static void start();
  static void stop();
  static void task();
}

namespace debouncer{
  static uint16_t Tcmd,Tact; //Turn-on time command
}

namespace pwm{//methods for pwm
  void init(){
    off();
    Ton=Tact=Count=Interval=Max=Duty=0;
    Block=true;
  }
  void on(){ digitalWrite(DO,HIGH);}
  void off(){ digitalWrite(DO,LOW);}
  static void intr_off(){
    off();
    ITimer1.setFrequency(1,dcore::init);
    if(Count==0){
      Block=true;
      Ton=Tact;
      sens::start();
    }
  }
  static void intr_off2(){
    off();
    ITimer1.setFrequency(1,dcore::init);
    Block=true;
    Ton=Tact;
    sens::start();
  }
  static void intr_on(){
    if(Count==0) return;
    Count--;
    if(Max<Duty) Max=Duty;
    if(!Block){
      int32_t tnow=micros();
      int16_t tw=(long)Interval*Duty>>8;
      int32_t tnex=sens::Tm+sens::Interval-T_WAIT;
      if(tnow+tw-tnex>0) tw=tnex-tnow;
      if(tnow+Interval-tnex>0){
        if(tw>T_PWM_MIN){
          on();
          ITimer1.setInterval(tw,intr_off2);
          Tact+=tw;
        }
        else{
          ITimer1.setFrequency(1,dcore::init);
          Block=true;
          Ton=Tact;
          sens::start();
        }
      }
      else{
        if(tw<T_PWM_MIN){
          uint8_t rn=tnow&0xFF; //as random number
          if(((int32_t)tw<<8)/T_PWM_MIN<=rn) return;
          else tw=T_PWM_MIN;
        }
        on();
        ITimer1.setInterval(tw,intr_off);
        Tact+=tw;
      }
    }
  }
  void stop(){ Count=0;}
  void start(uint32_t duration){
    if(Count==0){
      uint16_t pcnt=duration/T_PWM;
      pcnt|=1; //division should be odd number
      Interval=duration/pcnt;
      ITimer0.setInterval(Interval,intr_on);
      if(Duty>0){
        on();
        uint16_t tw=(long)Interval*Duty>>8;
        if(tw<T_PWM_MIN) tw=T_PWM_MIN;
        ITimer1.setInterval(tw,intr_off);
        Tact=tw;
      }
      else Tact=0;
      Count=pcnt-1;
      Block=false;
    }
    else{ //pwm running
      uint16_t pcnt=duration/T_PWM;
      Interval=T_PWM;
      Count=pcnt;
      Tact=0;
      Block=false;
    }
  }
}

namespace debouncer{
  void init(){ Tcmd=Tact=0;}
  void intr_ready(){
    sens::start();
    ITimer1.setFrequency(1,dcore::init);
  }
  void intr_off(){
    pwm::off();
    ITimer1.setInterval(50,intr_ready);
  }
  void restart(uint16_t dt){
    uint32_t tcm=Tcmd;
    if(tcm>dt-200) tcm=dt-200;
    if(Tcmd>=100){
      pwm::on();
      Tact+=tcm;
      ITimer1.setInterval(tcm,intr_off);
    }
    else{
      ITimer1.setInterval(500,intr_ready);
    }
  }
  void start(uint16_t dt){
    pwm::Ton=Tact;
    Tact=0;
    restart(dt);
  }
}

//utility
static int16_t log_lat;
static void log_pre();
static void log_post();

namespace sens{
  static uint8_t Nacc;
  void init(){ wdt=Interval=Nacc=0; }
  void stop(){
    irq->rise(NULL);
  }
  void start(){
    irq->rise(mbed::callback((voidFuncPtrParam)intr, (void *)NULL));
  }
  void intr() {
    stop();
    long tnow=micros();
    long dt=tnow-Tm;
    switch(dcore::Mode){
    case 0:
      logger::start();
      Tm=tnow;
      debouncer::start(dt);
      if(dt>20000 || dt>Interval){
        Nacc=0;
        Interval=dt;
        break;
      }
      if(++Nacc>=7) sema.release();
      Interval=dt;
      break;
    case 2:
      logger::latch();
      debouncer::start(dt);
      Interval+=dt;
      Tm=tnow;
      sema.release();
      break;
    case 4:
    case 5:
      logger::latch();
      Interval=dt;
      Tm=tnow;
      pwm::start(dcore::Mode==4? dt:50000);
      sema.release();
      break;
    case 1:
      Interval=dt;
      Tm=tnow;
    case 3:
      debouncer::restart(dt);
      dcore::Mode++;
      break;
    }
  }
  void task(){
  WAIT:
    sema.acquire();
    switch(dcore::Mode){
    case 0:
      Serial.print("Mod0=>");
      wdt=setTimeout.set([](){
        Serial.println("sens wdt:Mode1 => 0");
        dcore::init();
      },30);
      (*dcore::start_callback)();
      dcore::Mode=1;
      break;
    case 2:
      log_pre();
      debouncer::Tcmd=(uint16_t)(*dcore::cont_callback)(Interval,pwm::Ton)*10;
      log_post();
      if(dcore::Mode==2) dcore::Mode=1;
      else{
        Serial.print("Mod2=>");
        Serial.println(dcore::Mode);
      }
      setTimeout.clear(wdt);
      wdt=setTimeout.set([](){
        Serial.println("sens wdt:Mode2 => 0");
        (*dcore::end_callback)();
        dcore::init();
      },30);
      break;
    case 4:
    case 5:
      log_pre();
      pwm::Duty=(*dcore::cont_callback)(Interval,pwm::Ton);
      log_post();
      if(wdt!=0) setTimeout.clear(wdt);
      wdt=setTimeout.set([](){
        Serial.println("sens wdt:Mode4 => 0");
        (*dcore::end_callback)();
        pwm::Duty=pwm::Max>0? pwm::Max:128;
        stop();
//        pwm::stop();
//        pwm::start(1000000L);  //dcore::init will be invoked after this pwm sequence
      },100);
      break;
    }
    goto WAIT;
  }
}

namespace dcore{
  void init(){
    pwm::init();
    sens::init();
    debouncer::init();
    sens::start();
    Mode=0;
  }
  void shift(){
    switch(Mode){
    case 2:
      Mode=3;
      break;
    case 4:
      Mode=5;
      break;
    }
  }
  void run(int DI,int DO,StartCB_t cb_start,ContCB_t cb_cont,EndCB_t cb_end){
    start_callback=cb_start;
    cont_callback=cb_cont;
    end_callback=cb_end;
    sens::DI=DI;
    pwm::DO=DO;
    sens::Nint=digitalPinToInterrupt(sens::DI);
    sens::irq = new mbed::InterruptIn(digitalPinToPinName(sens::Nint));
    sens::thread.start(mbed::callback(sens::task));
    pwm::off();
    init();
    attachInterrupt(sens::Nint, sens::intr, RISING);
    pinMode(sens::DI,INPUT); //disable pullup after attachInterrupt
  }
  void sleep(uint16_t ms){
    long t=millis();
    do{
      ::sleep();
      // __WFI();
    }while((long)millis()-t<ms);
  }
}

void log_pre(){
  log_lat=micros();
  logger::stage.latency=log_lat-sens::Tm;
  logger::stage.interval=sens::Interval;
  logger::stage.mode=dcore::Mode;
  logger::stage.duty=((uint32_t)pwm::Ton<<8)/sens::Interval;
}
void log_post(){
  logger::stage.exetime=micros()-log_lat;
  logger::stage.cmd=pwm::Duty;
}
