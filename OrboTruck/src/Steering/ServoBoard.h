#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <math.h>

// Inland/Keyestudio KS0065 (PCA9685). Owns all 16 outputs.
// Keep the copy in ServoCalibration/src/Steering identical so each Arduino
// sketch remains self-contained when the IDE copies it to its build directory.
class ServoBoard {
  uint8_t address_, prescale_=0;
  uint32_t oscillator_;
  bool ready_=false;
  bool healthy() {
    bool ok=!Wire.getWireTimeoutFlag();
    Wire.clearWireTimeoutFlag();
    return ok;
  }
  bool write(uint8_t reg, const uint8_t *data, uint8_t size) {
    Wire.clearWireTimeoutFlag();
    Wire.beginTransmission(address_);
    Wire.write(reg); Wire.write(data,size);
    uint8_t result=Wire.endTransmission();
    return healthy() && result==0;
  }
  bool byte(uint8_t reg,uint8_t value) { return write(reg,&value,1); }
  bool read(uint8_t reg,uint8_t &value) {
    Wire.clearWireTimeoutFlag();
    Wire.beginTransmission(address_); Wire.write(reg);
    uint8_t result=Wire.endTransmission();
    if(!healthy() || result) return false;
    uint8_t count=Wire.requestFrom(address_,(uint8_t)1);
    if(!healthy() || count!=1) {
      while(Wire.available()) Wire.read();
      return false;
    }
    value=(uint8_t)Wire.read(); return true;
  }
public:
  ServoBoard(uint8_t address,uint32_t oscillator):address_(address),oscillator_(oscillator) {}
  bool off() {
    // ALL_LED full-off writes every channel, including stale pulses after
    // an Uno-only reset. Does not remove servo power.
    if(!byte(0xFD,0x10)) return false;
    if(ready_) {
      const uint8_t disabled[4]={0,0,0,0x10};
      for(uint8_t i=0;i<16;i++) if(!write(0x06+4*i,disabled,4)) return false;
    }
    return true;
  }
  bool begin(uint16_t frequency) {
    ready_=false;
    if(address_<0x40 || address_>0x77 || oscillator_<1000000UL ||
       frequency<40 || frequency>100) return false;
    float divider=(float)oscillator_/(4096.0f*frequency);
    if(divider<4 || divider>256) return false;
    prescale_=(uint8_t)(lroundf(divider)-1);
    if(!off() || !byte(0x00,0x30) || !byte(0x01,0x04) ||
       !byte(0xFE,prescale_)) return false;
    // Clear stale per-channel FULL_ON and PWM registers while disabled.
    const uint8_t disabled[4]={0,0,0,0x10};
    for(uint8_t i=0;i<16;i++) if(!write(0x06+4*i,disabled,4)) return false;
    if(!byte(0x00,0x20)) return false; // internal oscillator, auto increment
    delay(1); // oscillator startup >500 us
    if(!byte(0x00,0xA0)) return false;
    uint8_t mode,scale;
    if(!read(0x00,mode) || (mode&0x70)!=0x20 ||
       !read(0xFE,scale) || scale!=prescale_) return false;
    ready_=true; return true;
  }
  bool pulse(uint8_t channel,uint16_t microseconds) {
    if(!ready_ || channel>15 || microseconds<500 || microseconds>2500) return false;
    uint16_t ticks=(uint16_t)lroundf((float)microseconds*oscillator_/
                                    (1000000.0f*(prescale_+1)));
    if(ticks==0 || ticks>=4096) return false;
    const uint8_t data[4]={0,0,(uint8_t)ticks,(uint8_t)(ticks>>8)};
    return write(0x06+4*channel,data,4);
  }
};
