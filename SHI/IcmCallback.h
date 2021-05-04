#ifndef _IcmCallback_h
#define _IcmCallback_h

typedef void(*CallbackDoublePtr)(double *);
class ICM_20948;
class IcmCallback{
public:
  ICM_20948 *icm;
  double *data;
  int ev_loop,ev_trigger;
  CallbackDoublePtr callback;
  IcmCallback(ICM_20948 *,CallbackDoublePtr);
};

#endif
