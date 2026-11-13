#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#ifdef HOST_TEST
#include <sstream>
#include <string>
#endif
typedef char __FlashStringHelper;
#define F(x) (x)
#define constrain(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
extern uint32_t fakeTime;
inline uint32_t millis() { return fakeTime; }
inline void delay(unsigned long ms) { fakeTime+=ms; }
struct MockSerial {
  const char *incoming=nullptr;
#ifdef HOST_TEST
  std::string output;
  template<class T> void print(T value) { std::ostringstream s; s<<value; output+=s.str(); }
  template<class T> void print(T value,int) { print(value); }
  template<class T> void println(T value) { print(value); output+='\n'; }
  template<class T> void println(T value,int) { println(value); }
#else
  template<class T> void print(T) {}
  template<class T> void print(T,int) {}
  template<class T> void println(T) {}
  template<class T> void println(T,int) {}
#endif
  void begin(unsigned long) {}
  int available() { return incoming && *incoming; }
  char read() { return *incoming++; }
};
extern MockSerial Serial;
