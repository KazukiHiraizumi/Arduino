#include Timeout.h

int s2m_ev1=0;
char s2m_buf[20];
int s2m_bp=0;
void s2m_setup() {
  Serial2.begin(115200);
  ev1=Timeout.set
}
void (*ble_cb)(char *)=NULL;
void ble_loop(){
  if(Serial2.available()){
    if(ble_ev1!=0) Timeout.clear(ble_ev1);
    ble_buf[ble_bp++]=Serial2.read();
    ble_buf[ble_bp]=0;
    ble_ev1=Timeout.set([](){
      (*ble_cb)(ble_buf);
       ble_ev1=ble_bp=0;
    },5);
  }
}
