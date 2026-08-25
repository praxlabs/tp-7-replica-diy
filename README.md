# DIY TP-7: Motorized-Reel Field Recorder (Open Source Hardware & Firmware)

An open-source digital replica and hardware implementation of the **Teenage Engineering TP-7 Field Recorder**. Features a physical motorized tape reel spool, contactless magnetic angle scrubbing, digital MEMS microphone recording, 24-bit I2S audio processing, dual-stage rocker shuttle, and Bluetooth LE transcription streaming on an **ESP32-WROOM-32E** dual-core microcontroller.

---

## 🌟 Quick Links & Documentation Index

| Resource | Description | Format |
| :--- | :--- | :--- |
| 🕹️ [**Digital Twin Simulator**](index.html) | Interactive web-based hardware simulator with Web Audio engine and touch scrubbing | `HTML5 / JS` |
| 📘 [**Documentation CLI Hub**](documentation.cmd) | Interactive Windows command center for all guides, sketches, and tools | `documentation.cmd` |
| 🔌 [**Breadboard Prototyping Guide**](Breadboard-Prototyping-Guide.md) | Step-by-step wiring diagrams, dual-rail layout, and noise isolation rules | `Markdown` |
| 🧪 [**Breadboard Hardware Diagnostic Tool**](TP7_Replica/Breadboard_Subsystem_Tester.ino) | Interactive serial monitor diagnostic tool for testing sensors and motors | `Arduino (.ino)` |
| 💻 [**Main Firmware (v2.0)**](TP7_Replica/TP7_Replica.ino) | FreeRTOS dual-core firmware with touch-to-pause and memo recording | `Arduino (.ino)` |
| 🛠️ [**Arduino & ESP32 Setup Guide**](Arduino-Prototyping-Setup-Guide.md) | Complete guide for configuring Arduino IDE 2.x and flashing the ESP32 | `Markdown` |
| 📐 [**Custom PCB & Manufacturing Guide**](PCB-Design-And-Manufacturing-Guide.md) | 4-layer PCB stackup, KiCad netlist, and Gerber ordering instructions | `Markdown` |
| 📋 [**Required Parts List & Sourcing BOM**](Required-Parts-List-And-Sourcing.md) | Full bill of materials with footprints, part numbers, and suppliers | `Markdown` |
| ⚖️ [**Commercialization & Legal Guide**](DIY-TP7-Commercialization-And-Legal-Guide.pdf) | IP analysis (trade dress, patents, trademarks), FCC/CE, and battery laws | `PDF Document` |
| 🧊 [**3D Printable STL Files**](3D_Models/) | Production-ready STL models for unibody chassis, motorized reel, and rocker | `3D STL Files` |

---

## 📐 System Architecture

```
                          ┌─────────────────────────────┐
                          │         ESP32-WROOM         │
                          │   (Dual-Core 240MHz, BLE)   │
                          └───────────────┬─────────────┘
         ┌───────────┬───────────┬────────┼────────┬───────────┬───────────┐
         │           │           │        │        │           │           │
    I2S Mic      I2S Amp     I2C Bus   SPI Bus   PWM/GPIO    UART/USB   ADC (Batt)
   (INMP441)   (MAX98357A)  (AS5600  (microSD)  (DRV8833     (USB-C     (Voltage
                             Encoder,            Motor,       Audio /    Divider)
                             SSD1306             Buttons)     Data)
                             OLED)
```

### FreeRTOS Dual-Core Task Allocation
* **Core 1 (High Real-Time Priority - Pri 3):** Dedicated 44.1kHz / 16-bit I2S DMA double-buffered audio streaming for INMP441 microphone recording and MAX98357A Class-D speaker playback.
* **Core 0 (UI & Motor Loop - 500 Hz):** Polls the AS5600 magnetic rotary sensor via fast I2C, computes angular velocity, detects user touch-to-pause motor stall, and drives DRV8833 H-bridge PWM.

---

## 🎛️ Key Hardware Features (Teenage Engineering TP-7 Specifications)

1. **Motorized Tape Reel (Signature Mechanism):**
   * Driven by an N20 micro metal gearmotor (100:1 reduction) via DRV8833 PWM.
   * Position tracked by an **AS5600 12-bit contactless magnetic encoder** (4096 counts per rotation) paired with a 6x3mm N35 diametric magnet.
   * **Touch-to-Pause:** Placing a finger on the spinning reel detects motor stall through angular velocity differentiation, smoothly halting playback without stopping the tape head.
2. **Dedicated Controls:**
   * **MEMO Button:** Instant hold-to-record voice memo shortcut.
   * **REC Button:** Project recording toggle with red LED status glow.
   * **PLAY / PAUSE:** Standard playback toggle.
   * **2-Stage Rocker Switch:** Fast-forward and rewind shuttle scrub with speed acceleration.
3. **Multi-Jack Audio I/O:**
   * 3x 3.5mm TRRS two-way jacks for stereo line-in, microphone input, cue monitoring, and headphones.
4. **Wireless Companion Sync:**
   * Streams audio chunks over Bluetooth Low Energy (BLE) for real-time speech transcription.

---

## 🚀 How to Run the Project

### 1. Launch the Digital Twin Simulator
Open [`index.html`](index.html) in Google Chrome, Firefox, or Edge. You can interact with the virtual motorized reel, record via your computer microphone, and test audio scrubbing directly in your browser.

### 2. Open the Documentation Hub
Double-click [`documentation.cmd`](documentation.cmd) on Windows to launch an interactive menu for accessing all guides, schematics, 3D STL files, and firmware sketches.

### 3. Flash to Hardware
1. Connect your ESP32 board via USB.
2. Open [`TP7_Replica/TP7_Replica.ino`](TP7_Replica/TP7_Replica.ino) in **Arduino IDE**.
3. Select **Board: ESP32 Dev Module** and click **Upload**.

---

## 📜 License & Compliance Note
This project is open-source hardware and software created for educational, prototyping, and research purposes. *"TP-7"*, *"Teenage Engineering"*, and *"Field System"* are registered trademarks of Teenage Engineering AB. Please review [`DIY-TP7-Commercialization-And-Legal-Guide.pdf`](DIY-TP7-Commercialization-And-Legal-Guide.pdf) before considering any commercial distribution.
