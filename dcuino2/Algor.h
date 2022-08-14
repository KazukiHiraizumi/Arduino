#ifndef _Algor_h
#define _Algor_h

#include  "Arduino.h"

extern void algor_prepare();
extern uint8_t algor_update(int32_t time,int32_t duty);
extern uint8_t algor_param[8*7];

/*
inline float accumulate(float *arr1,float *arr2){
  float sum=0;
  for(;arr1<arr2;arr1++) sum+=*arr1;
  return sum;
}
inline uint32_t accumulate(uint32_t *arr1,uint32_t *arr2){
  uint32_t sum=0;
  for(;arr1<arr2;arr1++) sum+=*arr1;
  return sum;
}
*/

template<typename T> inline long accumulate(T *arr1,T *arr2){
  long sum=0;
  for(;arr1<arr2;arr1++) sum+=*arr1;
  return sum;
}

#endif
