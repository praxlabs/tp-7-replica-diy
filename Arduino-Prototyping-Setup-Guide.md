# Arduino IDE & ESP32 Prototyping Setup Guide

This guide shows you how to set up **Arduino IDE** to compile, test, and flash your breadboard prototype of the DIY TP-7 Field Recorder.

---

## 1. Installed Prototyping Software Suite

The following development environment has been installed on your system:
* **Arduino IDE 2.x:** Complete C++ development environment and Serial Monitor for ESP32.
* **KiCad 10.0.5:** Schematic capture, breadboard netlist viewer, and PCB layout designer.
* **Python 3.12 & esptool:** For direct high-speed serial flashing and bootloader diagnostics.

---

## 2. Setting Up ESP32 Board Support in Arduino IDE

1. Open **Arduino IDE**.
2. Open **File > Preferences** (or press `Ctrl + ,`).
3. In the **Additional boards manager URLs** field, paste the official Espressif ESP32 package URL:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Click **OK**.
5. Go to **Tools > Board > Boards Manager...** (or click the Board icon on the left sidebar).
6. Search for `esp32` and click **Install** on **esp32 by Espressif Systems**.

---

## 3. Recommended Arduino IDE Settings for DIY TP-7

When plugging your ESP32 DevKitC into USB, configure the **Tools** menu as follows:

| Setting | Value | Why |
| :--- | :--- | :--- |
| **Board** | `ESP32 Dev Module` | Standard pin mapping for 30-pin & 38-pin DevKit boards |
| **Upload Speed** | `921600` (or `115200`) | High-speed flashing |
| **CPU Frequency** | `240MHz (WiFi/BT)` | Maximum dual-core performance for real-time I2S DMA |
| **Flash Frequency** | `80MHz` | Fast code execution from SPI Flash |
| **Flash Mode** | `QIO` | Quad SPI mode |
| **Partition Scheme** | `Huge APP (3MB No OTA/1MB SPIFFS)` | Gives full memory space for BLE and audio buffer tasks |
| **Port** | `COM3` / `COM4` (your USB-to-UART port) | Select your connected ESP32 serial port |

---

## 4. Required Arduino Libraries

The TP-7 firmware uses native ESP32 FreeRTOS drivers (`driver/i2s.h`, `SPI.h`, `SD.h`, `Wire.h`, `BLEDevice.h`), so **no external 3rd-party library downloads are required**! Everything compiles out of the box with the core ESP32 board package.

*(Optional: For OLED display graphical animations, you can install `Adafruit SSD1306` and `Adafruit GFX` from **Tools > Manage Libraries**).*

---

## 5. Breadboard First-Flash Walkthrough

1. Connect your breadboarded ESP32 to your PC via a micro-USB or USB-C data cable.
2. In Arduino IDE, open [`TP7_Replica/Breadboard_Subsystem_Tester.ino`](file:///c:/Users/PRAKHYAT%20SURAPANENI/OneDrive/Desktop/tp%207%20replica%20a/TP7_Replica/Breadboard_Subsystem_Tester.ino).
3. Click the **Upload** arrow button (or press `Ctrl + U`).
4. Once flashed, open **Tools > Serial Monitor** (`Ctrl + Shift + M`) and set baud rate to **115200**.
5. Press `1` to scan the I2C bus, `2` to track reel angle, or `5` to hear a test tone through your speaker!
6. Once all breadboard modules pass diagnostics, open [`TP7_Replica/TP7_Replica.ino`](file:///c:/Users/PRAKHYAT%20SURAPANENI/OneDrive/Desktop/tp%207%20replica%20a/TP7_Replica/TP7_Replica.ino) and upload the full field recorder firmware.
