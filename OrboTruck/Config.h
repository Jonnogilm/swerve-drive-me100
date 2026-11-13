#pragma once
#include <stdint.h>

namespace Config {
// Set true ONLY after completing the guide's measurements and power checks.
#ifndef ORBOTRUCK_HARDWARE_CONFIRMED
#define ORBOTRUCK_HARDWARE_CONFIRMED true
#endif
const bool HARDWARE_CONFIRMED = ORBOTRUCK_HARDWARE_CONFIRMED;
const uint8_t DRIVER_ADDRESS = 0x26;
// All dimensions below are EXAMPLES, not measurements of your truck.
const float WHEEL_DIAMETER_MM = 67.0f;
const float WHEELBASE_MM = 200.0f; // steering pivot to steering pivot
const float TRACK_MM = 180.0f;
const uint16_t GEAR_RATIO = 30;
const uint16_t ENCODER_LINES = 11;
// Expected quadrature x4: measure counts for one OUTPUT shaft revolution.
const float COUNTS_PER_WHEEL_REV = 1320.0f;
const uint16_t MOTOR_DEADZONE = 1900; // Yahboom Uno example; tune unloaded

// Logical order: front-left, front-right, rear-left, rear-right.
const uint8_t SERVO_ADDRESS = 0x40; // Inland KS0065/PCA9685, address jumpers open
const uint8_t SERVO_CHANNEL[4] = {0,1,2,3}; // board outputs FL,FR,RL,RR
const uint16_t SERVO_FREQUENCY_HZ = 50;
const uint32_t SERVO_OSCILLATOR_HZ = 25000000UL; // nominal; calibrate if needed
const uint8_t CHANNEL[4] = {0,2,1,3}; // zero based: M1,M3,M2,M4
const int8_t MOTOR_SIGN[4] = {1,1,1,1}; // must be calibrated
const int8_t ENCODER_SIGN[4] = {1,1,1,1}; // independently calibrated
const int8_t SERVO_SIGN[4] = {1,1,1,1};
const uint16_t SERVO_CENTER_US[4] = {1500,1500,1500,1500};
const uint16_t SERVO_MIN_US[4] = {1000,1000,1000,1000};
const uint16_t SERVO_MAX_US[4] = {2000,2000,2000,2000};
const float US_PER_DEGREE[4] = {5.55556f,5.55556f,5.55556f,5.55556f};
// EXAMPLE limits: do not assume an MG996R has 180 degrees of usable travel.
const float MIN_ANGLE[4] = {-90,-90,-90,-90};
const float MAX_ANGLE[4] = {90,90,90,90};

const float DEMO_SPEED_MM_S = 150;
const float MAX_WHEEL_SPEED_MM_S = 500;
const float TEST_SPEED_MM_S = 500; // individual drive tests; capped by maximum above
const float TEST_DISTANCE_MM = 2500; // full ramp to 500 mm/s, then a short cruise
const float ACCEL_MM_S2 = 80;
const float STRAIGHT_MM = 200;
const float SIDEWAYS_MM = 150;
const float DIAGONAL_MM = 150;
const float ORBIT_RADIUS_MM = 400; // point initially straight ahead of truck
const float SPIN_RAD_S = 0.25f;
const uint16_t SETTLE_MS = 1000;
const uint16_t PAUSE_MS = 1000;
const uint16_t TICK_MS = 50;
const uint16_t STALL_MS = 2500;
const uint32_t SEGMENT_TIMEOUT_MS = 120000UL;
const uint32_t I2C_TIMEOUT_US = 25000UL;
const float MIN_MOVING_MM_S = 8;
const float POSITION_TOLERANCE_MM = 2;
}
