#ifndef _Timeout_h
#define _Timeout_h

typedef void(*TimeoutCallback)();
typedef void(*TimeoutCallbackP)(void *);
struct TimeoutTab;
class TimeoutClass{
public:
  struct TimeoutTab *tbl;
  TimeoutClass(void);
  long set(TimeoutCallback,int);
  long set(void *object,TimeoutCallbackP,int);
  void clear(long);
  int lookup(long);
  void spinOnce(void);
};

extern TimeoutClass Timeout;

#endif
