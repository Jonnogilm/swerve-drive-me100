#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
typedef char __FlashStringHelper;
#define F(x) (x)
#define constrain(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
extern uint32_t fakeTime;
inline uint32_t millis() { return fakeTime; }
inline void delay(unsigned long ms) { fakeTime+=ms; }
struct MockSerial {
  const char *incoming=nullptr;
  void begin(unsigned long) {}
  int available() { return incoming && *incoming; }
  char read() { return *incoming++; }
  template<class T> void print(T) {}
  template<class T> void print(T,int) {}
  template<class T> void println(T) {}
  template<class T> void println(T,int) {}
};
extern MockSerial Serial;
