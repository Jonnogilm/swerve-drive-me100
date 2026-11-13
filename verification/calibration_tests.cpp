#include "mocks/Arduino.h"
#include "mocks/Wire.h"
#include <assert.h>
uint32_t fakeTime=0;
MockSerial Serial;
MockWire Wire;
#include "../ServoCalibration/Calibration.cpp"
void send(const char *text) { Serial.incoming=text; loop(); }
int main() {
  setup();
  assert(ready && Wire.servoRegisters[0xFD]==0x10);
  send("S 0 1500\n");
  assert(Wire.servoRegisters[8]==0x33 && Wire.servoRegisters[9]==1);
  send("S 1 1500\n");
  assert(Wire.servoRegisters[13]==1);
  send("OFF\n");
  assert(Wire.servoRegisters[0xFD]==0x10 && Wire.servoRegisters[9]==0x10);
  send("S 0 1600\n");
  assert(Wire.servoRegisters[13]==0x10);
  send("S 0 999\n");
  assert(Wire.servoRegisters[8]==0x48); // invalid input leaves 1600 us unchanged
  Wire.servoFail=true; send("S 0 1500\n"); assert(!ready);
  Wire.servoFail=false; send("S 0 1500\n"); assert(!ready);
  setup(); assert(ready && Wire.servoRegisters[0xFD]==0x10);
  return 0;
}
