#ifndef _StreamCallback_h
#define _StreamCallback_h

typedef void(*CallbackCharPtr)(char *);
typedef void(*CallbackCharPtrInt)(char *,int);
class Stream;
class StreamCallback{
public:
  int ev_loop,ev_trigger;
  char *buf;
  int bptr,blen;
  CallbackCharPtrInt callback;
  Stream *serial;
  StreamCallback(Stream *serial,CallbackCharPtr callback);
  StreamCallback(Stream *serial,CallbackCharPtrInt callback);
};

#endif
