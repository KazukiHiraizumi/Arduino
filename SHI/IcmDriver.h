#ifndef _IcmDriver_h
#define _IcmDriver_h

typedef void(*CallbackDoublePtr)(double *);
class ICM_20948_I2C;
class Stream;
class IcmDriver{
public:
  ICM_20948_I2C *icm;
  Stream *wire;
  double *data;
  int adds,ev_loop,ev_trigger;
  CallbackDoublePtr callback;
  IcmDriver(ICM_20948_I2C *,Stream *,int,CallbackDoublePtr);
  void reset(void);
  bool parse(char *);
};

#endif
