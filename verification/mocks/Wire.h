#pragma once
#include <stdint.h>
struct MockWire {
  bool fail=false, timeout=false, shortRead=false, servoFail=false;
  bool motorReadPending=false;
  uint8_t address=0, servoRegisters[256]={0};
  uint8_t reg=0, used=0, readIndex=0, tx[12], rx[2], lastSpeed[8];
  uint8_t configs=0;
  int16_t motor[4]={0,0,0,0};
  uint32_t counts[4]={0,0,0,0};
  void begin() {}
  void setClock(uint32_t) {}
  void setWireTimeout(uint32_t,bool) {}
  bool getWireTimeoutFlag() { return timeout; }
  void clearWireTimeoutFlag() { timeout=false; }
  void beginTransmission(uint8_t a) { address=a; used=0; }
  void write(uint8_t b) { tx[used++]=b; }
  void write(const uint8_t *b,uint8_t n) { for(uint8_t i=0;i<n;i++) write(b[i]); }
  uint8_t endTransmission(bool sendStop=true) {
    if(fail || (address==0x40 && servoFail)) return 2;
    if(address==0x40) {
      reg=tx[0];
      for(uint8_t i=1;i<used;i++) {
        uint8_t r=reg+i-1;
        servoRegisters[r]=tx[i];
        // ALL_LED registers write the corresponding register of all channels.
        if(r>=0xFA && r<=0xFD) for(uint8_t ch=0;ch<16;ch++)
          servoRegisters[0x06+4*ch+(r-0xFA)]=tx[i];
      }
      return 0;
    }
    // Require the repeated-start register read used by Yahboom's example.
    // The previous mock accepted STOP here, hiding a driver mismatch.
    motorReadPending=(used==1 && !sendStop);
    reg=tx[0];
    if(used>1 && reg>=1 && reg<=5) configs|=1<<(reg-1);
    if(reg==6 && used==9) for(uint8_t i=0;i<4;i++) {
      motor[i]=(int16_t)(((uint16_t)tx[2*i+1]<<8)|tx[2*i+2]);
      lastSpeed[2*i]=tx[2*i+1]; lastSpeed[2*i+1]=tx[2*i+2];
    }
    return 0;
  }
  uint8_t requestFrom(uint8_t a,uint8_t size) {
    if(a==0x40) {
      if(fail || servoFail || shortRead) return 0;
      rx[0]=servoRegisters[reg]; readIndex=0; return size;
    }
    if(fail || !motorReadPending) return 0;
    motorReadPending=false;
    uint16_t v=0;
    if(reg>=0x20 && reg<=0x27) {
      uint32_t value=counts[(reg-0x20)/2]; v=(reg&1)?value:value>>16;
    }
    rx[0]=v>>8; rx[1]=v; readIndex=0;
    return shortRead?1:2;
  }
  int available() { return readIndex<2; }
  uint8_t read() { return rx[readIndex++]; }
};
extern MockWire Wire;
