#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <ESP8266WiFi.h>

#include "secrets.h"
#include "firestore.h"
#include "state_machine.h"

// Original pins from your source code
#define I2C_SDA 4       // D2 / GPIO4
#define I2C_SCL 5       // D1 / GPIO5
#define BUZZER_PIN 16   // D0 / GPIO16

#define WIFI_CONNECT_TIMEOUT_MS 20000

#define MPU_ADDR 0x68
#define REG_PWR_MGMT_1 0x6B
#define REG_ACCEL_XOUT_H 0x3B

// Change this depending on buzzer type:
// 1 = passive buzzer, needs tone()
// 0 = active buzzer module, only needs HIGH/LOW
#define BUZZER_IS_PASSIVE 1

// Some active buzzers are active-low. Most are active-high.
#define BUZZER_ACTIVE_LOW 0

#define BUZZER_FREQ 2000

// Non-blocking: starts the buzzer and returns immediately.
// settings.volume == 0 means muted; anything else is full volume.
void buzzerOn() {
#if BUZZER_IS_PASSIVE
  if (settings.volume <= 0) {
    noTone(BUZZER_PIN);
    return;
  }
  tone(BUZZER_PIN, BUZZER_FREQ);
#else
  digitalWrite(BUZZER_PIN, BUZZER_ACTIVE_LOW ? LOW : HIGH);
#endif
}

void buzzerOff() {
#if BUZZER_IS_PASSIVE
  noTone(BUZZER_PIN);
#else
  digitalWrite(BUZZER_PIN, BUZZER_ACTIVE_LOW ? HIGH : LOW);
#endif
}

// Blocking beep at full volume.
void playBeep(int durationMs) {
  buzzerOn();
  delay(durationMs);
  buzzerOff();
}

void playHappyChirp() {
  playBeep(60); delay(40);
  playBeep(60); delay(40);
  playBeep(120);
}

void playSadChirp() {
  playBeep(500);
}

bool connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > WIFI_CONNECT_TIMEOUT_MS) {
      Serial.println();
      Serial.println("WiFi connect TIMEOUT");
      return false;
    }
    Serial.print(".");
    delay(250);
  }

  Serial.println();
  Serial.print("WiFi connected, IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

bool writeMPU(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission(true) == 0;
}

bool readAccelRaw(int16_t &ax, int16_t &ay, int16_t &az) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(REG_ACCEL_XOUT_H);

  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  uint8_t n = Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)6, (uint8_t)true);
  if (n != 6) {
    return false;
  }

  ax = (int16_t)((Wire.read() << 8) | Wire.read());
  ay = (int16_t)((Wire.read() << 8) | Wire.read());
  az = (int16_t)((Wire.read() << 8) | Wire.read());

  return true;
}

void scanI2C() {
  Serial.println();
  Serial.println("Scanning I2C...");

  bool found = false;

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Found I2C device at 0x");
      if (addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
      found = true;
    }
  }

  if (!found) {
    Serial.println("No I2C devices found!");
  }

  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  randomSeed(micros());

  pinMode(BUZZER_PIN, OUTPUT);
  buzzerOff();

  Serial.println("MPU6050 + buzzer diagnostic");
  Serial.println("Expected wiring:");
  Serial.println("MPU SDA -> D2 / GPIO4");
  Serial.println("MPU SCL -> D1 / GPIO5");
  Serial.println("Buzzer S/IN -> D0 / GPIO16");
  Serial.println();

  Wire.begin(I2C_SDA, I2C_SCL);
  delay(100);

  scanI2C();

  Serial.println("Waking MPU6050...");
  if (writeMPU(REG_PWR_MGMT_1, 0x00)) {
    Serial.println("MPU6050 wake command OK");
  } else {
    Serial.println("MPU6050 wake command FAILED");
  }

  delay(300);

  if (connectWiFi()) {
    playHappyChirp();
    fetchSettings();
  } else {
    playSadChirp();
  }

  Serial.println("Starting motion test...");
}

void loop() {
  // Firestore polling does a TLS handshake that blocks for ~1–2 s.
  // Only poll while idle so it can't interrupt a measurement sequence.
  if (smIsIdle()) {
    maybeFetchSettings();
  }
  smTick();
}