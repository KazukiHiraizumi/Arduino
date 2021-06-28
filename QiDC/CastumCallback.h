#ifndef _CastumCallback_h
#define _CastumCallback_h

#include	<StreamCallback.h>
class Stream;
class CastumCallback:public StreamCallback{
public:
  unsigned char csum;
  CastumCallback(Stream *serial,void (*callback)(char *,int));
};

#endif
