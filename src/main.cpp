// esp32-autopilot — Stage 1: hardware bring-up and validation.
// Validates every sensor on the I2C bus + GPS UART before any flight code
// is added. Run this first on a new build; all checks must pass.
// Serial: 115200 8N1. Status report at 2 Hz, LED heartbeat, buzzer codes:
//   1 beep = boot, 3 beeps = all sensors OK, long tone = fault.

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

#include "config.h"

Adafruit_BMP280 bmp;

static bool mpuOk = false;
static bool bmpOk = false;
static bool magOk = false;
static uint32_t gpsBytes = 0;
static uint32_t lastReport = 0;
static uint32_t lastBlink = 0;
static bool ledState = false;

static void beep(uint8_t n, uint16_t ms = 120) {
  for (uint8_t i = 0; i < n; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(ms);
    digitalWrite(PIN_BUZZER, LOW);
    if (i + 1 < n) delay(120);
  }
}

static bool i2cWrite(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

static bool i2cRead(uint8_t addr, uint8_t reg, uint8_t *buf, size_t n) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(addr, n) != n) return false;
  for (size_t i = 0; i < n; i++) buf[i] = Wire.read();
  return true;
}

static void scanBus() {
  Serial.println(F("[I2C] scanning..."));
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.printf(F("[I2C] device at 0x%02X"), a);
      if (a == MPU6050_ADDR) Serial.print(F(" (MPU6050?)"));
      if (a == BMP280_ADDR) Serial.print(F(" (BMP280?)"));
      if (a == HMC5883_ADDR) Serial.print(F(" (HMC5883?)"));
      Serial.println();
    }
  }
}

static bool initMpu() {
  uint8_t who = 0;
  if (!i2cRead(MPU6050_ADDR, 0x75, &who, 1)) return false;
  Serial.printf("[MPU] WHO_AM_I=0x%02X\n", who);
  if (who != 0x68) return false;
  if (!i2cWrite(MPU6050_ADDR, 0x6B, 0x00)) return false;  // wake
  delay(100);
  i2cWrite(MPU6050_ADDR, 0x1B, 0x00);  // gyro +-250 deg/s
  i2cWrite(MPU6050_ADDR, 0x1C, 0x00);  // accel +-2 g
  return true;
}

static bool initMag() {
  uint8_t id[3] = {0, 0, 0};
  if (!i2cRead(HMC5883_ADDR, 0x0A, id, 3)) return false;
  Serial.printf("[MAG] ID=%02X %02X %02X\n", id[0], id[1], id[2]);
  if (id[0] != 0x48 || id[1] != 0x34 || id[2] != 0x33) return false;
  i2cWrite(HMC5883_ADDR, 0x00, 0x70);  // 8-sample avg, 15 Hz
  i2cWrite(HMC5883_ADDR, 0x01, 0x20);  // +-1.3 Ga
  i2cWrite(HMC5883_ADDR, 0x02, 0x00);  // continuous mode
  return true;
}

static void readMpu(float *ax, float *ay, float *az, float *gx, float *gy, float *gz) {
  uint8_t b[14];
  if (!i2cRead(MPU6050_ADDR, 0x3B, b, 14)) return;
  int16_t rawA[3] = {(int16_t)((b[0] << 8) | b[1]), (int16_t)((b[2] << 8) | b[3]),
                     (int16_t)((b[4] << 8) | b[5])};
  int16_t rawG[3] = {(int16_t)((b[8] << 8) | b[9]), (int16_t)((b[10] << 8) | b[11]),
                     (int16_t)((b[12] << 8) | b[13])};
  *ax = rawA[0] / 16384.0f;
  *ay = rawA[1] / 16384.0f;
  *az = rawA[2] / 16384.0f;
  *gx = rawG[0] / 131.0f;
  *gy = rawG[1] / 131.0f;
  *gz = rawG[2] / 131.0f;
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  Serial.begin(115200);
  delay(500);
  Serial.printf("\n%s %s boot\n", FW_NAME, FW_VERSION);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_BUS_HZ);
  scanBus();

  mpuOk = initMpu();
  Serial.printf("[MPU] %s\n", mpuOk ? "OK" : "FAIL");
  bmpOk = bmp.begin(BMP280_ADDR);
  Serial.printf("[BMP] %s\n", bmpOk ? "OK" : "FAIL");
  if (bmpOk) {
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16, Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500);
  }
  magOk = initMag();
  Serial.printf("[MAG] %s\n", magOk ? "OK" : "FAIL");

  Serial2.begin(GPS_BOOT_BAUD, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
  Serial.println(F("[GPS] listening for NMEA on UART2..."));

  float batt = analogRead(PIN_BATT) * 3.3f / 4095.0f * BATT_DIVIDER_RATIO;
  Serial.printf("[BATT] %.2f V (warn %.1f crit %.1f)\n", batt, BATT_WARN_V, BATT_CRIT_V);

  if (mpuOk && bmpOk && magOk) {
    beep(3);
  } else {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(800);
    digitalWrite(PIN_BUZZER, LOW);
  }
}

void loop() {
  while (Serial2.available()) {
    Serial2.read();
    gpsBytes++;
  }

  uint32_t now = millis();
  if (now - lastBlink > 500) {
    lastBlink = now;
    ledState = !ledState;
    digitalWrite(PIN_LED, ledState);
  }
  if (now - lastReport > 500) {
    lastReport = now;
    float ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0;
    if (mpuOk) readMpu(&ax, &ay, &az, &gx, &gy, &gz);
    float temp = bmpOk ? bmp.readTemperature() : NAN;
    float pres = bmpOk ? bmp.readPressure() : NAN;
    Serial.printf("MPU a=[%.2f %.2f %.2f]g g=[%.1f %.1f %.1f]dps "
                  "BMP T=%.1fC P=%.0fPa GPSbytes=%u\n",
                  ax, ay, az, gx, gy, gz, temp, pres, gpsBytes);
  }
}
