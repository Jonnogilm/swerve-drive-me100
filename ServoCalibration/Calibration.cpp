// STEERING ONLY. Disconnect drive battery. Use an external rated servo supply.
// Remove horns/linkage for the first test. Serial 115200, newline.
// S 0 1500 -> front-left on Inland KS0065 output 0.
// OFF disables all 16 outputs. This does not disconnect servo power.
#include <Arduino.h>
#include <Wire.h>
#include "src/Steering/ServoBoard.h"
#include <string.h>
#include <stdlib.h>
// Keep these settings consistent with OrboTruck/Config.h.
const uint8_t SERVO_ADDRESS=0x40;
const uint8_t SERVO_CHANNEL[4]={0,1,2,3};
const uint16_t SERVO_FREQUENCY_HZ=50;
const uint32_t SERVO_OSCILLATOR_HZ=25000000UL;
ServoBoard steering(SERVO_ADDRESS,SERVO_OSCILLATOR_HZ);
bool ready=false;
void busFault() {
  ready=false;
  steering.off(); // best effort; power disconnect needed if bus is unreachable
  Serial.println(F("Servo I2C fault. Disconnect servo power, correct wiring, reset."));
}
char line[32]; uint8_t used=0; bool tooLong=false;
void setup() {
  Serial.begin(115200);
  Wire.begin(); Wire.setClock(100000UL); Wire.setWireTimeout(25000UL,true);
  ready=steering.begin(SERVO_FREQUENCY_HZ);
  if(!ready) busFault();
  Serial.println(F("Servo calibration. S <0..3> <1000..2000 microseconds>, OFF"));
}
void loop() {
  for(uint8_t n=0;n<32 && Serial.available();n++) {
    char c=Serial.read();
    if(c=='\r'||c=='\n') {
      if(used && !tooLong) {
        line[used]=0;
        if(!strcmp(line,"OFF")) {
          if(!steering.off()) busFault();
          else Serial.println(F("All servo outputs disabled; power remains connected."));
        }
        else {
          char *end;
          long channel=-1, pulse=0;
          if(line[0]=='S' && line[1]==' ') {
            channel=strtol(line+2,&end,10);
            if(end!=line+2 && *end==' ') {
              char *begin=end+1;
              pulse=strtol(begin,&end,10);
              if(end==begin || *end) pulse=0;
            }
          }
          if(channel>=0 && channel<4 && pulse>=1000 && pulse<=2000) {
            if(!ready) Serial.println(F("Fault latched; reset after correcting wiring."));
            else if(!steering.pulse(SERVO_CHANNEL[channel],pulse)) busFault();
            else Serial.println(F("Pulse applied. Observe angle and current."));
          } else Serial.println(F("Invalid command; no change."));
        }
      }
      used=0; tooLong=false;
    } else if(!tooLong) {
      if(used<sizeof(line)-1) line[used++]=c; else tooLong=true;
    }
  }
}
