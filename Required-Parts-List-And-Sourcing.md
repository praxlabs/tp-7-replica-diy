# DIY TP-7 Field Recorder: Required Parts List & Sourcing Guide

A complete, production-ready Bill of Materials (BOM) for building the DIY Teenage Engineering TP-7 Replica. Every component has been selected for optimal audio fidelity, low mechanical jitter, power efficiency, and compact fit inside the 3D-printed or CNC enclosure.

---

## 1. Primary Electronics & ICs

| Ref Des | Part Name | Manufacturer / MPN | Package / Footprint | Description | Est. Cost (USD) | Sourcing Options |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **U1** | **ESP32-WROOM-32E** | Espressif Systems (`ESP32-WROOM-32E-N4` or DevKitC) | SMD Module (or 38-pin DIP DevBoard) | Dual-Core 240MHz MCU, 4MB Flash, BT/BLE 4.2, I2S Hardware Controller | $3.50 – $6.00 | [DigiKey](https://www.digikey.com) / [LCSC](https://www.lcsc.com) / [Mouser](https://www.mouser.com) |
| **U2** | **INMP441 MEMS Mic** | TDK InvenSense (`INMP441ACEZ-R7`) | Bottom-Port LGA-14 / Breakout | Omnidirectional Digital Microphone with integrated 24-bit I2S ADC (61 dB SNR) | $2.20 – $3.50 | [AliExpress](https://aliexpress.com) / [LCSC](https://www.lcsc.com) / [Amazon](https://amazon.com) |
| **U3** | **MAX98357A Amp** | Analog Devices / Maxim (`MAX98357AETE+T`) | 16-TQFN / Breakout | 3.2W Mono Class-D Audio Amplifier with built-in I2S DAC | $2.50 – $4.00 | [DigiKey](https://www.digikey.com) / [Adafruit](https://www.adafruit.com) / [LCSC](https://www.lcsc.com) |
| **U4** | **AS5600 Angle Sensor** | ams OSRAM (`AS5600-ASOT`) | SOIC-8 / Breakout Module | 12-Bit Programmable Contactless Magnetic Rotary Position Sensor (I2C 0x36) | $1.80 – $3.00 | [LCSC](https://www.lcsc.com) / [AliExpress](https://aliexpress.com) |
| **U5** | **DRV8833 Motor Driver** | Texas Instruments (`DRV8833PWPR`) | 16-HTSSOP / Breakout | Dual H-Bridge Motor Driver (1.5A RMS per channel, PWM speed control) | $1.50 – $2.50 | [DigiKey](https://www.digikey.com) / [LCSC](https://www.lcsc.com) / [Pololu](https://www.pololu.com) |
| **U6** | **WM8960 Audio Codec** | Cirrus Logic (`WM8960CGEFL/RV`) | 32-QFN / Waveshare Breakout | Low Power Stereo Codec with Class D Speaker Driver & 24-bit Stereo ADC/DAC | $7.00 – $11.00 | [LCSC](https://www.lcsc.com) / [Mouser](https://www.mouser.com) / [Waveshare](https://www.waveshare.com) |
| **U7** | **TP4056 + DW01A** | NanJing Top Power (`TP4056`) | SOP-8 / USB-C Charge Board | 1A Standalone Linear Li-Ion Battery Charger with Overcharge/Overdischarge Protection | $0.80 – $1.50 | [AliExpress](https://aliexpress.com) / [Amazon](https://amazon.com) |
| **DISP1** | **0.96" OLED Display** | Solomon Systech (`SSD1306` / `SSD1315`) | 128x64 I2C Monochrome OLED | Ultra-high contrast display for timecode, VU meters, file names, and battery level | $2.50 – $4.50 | [AliExpress](https://aliexpress.com) / [Amazon](https://amazon.com) |

---

## 2. Electromechanical, Motors & Mechanism

| Ref Des | Item Description | Specifications | Function / Notes | Est. Cost (USD) | Sourcing Options |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **M1** | **N20 Micro Metal Gearmotor** | 6V 100RPM – 150RPM (100:1 reduction, 3mm D-Shaft) | Drives the tape reel spool smoothly without excessive gear chatter | $2.50 – $4.00 | [AliExpress](https://aliexpress.com) / [Pololu](https://www.pololu.com) / [Amazon](https://amazon.com) |
| **MAG1** | **Diametric Neodymium Magnet** | 6mm Diameter x 3mm Thickness, N35 / N52 | **Crucial:** Must be *diametrically magnetized* (poles on curved sides) for AS5600 | $0.80 – $1.50 | [SuperMagnetMan](https://supermagnetman.com) / [K&J Magnetics](https://kjmagnetics.com) / [AliExpress](https://aliexpress.com) |
| **SPK1** | **Miniature Dynamic Speaker** | 8 Ohm 1W – 2W, 20mm round x 4mm slim | Internal monitoring speaker | $1.20 – $2.00 | [DigiKey](https://www.digikey.com) / [LCSC](https://www.lcsc.com) |
| **J1, J2, J3** | **3.5mm TRRS Audio Jacks** | PJ-320A / CUI Devices `SJ-43514-SMT` | 4-Pole stereo + mic I/O jacks (Tip, Ring 1, Ring 2, Sleeve) | $0.60 ea ($1.80 total) | [LCSC](https://www.lcsc.com) / [DigiKey](https://www.digikey.com) |
| **J4** | **microSD Card Socket** | Push-Push SPI/SDIO SMD Socket | High-speed FAT32 / exFAT storage for 44.1kHz / 96kHz PCM WAV files | $0.70 – $1.20 | [LCSC](https://www.lcsc.com) / [DigiKey](https://www.digikey.com) |
| **J5** | **USB-C 16-Pin Receptacle** | TYPE-C 16P Female SMD (`TYPE-C-31-M-12`) | 5V 1A charging + USB Mass Storage / UAC 2.0 Audio Interface data | $0.40 – $0.90 | [LCSC](https://www.lcsc.com) |
| **SW1–SW4** | **Tactile SMD Pushbuttons** | 6x6x5mm or 4x4x1.5mm Soft-Click SMD | Record, Memo, Play/Stop, Mode switches | $0.15 ea ($0.60 total) | [LCSC](https://www.lcsc.com) / [DigiKey](https://www.digikey.com) |
| **SW5** | **2-Way Rocker Switch** | SPDT Center-Off Momentary Seesaw Rocker | Side Fast-Forward / Rewind shuttle control | $1.50 – $2.50 | [AliExpress](https://aliexpress.com) / [Mouser](https://www.mouser.com) |
| **BATT1** | **Rechargeable LiPo Cell** | 3.7V 1S 500mAh – 1000mAh (Model 602535 / 803040) | Provides 4 to 8 hours of continuous field recording | $4.50 – $7.00 | [Adafruit](https://www.adafruit.com) / [Amazon](https://amazon.com) |

---

## 3. Passive Components & Hardware Fasteners

| Quantity | Component Type | Value / Rating | Purpose |
| :--- | :--- | :--- | :--- |
| **8** | Ceramic Capacitor 0805 | 100nF (0.1µF) 50V X7R | High-frequency power rail decoupling |
| **4** | Tantalum / Electrolytic Cap | 10µF – 100µF 10V | Bulk decoupling for motor drive and audio amp |
| **4** | SMD Resistor 0805 | 4.7 kΩ 1% | I2C pull-up resistors for SDA / SCL lines |
| **2** | SMD Resistor 0805 | 5.1 kΩ 1% | USB Type-C CC1 / CC2 pull-down resistors (standard 5V negotiation) |
| **2** | SMD Resistor 0805 | 330 Ω 1% | LED current limiting resistors |
| **2** | SMD LED 0805 | Red (REC active), Amber (Charging) | Hardware status indicators |
| **4** | M2 Socket Cap Screws | M2 x 6mm Stainless Steel | Chassis assembly & bottom lid fastening |
| **4** | M2 Brass Heat-Set Threaded Inserts | M2 x 3.5mm OD 3.2mm | Heat-pressed into 3D-printed chassis standoffs |
| **1** | Miniature Ball Bearing (Optional) | 683ZZ (3mm ID x 7mm OD x 3mm Width) | Eliminates reel wobble and ensures whisper-quiet motorized spinning |

---

## 4. 3D Printable Enclosure Parts (Included in `/3D_Models`)

| File Name | Recommended Filament | Infill & Layer Height | Print Notes |
| :--- | :--- | :--- | :--- |
| `TP7_Main_Chassis_Top.stl` | Matte Black PETG or PLA Pro | 25% Gyroid, 0.16mm layer height | Print top-face up with tree supports for jack recesses |
| `TP7_Chassis_Bottom_Lid.stl` | Matte Black PETG / PLA Pro | 30% Infill, 0.20mm layer height | Flat on print bed, no supports needed |
| `TP7_Motorized_Tape_Reel.stl` | Silver / Anodized Grey PETG or Resin | 100% Solid, 0.12mm (Resin preferred) | Ensures smooth balanced rotation around N20 shaft |
| `TP7_Side_Rocker_Switch.stl` | Orange or Black PETG | 100% Solid, 0.12mm | High wear resistance for finger shuttle action |
| `TP7_Tactile_Button_Caps.stl` | Orange (REC) & Black (Play/Mode) | 100% Solid, 0.12mm | Print in sets of 4 |

---

## 5. Total Project Cost Summary

* **DIY Hand-Wired Prototype (Using Breakout Modules):** ~$45.00 – $65.00
* **Custom SMT Assembled PCB (JLCPCB 5-Board Run):** ~$75.00 – $95.00 total
* **Comparison with OEM Teenage Engineering TP-7:** **$1,499.00 retail** (Save over 94% while building your own customized field recorder!)
