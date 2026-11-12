// STEERING ONLY. Disconnect drive battery. Use an external rated servo supply.
// Remove horns/linkage for the first test. Serial 115200, newline.
// S 0 1500 -> D4/front-left, 1500 us. Channels 0..3 map to D4..D7.
// OFF detaches signals. This does not disconnect servo power.
#include <Arduino.h>
#include <Servo.h>
#include <string.h>
#include <stdlib.h>
Servo steering[4];
char line[32]; uint8_t used=0; bool tooLong=false;
void setup() {
  Serial.begin(115200);
  Serial.println(F("Servo calibration. S <0..3> <1000..2000 microseconds>, OFF"));
}
void loop() {
  for(uint8_t n=0;n<32 && Serial.available();n++) {
    char c=Serial.read();
    if(c=='\r'||c=='\n') {
      if(used && !tooLong) {
        line[used]=0;
        if(!strcmp(line,"OFF")) { for(uint8_t i=0;i<4;i++) steering[i].detach(); }
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
            if(!steering[channel].attached()) steering[channel].attach(4+channel,1000,2000);
            steering[channel].writeMicroseconds(pulse);
            Serial.println(F("Pulse applied. Observe angle and current."));
          } else Serial.println(F("Invalid command; no change."));
        }
      }
      used=0; tooLong=false;
    } else if(!tooLong) {
      if(used<sizeof(line)-1) line[used++]=c; else tooLong=true;
    }
  }
}
