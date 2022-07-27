#include "Arduino.h"
#include "Algor.h"
#include "Param.h"
#include "Logger.h"
#include "Dcore.h"

//params
uint8_t algor_param[]={
  90,20,100,0,  80,50,20,255,
  0,255,40,255, 170,50,255,0,
  30,0,10,255,  2,0,10,255,
  0,241,9,216,  26,178,25,83,
  31,50,13,150, 192,229,255,231,
  0,50,13,100,  20,255,255,255,
  0,255,20,255, 100,120,255,50
};

//elapsed time
static uint32_t utime; //elapsed time in usec
//observer vars
static uint8_t wflag;
static float wh,bh,dbh;
static float pi2;
static float bhscl;
//watches
static uint16_t wrmax;//max ang.velocity
static uint8_t wovrd; //duty override by max velocity

//sliding mode
static uint8_t hsig,hdeg,hflag;
static float hvalue;
//profile
static uint8_t ivalue,iflag;
//steady control
static uint32_t ssum[30];
static uint16_t sflag;
static uint8_t svalue,sduty;
static uint8_t seval1,seval2;
static uint16_t sspan;

//table
static uint8_t tbl_index;
#define US2S(s) ((s)*0.000001)
#define US2MS(s) ((s)>>10)
#define MS2US(s) ((uint32_t)(s)<<10)
#define DIV10(s) ((uint32_t)(s)*13>>7)  // 13/128
#define DIV100(s) ((uint32_t)(s)*10>>10) // 10/1024
#define ODD(s) ((s)&1)
#define MAX(a,b) ((a)>(b)? a:b)
#define MIN(a,b) ((a)<(b)? a:b)
#define ARRSZ(a) (sizeof(a)/sizeof(a[0]))

#define PRM_ReadData(n) algor_param[n]

/**************************************************************************/
static uint16_t interp(int y1,int y2,int dx,int w){
  int v=((long)y2*w+(long)y1*(dx-w))/dx;
  return v<0? 0:v;
}
static uint16_t readTbl12(int p,int w){
  uint16_t x1=(uint16_t)PRM_ReadData(p)<<4;
  uint16_t x2=(uint16_t)PRM_ReadData(p+2)<<4;
  uint8_t y1,y2;
  if(w<0) w=0;
  if(w>4095) w=4095;
  tbl_index=0;
  while(x2<w){
    x1=x2;
    tbl_index++;
    p+=2;
    x2=(uint16_t)PRM_ReadData(p+2)<<4;
  }
  y1=PRM_ReadData(p+1);
  y2=PRM_ReadData(p+3);
  return interp(y1,y2,x2-x1,w-x1);
}
static uint16_t readTbl8(int p,int b){ return readTbl12(p,b<<4);}
static int compare(const void* a, const void* b){ return abs(*(int32_t *)a)-abs(*(int32_t *)b);}
void algor_prepare(){
  wflag=hflag=iflag=sflag=0;
  utime=0;
  pi2=6.28318530718;
  bhscl=PRM_ReadData(1);
}
static int algor_smode(int D,int h){
  float bhref=h*bhscl;
  hvalue= dbh/D+bh;
  return hvalue<bhref;
}
uint8_t algor_update(int32_t udt,int32_t uot){
  if(udt==0) return 0;
  utime+=udt;
  uint16_t tmsec=US2MS(utime);
//Block-I: duty profile
  ivalue=readTbl12(24,tmsec);
  iflag=tbl_index;
  uint8_t ivalue__=0;
//Block-W: measurement and observer
  float dt=US2S(udt);
  switch(wflag){
    case 0:{
      wh=pi2/dt;
      bh=dbh=0;
      wrmax=0;
      wflag=1;
    }
    case 1:{
      float pole=PRM_ReadData(2);
      float pold=0.4/dt;
      pole=pold<pole? pold:pole;
      float h1=2*pole;
      float h2=pole*pole;
      float b2pi=PRM_ReadData(0)*0.25; // 2pi/tau
      float u0=(float)uot/udt;
      float werr=pi2-wh*dt;
      wh=wh+werr*h1+bh*dt-b2pi*u0;
      bh=bh+(dbh=werr*h2);
      dbh=dbh/dt;
    }
  }
  uint16_t wrps=round(wh);
  if(wrmax<wrps){
    wrmax=wrps;
    wovrd=readTbl8(40,DIV100(wrmax));
  }
//Block-H: sliding mode, tension filter
  switch(hflag){
  case 0:
    hsig=0;
    if(PRM_ReadData(16)==0) break;
    if(tmsec>PRM_ReadData(16)) hflag=1;
    break;
  case 1:
    hsig=algor_smode(PRM_ReadData(5),PRM_ReadData(4));
    if(hsig){
      hflag=2;
      dcore::shift();  //mode 2=>4
    }
    break;
  case 2:
    hsig=algor_smode(PRM_ReadData(6),PRM_ReadData(4));
    break;
  }
//Block-S: update control input
  switch(sflag){
    case 0:
      memset(ssum,0,sizeof(ssum));
      sspan=PRM_ReadData(24 +(PRM_ReadData(20)<<1))-PRM_ReadData(24 +2);
      svalue=(uint16_t)ivalue*wovrd>>8;
      if(iflag==0) break;
      sflag=1;
      break;
    case 1:{
      uint8_t ssn=PRM_ReadData(22); //switching surface count
      uint8_t idx=PRM_ReadData(20); //reference point index
      if(iflag<idx){
        float thres=PRM_ReadData(4)*bhscl;
        for(int i=1;i<=ssn;i++){
          if(hvalue>thres*i) ssum[i]+=udt;
        }
        svalue=(uint16_t)ivalue*wovrd>>8;
        ivalue__=PRM_ReadData(18);
        break;
      }
      uint32_t s1span=sspan*ssn;
      uint32_t s1=accumulate<uint32_t>(ssum,ssum+ssn);
      uint16_t sv1=s1/s1span>>5;
      seval1=sv1>255? 255:sv1;
      seval2=readTbl8(8,seval1);
      uint8_t rduty=PRM_ReadData(24 +(idx<<1)+1); //reference duty
      sduty=((uint32_t)rduty*seval2>>8)*wovrd>>8;//start duty
      sflag=tmsec;  //switch time stamp
      dcore::shift();  //mode switch 4=>5
    }
    default:{
      uint8_t ads=PRM_ReadData(21)<<1; //end address
      uint8_t eduty=PRM_ReadData(24+ads+1); //end duty
      if(ads>0 && sduty>eduty){
        uint16_t etm=(uint16_t)PRM_ReadData(24+ads)<<4; //end time by 16ms tick
        svalue=interp(sduty,eduty,etm-sflag,tmsec-sflag);
      }
      else{
        svalue=sduty;
      }
      if(svalue<ivalue) svalue=ivalue;
      ivalue__=255;
      break;
    }
  }
  uint8_t duty=ivalue__;
  if(duty>svalue) duty=svalue;
  if(hsig) duty=svalue;

//logger
  logger::stage.omega=wrps;
  logger::stage.beta=round(bh)/bhscl;
  logger::stage.latency=tmsec;
  int t100ms=US2MS(logger::stage.stamp)/100;
  switch(PRM_ReadData(3)){
  default:
    switch(t100ms){
    case 5:
      logger::stage.eval=seval1;
      break;
    case 6:
      logger::stage.eval=seval2;
      break;
    default:
      if(t100ms<7 || t100ms>=ARRSZ(ssum)+7) break;
      logger::stage.eval=US2MS(ssum[t100ms-7]);
      break;
    }
    break;
  case 101:
    logger::stage.beta=round(hvalue)/bhscl;
    break;
  }
  return duty;
}
