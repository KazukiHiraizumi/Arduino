#ifndef _Ble_h
#define _Ble_h

#include  "Arduino.h"

namespace ble{
  void run(char *deviceName,char *serviceUuid,char *requestCharUuid,char *notifyCharUuid);
  void logdump();
}

#endif
