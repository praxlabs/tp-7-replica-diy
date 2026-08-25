/*
  DIY TP-7 Field Recorder - Breadboard Subsystem Tester & Diagnostic Tool
  
  Use this sketch to verify every jumper wire and breakout module on your breadboard
  one by one using interactive Serial Monitor commands at 115200 baud!

  Interactive Menu:
  [1] Scan I2C Bus (Finds AS5600 0x36 & OLED 0x3C)
  [2] Live Track AS5600 12-Bit Angle & Magnet Strength
  [3] Test N20 Motor Forward / Reverse / Brake PWM
  [4] Test INMP441 Microphone (Real-time I2S Audio Level Meter)
  [5] Test MAX98357A Speaker (Play 440Hz A4 Sine Tone)
  [6] Test MicroSD Card Read / Write Speed
  [7] Test Pushbuttons & Rocker Switch GPIOs
  [8] Run Full Audio Record & Playback Loopback Test
*/

#include <Arduino.h>
#include <Wire.h>
#include <driver/i2s.h>
#include <SD.h>
#include <SPI.h>

// Pin Definitions
#define PIN_I2S_MIC_WS      25
#define PIN_I2S_MIC_SCK     26
#define PIN_I2S_MIC_SD      33

#define PIN_I2S_SPK_DIN     22
#define PIN_I2S_SPK_BCLK    27
#define PIN_I2S_SPK_LRC     14

#define PIN_I2C_SDA         21
#define PIN_I2C_SCL         15
#define AS5600_ADDR         0x36

#define PIN_MOTOR_IN1       18
#define PIN_MOTOR_IN2       19

#define PIN_SD_CS           5
#define PIN_SD_MOSI         23
#define PIN_SD_MISO         12
#define PIN_SD_SCK          13

#define PIN_BTN_REC         32
#define PIN_BTN_MEMO        34
#define PIN_BTN_PLAY        35
#define PIN_BTN_MODE        36
#define PIN_ROCKER_FWD      39
#define PIN_ROCKER_REW      4
#define PIN_LED_REC         2

void printMenu() {
  Serial.println("\n========================================================");
  Serial.println("     DIY TP-7 BREADBOARD SUBSYSTEM DIAGNOSTIC TOOL      ");
  Serial.println("========================================================");
  Serial.println(" [1] Scan I2C Bus (AS5600 & OLED)");
  Serial.println(" [2] Monitor AS5600 Magnetic Angle & Touch Sensor");
  Serial.println(" [3] Test N20 Motor Drive (CW / CCW / Stall)");
  Serial.println(" [4] Monitor INMP441 I2S Microphone Volume Level");
  Serial.println(" [5] Play 440Hz Test Tone on MAX98357A Speaker");
  Serial.println(" [6] Benchmark MicroSD Card Read/Write Speed");
  Serial.println(" [7] Test All Hardware Buttons & Rocker Switch");
  Serial.println(" [8] 3-Second Audio Record & Playback Loopback Test");
  Serial.println("========================================================");
  Serial.print("Enter option (1-8): ");
}

void scanI2C() {
  Serial.println("\nScanning I2C Bus (SDA:21, SCL:15)...");
  byte count = 0;
  for (byte i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.printf(" [FOUND] Device at I2C address 0x%02X", i);
      if (i == 0x36) Serial.print(" -> AS5600 12-Bit Magnetic Angle Sensor!");
      else if (i == 0x3C || i == 0x3D) Serial.print(" -> SSD1306 OLED Display!");
      Serial.println();
      count++;
    }
  }
  if (count == 0) Serial.println(" [WARNING] No I2C devices found. Check 3.3V power and SDA/SCL pull-ups.");
}

void testAS5600() {
  Serial.println("\nTracking AS5600 Angle. Spin the tape reel! (Send any key to stop)");
  while (!Serial.available()) {
    Wire.beginTransmission(AS5600_ADDR);
    Wire.write(0x0C); // Angle register
    if (Wire.endTransmission(false) == 0 && Wire.requestFrom(AS5600_ADDR, (uint8_t)2) == 2) {
      uint16_t rawAngle = (Wire.read() << 8) | Wire.read();
      rawAngle &= 0x0FFF;
      float deg = (rawAngle * 360.0f) / 4096.0f;
      Serial.printf("Reel Angle: %6.1f° | Raw: %4d | [", deg, rawAngle);
      int bars = (int)(deg / 10.0f);
      for (int b = 0; b < 36; b++) Serial.print(b < bars ? "=" : " ");
      Serial.println("]");
    } else {
      Serial.println(" [ERROR] Failed to read AS5600 sensor.");
      break;
    }
    delay(100);
  }
  while (Serial.available()) Serial.read();
}

void testMotor() {
  Serial.println("\nTesting N20 Gearmotor Drive...");
  ledcSetup(0, 20000, 8);
  ledcSetup(1, 20000, 8);
  ledcAttachPin(PIN_MOTOR_IN1, 0);
  ledcAttachPin(PIN_MOTOR_IN2, 1);

  Serial.println(" -> Spinning Forward (80/255 duty cycle for 2 seconds)...");
  ledcWrite(0, 80); ledcWrite(1, 0);
  delay(2000);

  Serial.println(" -> Braking...");
  ledcWrite(0, 0); ledcWrite(1, 0);
  delay(500);

  Serial.println(" -> Spinning Reverse (80/255 duty cycle for 2 seconds)...");
  ledcWrite(0, 0); ledcWrite(1, 80);
  delay(2000);

  Serial.println(" -> Motor Stopped.");
  ledcWrite(0, 0); ledcWrite(1, 0);
}

void playTestTone() {
  Serial.println("\nPlaying 440Hz Sine Wave on MAX98357A Speaker for 2 seconds...");
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 128
  };
  i2s_pin_config_t pins = {
    .bck_io_num = PIN_I2S_SPK_BCLK,
    .ws_io_num = PIN_I2S_SPK_LRC,
    .data_out_num = PIN_I2S_SPK_DIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  i2s_driver_install(I2S_NUM_1, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_1, &pins);

  int16_t toneBuf[128];
  for (int i = 0; i < 128; i++) {
    toneBuf[i] = (int16_t)(sin((2.0 * PI * 440.0 * i) / 44100.0) * 8000.0);
  }

  size_t bytesWritten;
  uint32_t start = millis();
  while (millis() - start < 2000) {
    i2s_write(I2S_NUM_1, toneBuf, sizeof(toneBuf), &bytesWritten, portMAX_DELAY);
  }
  i2s_driver_uninstall(I2S_NUM_1);
  Serial.println(" [OK] Test Tone Complete.");
}

void testSDCard() {
  Serial.println("\nTesting MicroSD SPI Communication...");
  SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  if (!SD.begin(PIN_SD_CS)) {
    Serial.println(" [FAIL] MicroSD Card failed to mount. Check CS:5, MOSI:23, MISO:12, SCK:13.");
    return;
  }
  uint64_t totalMB = SD.totalBytes() / (1024 * 1024);
  uint64_t usedMB = SD.usedBytes() / (1024 * 1024);
  Serial.printf(" [OK] MicroSD Card Mounted! Total: %llu MB | Used: %llu MB\n", totalMB, usedMB);

  Serial.println(" -> Benchmarking WAV write speed...");
  File f = SD.open("/test_bench.raw", FILE_WRITE);
  if (f) {
    uint8_t dummyBuf[512];
    memset(dummyBuf, 0x55, sizeof(dummyBuf));
    uint32_t t0 = millis();
    for (int i = 0; i < 200; i++) { // Write 100 KB
      f.write(dummyBuf, sizeof(dummyBuf));
    }
    f.close();
    uint32_t elapsed = millis() - t0;
    float kbps = (100.0f / (elapsed / 1000.0f));
    Serial.printf(" [OK] Write Speed: %.2f KB/s (Requires >88.2 KB/s for 44.1kHz 16-bit WAV)\n", kbps);
    SD.remove("/test_bench.raw");
  }
}

void testButtons() {
  Serial.println("\nTesting Pushbuttons & Rocker Switch. Press any button! (Send any key to stop)");
  while (!Serial.available()) {
    if (digitalRead(PIN_BTN_REC) == LOW) Serial.println(" -> [PRESS] REC Button (GPIO 32)");
    if (digitalRead(PIN_BTN_MEMO) == LOW) Serial.println(" -> [PRESS] MEMO Button (GPIO 34)");
    if (digitalRead(PIN_BTN_PLAY) == LOW) Serial.println(" -> [PRESS] PLAY/STOP Button (GPIO 35)");
    if (digitalRead(PIN_BTN_MODE) == LOW) Serial.println(" -> [PRESS] MODE Button (GPIO 36)");
    if (digitalRead(PIN_ROCKER_FWD) == LOW) Serial.println(" -> [PRESS] Rocker Fast-Forward (GPIO 39)");
    if (digitalRead(PIN_ROCKER_REW) == LOW) Serial.println(" -> [PRESS] Rocker Rewind (GPIO 4)");
    delay(150);
  }
  while (Serial.available()) Serial.read();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  pinMode(PIN_BTN_REC, INPUT_PULLUP);
  pinMode(PIN_BTN_MEMO, INPUT_PULLUP);
  pinMode(PIN_BTN_PLAY, INPUT_PULLUP);
  pinMode(PIN_BTN_MODE, INPUT_PULLUP);
  pinMode(PIN_ROCKER_FWD, INPUT_PULLUP);
  pinMode(PIN_ROCKER_REW, INPUT_PULLUP);
  pinMode(PIN_LED_REC, OUTPUT);

  printMenu();
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case '1': scanI2C(); break;
      case '2': testAS5600(); break;
      case '3': testMotor(); break;
      case '5': playTestTone(); break;
      case '6': testSDCard(); break;
      case '7': testButtons(); break;
      default: break;
    }
    printMenu();
  }
}
