# esp32-autopilot

Standalone autonomous flight firmware for the **classic ESP32** (ESP32-WROOM-32, not S3),
built for a 450mm X-quad with **MPU6050 + BMP280 + NEO-M8N + HMC5883L** and PPM RC.

Goal: fill the gap left by existing ESP32 drone projects — stabilized flight is solved
many times over, but **GPS waypoint autonomy on a standalone ESP32 stack** is not.
This repo builds that: `STABILIZE → ALT_HOLD → POS_HOLD → RTL → AUTO` (waypoints).

> **Status: v0.1.0-bringup.** Hardware validation stage. Flight-control stages land
> incrementally; nothing here has flown yet. Read [Safety](#safety) before powering props.

## Hardware

| Part | Model | Interface | Notes |
|---|---|---|---|
| MCU | ESP32 38-pin DevKit | — | classic ESP32, 240 MHz dual-core |
| IMU | MPU6050 (GY-521) | I2C `0x68` | accel + gyro, complementary filter |
| Baro | BMP280 (on GY-91 or module) | I2C `0x77` (`0x76` if SDO low) | altitude hold |
| GPS | NEO-M8N | UART2 9600 boot → 115200 + UBX 10 Hz | position / missions |
| Compass | HMC5883L (on GPS puck) | I2C `0x1E` | heading for navigation |
| RC | PPM receiver | GPIO4 | 8-ch PPM sum |
| ESCs | 4× 30A BLHeli, PWM 50 Hz | GPIO 25/26/27/32 | standard protocol only |
| Power | PDB + buck 5 V | VIN/GND | buck prevents ESP32 brown-out resets |

## Wiring

| ESP32 | Device | Wire |
|---|---|---|
| GPIO21 (SDA) | MPU6050 SDA, BMP280 SDA, HMC5883 SDA | yellow |
| GPIO22 (SCL) | MPU6050 SCL, BMP280 SCL, HMC5883 SCL | red/alt |
| 3V3 | MPU6050 VCC, BMP280 VCC, HMC5883 VCC | red |
| GND | all sensor GND | black |
| GPIO16 (RX2) | GPS TX | yellow |
| GPIO17 (TX2) | GPS RX | — |
| 5V | GPS VCC | red |
| GND | GPS GND | black |
| GPIO4 | receiver PPM-out | yellow |
| GPIO25 / 26 / 27 / 32 | M1-FL / M2-FR / M3-BR / M4-BL ESC signal | yellow |
| GPIO35 | battery divider midpoint (ADC1, WiFi-safe) | — |
| GPIO33 | buzzer | — |
| GPIO2 | status LED (built-in) | — |

Rules: common GND everywhere; TX→RX crossover on GPS; 4.7 kΩ I2C pull-ups if the
bus is flaky; keep ESC signal ground tied to ESP32 GND; never power sensors from 5 V.

Motor order/direction: M1 FL CCW, M2 FR CW, M3 BR CCW, M4 BL CW. Props: M1/M3 CCW,
M2/M4 CW. Verify with the motor-test CLI before first arm.

## Build & flash

Requires [PlatformIO](https://platformio.org/) (`pio --version` ≥ 6).

```powershell
cd C:\Users\DELL\esp32-autopilot
pio run            # build
pio run -t upload  # flash (set upload_port in platformio.ini if autodetect fails)
pio device monitor # 115200 baud status + CLI
```

## Bring-up (do this first)

Flash the current firmware with props **off**, open the monitor, expect:

```
[I2C] device at 0x68 (MPU6050?) / 0x77 (BMP280?) / 0x1E (HMC5883?)
[MPU] OK / [BMP] OK / [MAG] OK + 3 beeps
GPSbytes counter increasing outdoors
```

Fault = long tone + `FAIL` line. Fix wiring/address (`BMP280_ADDR`) before continuing.

## Flight modes (roadmap)

| Stage | Mode | Requires | State |
|---|---|---|---|
| 1 | Bring-up / CLI | USB | ✅ this release |
| 2 | STABILIZE + motor mixer + PPM | IMU, RC | next |
| 3 | ALT_HOLD | + BMP280 | planned |
| 4 | POS_HOLD / LOITER | + M8N + HMC5883L | planned |
| 5 | RTL + AUTO waypoints + geofence | + all above | planned |

Failsafes (planned, enforced from stage 2): RC loss → RTL/land, GPS loss → ALT_HOLD,
low battery → RTL/land, geofence breach → RTL. No DShot, no CAN, single IMU —
same honest limits as ArduPilot's own ESP32 port.

## Tuning

Defaults in `src/config.h` target a 450mm frame. Tune stabilize P first (oscillation →
back off 20%), then D, then I. Never tune with GPS modes until stabilize is solid
in wind. PID values are all in `config.h` with units in comments.

## Similar projects

- ArduPilot `AP_HAL_ESP32` — https://github.com/ArduPilot/ardupilot
- `rtlopez/esp-fc` (ESP32 Betaflight-compatible stabilizer) — https://github.com/rtlopez/esp-fc
- Espressif `esp-drone` — https://github.com/espressif/esp-drone
- `cifertech/ESP32-Drone` — https://github.com/cifertech/ESP32-Drone
- `adithyadilum/esp32-flight-controller` — https://github.com/adithyadilum/esp32-flight-controller
- `nickrehm/dRehmFlight` — https://github.com/nickrehm/dRehmFlight
- `moltstudios/esp32-drone-navigation` — https://github.com/moltstudios/esp32-drone-navigation
- MultiWii — https://github.com/multiwii/multiwii-firmware

## Safety

Experimental firmware on DIY hardware with no sensor redundancy. Bench-test without
props, keep a manual-override switch, fly line-of-sight in open fields only, respect
local drone regulations. The authors accept no liability — see LICENSE.

## License

MIT — see LICENSE.
