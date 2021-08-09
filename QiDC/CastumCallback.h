#ifndef _CastumCallback_h
#define _CastumCallback_h

class HardwareSerial;
class CastumCallback{
public:
  int ev_loop,timeout;
  long timestamp;
  char buf[32];
  int bptr,blen,bsize;
  void (*callback)(char *,int);
  HardwareSerial *serial;
  unsigned char csum;
  void update();
  void write2(int,int);
  void write3(int,int,int);
  CastumCallback(HardwareSerial *serial,void (*recv_cb)(char *,int));
};

#endif
