# Verification

## Results, 12 November 2026

| Check | Result |
|---|---|
| Arduino Uno R3 target, default locked configuration | Compiled: 14,612 / 32,256 flash bytes; 970 / 2,048 static RAM bytes |
| Arduino Uno R3 target, confirmation macro enabled | Compiled: 15,392 / 32,256 flash bytes; 970 / 2,048 static RAM bytes |
| Steering-only calibration sketch, Uno R3 | Compiled: 4,010 / 32,256 flash bytes; 278 / 2,048 static RAM bytes |
| Actual firmware + motion logic, emulated ATmega328P with mock peripherals | 39 / 39 assertions passed |
| Shipped configuration lock, emulated ATmega328P | 3 / 3 assertions passed |

Build environment: Arduino AVR Boards **1.8.8**, Servo **1.3.0**, Wire **1.0**, AVR-GCC **15.2.0 x64**, Universal Ctags **6.1.0**, Arduino CLI from the installed Arduino IDE. The installed stock 32-bit AVR-GCC/ctags executables could not launch in this Windows environment (DLL relocation error). A separate 64-bit toolchain was used with CLI path overrides. The installed Arduino toolchain was not replaced. This establishes an actual ATmega328P firmware build, but is not a claim that the stock GCC 7.3 toolchain was successfully exercised here.

Both `.ino` entry files intentionally contain only comments. Their neighboring `.cpp` tabs contain setup/loop and compile as ordinary Arduino sketch source. This also avoids dependence on Arduino's automatic function-prototype generation.

The armed build leaves **1,078 bytes** beyond static allocations for stack and runtime needs. This is a compiler allocation report, not a measured worst-case stack watermark. The firmware uses fixed buffers and no Arduino String allocation.

## What the emulator checks

Tests include forward/reverse/sideways/diagonal vectors, retained zero-speed angle, rejected unreachable steering, equivalent-angle drive reversal, shortest steering selection for tested cases, encoder counter wraparound, bounded speed ramps, configured wheel mapping, speed byte order, little-endian float diameter, accumulated encoder reconstruction, orbit wheel-vector geometry, one-wheel commissioning completion, and the complete seven-segment sequence reaching stopped/ready state.

Fault scenarios cover a stalled wheel, wrong-direction/unreasonable encoder movement, a segment timeout, failed I²C writes, short encoder reads, latched faults after communication recovery, STOP disarming, blocked starts, an overlong serial command, and CRLF input. A separate build with the shipped false confirmation macro verifies that CONFIRM and START cannot enable motion.

The tests compile the **actual Firmware.cpp and Motion.h** to AVR machine code and execute it using **avr8js 0.21.0**. Serial, time, Servo, and Wire are mocks; encoder motion is an ideal synthetic response to commanded wheel speed. These tests do not validate electrical I²C timing, the physical driver firmware, its real control-loop tuning, servo mechanics, supply behavior, or real-world path accuracy. The actual Wire library's timeout implementation is compiled in the production build but not electrically fault-injected here.

## Reproduce

In Arduino IDE, open the corresponding `.ino` folder and click Verify. For CLI with a normal working Arduino installation:

```powershell
arduino-cli compile --fqbn arduino:avr:uno path/to/OrboTruck
arduino-cli compile --fqbn arduino:avr:uno path/to/ServoCalibration
```

For the emulator, install Node.js, an AVR compiler containing avr-g++, avr-objcopy and avr-nm, then run from the verification folder:

```powershell
npm install
./run-tests.ps1 -AvrBin 'C:/path/to/avr-toolchain/bin'
```

Expected assertion totals: **39** and **3**, with empty failed arrays. The helper writes intermediate AVR test binaries under verification/build. These are tests, not files to upload to the truck.

No physical truck, servo, motor, driver, or battery was tested. Use README.md's staged commissioning procedure before powered operation.
