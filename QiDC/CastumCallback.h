#ifndef _CastumCallback_h
#define _CastumCallback_h
#include <Arduino.h>
class HardwareSerial;
class CastumCallback{
public:
  int ev_loop,timeout;
  long timestamp;
  uint8_t buf[32];
  int bptr,blen,bsize;
  void (*callback)(uint8_t *,int);
  HardwareSerial *serial;
  uint8_t csum;
  void update();
  void write2(int,int);
  void write3(int,int,int);
  CastumCallback(HardwareSerial *serial,void (*recv_cb)(uint8_t *,int));
};

#endif
