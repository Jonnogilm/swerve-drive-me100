#include "mocks/Arduino.h"
#include "mocks/Wire.h"
uint32_t fakeTime=0;
MockSerial Serial;
MockWire Wire;
#include "../OrboTruck/Firmware.cpp"
extern "C" {
volatile uint8_t testResults[64];
volatile uint8_t testCount=0;
volatile uint8_t testDone=0;
}
int main() {
  setup();
  testResults[testCount++]=!validConfiguration()?1:2;
  strcpy(input,"CONFIRM"); command();
  testResults[testCount++]=state==LOCKED && !attached?1:2;
  strcpy(input,"START"); command();
  testResults[testCount++]=state==LOCKED && Wire.motor[0]==0?1:2;
  testDone=1;
#ifdef HOST_TEST
  for(uint8_t i=0;i<testCount;i++) if(testResults[i]!=1) return i+1;
  return 0;
#else
  for(;;) {}
#endif
}
