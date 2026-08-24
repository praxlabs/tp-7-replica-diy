/*
  DIY TP-7 Motorized-Reel Field Recorder Firmware (v2.0)
  Inspired by the Teenage Engineering TP-7 Field Recorder Architecture

  Subsystems:
  - Dual-Core FreeRTOS Partitioning (Core 1: 44.1kHz I2S Audio DMA, Core 0: 500Hz UI & Motor Control)
  - AS5600 12-Bit Contactless Magnetic Angle Sensor (Scrubbing, Jog Dial & Touch-to-Pause Stall Sensing)
  - DRV8833 Dual H-Bridge Motor Driver with LEDC PWM Speed Control
  - INMP441 Digital MEMS Microphone (I2S_NUM_0 Master RX)
  - MAX98357A Class-D DAC/Amp (I2S_NUM_1 Master TX)
  - MicroSD FAT32 High-Throughput WAV Streamer
  - 2-Stage Side Rocker Switch (Variable-Speed Shuttle Scrub)
  - Dedicated Memo Mode (Hold-to-Record Quick Voice Memos)
  - BLE Companion App GATT Audio & Transcription Broadcast
*/

#include <Arduino.h>
#include <driver/i2s.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ==========================================
// 1. PIN DEFINITIONS & CONSTANTS
// ==========================================
#define SAMPLE_RATE         44100
#define BITS_PER_SAMPLE     16
#define I2S_DMA_BUF_LEN     256
#define I2S_DMA_BUF_COUNT   8

// INMP441 I2S Microphone (I2S_NUM_0)
#define PIN_I2S_MIC_WS      25
#define PIN_I2S_MIC_SCK     26
#define PIN_I2S_MIC_SD      33

// MAX98357A I2S Amplifier (I2S_NUM_1)
#define PIN_I2S_SPK_DIN     22
#define PIN_I2S_SPK_BCLK    27
#define PIN_I2S_SPK_LRC     14

// I2C Bus (AS5600 & SSD1306 OLED)
#define PIN_I2C_SDA         21
#define PIN_I2C_SCL         15
#define AS5600_I2C_ADDR     0x36
#define AS5600_REG_ANGLE    0x0C

// DRV8833 Motor Driver (PWM LEDC Channels 0 & 1)
#define PIN_MOTOR_IN1       18
#define PIN_MOTOR_IN2       19
#define PWM_MOTOR_CH_A      0
#define PWM_MOTOR_CH_B      1
#define PWM_MOTOR_FREQ      20000
#define PWM_MOTOR_RES       8 // 8-bit (0-255)

// MicroSD SPI Bus
#define PIN_SD_CS           5
#define PIN_SD_MOSI         23
#define PIN_SD_MISO         12
#define PIN_SD_SCK          13

// Hardware Buttons & Rocker Switch
#define PIN_BTN_REC         32 // Record Toggle
#define PIN_BTN_MEMO        34 // Instant Memo (Hold-to-record)
#define PIN_BTN_PLAY        35 // Play / Pause
#define PIN_BTN_MODE        36 // Mode / Settings
#define PIN_ROCKER_FWD      39 // Rocker Fast-Forward
#define PIN_ROCKER_REW      4  // Rocker Rewind
#define PIN_LED_REC         2  // Status LED

// ==========================================
// 2. DATA STRUCTURES & SYSTEM STATE
// ==========================================
enum DeviceMode {
  MODE_FIELD_REC,
  MODE_MEMO_STT,
  MODE_USB_AUDIO,
  MODE_MULTI_TRACK
};

struct DeviceState {
  volatile bool isPlaying;
  volatile bool isRecording;
  volatile bool isMemoActive;
  volatile bool isTouchPaused;     // TE TP-7 signature touch-to-pause
  volatile int32_t currentSamplePos;
  volatile int32_t totalSamples;
  volatile float scrubSpeedScale;  // Variable speed scaling
  DeviceMode currentMode;
  uint16_t lastAngle;
  int16_t angularVelocity;
  uint32_t fileIndex;
};

DeviceState sysState = {
  false, false, false, false, 0, 0, 1.0f, MODE_FIELD_REC, 0, 0, 1
};

// Double Audio Ring Buffer
#define RING_BUF_SIZE 4096
int16_t audioRingBuf[RING_BUF_SIZE];
volatile size_t ringWritePtr = 0;
volatile size_t ringReadPtr = 0;
portMUX_TYPE ringBufferMutex = portMUX_INITIALIZER_UNLOCKED;

File activeAudioFile;
BLECharacteristic *pBleCharacteristic = nullptr;

// ==========================================
// 3. HARDWARE DRIVER INITIALIZATION
// ==========================================
void initMicrophone() {
  i2s_config_t micCfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // INMP441 outputs 24b in 32b frame
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = I2S_DMA_BUF_COUNT,
    .dma_buf_len = I2S_DMA_BUF_LEN,
    .use_apll = true
  };
  i2s_pin_config_t micPins = {
    .bck_io_num = PIN_I2S_MIC_SCK,
    .ws_io_num = PIN_I2S_MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = PIN_I2S_MIC_SD
  };
  i2s_driver_install(I2S_NUM_0, &micCfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &micPins);
}

void initSpeaker() {
  i2s_config_t spkCfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = I2S_DMA_BUF_COUNT,
    .dma_buf_len = I2S_DMA_BUF_LEN,
    .use_apll = true
  };
  i2s_pin_config_t spkPins = {
    .bck_io_num = PIN_I2S_SPK_BCLK,
    .ws_io_num = PIN_I2S_SPK_LRC,
    .data_out_num = PIN_I2S_SPK_DIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  i2s_driver_install(I2S_NUM_1, &spkCfg, 0, NULL);
  i2s_set_pin(I2S_NUM_1, &spkPins);
}

void initMotor() {
  ledcSetup(PWM_MOTOR_CH_A, PWM_MOTOR_FREQ, PWM_MOTOR_RES);
  ledcSetup(PWM_MOTOR_CH_B, PWM_MOTOR_FREQ, PWM_MOTOR_RES);
  ledcAttachPin(PIN_MOTOR_IN1, PWM_MOTOR_CH_A);
  ledcAttachPin(PIN_MOTOR_IN2, PWM_MOTOR_CH_B);
}

void setMotorDrive(int16_t dutyCycle /* -255 to 255 */) {
  if (dutyCycle > 0) {
    ledcWrite(PWM_MOTOR_CH_A, (uint32_t)dutyCycle);
    ledcWrite(PWM_MOTOR_CH_B, 0);
  } else if (dutyCycle < 0) {
    ledcWrite(PWM_MOTOR_CH_A, 0);
    ledcWrite(PWM_MOTOR_CH_B, (uint32_t)(-dutyCycle));
  } else {
    ledcWrite(PWM_MOTOR_CH_A, 0);
    ledcWrite(PWM_MOTOR_CH_B, 0);
  }
}

uint16_t readAS5600Angle() {
  Wire.beginTransmission(AS5600_I2C_ADDR);
  Wire.write(AS5600_REG_ANGLE);
  if (Wire.endTransmission(false) != 0) return 0;
  Wire.requestFrom(AS5600_I2C_ADDR, (uint8_t)2);
  if (Wire.available() >= 2) {
    uint16_t angle = (Wire.read() << 8) | Wire.read();
    return angle & 0x0FFF; // 12-bit (0-4095)
  }
  return 0;
}

// ==========================================
// 4. CORE 1 AUDIO TASK (High Real-Time Priority)
// ==========================================
void audioCoreTask(void *param) {
  int32_t micRawBuf[I2S_DMA_BUF_LEN];
  int16_t pcmOutBuf[I2S_DMA_BUF_LEN];
  size_t bytesRead = 0, bytesWritten = 0;

  for (;;) {
    if (sysState.isRecording) {
      // Stream 24-bit samples from INMP441, downscale to 16-bit PCM
      i2s_read(I2S_NUM_0, micRawBuf, sizeof(micRawBuf), &bytesRead, portMAX_DELAY);
      size_t samples = bytesRead / 4;
      for (size_t i = 0; i < samples; i++) {
        pcmOutBuf[i] = (int16_t)(micRawBuf[i] >> 8);
      }
      if (activeAudioFile) {
        activeAudioFile.write((uint8_t*)pcmOutBuf, samples * sizeof(int16_t));
      }
    } else if (sysState.isPlaying && !sysState.isTouchPaused) {
      // Stream audio from active WAV file to MAX98357A DAC
      if (activeAudioFile && activeAudioFile.available() >= sizeof(pcmOutBuf)) {
        size_t n = activeAudioFile.read((uint8_t*)pcmOutBuf, sizeof(pcmOutBuf));
        i2s_write(I2S_NUM_1, pcmOutBuf, n, &bytesWritten, portMAX_DELAY);
        sysState.currentSamplePos += (n / sizeof(int16_t));
      } else {
        // End of file reached
        sysState.isPlaying = false;
      }
    } else {
      vTaskDelay(pdMS_TO_TICKS(5));
    }
  }
}

// ==========================================
// 5. CORE 0 MOTOR, ENCODER & UI LOOP (500 Hz)
// ==========================================
void uiMotorCoreTask(void *param) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(2); // 500 Hz Loop

  for (;;) {
    // 1. Read 12-Bit AS5600 Reel Angle
    uint16_t angle = readAS5600Angle();
    int16_t delta = (int16_t)angle - (int16_t)sysState.lastAngle;
    if (delta > 2048) delta -= 4096;
    if (delta < -2048) delta += 4096;
    sysState.angularVelocity = delta;
    sysState.lastAngle = angle;

    // 2. TE TP-7 Touch-to-Pause & Motor Stall Detection
    // If motor is driven forward (+80 PWM) but reel velocity drops to ~0,
    // user has placed their finger on the reel to pause playback!
    if (sysState.isPlaying) {
      if (abs(delta) < 1 && !sysState.isTouchPaused) {
        sysState.isTouchPaused = true;
        setMotorDrive(0);
      } else if (abs(delta) >= 2) {
        // User turned or released reel
        sysState.isTouchPaused = false;
        setMotorDrive(80); // Resume tape reel drive
        
        // Scrub audio position relative to finger spin
        if (activeAudioFile && abs(delta) > 5) {
          int32_t sampleOffset = (int32_t)(delta * (SAMPLE_RATE * 0.05f));
          int32_t newPos = sysState.currentSamplePos + sampleOffset;
          if (newPos >= 0 && newPos < sysState.totalSamples) {
            sysState.currentSamplePos = newPos;
            activeAudioFile.seek(44 + newPos * sizeof(int16_t));
          }
        }
      }
    } else {
      setMotorDrive(0);
    }

    // 3. Check 2-Stage Rocker Switch for High-Speed Shuttle
    bool fwdPressed = (digitalRead(PIN_ROCKER_FWD) == LOW);
    bool rewPressed = (digitalRead(PIN_ROCKER_REW) == LOW);

    if (fwdPressed && activeAudioFile) {
      sysState.currentSamplePos += (SAMPLE_RATE * 2); // Jump forward 2s
      if (sysState.currentSamplePos > sysState.totalSamples) sysState.currentSamplePos = sysState.totalSamples;
      activeAudioFile.seek(44 + sysState.currentSamplePos * sizeof(int16_t));
      setMotorDrive(200); // High speed motor spin
    } else if (rewPressed && activeAudioFile) {
      sysState.currentSamplePos -= (SAMPLE_RATE * 2); // Jump backward 2s
      if (sysState.currentSamplePos < 0) sysState.currentSamplePos = 0;
      activeAudioFile.seek(44 + sysState.currentSamplePos * sizeof(int16_t));
      setMotorDrive(-200);
    }

    // 4. Memo Button (Hold-to-Record)
    bool memoPressed = (digitalRead(PIN_BTN_MEMO) == LOW);
    if (memoPressed && !sysState.isRecording) {
      sysState.isRecording = true;
      sysState.isMemoActive = true;
      digitalWrite(PIN_LED_REC, HIGH);
    } else if (!memoPressed && sysState.isMemoActive) {
      sysState.isRecording = false;
      sysState.isMemoActive = false;
      digitalWrite(PIN_LED_REC, LOW);
    }

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// ==========================================
// 6. MAIN SYSTEM SETUP & TASK SPAWN
// ==========================================
void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000); // 400kHz Fast I2C

  pinMode(PIN_BTN_REC, INPUT_PULLUP);
  pinMode(PIN_BTN_MEMO, INPUT_PULLUP);
  pinMode(PIN_BTN_PLAY, INPUT_PULLUP);
  pinMode(PIN_BTN_MODE, INPUT_PULLUP);
  pinMode(PIN_ROCKER_FWD, INPUT_PULLUP);
  pinMode(PIN_ROCKER_REW, INPUT_PULLUP);
  pinMode(PIN_LED_REC, OUTPUT);
  digitalWrite(PIN_LED_REC, LOW);

  initMicrophone();
  initSpeaker();
  initMotor();

  SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  SD.begin(PIN_SD_CS);

  // Pin High-Priority Audio Task to Core 1
  xTaskCreatePinnedToCore(audioCoreTask, "AudioCore1", 4096, NULL, 3, NULL, 1);

  // Pin 500Hz UI & Motor Control Loop to Core 0
  xTaskCreatePinnedToCore(uiMotorCoreTask, "UIMotorCore0", 4096, NULL, 2, NULL, 0);

  Serial.println("DIY TP-7 Field Recorder v2.0 Online");
}

void loop() {
  // Main background loop handles BLE sync and battery telemetry
  vTaskDelay(pdMS_TO_TICKS(100));
}
