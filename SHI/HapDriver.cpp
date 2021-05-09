#include "HapDriver.h"
#include "Timeout.h"
#include "Arduino.h"

struct Vector3d{
  double x,y,z;
  Vector3d& operator =(double *d){ x=d[0];y=d[1];z=d[2];return *this;}
  Vector3d& operator =(Vector3d& v){ x=v.x;y=v.y;z=v.z;return *this;}
};
struct Matrix3d{
  double x[3],y[3],z[3],w[3];
  double *dot(Vector3d& v){
    for(int i=0;i<3;i++){
      w[i]=x[i]*v.x+y[i]*v.y+z[i]*v.z;
    }
    return w;
  }
  Matrix3d& array(double x0,double y0,double z0,double x1,double y1,double z1,double x2,double y2,double z2){ x[0]=x0;y[0]=y0;z[0]=z0;x[1]=x1;y[1]=y1;z[1]=z1;x[2]=x2;y[2]=y2;z[2]=z2;return *this;}
};

HapDriver::HapDriver(Stream *ser,CallbackCharPtr cb):(ser,cb){
  norm=theta=phi=0;
  x1=y1=z1=0;
  w1=1;
}
void HapDriver::setQuat(double *q){ x1=q[0],y1=q[1],z1=q[2],w1=q[3];}
void HapDriver::setAccel(double *){}
bool HapDriver::parse(char *s){
  if(strncmp(s,"SF",2)==0){ //Force sense
    int mcmd='F', scmd=1, ptyp=1;
    char *si=strchr(s+2,'I');
    if(si!=NULL){
      norm=atoi(si+1);
      if(norm>255) norm=255;
    }
    char *sc=strchr(s+2,'C');
    if(sc!=NULL) theta=atoi(sc+1)*3.1415/180;
    char *sb=strchr(s+2,'B');
    if(sb!=NULL) phi=atoi(sb+1)*3.1415/180;
    double x2=norm*cos(phi)*cos(theta);
    double y2=norm*cos(phi)*sin(theta);
    double z2=norm*sin(phi);
    double x_=(x1*x2+y1*y2+z1*z2)*x1+w1*(w1*x2-y1*y2+z1*z2)+(w1*y2-z1*x2+x1*z2)*z1-(w1*z2-x1*y2+y1*x2)*y1;
    double y_=(x1*x2+y1*y2+z1*z2)*y1+w1*(w1*y2-z1*x2+x1*z2)+(w1*z2-x1*y2+y1*x2)*x1-(w1*x2-y1*y2+z1*z2)*z1;
    double z_=(x1*x2+y1*y2+z1*z2)*z1+w1*(w1*y2-z1*x2+x1*z2)+(w1*x2-y1*y2+z1*z2)*y1-(w1*y2-z1*x2+x1*z2)*x1;
    int x=floor(x_)+255;
    int y=floor(y_)+255;
    int z=floor(z_)+255;    
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
