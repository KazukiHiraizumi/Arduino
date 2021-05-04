#ifndef _StreamCallback_h
#define _StreamCallback_h

typedef void(*CallbackCharPtr)(char *);
class Stream;
class StreamCallback{
public:
  int ev_loop,ev_trigger;
  char *buf;
  int bptr;
  CallbackCharPtr callback;
  Stream *serial;
  StreamCallback(Stream *serial,CallbackCharPtr callback);
};

#endif
