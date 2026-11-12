#pragma once
struct Servo {
  int pulse=0;
  void attach(int,int,int) {}
  void writeMicroseconds(int value) { pulse=value; }
};
