// Runs the actual sketch and motion code on an emulated ATmega328P.
// Only GPIO/Serial/Wire/time are replaced; no duplicate motion implementation.
#define ORBOTRUCK_HARDWARE_CONFIRMED true
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
void check(bool pass) { testResults[testCount++]=pass?1:2; }
bool near(float a,float b,float tolerance=0.01f) { return fabsf(a-b)<tolerance; }
void send(const char *s) { strcpy(input,s); command(); }
void clearState() {
  Wire.fail=false; Wire.shortRead=false; Wire.timeout=false;
  state=LOCKED; sequence=false; attached=false; inputLength=0; overflow=false;
  for(uint8_t i=0;i<4;i++) {
    wheel[i]={0,0}; commanded[i]=0; travelled[i]=goal[i]=0;
    Wire.counts[i]=0; Wire.motor[i]=0;
  }
}
void arm() {
  send("CONFIRM");
  for(uint8_t i=0;i<6;i++) { fakeTime+=101; loop(); }
}
void simulationTick(bool move=true) {
  static float remainder[4]={0,0,0,0};
  if(move) for(uint8_t i=0;i<4;i++) {
    float delta=Wire.motor[i]*0.05f*Config::COUNTS_PER_WHEEL_REV/(Motion::PI_F*Config::WHEEL_DIAMETER_MM);
    remainder[i]+=delta;
    int32_t n=(int32_t)remainder[i]; remainder[i]-=n; Wire.counts[i]+=(uint32_t)n;
  }
  fakeTime+=50; loop();
}
int main() {
  Motion::Wheel w;
  check(Motion::solve(100,0,0,100,100,-90,90,0,w) && near(w.angle,0) && near(w.speed,100));
  check(Motion::solve(-100,0,0,100,100,-90,90,0,w) && near(w.angle,0) && near(w.speed,-100));
  check(Motion::solve(0,100,0,0,0,-90,90,0,w) && near(fabsf(w.angle),90));
  check(Motion::solve(100,100,0,0,0,-90,90,0,w) && near(w.angle,45));
  check(!Motion::solve(0,100,0,0,0,-45,45,0,w));
  check(Motion::solve(0,0,0,0,0,-90,90,37,w) && near(w.angle,37) && near(w.speed,0));
  check(Motion::countDelta(1,0xFFFFFFFFUL)==2 && Motion::countDelta(0xFFFFFFFFUL,1)==-2);
  check(near(Motion::approach(0,10,4),4) && near(Motion::approach(9,10,4),10));
  check(validConfiguration());
  setup(); check(state==LOCKED && Wire.motor[0]==0 && !attached);
  check(Wire.servoRegisters[0xFD]==0x10 && Wire.servoRegisters[0xFE]==121);
  check(servo.pulse(3,1500) && Wire.servoRegisters[0x14]==0x33 && Wire.servoRegisters[0x15]==1);
  check(!servo.pulse(16,1500) && !servo.pulse(0,0));
  Wire.servoFail=true; send("CONFIRM"); check(state==FAULT && Wire.motor[0]==0);
  Wire.servoFail=false; clearState();
  send("START"); check(state==LOCKED);
  arm(); check(state==READY && Wire.configs==31);
  for(uint8_t d=0;d<7;d++) check(buildDemo(d));
  check(buildDemo(ORBIT));
  // Reconstruct wheel vector: for orbit, u=-omega*y, v=omega*(x-radius).
  float omega=Config::DEMO_SPEED_MM_S/Config::ORBIT_RADIUS_MM;
  check(near(wheel[0].speed*cosf(wheel[0].angle*Motion::PI_F/180),-omega*Config::TRACK_MM/2));
  check(near(wheel[0].speed*sinf(wheel[0].angle*Motion::PI_F/180),omega*(Config::WHEELBASE_MM/2-Config::ORBIT_RADIUS_MM)));
  float values[4]={200,-200,0,500}; driver.speeds(values);
  check(Wire.lastSpeed[0]==0 && Wire.lastSpeed[1]==200 && Wire.lastSpeed[4]==255 && Wire.lastSpeed[5]==56);
  Wire.counts[0]=0xFFFF8001UL; uint32_t count=0;
  check(driver.encoder(0,count) && count==0xFFFF8001UL);
  Wire.beginTransmission(Config::DRIVER_ADDRESS); Wire.write(0x21);
  Wire.endTransmission();
  check(Wire.requestFrom(Config::DRIVER_ADDRESS,(uint8_t)2)==0);
  uint16_t low=0;
  check(driver.read16(0x21,low) && low==0x8001);
  driver.configureStep(3);
  check(Wire.tx[0]==5 && Wire.tx[1]==0 && Wire.tx[2]==0 && Wire.tx[3]==0x86 && Wire.tx[4]==0x42);
  clearState(); arm(); send("TESTFL");
  check(attached && Wire.servoRegisters[9]==1 && Wire.servoRegisters[21]==1);
  float testPeak=0;
  for(uint16_t n=0;n<400 && state!=READY && state!=FAULT;n++) {
    simulationTick();
    testPeak=fmaxf(testPeak,Wire.motor[0]);
  }
  check(state==READY && near(testPeak,fminf(Config::TEST_SPEED_MM_S,Config::MAX_WHEEL_SPEED_MM_S)) &&
        Wire.counts[0]>0 && Wire.counts[1]==0 && Wire.counts[2]==0 && Wire.counts[3]==0);
  clearState(); arm(); send("START");
  check(state==SETTLING && Wire.motor[0]==0);
  for(uint16_t n=0;n<4000 && state!=READY && state!=FAULT;n++) simulationTick();
  check(state==READY && !sequence && sequenceIndex==7);
  check(Wire.motor[0]==0 && Wire.motor[1]==0 && Wire.motor[2]==0 && Wire.motor[3]==0);
  send("FORWARD"); simulationTick(); send("STOP");
  check(state==LOCKED && Wire.motor[0]==0);
  arm(); send("FORWARD");
  for(uint8_t n=0;n<100 && state!=FAULT;n++) simulationTick(false);
  check(state==FAULT); // stalled wheel
#ifdef HOST_TEST
  check(Serial.output.find("driver=M1")!=std::string::npos &&
        Serial.output.find("Command mm/s=150")!=std::string::npos &&
        Wire.motor[0]==0 && Wire.motor[1]==0 && Wire.motor[2]==0 && Wire.motor[3]==0);
#endif
  send("CONFIRM"); check(state==FAULT); // fault stays latched
  // A 500 mm/s target cannot bypass a frozen encoder during the slow ramp.
  clearState(); arm(); send("TESTFL");
  float stalledPeak=0;
  for(uint16_t n=0;n<200 && state!=FAULT;n++) {
    simulationTick(false);
    stalledPeak=fmaxf(stalledPeak,fabsf(commanded[0]));
  }
  check(state==FAULT && stalledPeak>0 && stalledPeak<Config::TEST_SPEED_MM_S && Wire.motor[0]==0);
  clearState(); arm(); send("FORWARD"); Wire.fail=true; fakeTime+=1000; loop();
  check(state==FAULT);
  Wire.fail=false; fakeTime+=300; loop(); check(state==FAULT && Wire.motor[0]==0);
  clearState(); arm(); Wire.shortRead=true; send("FORWARD"); fakeTime+=1100; loop();
  check(state==FAULT); // short encoder read
  clearState(); arm(); send("FORWARD");
  fakeTime+=1100; loop(); // enter RUNNING
  segmentStart=fakeTime-Config::SEGMENT_TIMEOUT_MS-1; fakeTime+=50; loop();
  check(state==FAULT);
  clearState(); arm(); send("FORWARD"); fakeTime+=1100; loop();
  Wire.counts[0]-=100; fakeTime+=50; loop(); check(state==FAULT);
  clearState(); Serial.incoming="XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXSTART\n";
  pollSerial(); pollSerial(); check(state==LOCKED && inputLength==0);
  Serial.incoming="STATUS\r\n"; pollSerial(); check(inputLength==0);
  clearState(); arm(); Wire.servoFail=true; send("FORWARD");
  check(state==FAULT && Wire.motor[0]==0 && !attached);
  Wire.servoFail=false; clearState(); Wire.servoFail=true; setup();
  check(state==FAULT && Wire.motor[0]==0);
  Wire.servoFail=false;
  testDone=1;
#ifdef HOST_TEST
  for(uint8_t i=0;i<testCount;i++) if(testResults[i]!=1) return i+1;
  return 0;
#else
  for(;;) {}
#endif
}
