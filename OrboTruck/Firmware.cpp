/* OrboTruck - Arduino Uno R3, Wire only; Inland KS0065 steering.
   Read ../README.md BEFORE connecting power. Configuration ships locked.
   Serial: 115200 baud, newline. STOP is a software stop, not a power disconnect.
*/
#include <Arduino.h>
#include <ctype.h>
#include <string.h>
#include "Config.h"
#include "Motion.h"
#include "YahboomDriver.h"
#include "src/Steering/ServoBoard.h"

enum State : uint8_t { LOCKED, READY, CONFIGURING, SETTLING, RUNNING, PAUSING, FAULT };
enum Demo : uint8_t { FORWARD, BACKWARD, LEFT, RIGHT, DIAGONAL, SPIN, ORBIT };
State state=LOCKED;
ServoBoard servo(Config::SERVO_ADDRESS,Config::SERVO_OSCILLATOR_HZ);
YahboomDriver driver;
Motion::Wheel wheel[4];
float commanded[4]={0,0,0,0}, travelled[4], goal[4];
uint32_t lastCount[4], lastMotion[4];
uint32_t since=0, segmentStart=0, lastTick=0, lastHeartbeat=0;
uint8_t sequenceIndex=0, selectedDemo=0, configStep=0;
bool sequence=false, attached=false;
char input[48]; uint8_t inputLength=0; bool overflow=false;

bool validConfiguration() {
  using namespace Config;
  if(!HARDWARE_CONFIRMED || WHEEL_DIAMETER_MM<=0 || WHEELBASE_MM<=0 || TRACK_MM<=0 ||
     COUNTS_PER_WHEEL_REV<=0 || MAX_WHEEL_SPEED_MM_S<=0 || MAX_WHEEL_SPEED_MM_S>1000 ||
     DEMO_SPEED_MM_S<=0 || ACCEL_MM_S2<=0 || ORBIT_RADIUS_MM<=0 || SPIN_RAD_S<=0 ||
     STRAIGHT_MM<=0 || SIDEWAYS_MM<=0 || DIAGONAL_MM<=0 || GEAR_RATIO==0 || ENCODER_LINES==0 ||
     MOTOR_DEADZONE>3600 || POSITION_TOLERANCE_MM<=0) return false;
  if(SERVO_ADDRESS<0x40 || SERVO_ADDRESS>0x77 || SERVO_ADDRESS==DRIVER_ADDRESS ||
     SERVO_FREQUENCY_HZ<40 || SERVO_FREQUENCY_HZ>100) return false;
  uint8_t mask=0; uint16_t servoMask=0;
  for(uint8_t i=0;i<4;i++) {
    if(CHANNEL[i]>3 || (mask & (1<<CHANNEL[i]))) return false;
    mask|=1<<CHANNEL[i];
    if(SERVO_CHANNEL[i]>15 || (servoMask & (1U<<SERVO_CHANNEL[i]))) return false;
    servoMask|=1U<<SERVO_CHANNEL[i];
    if(SERVO_MIN_US[i]<500 || SERVO_MAX_US[i]>2500) return false;
    if(abs(MOTOR_SIGN[i])!=1 || abs(ENCODER_SIGN[i])!=1 || abs(SERVO_SIGN[i])!=1 ||
       US_PER_DEGREE[i]<=0 || MIN_ANGLE[i]>0 || MAX_ANGLE[i]<0 ||
       MIN_ANGLE[i]<-180 || MAX_ANGLE[i]>180 || MIN_ANGLE[i]>=MAX_ANGLE[i] ||
       SERVO_MIN_US[i]>=SERVO_CENTER_US[i] || SERVO_CENTER_US[i]>=SERVO_MAX_US[i]) return false;
    for(uint8_t j=0;j<2;j++) {
      float angle=j?MAX_ANGLE[i]:MIN_ANGLE[i];
      float pulse=SERVO_CENTER_US[i]+SERVO_SIGN[i]*US_PER_DEGREE[i]*angle;
      if(pulse<SERVO_MIN_US[i]-1 || pulse>SERVO_MAX_US[i]+1) return false;
    }
  }
  return true;
}
void fault(const __FlashStringHelper *reason) {
  state=FAULT; sequence=false;
  for(uint8_t i=0;i<4;i++) commanded[i]=0;
  driver.stop(); // bounded, best effort; cut motor power if unreachable
  Serial.print(F("FAULT: ")); Serial.println(reason);
}
void stopNow() {
  bool wasFault=state==FAULT;
  sequence=false;
  for(uint8_t i=0;i<4;i++) commanded[i]=0;
  if(!driver.stop()) { fault(F("STOP not acknowledged: disconnect motor power")); return; }
  state=wasFault?FAULT:LOCKED; // STOP disarms; CONFIRM required again
  Serial.println(F("Stopped/disarmed. Servos hold position. CONFIRM to re-arm."));
}
bool buildDemo(uint8_t demo) {
  using namespace Config;
  if(demo>=7) {
    uint8_t selected=demo-7;
    if(selected>=4) return false;
    for(uint8_t i=0;i<4;i++) {
      wheel[i]={0,i==selected?fminf(20,MAX_WHEEL_SPEED_MM_S):0};
      goal[i]=i==selected?40:0;
    }
    return true;
  }
  float vx=0,vy=0,omega=0,duration=0;
  switch(demo) {
    case FORWARD: vx=DEMO_SPEED_MM_S; duration=STRAIGHT_MM/DEMO_SPEED_MM_S; break;
    case BACKWARD: vx=-DEMO_SPEED_MM_S; duration=STRAIGHT_MM/DEMO_SPEED_MM_S; break;
    case LEFT: vy=DEMO_SPEED_MM_S; duration=SIDEWAYS_MM/DEMO_SPEED_MM_S; break;
    case RIGHT: vy=-DEMO_SPEED_MM_S; duration=SIDEWAYS_MM/DEMO_SPEED_MM_S; break;
    case DIAGONAL: vx=vy=DEMO_SPEED_MM_S*0.70710678f; duration=DIAGONAL_MM/DEMO_SPEED_MM_S; break;
    case SPIN: omega=SPIN_RAD_S; duration=2*Motion::PI_F/omega; break;
    default:
      // Target initially +radius along body x. CCW motion has vy=-omega*radius.
      omega=DEMO_SPEED_MM_S/ORBIT_RADIUS_MM;
      vy=-DEMO_SPEED_MM_S; duration=2*Motion::PI_F/omega; break;
  }
  Motion::Wheel candidate[4]; float peak=0;
  for(uint8_t i=0;i<4;i++) {
    float x=(i<2?1:-1)*WHEELBASE_MM/2;
    float y=((i==0||i==2)?1:-1)*TRACK_MM/2;
    if(!Motion::solve(vx,vy,omega,x,y,MIN_ANGLE[i],MAX_ANGLE[i],wheel[i].angle,candidate[i])) return false;
    peak=fmaxf(peak,fabsf(candidate[i].speed));
  }
  float scale=peak>MAX_WHEEL_SPEED_MM_S ? MAX_WHEEL_SPEED_MM_S/peak : 1;
  for(uint8_t i=0;i<4;i++) {
    goal[i]=fabsf(candidate[i].speed)*duration;
    candidate[i].speed*=scale; wheel[i]=candidate[i];
  }
  return true;
}
void startSegment() {
  uint8_t demo=sequence?sequenceIndex:selectedDemo;
  if(!buildDemo(demo)) { fault(F("Steering angle unreachable; check travel/linkage")); return; }
  if(!driver.stop()) { fault(F("I2C stop before steering")); return; }
  for(uint8_t i=0;i<4;i++) {
    commanded[i]=0;
    int pulse=lroundf(Config::SERVO_CENTER_US[i]+Config::SERVO_SIGN[i]*Config::US_PER_DEGREE[i]*wheel[i].angle);
    pulse=constrain(pulse,Config::SERVO_MIN_US[i],Config::SERVO_MAX_US[i]);
    if(!servo.pulse(Config::SERVO_CHANNEL[i],pulse)) {
      fault(F("Steering write failed; disconnect power")); return;
    }
  }
  attached=true;
  state=SETTLING; since=millis();
  Serial.print(F("Segment ")); Serial.println(demo);
}
void startRun(bool all,uint8_t demo) {
  if(state!=READY) { Serial.println(F("Not armed/idle. Set configuration, then CONFIRM.")); return; }
  sequence=all; sequenceIndex=0; selectedDemo=demo; startSegment();
}
void status() {
  Serial.print(F("State=")); Serial.print((uint8_t)state);
  Serial.println(F(" (0 locked,1 ready,2 config,3 settle,4 run,5 pause,6 fault)"));
  Serial.print(F("HARDWARE_CONFIRMED=")); Serial.println(Config::HARDWARE_CONFIRMED);
  for(uint8_t i=0;i<4;i++) {
    Serial.print(i); Serial.print(F(": angle=")); Serial.print(wheel[i].angle,1);
    Serial.print(F(" speed=")); Serial.print(commanded[i],1);
    Serial.print(F(" progress/goal mm=")); Serial.print(travelled[i],1);
    Serial.print('/'); Serial.println(goal[i],1);
  }
}
void command() {
  if(!strcmp(input,"STOP")) { stopNow(); return; }
  if(!strcmp(input,"STATUS")) { status(); return; }
  if(!strcmp(input,"ENCODERS")) {
    if(state==RUNNING || state==SETTLING) { Serial.println(F("Read raw counts while idle.")); return; }
    for(uint8_t i=0;i<4;i++) {
      uint32_t count;
      if(!driver.encoder(Config::CHANNEL[i],count)) { fault(F("Encoder query failed")); return; }
      Serial.print(i); Serial.print(':'); Serial.println((int32_t)count);
    }
    return;
  }
  if(!strcmp(input,"CONFIRM")) {
    if(state!=LOCKED) { Serial.println(F("CONFIRM only while locked; faults require reset.")); return; }
    if(!validConfiguration()) { Serial.println(F("Configuration locked/invalid. Read Config.h and guide.")); return; }
    if(!driver.stop()) { fault(F("Driver unavailable")); return; }
    if(!servo.begin(Config::SERVO_FREQUENCY_HZ)) { fault(F("Servo board unavailable/configuration failed")); return; }
    attached=false;
    state=CONFIGURING; configStep=0; since=millis(); return;
  }
  if(!strcmp(input,"START")) { startRun(true,0); return; }
  const char *names[]={"FORWARD","BACKWARD","LEFT","RIGHT","DIAGONAL","SPIN","ORBIT"};
  for(uint8_t i=0;i<7;i++) if(!strcmp(input,names[i])) { startRun(false,i); return; }
  const char *tests[]={"TESTFL","TESTFR","TESTRL","TESTRR"};
  for(uint8_t i=0;i<4;i++) if(!strcmp(input,tests[i])) { startRun(false,i+7); return; }
  Serial.println(F("CONFIRM START STOP STATUS ENCODERS FORWARD BACKWARD LEFT RIGHT DIAGONAL SPIN ORBIT TESTFL TESTFR TESTRL TESTRR"));
}
void pollSerial() {
  // Bounded work per loop; a continuous serial stream cannot starve motor service.
  for(uint8_t n=0;n<32 && Serial.available();n++) {
    char c=Serial.read();
    if(c=='\r'||c=='\n') {
      if(overflow) Serial.println(F("Command too long; discarded."));
      else if(inputLength) { input[inputLength]=0; command(); }
      inputLength=0; overflow=false;
    } else if(!overflow) {
      if(inputLength<sizeof(input)-1) input[inputLength++]=toupper((unsigned char)c);
      else overflow=true;
    }
  }
}
void controlTick(uint32_t now) {
  using namespace Config;
  if(now-segmentStart>SEGMENT_TIMEOUT_MS) { fault(F("Segment timeout")); return; }
  bool done=true;
  float peak=0;
  for(uint8_t i=0;i<4;i++) peak=fmaxf(peak,fabsf(wheel[i].speed));
  for(uint8_t i=0;i<4;i++) {
    uint32_t count;
    if(!driver.encoder(CHANNEL[i],count)) { fault(F("Encoder I2C read failed")); return; }
    int32_t delta=Motion::countDelta(count,lastCount[i]); lastCount[i]=count;
    float mm=(float)delta*ENCODER_SIGN[i]*(Motion::PI_F*WHEEL_DIAMETER_MM/COUNTS_PER_WHEEL_REV);
    // Signed projection prevents backwards encoder counts from completing a path.
    float along=wheel[i].speed>=0?mm:-mm;
    if(fabsf(mm)>MAX_WHEEL_SPEED_MM_S*0.05f*4+5 || along < -3) {
      fault(F("Implausible/wrong-direction encoder")); return;
    }
    travelled[i]+=along;
    float remaining=goal[i]-travelled[i];
    float target=0;
    if(remaining>POSITION_TOLERANCE_MM && goal[i]>POSITION_TOLERANCE_MM) {
      done=false;
      // Acceleration scales with each wheel's final speed to preserve ratios.
      target=wheel[i].speed;
      if(along>0.05f || fabsf(commanded[i])<MIN_MOVING_MM_S) lastMotion[i]=now;
      if(now-lastMotion[i]>STALL_MS) { fault(F("Wheel stalled / encoder not advancing")); return; }
    }
    float step=peak>0 ? ACCEL_MM_S2*TICK_MS/1000.0f*fabsf(wheel[i].speed)/peak : 0;
    commanded[i]=Motion::approach(commanded[i],target,step);
  }
  if(done) {
    if(!driver.stop()) { fault(F("End-of-segment stop failed")); return; }
    for(uint8_t i=0;i<4;i++) commanded[i]=0;
    state=PAUSING; since=millis(); return;
  }
  if(!driver.speeds(commanded)) fault(F("Motor command failed; disconnect power"));
}
void setup() {
  Serial.begin(115200); driver.begin();
  for(uint8_t i=0;i<4;i++) wheel[i]={0,0};
  Serial.println(F("OrboTruck Uno R3. Locked. Read guide, then CONFIRM. HELP for commands."));
  bool stopped=driver.stop();
  bool disabled=servo.begin(Config::SERVO_FREQUENCY_HZ);
  if(!stopped) fault(F("Boot stop failed; driver absent or bus error"));
  else if(!disabled) fault(F("Servo board absent/configuration failed"));
}
void loop() {
  pollSerial(); uint32_t now=millis();
  if(state==CONFIGURING && now-since>=100) {
    if(!driver.configureStep(configStep++)) { fault(F("Driver configuration write failed")); return; }
    since=now;
    if(configStep==5) { state=READY; Serial.println(F("Ready. START runs once; STOP disarms.")); }
  }
  if(state==SETTLING && now-since>=Config::SETTLE_MS) {
    for(uint8_t i=0;i<4;i++) {
      if(!driver.encoder(Config::CHANNEL[i],lastCount[i])) { fault(F("Encoder baseline failed")); return; }
      travelled[i]=0; lastMotion[i]=millis();
    }
    segmentStart=lastTick=millis(); state=RUNNING;
    return; // next pass samples a fresh time after the I2C baseline reads
  }
  if(state==RUNNING && now-lastTick>=Config::TICK_MS) {
    if(now-lastTick>250) { fault(F("Control loop late")); return; }
    lastTick=now; controlTick(now);
  }
  if(state==PAUSING && now-since>=Config::PAUSE_MS) {
    if(sequence && ++sequenceIndex<7) startSegment();
    else { sequence=false; state=READY; Serial.println(F("Complete. Motors stopped.")); }
  }
  // Reissue stop while idle/faulted; never auto-resume following driver recovery.
  if(state!=RUNNING && now-lastHeartbeat>=250) {
    lastHeartbeat=now;
    if(!driver.stop() && state!=FAULT) fault(F("Idle stop failed"));
  }
}
