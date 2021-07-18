#ifndef _CastumCallback_h
#define _CastumCallback_h

#include	<StreamCallback.h>
class Stream;
class CastumCallback:public StreamCallback{
public:
  unsigned char csum;
  void (*callback2)(char *,int);
  CastumCallback(Stream *serial,void (*recv_cb)(char *,int),void (*result_cb)(char *,int));
};

#endif
