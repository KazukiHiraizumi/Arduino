#include "HapDriver.h"
#include "Timeout.h"
#include "Arduino.h"

HapDriver::HapDriver(Stream *ser,CallbackCharPtr cb):(ser,cb){
  norm=theta=phi=0;
}
void HapDriver::setQuat(double *){}
void HapDriver::setAccel(double *){}
bool HapDriver::parse(char *s){
  if(strncmp(s,"SF",2)==0){ //Force sense
    char *si=strchr(s+2,'I');
    if(si!=NULL){
      norm=atoi(si+1);
      if(norm>255) norm=255;
    }
    char *sc=strchr(s+2,'C');
    if(sc!=NULL) theta=atoi(sc+1)*3.1415/180;
    char *sb=strchr(s+2,'B');
    if(sb!=NULL) phi=atoi(sb+1)*3.1415/180;
    int x=floor(norm*cos(phi)*cos(theta))+255;
    int y=floor(norm*cos(phi)*sin(theta))+255;
    int z=floor(norm*sin(phi))+255;
    int mcmd='F';
    int scmd=1;
    int ptyp=1;
    char buf[20];
    sprintf(buf,"%c%03d%1d%03d%03d%03d",mcmd,scmd,ptyp,x,y,z);
    serout(buf);
    return true;
  }
  return false;
}
void HapDriver::serout(char *s){
  char header[4];
  int len=strlen(s);
  sprintf(header,"$%02d",len+4); //DLC=strlen(s)+len(DLC)+len(CKSUM)
  serial->write(header,3);
  serial->write(s,len);
  byte sum=0;
  for(int i=1;i<3;i++) sum+=header[i];
  for(int i=0;i<len;i++) sum+=s[i];
  char footer[4];
  sprintf(footer,"%02X;",sum);
  serial->write(footer,3);  
}
