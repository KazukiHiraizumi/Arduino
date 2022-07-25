#include "Arduino.h"
#include "Algor.h"
#include "Param.h"
#include "Logger.h"
#include "Dcore.h"

//params
uint8_t algor_param[]={
  90,20,100,0,   150,50,70,30,
  120,100,5,255,  255,255,255,255,
  30,10,2,0,      255,150,35,255,
  0,241,9,216,    26,178,25,83,
  31,50,13,150,   192,229,255,231,
  0,50,13,150,    17,255,255,255,
  0,255,20,255,   100,120,255,50
};

//elapsed time
static uint32_t utime; //elapsed time in usec
//observer vars
static float wh,bh,dbh;
static float pi2;
static float bhscl;
static float bhmin;
//watches
static uint32_t wrps;//ang.velocity by rad/s
static uint16_t wrmax;//max ang.velocity by rad/s
static uint8_t duovd; //duty override

//sliding mode
static uint8_t hsig,hdeg,hflag;
static float hvalue;
//profile
static uint8_t ivalue,iflag;
//steady control
static uint32_t ssum[10];
static uint16_t sflag;
static uint8_t svalue,shold,seval1,seval2;
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
static int algor_smode(int D,int h){
  float bhref=h*bhscl;
//  if(h&1) bhref+=bhmin;
  hvalue= dbh/D+bh;
  return hvalue<bhref;
}
void algor_prepare(){
  hflag=iflag=sflag=0;
  utime=wh=bh=dbh=wrmax=0;
  pi2=6.28318530718;
  svalue=shold=PRM_ReadData(25); //the 1st point of profile
  bhscl=PRM_ReadData(1);
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
  if(wh==0){
    wh=pi2/dt;
    bh=dbh=0;
  }
  else{
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
  //output filter
  wrps=round(wh);
  if(wrmax<wrps){
    wrmax=wrps;
    duovd=readTbl8(40,DIV100(wrmax));
  }
//Block-H: sliding mode, tension filter
  switch(hflag){
  case 0:
    bhmin=0;
    hsig=0;
    if(PRM_ReadData(16)==0) break;
    if(tmsec>PRM_ReadData(16)) hflag=1;
    break;
  case 1:
    hsig=algor_smode(PRM_ReadData(5),PRM_ReadData(4));
    if(hsig){
      hflag=2;
    }
    break;
  case 2:
    hsig=algor_smode(PRM_ReadData(7),PRM_ReadData(6));
    ivalue__=PRM_ReadData(17);
  //filter
    if(bhmin>bh) bhmin=bh;
    break;
  }
//Block-S: update control input
  ivalue=(uint16_t)ivalue*duovd>>8;
  switch(sflag){
    uint16_t end_tm;
    uint8_t mod_h,end_i;
  case 0:
    memset(ssum,0,sizeof(ssum));
    sspan=PRM_ReadData(24 +(PRM_ReadData(18)<<1))-PRM_ReadData(24 +2);
    svalue=ivalue;
    if(iflag==0) break;
    sflag=1;
    break;
  case 1:
    if(iflag<PRM_ReadData(18)){
      for(int i=0;i<ARRSZ(ssum);i++){
        int thres=PRM_ReadData(6)+((uint32_t)PRM_ReadData(22)*10-PRM_ReadData(6))*i/ARRSZ(ssum);
        if(hvalue>thres*bhscl) ssum[i]+=udt;
      }
      seval1=US2MS(ssum[0]);
      seval2=US2MS(ssum[5]);
      svalue=ivalue;
      ivalue__=PRM_ReadData(17);
      break;
    }
    seval1=255-(ssum[0]*PRM_ReadData(20)/sspan>>14);
    for(seval2=1;seval2<ARRSZ(ssum);seval2++){
      if(ssum[seval2]<ssum[0]/2) break;
    }
    seval2=255-(uint32_t)PRM_ReadData(21)*seval2/ARRSZ(ssum);
/*    Serial.print("Eval ");
    Serial.print(seval1);
    Serial.print(" ");
    Serial.println(seval2);*/
    svalue=shold=((uint32_t)PRM_ReadData(24 +(PRM_ReadData(18)<<1)+1)*seval1>>8)*duovd>>8;
    sflag=tmsec;
    dcore::mode5();
  default:
    mod_h=(uint16_t)shold*seval2>>8;
    end_i=PRM_ReadData(19);
    if(end_i>0){
      end_i<<=1;
      end_tm=(uint16_t)PRM_ReadData(24+end_i)<<4; //time span
      svalue=interp(mod_h,(uint8_t)PRM_ReadData(24+end_i +1),end_tm-sflag,tmsec-sflag);
    }
    else{
      svalue=mod_h;
    }
    if(svalue<ivalue) svalue=ivalue;
    ivalue__=255;
    break;
  }
  uint8_t duty=ivalue__;
  if(duty>svalue) duty=svalue;
  if(hsig) duty=svalue;

//logger
  logger::stage.omega=wrps;
  logger::stage.beta=round(bh)/bhscl;
  logger::stage.latency=tmsec;
  switch(PRM_ReadData(3)){
  default:
    switch((US2MS(logger::stage.stamp)/100)%2){
    case 0:;
      logger::stage.eval=seval1;
      break;
    case 1:
      logger::stage.eval=seval2;
      break;      
    }
    break;
  case 10:
    logger::stage.beta=round(bhmin)/bhscl;
    break;
  case 11:
    logger::stage.beta=round(hvalue)/bhscl;
    break;
  }
  return duty;
}
