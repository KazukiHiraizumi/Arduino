#ifndef _HapDriver_h
#define _HapDriver_h

#include <StreamCallback.h>

class HapDriver:public StreamCallback{
  double x1,y1,z1,w1; //Pose of the device in Quaternion
public:
  double norm,theta,phi;
  HapDriver(Stream *ser,CallbackCharPtr callback);
  void setQuat(double *);
  void setAccel(double *);
  void serout(char *);
  bool parse(char *);
};

#endif
