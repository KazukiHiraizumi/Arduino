#ifndef _StreamCallback_h
#define _StreamCallback_h

typedef void(*StreamCallbackFunc)(char *);
class Stream;
class StreamCallback{
public:
  char *buf;
  int bptr,evtmr,evrec;
  StreamCallbackFunc callback;
  Stream *serial;
  StreamCallback(Stream *serial,StreamCallbackFunc callback);
};

#endif
