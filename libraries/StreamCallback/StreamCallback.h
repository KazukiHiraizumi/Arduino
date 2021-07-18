#ifndef _StreamCallback_h
#define _StreamCallback_h

class Stream;
class StreamCallback{
public:
  int ev_loop,ev_timeout,timer;
  char *buf;
  int bptr,blen,bsize;
  void (*callback)(char *,int);
  Stream *serial;
  StreamCallback(Stream *serial,int bufsize,void (*callback)(char *,int),int timeout=10);
};

#endif
