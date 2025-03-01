
# LoRa_est32

This project implements a LoRa-based communication system using ESP32 microcontrollers. It consists of two devices: a Sender and a Receiver, both running version 1.0 of the firmware.

## Overview

- **Sender (v1.0):** Transmits the state of a single contact (CLOSED or OPEN) via LoRa to the Receiver. It enters deep sleep mode after 30 minutes of inactivity and can be woken up using the PRG button.
- **Receiver (v1.0):** Receives contact state messages, displays them on an OLED screen, logs up to 6 state changes in EEPROM, and signals "CLOSED" with a buzzer. It also enters deep sleep after 30 minutes and supports log reset with a long PRG button press (1 second).

## Hardware Requirements

- ESP32 microcontroller (e.g., ESP32 DevKit)
- SX127x LoRa module
- SSD1306 OLED display (128x64)
- LED (GPIO 2)
- PRG button (GPIO 0)
- Buzzer (GPIO 13 for Receiver)

## Directory Structure

- `sender_v.1.0/sender_v.1.0.ino`: Firmware for the Sender device.
- `reseiver_v.1.0/reseiver_v.1.0.ino`: Firmware for the Receiver device (note: folder name has a typo "reseiver" instead of "receiver").

## Installation

1. Clone this repository:
# LoRa_est32

This project implements a LoRa-based communication system using ESP32 microcontrollers. It consists of two devices: a Sender and a Receiver, both running version 1.0 of the firmware.

## Overview

- **Sender (v1.0):** Transmits the state of a single contact (CLOSED or OPEN) via LoRa to the Receiver. It enters deep sleep mode after 30 minutes of inactivity and can be woken up using the PRG button.
- **Receiver (v1.0):** Receives contact state messages, displays them on an OLED screen, logs up to 6 state changes in EEPROM, and signals "CLOSED" with a buzzer. It also enters deep sleep after 30 minutes and supports log reset with a long PRG button press (1 second).

## Hardware Requirements

- ESP32 microcontroller (e.g., ESP32 DevKit)
- SX127x LoRa module
- SSD1306 OLED display (128x64)
- LED (GPIO 2)
- PRG button (GPIO 0)
- Buzzer (GPIO 13 for Receiver)

## Directory Structure

- `sender_v.1.0/sender_v.1.0.ino`: Firmware for the Sender device.
- `reseiver_v.1.0/reseiver_v.1.0.ino`: Firmware for the Receiver device (note: folder name has a typo "reseiver" instead of "receiver").

## Installation

1. Clone this repository: git clone https://github.com/drpoh/LoRa_est32.git
2. Open the `.ino` files in the Arduino IDE.
3. Install required libraries:
- `SPI`
- `LoRa` (by Sandeep Mistry)
- `Wire`
- `Adafruit_GFX`
- `Adafruit_SSD1306`
- `EEPROM`
4. Connect the hardware as per the pin definitions in the code.
5. Upload the firmware to the respective ESP32 devices.

## Usage

- **Sender:** Powers on, displays startup screen, and continuously sends contact state ("Contacts CLOSED" or "Contacts OPEN") every 50 ms.
- **Receiver:** Powers on, performs hardware check (via Serial), receives messages, displays state, logs changes, and supports PRG button for log view/reset.

## License

This project is open-source and available under the [MIT License](LICENSE).
