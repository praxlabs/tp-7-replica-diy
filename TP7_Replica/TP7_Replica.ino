#include <driver/i2s.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEServer.h>

#define I2S_WS   25
#define I2S_SCK  26
#define I2S_SD   33
#define I2S_PORT I2S_NUM_0
#define SAMPLE_RATE 44100

// --- Dummy Functions & Variables for Completeness ---
void writeToRingBuffer(int32_t sample) {}
size_t readFromFileOrBuffer(int16_t* buf, size_t size) { return size; }
void writeWavHeaderPlaceholder(File f) {}
void writeLE32(File f, uint32_t val) {}
bool userIsTouchingReel() { return false; }
void seekPlayback(int32_t samples) {}
bool isPlaying = false;
#define PWM_CHANNEL_A 0
#define PWM_CHANNEL_B 1
void toggleRecording() {}
void micInit();
void speakerInit() {}
void sdInit() {}
void i2cInit() {}
void motorInit() {}
void buttonsInit() {}
void bleInit() {}
void audioRecordTask(void*) {}
void audioPlaybackTask(void*) {}
void uiTask(void*) {}
i2s_pin_config_t out_pins;
BLECharacteristic* pCharacteristic = nullptr;
// --------------------------------------------------

// 3. Microphone
void micInit() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, 
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 256
  };
  i2s_pin_config_t pins = {
    .bck_io_num = I2S_SCK, .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE, .data_in_num = I2S_SD
  };
  i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
  i2s_set_pin(I2S_PORT, &pins);
}

void micReadTask(void*) {
  int32_t raw[256];
  size_t bytesRead;
  for (;;) {
    i2s_read(I2S_PORT, raw, sizeof(raw), &bytesRead, portMAX_DELAY);
    for (int i = 0; i < bytesRead / 4; i++) {
      int32_t sample = raw[i] >> 8;
      writeToRingBuffer(sample);
    }
  }
}

// 4. Speaker Output
void playbackTask(void*) {
  int16_t buf[256];
  for (;;) {
    size_t n = readFromFileOrBuffer(buf, sizeof(buf));
    size_t written;
    i2s_write(I2S_NUM_1, buf, n, &written, portMAX_DELAY);
  }
}

// 5. Storage
File audioFile;

void startRecording(const char* filename) {
  audioFile = SD.open(filename, FILE_WRITE);
  writeWavHeaderPlaceholder(audioFile); 
}

void onAudioChunk(int16_t* samples, size_t count) {
  audioFile.write((uint8_t*)samples, count * 2);
}

void stopRecording() {
  uint32_t dataSize = audioFile.size() - 44;
  audioFile.seek(4);
  writeLE32(audioFile, dataSize + 36);   
  audioFile.seek(40);
  writeLE32(audioFile, dataSize);        
  audioFile.close();
}

// 6. The Motorized Tape Reel
#define AS5600_ADDR 0x36
#define ANGLE_REG   0x0C

uint16_t readReelAngle() {
  Wire.beginTransmission(AS5600_ADDR);
  Wire.write(ANGLE_REG);
  Wire.endTransmission(false);
  Wire.requestFrom(AS5600_ADDR, 2);
  uint16_t angle = (Wire.read() << 8) | Wire.read();
  return angle & 0x0FFF; 
}

int32_t lastAngle = 0;
int32_t audioPositionSamples = 0;
const float SAMPLES_PER_DEGREE = SAMPLE_RATE * 0.05; 

void scrubTask(void*) {
  for (;;) {
    int32_t angle = readReelAngle();
    int32_t delta = angle - lastAngle;
    if (delta > 2048) delta -= 4096;
    if (delta < -2048) delta += 4096;

    if (userIsTouchingReel()) {
      audioPositionSamples += delta * SAMPLES_PER_DEGREE;
      seekPlayback(audioPositionSamples);
    }
    lastAngle = angle;
    vTaskDelay(pdMS_TO_TICKS(2)); 
  }
}

void driveMotor(int pwmDuty) {
  if (pwmDuty >= 0) {
    ledcWrite(PWM_CHANNEL_A, pwmDuty);
    ledcWrite(PWM_CHANNEL_B, 0);
  } else {
    ledcWrite(PWM_CHANNEL_A, 0);
    ledcWrite(PWM_CHANNEL_B, -pwmDuty);
  }
}

void motorTask(void*) {
  for (;;) {
    if (isPlaying && !userIsTouchingReel()) {
      driveMotor(80); 
    } else if (!isPlaying) {
      driveMotor(0);
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// 7. Buttons & Rocker
struct Button { uint8_t pin; bool lastState; uint32_t lastChange; };
Button recordBtn = {32, HIGH, 0};

void pollButton(Button &b, void (*onPress)()) {
  bool state = digitalRead(b.pin);
  if (state != b.lastState && millis() - b.lastChange > 30) {
    b.lastChange = millis();
    b.lastState = state;
    if (state == LOW) onPress();
  }
}

// 10. Bluetooth
void sendFileOverBLE(File &f) {
  uint8_t chunk[512];
  while (f.available()) {
    size_t n = f.read(chunk, sizeof(chunk));
    pCharacteristic->setValue(chunk, n);
    pCharacteristic->notify();
    vTaskDelay(pdMS_TO_TICKS(10)); 
  }
}

// 12. Firmware Task Architecture
void setup() {
  micInit(); speakerInit(); sdInit(); i2cInit();
  motorInit(); buttonsInit(); bleInit();

  xTaskCreatePinnedToCore(audioRecordTask, "rec",  4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(audioPlaybackTask,"play", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(scrubTask,        "scrub",2048, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(motorTask,        "motor",2048, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(uiTask,           "ui",   2048, NULL, 1, NULL, 0);
}

void loop() {
  pollButton(recordBtn, toggleRecording);
}
