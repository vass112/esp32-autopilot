#pragma once
// esp32-autopilot — hardware + default tuning configuration.
// Target: classic ESP32 38-pin DevKit (ESP32-WROOM-32, NOT S3).
// Sensors: MPU6050 (IMU) + BMP280 (baro) + NEO-M8N (GPS) + HMC5883L (compass).
// Frame: 450mm X-quad, see README.md wiring table.

#include <Arduino.h>

// ---- Firmware identity ----
#define FW_NAME "esp32-autopilot"
#define FW_VERSION "0.1.0-bringup"

// ---- I2C bus (shared IMU + baro + compass) ----
#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22
#define I2C_BUS_HZ 400000UL

#define MPU6050_ADDR 0x68
#define BMP280_ADDR 0x77  // SDO high; use 0x76 if SDO is low
#define HMC5883_ADDR 0x1E

// ---- GPS (NEO-M8N, u-blox) on UART2 ----
// NOTE: GPIO16/17 are ALSO used by ArduPilot esp32buzz for GPS.
// Do NOT route motor signals to 16/17 on this firmware.
#define GPS_UART_NUM 2
#define PIN_GPS_RX 16  // ESP32 RX <- GPS TX
#define PIN_GPS_TX 17  // ESP32 TX -> GPS RX
#define GPS_BOOT_BAUD 9600
#define GPS_OP_BAUD 115200

// ---- RC input: PPM sum on GPIO4 (matches AP_HAL_ESP32 convention) ----
#define PIN_PPM 4
#define RC_MIN_US 1000
#define RC_MAX_US 2000
#define RC_TIMEOUT_MS 500  // RC loss triggers failsafe beyond this

// ---- Motors: X-quad, LEDC PWM for standard 50Hz ESCs ----
// M1 = front-left (CCW), M2 = front-right (CW),
// M3 = back-right (CCW), M4 = back-left (CW).
#define PIN_MOTOR1 25
#define PIN_MOTOR2 26
#define PIN_MOTOR3 27
#define PIN_MOTOR4 32
#define ESC_PWM_HZ 50
#define ESC_MIN_US 1000
#define ESC_MAX_US 2000

// ---- Battery: 3S via divider into ADC1 (WiFi-safe ADC unit) ----
#define PIN_BATT 35
#define BATT_DIVIDER_RATIO 4.0f   // (Rtop+Rbot)/Rbot, e.g. 30k/10k
#define BATT_WARN_V 10.8f         // 3S low warning
#define BATT_CRIT_V 10.2f         // 3S critical -> auto-land

// ---- Indicators ----
#define PIN_LED 2
#define PIN_BUZZER 33

// ---- Control defaults (v1 targets, tuned on 450mm frame) ----
#define PID_STAB_P 4.5f
#define PID_STAB_I 0.05f
#define PID_STAB_D 0.18f
#define PID_RATE_P 0.15f
#define PID_RATE_I 0.10f
#define PID_RATE_D 0.004f
#define MAX_TILT_DEG 30.0f

// ---- Navigation limits (safety-first) ----
#define NAV_MAX_SPEED_MS 5.0f
#define NAV_WP_RADIUS_M 2.0f
#define NAV_MAX_ALT_M 50.0f
#define NAV_GEOFENCE_M 200.0f
#define NAV_RTL_ALT_M 15.0f
