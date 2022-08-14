//
//PIN Assign:https://github.com/arduino/ArduinoCore-nRF528x-mbedos/blob/master/variants/ARDUINO_NANO33BLE/pins_arduino.h
//
#include <Arduino.h>
#include <mbed.h>
#include <rtos.h>
#include <SetTimeout.h>
#include "Dcore.h"
#include "Logger.h"
#include "Param.h"
#include "Algor.h"
#include "Ble.h"
#define DI_SENS D10
#define DO_FET D2

void setup() {
  Serial.begin(115200);
  digitalWrite(LED_PWR,LOW);//Power LED Turn off

  pinMode(DI_SENS,INPUT);  //rotation sensor
  pinMode(DO_FET,OUTPUT);  //FET
  dcore::run(DI_SENS,DO_FET,
    [](){//start callback
      Serial.println("Start callback");
      algor_prepare();
    },
    [](int32_t dt,int32_t on_dt){//cyclic callback
      uint8_t d=algor_update(dt,on_dt);
      return d;
    },
    [](){//end callback
      ble::logdump();
      logger::dump();
    }
  );

//  while (!Serial);

  param::run(algor_param,sizeof(algor_param));

  ble::run(
    "Shimano",  //device name 
    "10014246-f5b0-e881-09ab-42000ba24f83",  //service uuid
    "20024246-f5b0-e881-09ab-42000ba24f83",  //request uuid
    "20054246-f5b0-e881-09ab-42000ba24f83"   //notification uuid
  );
}

void loop() {
  setTimeout.spinOnce();
  dcore::sleep(10);
}
