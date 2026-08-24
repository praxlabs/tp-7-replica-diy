# DIY TP-7: Building Your Own Motorized-Reel Field Recorder

A full technical deep-dive into cloning the core functionality of the Teenage Engineering TP-7: a pocket audio recorder with a motorized "tape reel," multi-jack I/O, USB-C audio interface, Bluetooth transcription hand-off, and single-hand tactile control.

This guide goes subsystem by subsystem: the physics of how each part works, the exact electrical interface, and the firmware that drives it.

---

## 1. System Architecture

```
                         ┌─────────────────────────────┐
                         │         ESP32-WROOM          │
                         │   (dual-core, 240MHz, BT+BLE)│
                         └───────────────┬───────────────┘
        ┌───────────┬───────────┬────────┼────────┬───────────┬───────────┐
        │           │           │        │        │           │           │
   I2S Mic      I2S Amp     I2C Bus   SPI Bus   PWM/GPIO    UART/USB   ADC (batt)
  (INMP441)   (MAX98357A)  (AS5600  (microSD)  (DRV8833     (USB-C     (voltage
                            encoder,            motor,       audio /    divider)
                            WM8960              buttons)     data)
                            codec)
```

Everything hangs off one MCU. The two hard real-time constraints are:
1. **Audio I/O** — must keep a steady sample clock or you get clicks/dropouts.
2. **Motor/encoder loop** — must poll fast enough (>500 Hz) for the reel to feel responsive under a fingertip.

Both are handled with FreeRTOS tasks pinned to separate cores on the ESP32 (audio on core 1, UI/motor on core 0), so a slow SD card write never stalls reel feedback.

---

## 2. Full Bill of Materials

| # | Part | Example / Part Number | Role | Approx. Cost |
|---|------|------------------------|------|--------------|
| 1 | MCU dev board | ESP32-WROOM-32 DevKitC | Brain, BT, I2S, WiFi (OTA updates) | $6–10 |
| 2 | Digital MEMS mic | INMP441 (I2S) | Internal microphone | $3 |
| 3 | I2S Class-D amp | MAX98357A | Drives internal speaker | $4 |
| 4 | Speaker | 8Ω 1W 20mm round | Playback | $2 |
| 5 | Audio codec | WM8960 breakout | External TRRS in/out, line level | $8–12 |
| 6 | TRRS jacks x3 | PJ-320A 3.5mm 4-pole | External mic/headphone I/O | $1.50 ea |
| 7 | microSD breakout | SPI microSD module | Storage (use a 128GB+ card) | $2 |
| 8 | Magnetic rotary encoder | AS5600 (I2C, 12-bit) | Senses reel angle for scrubbing | $3 |
| 9 | Geared DC motor | 6mm N20 micro gearmotor | Spins the reel | $3 |
| 10 | Motor driver | DRV8833 breakout | Drives the N20 motor (H-bridge) | $2 |
| 11 | Diametric magnet | 6x3mm N35 | Mounted on reel shaft, read by AS5600 | $1 |
| 12 | USB-C breakout | USB-C PD/data breakout | Charging + data | $1.50 |
| 13 | LiPo charge/protect IC | TP4056 w/ protection | Battery charging + safety | $1.50 |
| 14 | Battery | 1S LiPo 500mAh | Power | $6 |
| 15 | Buttons | 6mm tactile x4 | Record, mode, play/stop | $0.20 ea |
| 16 | Rocker switch | SPDT momentary rocker (2 stages) | Scrub FF/RW | $2 |
| 17 | Status LED | RGB or single red | Record indicator | $0.20 |
| 18 | Perfboard / custom PCB | — | Assembly | $5–20 |
| 19 | 3D-printed enclosure | PETG/PLA | Housing + reel | filament |

**Total: roughly $60–90** in parts for a hand-built version, versus $1,499 retail — you're trading the CNC-milled aluminum body, brushless motor, and polished firmware for a DIY equivalent that hits the same functional points.

---

## 3. Microphone — Physical & Code

### Physically
The INMP441 is a **MEMS (Micro-Electro-Mechanical System) microphone**. Inside its package is a few-micron-thick silicon diaphragm suspended over a backplate, forming a capacitor. Sound pressure waves deflect the diaphragm, changing the capacitance. An on-chip ASIC senses that capacitance change, amplifies it, and — critically — runs it straight through a **sigma-delta ADC** on the same die. The chip outputs the result as a digital bitstream rather than an analog voltage, which means no external ADC or amplifier is needed and there's very little noise pickup on the wire to the MCU.

The digital protocol used is **I2S (Inter-IC Sound)**, a 3-wire synchronous serial bus:
- **BCLK (bit clock)** — toggles once per data bit
- **WS/LRCLK (word select)** — toggles once per audio sample, tells the receiver whether the current bits are the Left or Right channel
- **SD (serial data)** — the actual PCM sample bits, MSB first

The INMP441 is mono, and you select Left or Right channel by tying its `L/R` pin to GND (Left) or VDD (Right).

### Wiring
| INMP441 pin | ESP32 pin |
|---|---|
| VDD | 3.3V |
| GND | GND |
| WS | GPIO25 |
| SCK (bit clock) | GPIO26 |
| SD | GPIO33 |
| L/R | GND (selects left channel) |

### Code
```cpp
#include <driver/i2s.h>

#define I2S_WS   25
#define I2S_SCK  26
#define I2S_SD   33
#define I2S_PORT I2S_NUM_0
#define SAMPLE_RATE 44100

void micInit() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // INMP441 outputs 24-bit in a 32-bit frame
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
    // raw samples are 32-bit, real audio data is the top 24 bits — shift down
    for (int i = 0; i < bytesRead / 4; i++) {
      int32_t sample = raw[i] >> 8;
      writeToRingBuffer(sample);
    }
  }
}
```

---

## 4. Speaker Output — Physical & Code

### Physically
The **MAX98357A** is a Class-D amplifier with a built-in I2S DAC. Class-D means it doesn't linearly amplify the signal (like Class-A/AB) — it converts the digital audio into a high-frequency **PWM (pulse-width modulation)** signal whose duty cycle tracks the waveform, then switches a power transistor fully on/off at that duty cycle. An LC filter (often just the speaker coil's own inductance) smooths this into an analog waveform. This is why Class-D is so efficient (>90%) and stays cool — the transistors are never in a lossy "partially on" state.

The speaker itself is a standard **electrodynamic driver**: a voice coil attached to a diaphragm sits in the field of a permanent magnet. Current through the coil creates a varying magnetic field that pushes/pulls against the magnet, moving the diaphragm and displacing air — that's the sound.

### Wiring
| MAX98357A pin | ESP32 pin |
|---|---|
| VIN | 5V (or 3.3V, lower volume) |
| GND | GND |
| DIN | GPIO22 |
| BCLK | GPIO27 |
| LRC | GPIO14 |
| SD (shutdown, active high enables) | GPIO21 |
| Speaker + / − | Speaker terminals |

Use a **second I2S port** (`I2S_NUM_1`) for playback so mic and speaker never contend for the same bus.

### Code
```cpp
i2s_config_t out_cfg = {
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
  .sample_rate = SAMPLE_RATE,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
  .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  .communication_format = I2S_COMM_FORMAT_STAND_I2S,
  .dma_buf_count = 8, .dma_buf_len = 256
};
i2s_driver_install(I2S_NUM_1, &out_cfg, 0, NULL);
i2s_set_pin(I2S_NUM_1, &out_pins);

void playbackTask(void*) {
  int16_t buf[256];
  for (;;) {
    size_t n = readFromFileOrBuffer(buf, sizeof(buf));
    size_t written;
    i2s_write(I2S_NUM_1, buf, n, &written, portMAX_DELAY);
  }
}
```

---

## 5. Storage — Physical & Code

### Physically
A microSD card is **NAND flash memory** — data is stored as trapped charge in floating-gate transistors, read by sensing threshold voltage shifts. The card exposes an SPI (or faster SDIO) interface with an onboard controller that handles wear-leveling and the FAT filesystem block layer, so from the MCU's side it just looks like a block storage device you talk to over SPI.

### Wiring (SPI mode)
| microSD module | ESP32 pin |
|---|---|
| CS | GPIO5 |
| MOSI | GPIO23 |
| MISO | GPIO19 |
| SCK | GPIO18 |
| VCC | 3.3V |
| GND | GND |

### Code
Audio is written as a standard **WAV file**: a 44-byte header (RIFF chunk descriptor + fmt subchunk + data subchunk) followed by raw PCM samples. You write a placeholder header, stream samples, then seek back and patch the header's size fields once recording stops (since you don't know the final length in advance).

```cpp
#include <SD.h>
#include <SPI.h>

File audioFile;

void startRecording(const char* filename) {
  audioFile = SD.open(filename, FILE_WRITE);
  writeWavHeaderPlaceholder(audioFile); // 44 bytes, sizes = 0 for now
}

void onAudioChunk(int16_t* samples, size_t count) {
  audioFile.write((uint8_t*)samples, count * 2);
}

void stopRecording() {
  uint32_t dataSize = audioFile.size() - 44;
  audioFile.seek(4);
  writeLE32(audioFile, dataSize + 36);   // RIFF chunk size
  audioFile.seek(40);
  writeLE32(audioFile, dataSize);        // data chunk size
  audioFile.close();
}
```

---

## 6. The Motorized Tape Reel — Physical & Code (the signature mechanism)

This is the part that makes a TP-7 feel like a TP-7, and it's worth understanding precisely: **it isn't moving physical tape.** It's a free-spinning wheel, instrumented with a rotary sensor, that a small motor can also drive. It functions simultaneously as:
- a **scrub wheel** (you turn it, playback position jumps)
- a **jog dial** (menu navigation)
- a **visual/haptic status indicator** (it spins on its own during record/playback, and you can feel it resist or stop under your finger)

### Physically — sensing rotation
A small **diametrically-magnetized magnet** (north/south poles across its diameter, not top/bottom) is glued to the reel's center shaft. Below it sits the **AS5600**, a Hall-effect angle sensor. Hall-effect sensing works because the magnetic field from the rotating magnet, sensed by an array of Hall elements on the AS5600 die, changes its measured *angle* (not just its strength) as the magnet rotates — the chip's internal DSP computes the arctangent of the field vector components and outputs an absolute 12-bit angle (0–4095, i.e., 0.088°/count) over I2C. Absolute means it always knows the exact angle immediately at power-on, no homing/calibration pass needed like an incremental encoder.

### Physically — driving rotation
The **N20 micro gearmotor** is a small brushed DC motor with an integrated gearbox (typically ~50:1 to ~1000:1 reduction, pick a slower ratio like 100:1 for smooth, low-speed torque). Current through the motor's windings creates a magnetic field that interacts with fixed magnets in the motor housing, producing torque on the rotor; the gearbox trades speed for torque so it can move the reel smoothly rather than spinning too fast to look tape-like.

The **DRV8833** is a dual H-bridge driver: four transistors arranged so current can flow through the motor in either direction depending on which pair is switched on, giving you forward/reverse. Speed is controlled via **PWM** on the enable/input lines — the average voltage the motor "feels" is proportional to the PWM duty cycle.

### Wiring
| Component | ESP32 pin |
|---|---|
| AS5600 VCC | 3.3V |
| AS5600 GND | GND |
| AS5600 SDA | GPIO21 |
| AS5600 SCL | GPIO22 |
| DRV8833 AIN1 | GPIO18 |
| DRV8833 AIN2 | GPIO19 |
| DRV8833 VM | Battery+ (3.7–5V) |
| DRV8833 AOUT1/2 | Motor terminals |

*(Note: AS5600 and SD card share I2C/SPI address space but different buses — if GPIO18/19 collide with your SD wiring, remap either the SD CS/SCK or motor driver pins; ESP32 GPIOs are largely reassignable in software.)*

### Code — reading the reel for scrubbing
```cpp
#include <Wire.h>
#define AS5600_ADDR 0x36
#define ANGLE_REG   0x0C

uint16_t readReelAngle() {
  Wire.beginTransmission(AS5600_ADDR);
  Wire.write(ANGLE_REG);
  Wire.endTransmission(false);
  Wire.requestFrom(AS5600_ADDR, 2);
  uint16_t angle = (Wire.read() << 8) | Wire.read();
  return angle & 0x0FFF; // 12-bit
}

int32_t lastAngle = 0;
int32_t audioPositionSamples = 0;
const float SAMPLES_PER_DEGREE = SAMPLE_RATE * 0.05; // tune: ~50ms of audio per degree of turn

void scrubTask(void*) {
  for (;;) {
    int32_t angle = readReelAngle();
    int32_t delta = angle - lastAngle;
    // handle 0/4095 wrap-around
    if (delta > 2048) delta -= 4096;
    if (delta < -2048) delta += 4096;

    if (userIsTouchingReel()) {
      audioPositionSamples += delta * SAMPLES_PER_DEGREE;
      seekPlayback(audioPositionSamples);
    }
    lastAngle = angle;
    vTaskDelay(pdMS_TO_TICKS(2)); // ~500Hz poll rate
  }
}
```

`userIsTouchingReel()` in the real device is inferred from sudden deceleration/resistance; a simple DIY approach is a small capacitive-touch pad (ESP32 has native touch-sensing pins) on the reel's rim, or just detecting when angle changes faster than the motor's own commanded speed (meaning an external hand is overriding it).

### Code — driving the motor to spin during playback
```cpp
void driveMotor(int pwmDuty /* -255..255, sign = direction */) {
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
      driveMotor(80); // gentle constant spin during playback
    } else if (!isPlaying) {
      driveMotor(0);
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}
```

---

## 7. Buttons & Rocker — Physical & Code

### Physically
Tactile buttons are simple momentary SPST switches — a conductive dome collapses under pressure to bridge two contacts, pulling a GPIO pin from HIGH (via an internal pull-up resistor) to LOW. Mechanical contacts "bounce" for a few milliseconds when pressed (the metal contacts physically vibrate before settling), so software debouncing is required or you'll register multiple presses per click.

The scrub rocker is just two of these switches under a single seesaw-shaped cap — pressing the top edge closes one contact (fast-forward), pressing the bottom closes the other (rewind).

### Wiring
| Button | ESP32 pin |
|---|---|
| Record/Memo | GPIO32 |
| Play/Stop | GPIO27 (reused if not driving amp LRC on same pin — remap as needed) |
| Mode | GPIO35 |
| Rocker FF | GPIO34 |
| Rocker RW | GPIO39 |

All wired: one leg to GPIO, other leg to GND, using `INPUT_PULLUP` mode internally.

### Code
```cpp
struct Button { uint8_t pin; bool lastState; uint32_t lastChange; };
Button recordBtn = {32, HIGH, 0};

void pollButton(Button &b, void (*onPress)()) {
  bool state = digitalRead(b.pin);
  if (state != b.lastState && millis() - b.lastChange > 30) { // 30ms debounce
    b.lastChange = millis();
    b.lastState = state;
    if (state == LOW) onPress(); // active-low
  }
}

void loop() {
  pollButton(recordBtn, toggleRecording);
  // ...repeat for other buttons
}
```

---

## 8. External I/O Jacks & Codec — Physical & Code

### Physically
A **TRRS jack** has 4 contacts (Tip, Ring, Ring, Sleeve), letting one 3.5mm connector carry stereo in + mono mic, or be reconfigured as two-way I/O like the real TP-7's jacks. To use these at *line level* (not just direct-to-MCU digital mic level), you need a proper **audio codec chip** like the WM8960, which contains real analog-domain ADCs/DACs, programmable gain amplifiers, and analog switching — because raw GPIO/I2S from the ESP32 can't safely accept the wider voltage swings and impedances of external pro/consumer audio gear.

The codec is controlled two ways simultaneously:
- **I2C** — a low-speed control bus to set gain, routing, and mode registers
- **I2S** — the actual high-speed audio sample stream

### Wiring
| WM8960 pin | ESP32 pin |
|---|---|
| SDA/SCL | GPIO21/GPIO22 (shared I2C bus with AS5600 — different address, fine) |
| I2S BCLK/WS/DOUT/DIN | dedicated I2S port pins |
| VDD | 3.3V |

### Code (register config is codec-specific; conceptually)
```cpp
// I2C register writes to configure input gain, routing, output mode
wm8960_write(0x00, 0x0197); // Left input volume
wm8960_write(0x02, 0x0179); // Left output volume
wm8960_write(0x1A, 0x0008); // Route line-in -> ADC
// Then treat its I2S stream exactly like the internal mic's
```

---

## 9. USB-C — Charging & Data

### Physically
USB-C is just a connector standard with 24 pins supporting multiple *protocols* over the same physical port. For this project you need exactly two of its capabilities:
1. **VBUS power delivery** (5V, or negotiated higher via CC-line resistors) — for charging.
2. **USB Mass Storage / Audio Class** — for data.

The **TP4056** charge controller monitors battery voltage and regulates charge current in two phases: constant-current (bulk charging, until cell reaches ~4.2V) then constant-voltage (float charging, current tapers off) — this two-stage profile is standard for all LiPo chemistry and prevents overcharging, which can otherwise cause thermal runaway.

### Simplification for DIY builds
Getting full USB Audio Class 2.0 working on ESP32 firmware is a deep rabbit hole (it doesn't have native high-speed USB on most variants). Two practical paths:
- **Easiest:** expose the SD card as a **USB Mass Storage Device** via TinyUSB — drag files off like a USB flash drive, and use your phone's own mic recorder + a separate transcription app instead of true USB audio streaming.
- **Full-featured:** use an **ESP32-S3** (native USB OTG) with TinyUSB's UAC (USB Audio Class) driver for true "shows up as an audio interface" behavior like the real TP-7.

### Wiring
| TP4056 | Connection |
|---|---|
| IN+/IN− | USB-C VBUS/GND |
| BAT+/BAT− | LiPo cell |
| OUT+/OUT− | System power rail → ESP32 VIN (via a boost/buck as needed) |

---

## 10. Bluetooth & Transcription

### Physically
The ESP32's radio does the same digital-to-RF conversion any Bluetooth chip does: your audio/data bytes are chunked into packets, spread across the 2.4GHz ISM band's 79 channels via frequency-hopping spread spectrum, modulated (GFSK), and broadcast.

### Code
Two options depending on features desired:
- **BLE file transfer**: send the finished WAV file to a phone app as byte chunks over a GATT characteristic once recording stops.
- **A2DP source**: stream live audio continuously (more power-hungry, more complex).

For transcription itself — that's not a physical/firmware component at all, it's a network API call the *phone app* makes to a speech-to-text service after receiving the audio. The device's job ends at getting bytes to the phone.

```cpp
#include <BLEDevice.h>
#include <BLEServer.h>

void sendFileOverBLE(File &f) {
  uint8_t chunk[512];
  while (f.available()) {
    size_t n = f.read(chunk, sizeof(chunk));
    pCharacteristic->setValue(chunk, n);
    pCharacteristic->notify();
    vTaskDelay(pdMS_TO_TICKS(10)); // pace to avoid overrunning BLE MTU/throughput
  }
}
```

---

## 11. Power Budget & Battery Life

| Subsystem | Typical draw |
|---|---|
| ESP32 active (WiFi/BT off) | ~40 mA |
| ESP32 with BLE active | ~100–130 mA |
| INMP441 mic | ~1.4 mA |
| MAX98357A (playing, moderate volume) | ~100–500 mA |
| N20 motor (light load) | ~50–150 mA |
| SD card write | ~30–100 mA (bursty) |

Rough continuous-recording draw (mic + SD + MCU, no BT, no motor): **~120 mA** → a 500mAh cell gives **~4 hours**; scale battery capacity up (1000mAh+) to approach the real device's 7-hour spec, remembering to size your enclosure accordingly.

---

## 12. Firmware Task Architecture (put it all together)

```cpp
void setup() {
  micInit(); speakerInit(); sdInit(); i2cInit();
  motorInit(); buttonsInit(); bleInit();

  xTaskCreatePinnedToCore(audioRecordTask, "rec",  4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(audioPlaybackTask,"play", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(scrubTask,        "scrub",2048, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(motorTask,        "motor",2048, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(uiTask,           "ui",   2048, NULL, 1, NULL, 0);
}
```
Audio tasks get **higher priority** and live on **core 1** exclusively, since a missed I2S DMA deadline causes audible glitches. UI/motor/button polling lives on **core 0**, where occasional millisecond-scale jitter is imperceptible.

---

## 13. Build Order (recommended)

1. **Audio core first**: mic → SD → WAV file → speaker playback, no motor, no buttons except a hardcoded record/stop via serial console. Get clean audio round-tripping before anything else.
2. **Add buttons**: physical record/play/stop.
3. **Add the encoder + motor as a pure UI layer** on top of already-working playback — map angle deltas to seek offsets.
4. **Add the codec + TRRS jacks** for external mic/line-level I/O.
5. **Add BLE file transfer** last, since it's fully decoupled from the rest.
6. **Enclosure + reel machining** — a 3D-printed reel hub press-fit onto the N20 motor shaft, with the AS5600 magnet embedded in the center, sitting just above the sensor on the PCB.

---

## 14. Key Differences From the Real TP-7 (so expectations are calibrated)

- Real TP-7 uses a **custom brushless motor with ball bearings** for a smoother, silent reel feel — an N20 brushed gearmotor will have more mechanical noise and cogging.
- Real device does **on-device DSP** for the VU meter, multi-channel mixing, and 24-bit/96kHz — this guide targets 16-bit/44.1kHz, which is CD-quality and plenty for voice/most music demos, but not matching their pro-audio spec.
- True **USB Audio Class 2.0 multi-channel interface** mode requires an ESP32-S3 (or dedicated USB audio codec IC) — plain ESP32-WROOM can't natively present as a multichannel USB audio device.
- Aluminum unibody vs. 3D-printed shell is obviously a fit-and-finish gap, not a functional one.

---

## 15. Reference Component Datasheets to Pull Up While Building

- INMP441 I2S MEMS microphone datasheet
- MAX98357A I2S Class-D amplifier datasheet
- AS5600 magnetic rotary position sensor datasheet
- DRV8833 dual H-bridge motor driver datasheet
- WM8960 audio codec datasheet
- TP4056 linear Li-ion battery charger datasheet
- ESP32 Technical Reference Manual (I2S, I2C, SPI, PWM/LEDC peripheral chapters)
