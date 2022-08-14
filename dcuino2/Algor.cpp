#include <Arduino.h>
#include <KickFFT.h>
#include <SetTimeout.h>
#include "Algor.h"
#include "Param.h"
#include "Logger.h"
#include "Dcore.h"

//params
uint8_t algor_param[]={
  90,20,100,0,  80,50,20,255,
  80,1,16,30, 255,110,50,100,
  30,0,0,2,  3,5,255,255,
  0,241,9,216,  26,178,25,83,
  31,50,13,150, 192,229,255,231,
  0,50,13,100,  20,255,255,255,
  0,255,20,255, 100,120,255,50
};

//elapsed time
static uint32_t utime,udt; //elapsed time in usec
//observer vars
static uint8_t wflag;
static float wh,bh,dbh;
static float pi2,pole;
//watches
static uint16_t wrmax;//max ang.velocity
static uint8_t wovrd; //duty override by max velocity

//sliding mode
static uint8_t hsig,hflag;
static float hvalue;
//profile
static uint8_t ivalue,iflag;
//steady control
#define FFTN 32  //samples
static uint16_t sflag,sscore;
static uint8_t svalue,sduty;
static int16_t sfsam[FFTN];
static uint32_t sfmag[FFTN];
static void (*scrcb)();

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
static uint16_t readTblN(int p,int b,int n){
  if(b<PRM_ReadData(p)) return PRM_ReadData(p+1);
  n=(n-1)<<1;
  if(b>PRM_ReadData(p+n)) return PRM_ReadData(p+n+1);
  return readTbl8(p,b);
}
static uint16_t readTbl2(int p,int b){ return readTblN(p,b,2);}
static uint16_t readTbl2(int p,int b,int lv){
  if(b<PRM_ReadData(p)) return lv;
  else return readTbl2(p,b);
}
void algor_prepare(){
  wflag=hflag=iflag=sflag=0;
  utime=0;
  pi2=2*M_PI;
}
static float algor_sref(int h){
  float kp=pole/PRM_ReadData(1);
  return h*100*kp;
}
static int algor_smode(int D,int h){
  float kp=pole/PRM_ReadData(1);
  float hsref=h*100.0;
  hvalue= dbh/(D*kp)+bh;
  return hvalue<hsref;
}
uint8_t algor_update(int32_t dtu,int32_t otu){
  if(dtu==0) return 0;
  udt=dtu;
  utime+=udt;
  uint16_t tmsec=US2MS(utime);
//Block-I: duty profile
  ivalue=readTbl12(24,tmsec);
  iflag=tbl_index;
  uint8_t ivalue__=PRM_ReadData(17);
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
      float polm=PRM_ReadData(1);//max pole
      float pold=0.33/dt;
      pole=MIN(pold,polm);
      float h1=2*pole;
      float h2=pole*pole;
      float b2pi=PRM_ReadData(0)*0.25; // 2pi/tau
      float u0=(float)otu/udt;
      float werr=pi2-wh*dt;
      wh=wh+werr*h1+bh*dt-b2pi*u0;
      bh=bh+(dbh=werr*h2);
      dbh=dbh/dt;
    }
  }
  uint16_t wrps=round(pi2/dt);
  if(wrmax<wrps){
    wrmax=wrps;
    wovrd=readTbl8(40,DIV100(wrmax));
  }
//Block-H: sliding mode, tension filter
  switch(hflag){
    case 0:{
      hsig=0;
      if(PRM_ReadData(16)==0) break;
      if(tmsec>PRM_ReadData(16)) hflag=1;
      break;
    }
    case 1:{
      hsig=algor_smode(PRM_ReadData(5),PRM_ReadData(4));
      if(hsig){
        hflag=2;
        dcore::shift();  //mode 2=>4
      }
      break;
    }
    case 2:{
      if(dbh<0){
        hsig=algor_smode(PRM_ReadData(5),PRM_ReadData(4));
      }
      else{
        hsig=algor_smode(PRM_ReadData(7),PRM_ReadData(6));        
      }
      ivalue__=PRM_ReadData(18);
      break;
    }
  }
//Block-S: update control input
  svalue=(uint16_t)ivalue*wovrd>>8;
  switch(sflag){
    case 0:{
      sscore=0;
      if(hflag==2){
        sflag=1;
        sscore=65535;
        sduty=255;
        setTimeout.set(scrcb=[](){
          for(int i=0,n=logger::length()-1;i<FFTN;i++,n--){
            sfsam[i]=logger::data[n].beta;
          }
          KickFFT<int16_t>::fft(FFTN,sfsam,sfmag);
          long scr=0;
          for(int i=PRM_ReadData(9);i<PRM_ReadData(10);i++) scr+=sfmag[i];
          scr/=(uint16_t)(PRM_ReadData(10)-PRM_ReadData(9))*25;
          uint16_t kr=scr*100/MAX(sscore,PRM_ReadData(11));
          uint16_t gain=readTbl2(12,kr,0);
//        sduty-=udt*gain*sduty>>20; // >>20= 1/256 1/4096
          sduty-=gain*sduty>>8;
          sscore=scr;
          if(dcore::Mode>=4){
            uint16_t tms=udt*PRM_ReadData(8)/1000; //FFT window
            setTimeout.set(scrcb,tms);
          }
        },udt*FFTN/1000);
      }
      break;
    }
    case 1:{
      uint8_t ri=PRM_ReadData(20); //reference point index
      if(iflag<ri) break;
      dcore::shift();  //mode switch 4=>5
      uint32_t dsum=0;
      uint8_t dn=0;
      for(int n=logger::length()-1;;n--,dn++){
        if(dn>=PRM_ReadData(19)) break;
        else if(logger::data[n].mode!=4) break;
        dsum+=logger::data[n].duty;
      }
      uint8_t sd1=PRM_ReadData(24+(ri<<1)+1);
      uint8_t sd2=dsum/dn;
      sduty=svalue=(uint16_t)MIN(sd1,sd2)*sduty>>8;
      sflag=tmsec;
      ivalue__=255;
      break;
    }
    default:{
      svalue=sduty;
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
  logger::stage.eval=MIN(sscore,255);
  logger::stage.beta=round(MIN(bh,32767));
/*  switch(PRM_ReadData(3)){
    case 1:
      logger::stage.beta=hsref;
      break;
  }*/
  return duty;
}
