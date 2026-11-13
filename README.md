# OrboTruck: Arduino Uno R3

This package implements your corrected electronics notes and subsequent Uno/MG996R choices. The report's template instructions, older pump requirements, and older ESP32 selection are not executable requirements. There is no pump, light, ladder, phone controller, or autonomous navigation code.

## Open and upload

1. Keep the complete `OrboTruck` folder together. Open `OrboTruck/OrboTruck.ino` in Arduino IDE. The program is in the **Firmware.cpp** tab; hardware settings are in **Config.h**. The short `.ino` entry file is intentional; standard C++ source files compile with the sketch.
2. Select **Arduino AVR Boards > Arduino Uno** (Uno R3/ATmega328P), and the correct USB port. This package has not been adapted to Uno R4.
3. No external servo library is needed. The included `src/Steering/ServoBoard.h` adapter controls the Inland KS0065/PCA9685 through `Wire`, which comes with Arduino AVR Boards. Use an updated AVR core supporting `Wire.setWireTimeout` (verified with AVR core 1.8.8).
4. Initially leave `ORBOTRUCK_HARDWARE_CONFIRMED false` in Config.h. Upload with drive and servo power disconnected. The sketch remains locked and keeps all servo-board outputs disabled after successful initialization.
5. Complete the checks below. Replace the EXAMPLE measurements, calibrate signs and servo limits, then change the macro to `true` and upload again.
6. With the truck supported, power the reviewed circuit, reset the Uno if it booted before the driver, and open Serial Monitor at **115200 baud**, **Newline** (Both NL & CR also works).
7. Send `CONFIRM`. Wait for `Ready`. Send an individual test or `START`. Configuration takes about half a second and never starts movement itself.

On boot, the firmware attempts to stop the Yahboom motors and initialize the Inland board with all 16 outputs disabled. Missing logic power or failed communication with either board produces a latched fault. Correct the cause, then reset. Opening Serial Monitor commonly resets an Uno; movement never resumes automatically.

## Wiring

Logical wheel order is front-left, front-right, rear-left, rear-right. Forward is +x, left is +y, and counterclockwise rotation is positive. All angles refer to the **wheel rolling direction viewed from above**, not the servo shaft angle.

| Connection | Destination / requirement |
|---|---|
| Uno A4 / SDA | Yahboom SDA and Inland KS0065 SDA (shared I²C bus) |
| Uno A5 / SCL | Yahboom SCL and Inland KS0065 SCL (shared I²C bus) |
| Uno GND | Driver GND and servo supply negative: shared signal reference |
| Uno 5 V | Inland VCC (logic only), during USB-powered bench testing |
| Inland output 0 signal | Front-left MG996R signal |
| Inland output 1 signal | Front-right MG996R signal |
| Inland output 2 signal | Rear-left MG996R signal |
| Inland output 3 signal | Rear-right MG996R signal |
| Inland V+ / servo power terminal | External regulated 5 V servo supply; never Uno 5 V |
| Inland GND | Uno GND, Yahboom GND, and external servo supply negative |
| Inland OE | Low enables signals; verify the board’s pull-down or connect to GND |
| Four servo positive wires | Inland V+ headers only if board/connectors can carry combined peak current; otherwise distribute external 5 V directly |
| Four servo ground wires | Servo supply negative through suitably sized power wiring |
| Driver M1 encoder/motor connector | Front-left motor |
| Driver M3 encoder/motor connector | Front-right motor |
| Driver M2 encoder/motor connector | Rear-left motor |
| Driver M4 encoder/motor connector | Rear-right motor |
| Battery + / - | Driver dedicated power input, through appropriate fuse and accessible disconnect; verify polarity and voltage |
| Battery to servo converter | Converter input only; measure regulated output before connecting servos |
| Uno power during bench tests | USB; avoid joining USB 5 V to another regulated 5 V source |

Use motor connector labels, not assumed cable colors. For each channel, Yahboom specifies **motor M2 -> M-, encoder V -> 3V3, A -> HnA, B -> HnB, G -> GND, motor M1 -> M+**. Here “motor M1/M2” means the motor's two lead names, not the board's channel numbers. Use the correct PH2.0 six-pin cable/pin order, or the matching labeled headers. Do not connect the motor twice through both its six-pin connector and a separate motor output. Encoder A/B are digital and terminate at the driver; the Uno does not need eight encoder GPIOs.

The manufacturer's Uno tutorial documents direct SDA/SCL wiring and a 5 V-to-5 V connection. For this package's USB-powered bench setup, **leave the board's communication-header 5 V pin disconnected** if it is an output, rather than paralleling two supplies. Verify the pin direction and bus pull-up voltage on your exact board. A “5 V” connector label alone does not establish the SDA/SCL logic voltage. If your board revision requires 3.3 V I²C, use a bidirectional open-drain I²C level shifter with suitable pull-ups on both sides. Never substitute a UART TX/RX header for the I²C header. Grounds must be common in either arrangement.

### Power facts to resolve before connection

- **LM2596 is a buck regulator, not a servo signal driver.** The Inland KS0065 generates all four servo signals; the Uno sends it I²C commands. The 3 A LM2596 chip rating does not establish that a particular inexpensive module can continuously deliver 3 A.
- TowerPro lists MG996R operation at 4.8–6.6 V and 1.4 A stall current; four could therefore demand at least 5.6 A simultaneously according to that specification. Other variants may differ. Use a verified regulated 5 V supply with headroom over your servos' combined measured/specification peak current, appropriate wiring/connectors, and cooling. A properly specified 5 V 8–10 A supply is a sensible starting point, subject to actual servo variants and testing. Do not power servos through Uno 5 V, the Yahboom board's auxiliary outputs, or a breadboard power rail. The Inland V+ rail distributes external servo power; its terminal, traces and headers also need adequate current ratings.
- Yahboom's introduction lists **5–12 V recommended input**, **4 A continuous per motor** and **5.5 A maximum output**. Its product page separately recommends **12.6 V for 520 motors**. These statements do not establish a universal absolute maximum. Confirm the actual board revision, motor ratings/stall current, and allowable **fully charged** battery voltage before using a 3S pack or any nominal “12 V” battery. The software does not measure or protect battery state of charge.
- Do not infer battery chemistry, protection circuit, fuse size, or wire gauge from these notes. Select them from the actual battery, motor current, lead length, and connector ratings. Have the finished wiring reviewed before powering the mechanism.
- A physical disconnect must cut drive power and ideally servo power. `STOP` cannot disconnect power. The protocol does not document a watchdog guarantee, so the driver could retain its last speed when communication or the Uno fails.

### Inland KS0065 setup and reset behavior

The default addresses are Yahboom `0x26` and Inland `0x40` (address jumpers open). Set `SERVO_ADDRESS` and `SERVO_CHANNEL` in Config.h to match your wiring; channel numbers are 0–15 and must be unique. D4–D7 are unused. The included adapter uses a nominal 25 MHz oscillator and 50 Hz refresh. Servo limits remain in microseconds; calibrate them with this board because its oscillator and PWM quantization affect actual pulse widths.

Both boards share the Uno bus at 100 kHz. Check the combined pull-ups and logic voltage, including the Inland pull-ups to VCC, against the Yahboom revision; use level shifting where required. Keep VCC (logic) separate from V+ (servo power). An I²C acknowledgment does not prove servo power is present or that a wheel reached its angle.

The Inland board retains PWM independently of the Uno. Resetting or uploading to the Uno does not immediately disable existing pulses. Successful boot initialization disables all 16 outputs, including unused channels; reserve the entire board for this project. If the Uno cannot reach it, pulses can persist. OE is active low; this software does not control OE, so a hardware disable or servo-power disconnect is needed for an independent stop. STOP intentionally stops drive motors while steering continues to hold; CONFIRM disables steering until another motion command.

## Steering calibration first

The **ServoCalibration** folder is a separate, optional Uno sketch for measuring servo travel. Open its `.ino`; the code is in **Calibration.cpp**. Keep the drive supply disconnected, use a reviewed external servo supply, and initially remove the horn/linkage. Connect only one servo at a time for the first check.

Keep the complete ServoCalibration folder, including src, together. Its address, channel mapping, frequency and oscillator constants at the top of Calibration.cpp must match Config.h. Inland logic VCC must be powered even while drive power is disconnected. At 115200 baud, newline, send `S 0 1500` to apply a 1500 microsecond signal on Inland output 0. Logical indices 1, 2, 3 default to Inland outputs 1, 2, 3. Start at center; move in small increments, for example `S 0 1520`. The tool accepts only 1000–2000 microseconds; it does not automatically sweep or extend travel. `OFF` disables all 16 Inland outputs and clears their saved pulses, but does not remove power. The next S command enables only channels commanded since OFF. A communication failure latches calibration off until reset; use the physical disconnect if OFF cannot reach the board. Stop and remove power if the servo binds, buzzes under load, or draws excessive current. Disconnect before repositioning a horn.

Measure the wheel angle at several pulse widths. Record `SERVO_CENTER_US`, `US_PER_DEGREE` (pulse difference divided by wheel-angle difference), `SERVO_SIGN`, pulse limits, and wheel-angle limits separately for each module in Config.h. Center means wheels roll straight forward. Reattach the linkage only after the initial unloaded test, then repeat with conservative mechanical limits. A nonlinear linkage may require a different angle-to-pulse mapping; this version assumes an approximately linear calibrated linkage.

**MG996R does not prove 180 degrees of usable positional travel.** TowerPro lists separate standard, 180-degree, and continuous-rotation variants. Continuous-rotation servos without steering feedback cannot be used by this code. With a direct linkage and less than a full 180-degree wheel steering span, some commanded directions are unreachable even after reversing drive direction. The program faults on these requests; it does not force the endpoint or silently change the requested path. Full swerve requires suitable positional servo travel, gearing/linkage, clearances, and cable routing.

After servo calibration, disconnect the servo supply before uploading OrboTruck again. CONFIRM reinitializes the Inland board with all outputs disabled, so rearming releases any previously held steering. Servo motion occurs when a demonstration begins, after a successful motor stop command and acknowledged steering writes. A steering failure latches FAULT and attempts a motor stop; previously delivered servo pulses may remain active. The default settle delay is 1000 ms; it is an allowance for movement, not a measurement of actual angle.

## Configuration and staged commissioning

1. Measure wheel diameter under load, wheelbase between front/rear steering pivots, and track between left/right pivots. Enter millimeters in Config.h. The supplied **67 / 200 / 180 mm** values are examples. Wheels are assumed equal diameter and pivot locations symmetric about the chassis center; significant steering scrub offsets are not modeled.
2. Confirm 520 motor type, 1:30 gearing, 11 encoder lines, the purchased driver revision and matching I²C firmware. The adapter uses address `0x26`; an ACK alone cannot identify a board or its firmware. The documented I²C interface has no firmware-version register. Do not run a separate UART host controller at the same time.
3. Check the board's configured closed-loop speed response. The adapter interprets speed commands as **wheel surface mm/s**, consistent with wheel-diameter-based speed control. The English protocol table labels this simply “mm”; verify commanded speed against measured travel on your revision. Do not replace this with RPM values or raw PWM.
4. Enter servo settings from the previous section. Verify supply and wiring. Once the configuration is reviewed, set the confirmation macro to `true`. This acknowledges measurements and compatibility; runtime `CONFIRM` is still required after upload/reset or STOP.
5. Support the chassis with wheels clear of the bench. Initially connect **one drive motor/encoder** and use the corresponding `TESTFL`, `TESTFR`, `TESTRL`, or `TESTRR` command after `CONFIRM`. Other channels remain stopped and do not need to advance. These tests command TEST_SPEED_MM_S (default 500 mm/s) or the configured maximum, whichever is lower, for an estimated TEST_DISTANCE_MM (default 2500 mm) at straight steering. Keep the physical disconnect accessible. Observe that positive motion rolls the truck forward. Correct `MOTOR_SIGN` if required. Independently correct `ENCODER_SIGN` so forward rolling gives positive logical progress. An incorrect sign may trigger a fault; fix it before reset and retry.
6. Repeat for all four modules. `ENCODERS` prints raw signed counts in logical wheel order while idle. Check the change across an accurately observed **output shaft revolution**, using marked wheels and the single-wheel tests; several tests may be needed. Expected initial value is `11 × 30 × 4 = 1320` counts/revolution, but this is an assumption about quadrature decoding and **must be verified**. Enter the measured count in `COUNTS_PER_WHEEL_REV`. Raw counters are not zeroed; segment progress uses counter differences. The signed projection prevents reversed counts from completing a path. Driver speed calibration and host count calibration are separate.
7. Connect all modules. Test `FORWARD` and `BACKWARD` while raised, then `LEFT`, `RIGHT`, `DIAGONAL`, and `SPIN`. Use STATUS to inspect commanded angle/speed and estimated progress. Confirm all servo directions and motor/encoder mappings. If a required angle is outside a safe limit, fix the mechanism or restrict the demo rather than widening limits blindly.
8. Test STOP during settling and driving, then a complete short sequence. STOP sends zero speeds immediately (no acceleration ramp) and disarms; it leaves Inland servo signals holding their current position. At zero speed the board's PID may actively hold drive wheels. Do not force them by hand. The servo supply remains energized.
9. Move to a clear, level floor. Start with short distances and conservative speed. Measure actual sideways error, spin angle, orbit radius, and return error. Adjust geometry and calibration, then increase operation gradually. Check supply voltage, converter temperature, wiring temperature, servo current, and binding under simultaneous loads.
10. Test `ORBIT` last. Place the intended target initially **ORBIT_RADIUS_MM directly in front of the chassis center**, with the truck facing it. The default is an example 400 mm radius. Provide clear space for the entire swept chassis, not merely the wheel path. Test `START` only after each component demonstration succeeds.

## Commands and path settings

| Serial command | Behavior |
|---|---|
| `CONFIRM` | Validate settings and configure driver; arm from locked state only |
| `START` | Run the seven demonstrations once, then stop |
| `STOP` | Attempt immediate zero drive speeds and disarm; does not clear a fault |
| `STATUS` | State, hardware confirmation flag, wheel angles/speeds and segment progress |
| `ENCODERS` | Raw signed driver counts, logical FL/FR/RL/RR; use while idle |
| `FORWARD`, `BACKWARD` | Travel STRAIGHT_MM in the selected direction |
| `LEFT`, `RIGHT` | Crab sideways SIDEWAYS_MM |
| `DIAGONAL` | Forward-left translation DIAGONAL_MM |
| `SPIN` | Estimate one counterclockwise rotation about chassis center |
| `ORBIT` | Estimate one counterclockwise circle around a point initially ahead, facing inward |
| `TESTFL`, `TESTFR`, `TESTRL`, `TESTRR` | One-wheel straight commissioning test |
| `HELP` or unknown command | Print command list |

Commands are case-insensitive. Avoid leading/trailing spaces. Overlong commands are discarded. A running demo cannot be replaced by another motion command; use STOP, CONFIRM, then the next command. After normal completion, the sketch remains ready for another explicit command. It never repeats automatically.

Edit Config.h for distances, radius, speed, acceleration, steering-settle time, pause time, stall limit, and segment timeout. Default demo speed is 150 mm/s, individual test speed is 500 mm/s over 2500 mm, maximum wheel speed 500 mm/s, and acceleration of the fastest wheel is 80 mm/s². The individual tests take about 6.25 seconds to ramp from zero to 500 mm/s, then cruise for about 1.9 seconds before stopping; completion and STOP command an immediate stop rather than a deceleration ramp. Each segment has a 120-second timeout. A very large/slower orbit may need a longer timeout. The program preserves wheel-speed ratios while ramping up and uniformly scales commands exceeding the speed limit. At completion, individual wheels approach zero or receive a final immediate zero-speed command; this is not a precision trajectory planner with coordinated braking.

### Diagnosing a stall fault

Encoder reads use the repeated-start sequence from Yahboom’s Arduino example (`endTransmission(false)` followed by `requestFrom`). Earlier versions inserted a STOP between register selection and reading; this has been corrected. Zero readings on the physical board still need to be rechecked after uploading.

A stall means the host has not observed forward encoder progress for 2500 ms while commanding motion above the minimum moving speed. It does not measure motor current or prove a mechanical jam. With the 80 mm/s² ramp, missing encoder feedback can stop a 500 mm/s test near 210 mm/s, before it reaches the requested speed. Raising the target alone does not resolve missing feedback.

The stall message now prints the logical wheel, physical driver channel, commanded and target speeds, raw encoder count, last count change, and accumulated progress. Motors are commanded to stop before these details print. After uploading, send ENCODERS before the test, run CONFIRM and the selected test, then send ENCODERS after the fault and compare all four counts. A visibly turning wheel with unchanged counts points to encoder wiring/power or incompatible driver feedback; a different changing channel points to mapping. Negative logical progress requires checking encoder direction. Correct the cause and reset to clear the latched fault; keep stall protection enabled.

## How movement works and practical limits

For a wheel at `(x,y)` relative to chassis center, desired velocity is `(vx - omega*y, vy + omega*x)`. Steering is the direction of this vector, and drive speed is its signed length. A 180-degree-equivalent direction with reversed drive is selected when it fits calibrated limits and reduces steering travel. Zero-speed wheels retain their previous angle.

For the inward-facing orbit, `vx=0`, `vy=-speed`, `omega=speed/radius`. Thus the point initially straight ahead remains the instantaneous center of rotation in the ideal no-slip model. Encoder goals are each wheel's own calculated arc length for a full turn, not the same count on every wheel. Spin uses the corresponding wheel paths around chassis center.

The driver closes the individual motor speed loops. The Uno samples accumulated encoder counts every nominal 50 ms, accumulates signed distance along each requested wheel direction, checks for stalls/unreasonable jumps, and stops when wheel goals are reached within tolerance. Split high/low counter reads are checked for rollover consistency. I²C calls have 25 ms timeouts; errors latch a fault. A successful stop is reissued periodically while idle/faulted, and recovery never restarts motion automatically. A bus failure could delay several calls and prevent stop delivery: disconnect motor power if necessary.

There is **no IMU, absolute steering sensor, camera, or target sensor**. This implementation does not measure chassis heading or locate the orbit center. Wheel slip, backlash, servo error, unequal wheels, and endpoint stopping error accumulate. A completed encoder estimate does not guarantee an exact circle, exact sideways tracking, or continued physical alignment to a target. Add heading/position feedback if those are accuracy requirements.

## Manufacturer references and compatibility limits

- [Inland KS0065 product](https://www.microcenter.com/product/639725/inland-ks006516-channel-12-bit-pwm-servo-driver-i2c-interface), [KS0065 documentation](https://docs.keyestudio.com/projects/KS0065/en/latest/docs/KS0065.html), and [NXP PCA9685 datasheet](https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf): servo-board address, PWM registers, full-off and oscillator configuration.

- [Yahboom Uno I²C tutorial](https://www.yahboom.net/study/Quad-MD-Module/13992): wiring, 520/30/11 configuration and encoder reads.
- [Yahboom protocol repository](https://github.com/YahboomTechnology/4-Channel-Motor-Drive-Module): inspected commit `de79d8327642e83f8f2ea34f9398e4ee7bce084f`, especially `1.Introduction/1.2 Control command.pdf`. Registers: type 0x01, deadzone 0x02, lines 0x03, ratio 0x04, diameter 0x05, speed 0x06, accumulated encoder words 0x20–0x27. Integers are big-endian; diameter is little-endian float32. No undocumented firmware query or watchdog is assumed.
- [Yahboom board introduction](https://www.yahboom.net/public/upload/upload-html/1750303224/1.1%20Introduction%20to%204-channel%20motor%20drive%20board.html) and [product page](https://category.yahboom.net/products/quad-md-module): supply/current specifications and conflicting 12/12.6 V guidance.
- [TowerPro MG996R](https://towerpro.com.tw/product/mg996r/), [180-degree variant](https://towerpro.com.tw/product/mg995-robot-servo-180-rotation/): servo specifications; do not identify travel from the model family name alone.
- [TI LM2596](https://www.ti.com/product/LM2596): 3 A regulator IC; board construction and thermal limits still matter.

See **VERIFICATION.md** for the actual build/test results and reproducible test commands. No physical motor, servo, battery, or truck was available for testing here.
