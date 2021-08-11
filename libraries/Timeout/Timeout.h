#ifndef _Timeout_h
#define _Timeout_h

typedef void (*TimeoutCallback)();
typedef void (*TimeoutCallbackP)(char *);
typedef void (*TimeoutCallbackPN)(uint8_t *,int);
struct TimeoutTab;
class TimeoutClass{
public:
  struct TimeoutTab *tbl;
  TimeoutClass(void);
  long set(TimeoutCallback,int delay);
  long set(char *message,TimeoutCallbackP,int delay);
  long set(uint8_t *message,int length,TimeoutCallbackPN,int delay);
  int clear(long);
  int lookup(long);
  void spinOnce(void);
};

extern TimeoutClass *Timeout;

#endif
