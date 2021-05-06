#ifndef _HapDriver_h
#define _HapDriver_h

#include <StreamCallback.h>

class HapDriver:public StreamCallback{
public:
  double norm,theta,phi;
  HapDriver(Stream *ser,CallbackCharPtr callback);
  void setQuat(double *);
  void setAccel(double *);
  void serout(char *);
  bool parse(char *);
};

#endif
