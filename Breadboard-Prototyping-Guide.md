# DIY TP-7: Complete Breadboard Prototyping Guide

This guide is a step-by-step blueprint for building a fully working **DIY TP-7 Motorized-Reel Field Recorder** on solderless breadboards before committing to a custom PCB or permanent enclosure.

---

## 1. Breadboard Tools & Materials Needed

| Item | Recommendation | Purpose |
| :--- | :--- | :--- |
| **Solderless Breadboard** | 2x Standard 830 Tie-Point Breadboards (interlocked side-by-side) | Dual breadboards provide ample space for the wide ESP32 DevKitC board + 6 breakout modules without cramped wiring. |
| **Jumper Wires** | 65-piece Male-to-Male flexible wire bundle + 20-piece Male-to-Female ribbons | Short, flat jumper wires keep signal capacitance low for I2S audio and SPI clock. |
| **Decoupling Capacitors** | 4x 100nF (0.1µF) Ceramic + 2x 100µF Electrolytic | Critical for breadboards to suppress contact bounce and motor PWM voltage dips. |
| **Multimeter** | Any standard digital multimeter | Verifying 3.3V / 5.0V voltage rails and checking continuity before power-up. |
| **Power Source** | USB-C cable plugged into ESP32 DevKitC (or 5V breadboard power supply module) | Provides clean 5V and on-board regulated 3.3V rail. |

---

## 2. Breadboard Layout & Power Distribution Architecture

Because the TP-7 combines **high-speed digital audio (I2S DMA)**, **high-current motor switching (DRV8833)**, and **sensitive magnetic sensing (AS5600)**, proper breadboard component placement is essential to eliminate audio buzz:

```
========================================================================================
                                BREADBOARD 1 (AUDIO & MCU)
========================================================================================
   [+ 5V Rail -------------------------------------------------------------------------]
   [- GND Rail ------------------------------------------------------------------------]

     +-----------------------+     +-------------------+     +--------------------+
     |   INMP441 MEMS MIC    |     |  ESP32 DEVKITC    |     |  MAX98357A I2S AMP |
     | (I2S RX Master Bus 0) |     |  (38-Pin Module)  |     | (I2S TX Master 1)  |
     +-----------------------+     +-------------------+     +--------------------+
                │                            │                          │
                ▼                            ▼                          ▼
          Left Channel                  Dual-Core MCU              8Ω 1W Speaker
          (Tie L/R to GND)              (Audio + UI)               (Mono Output)

   [- GND Rail ------------------------------------------------------------------------]
   [+ 3.3V Rail -----------------------------------------------------------------------]
========================================================================================
                                BREADBOARD 2 (STORAGE & MECHANISM)
========================================================================================
   [+ 5V / Batt Rail ------------------------------------------------------------------]
   [- GND Rail (COMMON GROUND TIE) ----------------------------------------------------]

     +-----------------------+     +-------------------+     +--------------------+
     |   SPI MicroSD MODULE  |     |  AS5600 ENCODER   |     |  DRV8833 H-BRIDGE  |
     | (FAT32 WAV Streamer)  |     |  (12-Bit I2C Reel)|     |  (N20 Motor Drive) |
     +-----------------------+     +-------------------+     +--------------------+
                │                            │                          │
                ▼                            ▼                          ▼
          SPI Bus Pins                  Directly under              N20 Gearmotor
          (13, 12, 23, 5)               Magnet Shaft                (100:1, 3mm D)

   [- GND Rail ------------------------------------------------------------------------]
   [+ 3.3V Rail -----------------------------------------------------------------------]
```

---

## 3. Step-by-Step Subsystem Wiring Matrix

### Subsystem 1: Power & Common Ground
* Connect the **GND pin** of the ESP32 to the blue breadboard ground rails.
* Connect the **3V3 pin** of the ESP32 to the red 3.3V power rails.
* Connect the **VIN / 5V pin** of the ESP32 to the 5V power rail (used to power the MAX98357A audio amp for maximum speaker loudness).
* **Crucial:** Interconnect all breadboard ground rails so the entire circuit shares a single, low-impedance ground reference.

---

### Subsystem 2: INMP441 Digital MEMS Microphone (I2S Bus 0)
The INMP441 provides studio-quality digital audio with zero analog noise pickup.

| INMP441 Pin | ESP32 GPIO Pin | Wire Color Recommendation | Function |
| :--- | :--- | :--- | :--- |
| **VDD** | 3.3V Power Rail | Red | 3.3V DC Power |
| **GND** | GND Ground Rail | Black | Power & Signal Ground |
| **SD** | **GPIO 33** | Blue | Serial PCM Audio Data |
| **WS** | **GPIO 25** | Yellow | Word Select (Left/Right clock, 44.1 kHz) |
| **SCK** | **GPIO 26** | Green | Bit Clock (Continuous I2S Clock, 2.822 MHz) |
| **L/R** | **GND** | Black | Selects Left audio channel |

---

### Subsystem 3: MAX98357A I2S Class-D Amplifier & Speaker (I2S Bus 1)
Using a second dedicated I2S port prevents input/output bus contention.

| MAX98357A Pin | ESP32 / Connection | Wire Color Recommendation | Function |
| :--- | :--- | :--- | :--- |
| **VIN** | **5V Power Rail** (or 3.3V) | Red | Class-D Power Supply |
| **GND** | GND Ground Rail | Black | Ground |
| **DIN** | **GPIO 22** | Blue | Audio Sample Data In |
| **BCLK** | **GPIO 27** | Green | Bit Clock |
| **LRC** | **GPIO 14** | Yellow | Left/Right Word Select Clock |
| **SD / GAIN** | Leave Unconnected (or 3.3V) | — | 12dB default gain, auto-stereo mix |
| **+ / − (Screw Terminals)** | **8Ω 1W Speaker Terminals** | Red / Black Pair | Direct acoustic output |

---

### Subsystem 4: MicroSD Card Storage Module (SPI Bus)
Streams raw 16-bit 44.1kHz PCM samples directly to standard `.wav` files.

| MicroSD Module Pin | ESP32 GPIO Pin | Wire Color Recommendation | Function |
| :--- | :--- | :--- | :--- |
| **VCC** | 3.3V Power Rail (or 5V if module has 3.3V regulator) | Red | Power |
| **GND** | GND Ground Rail | Black | Ground |
| **CS** | **GPIO 5** | Orange | SPI Chip Select |
| **MOSI** | **GPIO 23** | Blue | SPI Master Out Slave In |
| **MISO** | **GPIO 12** | Green | SPI Master In Slave Out |
| **SCK** | **GPIO 13** | Yellow | SPI Clock (20 MHz) |

---

### Subsystem 5: AS5600 Magnetic Angle Sensor (I2C Bus)
The AS5600 contactless sensor measures the tape reel rotation angle with 12-bit precision (4096 steps per turn).

| AS5600 Module Pin | ESP32 GPIO Pin | Wire Color Recommendation | Function |
| :--- | :--- | :--- | :--- |
| **VCC** | 3.3V Power Rail | Red | 3.3V Power |
| **GND** | GND Ground Rail | Black | Ground |
| **SDA** | **GPIO 21** | Blue | I2C Serial Data (with 4.7kΩ pull-up) |
| **SCL** | **GPIO 15** | Yellow | I2C Serial Clock (with 4.7kΩ pull-up) |
| **DIR** | GND (Clockwise positive) | Black | Direction selection |

---

### Subsystem 6: DRV8833 Dual H-Bridge & N20 Motor Rig
Drives the tape spool forward during recording/playback and reverse during rewind.

| DRV8833 Pin | ESP32 / Connection | Wire Color Recommendation | Function |
| :--- | :--- | :--- | :--- |
| **VM** | **5V Power Rail** (or LiPo Battery +) | Red | Motor Supply Voltage |
| **GND** | GND Ground Rail | Black | Ground |
| **IN1** | **GPIO 18** | Orange | Motor Forward PWM (LEDC Ch 0) |
| **IN2** | **GPIO 19** | Yellow | Motor Reverse PWM (LEDC Ch 1) |
| **OUT1 / OUT2** | **N20 Motor 2 Terminals** | Red / Black Pair | DC Gearmotor Power |
| **EEP (Sleep)** | **3.3V Power Rail** | Red | Active-high enable |

---

### Subsystem 7: Tactile Buttons & Rocker Switch

| Component | ESP32 GPIO Pin | Wiring Mode | Action Triggered |
| :--- | :--- | :--- | :--- |
| **REC Button** | **GPIO 32** | Pin to Ground (`INPUT_PULLUP`) | Toggles project recording |
| **MEMO Button** | **GPIO 34** | Pin to Ground (`INPUT_PULLUP`) | Instant hold-to-record voice note |
| **PLAY / STOP** | **GPIO 35** | Pin to Ground (`INPUT_PULLUP`) | Starts / pauses playback |
| **MODE Button** | **GPIO 36** | Pin to Ground (`INPUT_PULLUP`) | Cycles field recording modes |
| **Rocker FWD** | **GPIO 39** | Pin to Ground (`INPUT_PULLUP`) | Fast-forward shuttle scrub |
| **Rocker REW** | **GPIO 4** | Pin to Ground (`INPUT_PULLUP`) | Rewind shuttle scrub |
| **REC LED** | **GPIO 2** | In series with 330Ω resistor to GND | Red recording glow ring |

---

## 4. Breadboard Noise Reduction & Prototyping Rules

Solderless breadboards introduce stray capacitance and ground resistance. Follow these 4 golden rules to guarantee clean audio:

1. **Keep I2S & SPI Wires Short (<10 cm / 4 in)**: Long jumper wires act as antennas. Keep clock lines (`SCK`, `BCLK`, `WS`) as direct and short as possible.
2. **Add a 100µF Bulk Capacitor Near the Motor Driver**: Place an electrolytic capacitor right across the DRV8833 `VM` and `GND` pins on the breadboard. This absorbs inductive current spikes when the motor starts or stops.
3. **Decouple 3.3V Rail**: Place a 100nF (0.1µF) ceramic capacitor across 3.3V and GND right next to the INMP441 microphone and AS5600 sensor.
4. **Mechanical Jig for the AS5600 Magnet**:
   * Mount the N35 6x3mm diametric magnet onto the N20 motor shaft.
   * Position the AS5600 breakout board so the IC chip is suspended **0.8 mm to 1.5 mm directly beneath the magnet**.

---

## 5. Breadboard Testing & Verification Sequence

Before flashing the full FreeRTOS firmware, verify each subsystem sequentially using the included [`TP7_Replica/Breadboard_Subsystem_Tester.ino`](file:///c:/Users/PRAKHYAT%20SURAPANENI/OneDrive/Desktop/tp%207%20replica%20a/TP7_Replica/Breadboard_Subsystem_Tester.ino) sketch:

1. **Step 1: Smoke & Voltage Test**
   * Plug in USB-C.
   * Measure 3.3V rail with multimeter: Must read between **3.25V and 3.35V**.
   * Verify ESP32 power LED turns on and no components get hot to the touch.

2. **Step 2: I2C Scanner Test (AS5600)**
   * Open Serial Monitor at **115200 baud**.
   * Run I2C scan: Sensor should respond at address `0x36`.
   * Spin the magnet by hand: Angle in serial monitor should smoothly track from `0°` to `359°` (`0` to `4095` counts).

3. **Step 3: MicroSD SPI Mount Test**
   * Insert FAT32 formatted microSD card (16GB–128GB).
   * Verify serial reports: `[OK] MicroSD Card Mounted. Total Space: 124.8 GB`.

4. **Step 4: I2S Audio Loopback Test**
   * Speak into the INMP441 microphone for 3 seconds.
   * Audio is recorded directly to `TEST_REC.WAV` on the microSD card and immediately played back through the MAX98357A speaker!

5. **Step 5: Motor PWM & Touch-to-Pause Test**
   * Motor spins forward at 80/255 duty cycle.
   * Grab the motor shaft with your fingers: Verify Serial reports `[TOUCH-PAUSE DETECTED: Motor Stalled]`.
   * Release shaft: Verify motor smoothly resumes spinning!
