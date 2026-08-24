# Custom PCB Design & Manufacturing Guide for DIY TP-7

This guide walks you through designing, ordering, and assembling the custom mainboard PCB for the **DIY TP-7 Motorized-Reel Field Recorder**.

---

## 1. PCB Software Installation & Setup

We recommend **KiCad (v8/v10)**, the industry-standard open-source electronics design automation (EDA) suite.

### Installing KiCad on Windows
1. Open PowerShell or Command Prompt.
2. Run the automated winget installer:
   ```powershell
   winget install --id KiCad.KiCad -e --accept-package-agreements --accept-source-agreements
   ```
3. Alternatively, download the direct installer from [kicad.org/download](https://www.kicad.org/download/windows/).

---

## 2. Board Dimensions & Mechanical Constraints

| Parameter | Value | Reason |
| :--- | :--- | :--- |
| **Dimensions** | 64.0 mm x 92.0 mm (with 3.0mm corner radiuses) | Fits snugly within the 68mm x 96mm aluminum/3D chassis |
| **Board Thickness** | 1.6 mm (FR-4) | Standard thickness, rigid under motor torque |
| **Layer Count** | 4 Layers (Signal / GND / Power / Signal) | Superior ground return paths for whisper-quiet audio SNR |
| **Copper Weight** | 1 oz (35 µm) outer, 1 oz inner | Adequate current capacity for N20 motor pulses (up to 500mA) |
| **Solder Mask** | Matte Black or Aluminum White | Matches Teenage Engineering minimalist design language |
| **Surface Finish** | **ENIG (Electroless Nickel Immersion Gold)** | Flat pads for LGA MEMS mic & reliable button contacts |

```
                       64.0 mm
         ┌───────────────────────────────────┐
         │ [3.5mm-1]  [3.5mm-2]  [3.5mm-3]   │ ◄ Top Audio Jacks
         │                                   │
         │   ┌────────┐        ┌─────────┐   │
         │   │ OLED   │        │ MEMS    │   │ ◄ SSD1306 Display & INMP441 Mic
         │   └────────┘        └─────────┘   │
  92.0   │                                   │
  mm     │             ┌──────┐              │
         │   [Rocker]  │AS5600│              │ ◄ AS5600 Magnet Sensor in Center
         │             └──────┘              │   (Aligned directly under reel shaft)
         │                                   │
         │   [REC]   [PLAY]   [STOP]  [MODE] │ ◄ Tactile Control Buttons
         │                                   │
         │            [ USB-C ]              │ ◄ Bottom USB-C Port
         └───────────────────────────────────┘
```

---

## 3. Schematic Pin Mapping & Electrical Interconnects

### ESP32-WROOM-32 Pin Allocation Table

```
+--------------------+--------------+-----------------------------------------------+
| ESP32 Pin Name     | GPIO Number  | Connected Peripheral & Protocol               |
+--------------------+--------------+-----------------------------------------------+
| GPIO25             | GPIO25       | INMP441 I2S_0 Word Select (WS / LRCLK)        |
| GPIO26             | GPIO26       | INMP441 I2S_0 Continuous Bit Clock (SCK/BCLK) |
| GPIO33             | GPIO33       | INMP441 I2S_0 Serial Data Input (SD)          |
| GPIO22             | GPIO22       | MAX98357A I2S_1 Data In (DIN)                 |
| GPIO27             | GPIO27       | MAX98357A I2S_1 Bit Clock (BCLK)              |
| GPIO14             | GPIO14       | MAX98357A I2S_1 Word Select (LRC / WS)        |
| GPIO21             | GPIO21       | I2C SDA (AS5600 0x36 & OLED 0x3C & WM8960)    |
| GPIO15 (or 22)     | GPIO15       | I2C SCL (Clock line with 4.7kΩ pullups)       |
| GPIO18             | GPIO18       | DRV8833 Motor H-Bridge Channel A (PWM Forward)|
| GPIO19             | GPIO19       | DRV8833 Motor H-Bridge Channel B (PWM Reverse)|
| GPIO5              | GPIO5        | microSD SPI Chip Select (CS)                  |
| GPIO23             | GPIO23       | microSD SPI Master Out Slave In (MOSI)        |
| GPIO19 (remap)     | GPIO12       | microSD SPI Master In Slave Out (MISO)        |
| GPIO18 (remap)     | GPIO13       | microSD SPI Serial Clock (SCK)                |
| GPIO32             | GPIO32       | REC Pushbutton (Active LOW, internal pull-up) |
| GPIO34             | GPIO34       | MEMO Pushbutton (Dedicated voice memo)        |
| GPIO35             | GPIO35       | PLAY/STOP Pushbutton                          |
| GPIO36 (VP)        | GPIO36       | MODE Pushbutton                               |
| GPIO39 (VN)        | GPIO39       | 2-Stage Rocker FF (Fast-Forward switch)       |
| GPIO4              | GPIO4        | 2-Stage Rocker REW (Rewind switch)            |
| GPIO2              | GPIO2        | Status LED (Red Record Ring Indicator)        |
| GPIO36 (ADC1_0)    | GPIO36       | Battery Voltage Divider (100kΩ / 100kΩ)       |
+--------------------+--------------+-----------------------------------------------+
```

---

## 4. Critical PCB Routing & Audio Isolation Rules

To prevent motor switching noise and digital I2S harmonics from bleeding into your audio recordings:

1. **Star Grounding & Solid Ground Plane**:
   - Layer 2 is a dedicated, uninterrupted **Ground Plane (GND)**.
   - Keep digital high-speed signals (SPI SCK at 20MHz, I2S BCLK at 2.8MHz) away from analog input traces.
2. **Motor Isolation & Decoupling**:
   - Place a **100µF low-ESR tantalum capacitor** right at the DRV8833 `VM` motor power pin.
   - Place **100nF ceramic capacitors** across the N20 motor terminals inside the casing to suppress inductive brush sparks.
3. **AS5600 Magnetic Placement**:
   - The AS5600 IC must be positioned **precisely at the geometric center** of the tape reel recess.
   - Keep the air gap between the rotating N35 diametric magnet and the top surface of the AS5600 IC between **0.5 mm and 1.5 mm**.
4. **USB-C CC Pull-Down Resistors**:
   - Populate **5.1 kΩ (±1%) resistors** from `CC1` to GND and `CC2` to GND. This ensures USB-C Power Delivery chargers reliably supply 5V 1A.

---

## 5. Exporting Gerbers & Ordering from JLCPCB / PCBWay

Once your layout is complete in KiCad:

1. **Run DRC (Design Rules Check)**: Ensure 0 clearance errors, minimum trace width 0.15mm (6 mil), minimum via hole 0.3mm.
2. **Plot Gerber Files**:
   - Go to `File` > `Plot` in KiCad PCB Editor.
   - Select layers: `F.Cu`, `B.Cu`, `In1.Cu`, `In2.Cu`, `F.Paste`, `B.Paste`, `F.Silkscreen`, `B.Silkscreen`, `F.Mask`, `B.Mask`, `Edge.Cuts`.
   - Click **Generate Drill Files** (`.drl` / Excellon format).
   - Zip all output files into `TP7_Gerber_v1.0.zip`.
3. **Upload to JLCPCB or PCBWay**:
   - Upload the ZIP file.
   - Set **Layers: 4**, **Surface Finish: ENIG**, **PCB Color: Matte Black**.
   - Optional: Enable **SMT Assembly** and upload the BOM (`Required-Parts-List-And-Sourcing.md`) and CPL centroid coordinates file.

---

## 6. SMT Assembly & Testing Checklist

- [ ] **Power Rail Check**: Measure resistance between 3.3V and GND (should be >10kΩ).
- [ ] **USB-C 5V Verification**: Plug in USB-C cable and verify TP4056 outputs 4.2V charging voltage to battery terminals.
- [ ] **Flash Firmware**: Connect USB-to-UART or ESP-Prog to test firmware upload.
- [ ] **I2C Bus Scan**: Verify AS5600 (`0x36`) and SSD1306 OLED (`0x3C`) respond on I2C bus.
- [ ] **Motor Rotation Test**: Test PWM spinning forward (CW) and reverse (CCW).
- [ ] **Audio Loopback**: Record a 5-second test clip through INMP441 mic to microSD, and verify clean playback through MAX98357A speaker.
