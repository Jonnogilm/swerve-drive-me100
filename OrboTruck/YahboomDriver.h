#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "Config.h"

// Yahboom Quad-MD register protocol (manufacturer repository de79d83).
// No firmware-ID register is documented: ACK is NOT a firmware identity check.
class YahboomDriver {
  bool healthy() {
    if (Wire.getWireTimeoutFlag()) { Wire.clearWireTimeoutFlag(); return false; }
    return true;
  }
public:
  void begin() {
    Wire.begin();
    Wire.setClock(100000UL);
    Wire.setWireTimeout(Config::I2C_TIMEOUT_US, true);
  }
  bool write(uint8_t reg, const uint8_t *data, uint8_t size) {
    Wire.clearWireTimeoutFlag();
    Wire.beginTransmission(Config::DRIVER_ADDRESS);
    Wire.write(reg); Wire.write(data,size);
    uint8_t result = Wire.endTransmission();
    return healthy() && result == 0;
  }
  bool read16(uint8_t reg, uint16_t &value) {
    Wire.clearWireTimeoutFlag();
    Wire.beginTransmission(Config::DRIVER_ADDRESS); Wire.write(reg);
    // Match Yahboom's i2cRead: keep register selection and read in one
    // transaction using a repeated START, without an intervening STOP.
    uint8_t result = Wire.endTransmission(false);
    if (!healthy() || result) return false;
    uint8_t count = Wire.requestFrom(Config::DRIVER_ADDRESS, (uint8_t)2);
    if (!healthy() || count != 2) { while(Wire.available()) Wire.read(); return false; }
    value = ((uint16_t)Wire.read()<<8); value |= (uint8_t)Wire.read();
    return true;
  }
  bool write16(uint8_t reg, uint16_t value) {
    uint8_t b[2] = {(uint8_t)(value>>8),(uint8_t)value}; return write(reg,b,2);
  }
  bool configureStep(uint8_t step) {
    switch(step) {
      case 0: { uint8_t type=1; return write(0x01,&type,1); }
      case 1: return write16(0x04,Config::GEAR_RATIO);
      case 2: return write16(0x03,Config::ENCODER_LINES);
      case 3: {
        static_assert(sizeof(float)==4,"IEEE float32 required");
        union { float f; uint32_t u; } v; v.f=Config::WHEEL_DIAMETER_MM;
        uint8_t b[4]; for(uint8_t i=0;i<4;i++) b[i]=(uint8_t)(v.u>>(8*i));
        return write(0x05,b,4); // diameter is little-endian; integers big-endian
      }
      default: return write16(0x02,Config::MOTOR_DEADZONE);
    }
  }
  bool speeds(const float logical[4]) {
    int16_t channel[4]={0,0,0,0}; uint8_t b[8];
    for(uint8_t i=0;i<4;i++) channel[Config::CHANNEL[i]]=(int16_t)lroundf(logical[i]*Config::MOTOR_SIGN[i]);
    for(uint8_t i=0;i<4;i++) { uint16_t v=(uint16_t)channel[i]; b[2*i]=v>>8; b[2*i+1]=v; }
    return write(0x06,b,8); // wheel surface speed, mm/s; board closes speed loop
  }
  bool stop() { float z[4]={0,0,0,0}; return speeds(z); }
  bool encoder(uint8_t channel, uint32_t &value) {
    // High-low-high read avoids a torn counter at a 16-bit rollover.
    uint8_t reg=0x20+2*channel;
    for(uint8_t attempt=0;attempt<2;attempt++) {
      uint16_t hi,lo,check;
      if(!read16(reg,hi)||!read16(reg+1,lo)||!read16(reg,check)) return false;
      if(hi==check) { value=((uint32_t)hi<<16)|lo; return true; }
    }
    return false;
  }
};
