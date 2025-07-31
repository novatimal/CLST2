[![Status](https://img.shields.io/badge/status-early%20beta-orange)](https://shields.io)


> **Note:** This project is in early beta. Expect bugs, missing features, and frequent changes. Contributions and feedback are welcome!

Перевод / Translation:
[English](README.md) | [Русский](README.ru.md)


# Project CLST2: Building a Cyberdeck based on Orange Pi Zero 2W

<!-- Здесь будет фото -->

This is the repository for a project to build a retro-style handheld computer (PDA/Cyberdeck). The device is based on the Orange Pi Zero 2W single-board computer and uses custom drivers and a finely-tuned Linux environment for a comfortable user experience on a small screen.

**This README contains detailed instructions for assembly, software installation, and solutions to problems encountered during development.**

## I. Concept and Philosophy

*   **Goal:** To create a fully functional, pocket-sized Linux computer with a physical keyboard for development, system administration, and retro gaming, featuring a modular expansion system.
*   **Inspiration:** Classic 90s PDAs (Psion, Sharp Zaurus), Windows CE navigators (G-Sat GV-370), and cyberpunk aesthetics.
*   **Interface:** A full-featured, customizable desktop environment adapted for a 320x240 screen, without artificial limitations or shells.

## II. Hardware Components

### Core Components:
*   **Processing Module:** Orange Pi Zero 2W
*   **Display:** 2.4" SPI 320x240 LCD (ST7789V controller)
*   **Keyboard:** M5Stack CardKB (I2C connection)
*   **Input Device (Mouse):** Analog joystick connected via Digispark ATTiny85 (emulates a USB mouse)
*   **Expansion Port:** Custom slot based on a **NAGRA** card reader for modular cartridges.
*   **Audio System:** **MAX98357A** I2S amplifier with an external speaker.
*   **Cooling System:** **4010 5V** Blower Fan with copper heatsinks.
*   **Power Supply:** Li-Pol battery with a **TP4056** charging board.
*   **Case:** 3D Printed (design in progress).

### Connection Diagram (GPIO Pinout)

*(Pin numbers are physical for the 26-pin header of the Orange Pi Zero 2W. Always verify with the official pinout diagram for your board revision!)*

| Component      | Device Pin     | Pin on Orange Pi Zero 2W | Function           |
|----------------|----------------|--------------------------|-------------------|
| **Display (SPI1)** | VCC          | 3.3V (Pin 1)             | Power             |
|                | GND            | GND (Pin 6)              | Ground            |
|                | MOSI           | SPI1_MOSI (Pin 19)       | SPI Data          |
|                | SCLK           | SPI1_SCLK (Pin 23)       | SPI Clock         |
|                | CS             | SPI1_CS (Pin 24)         | SPI Chip Select   |
|                | DC             | PA10 (Pin 7)             | Data/Command      |
|                | RST            | PA0 (Pin 11)             | Reset             |
|                | BL             | PA2 (Pin 13)             | Backlight (PWM)   |
| **Keyboard (I2C0)**| VCC        | 3.3V (Pin 17)            | Power             |
|                | GND            | GND (Pin 9)              | Ground            |
|                | SDA            | I2C0_SDA (Pin 3)         | I2C Data          |
|                | SCL            | I2C0_SCL (Pin 5)         | I2C Clock         |
| **Audio (I2S0)** | VIN          | 5V (Pin 2 or 4)          | Amplifier Power   |
|                | GND            | GND (Pin 14 or 20)       | Ground            |
|                | DIN            | I2S0_SDOUT (Pin 12)      | I2S Data Out      |
|                | BCLK           | I2S0_SCLK (Pin 35)*      | I2S Bit Clock     |
|                | LRC            | I2S0_LRCK (Pin 38)*      | I2S LR Clock      |
| **Fan**        | VCC (+)        | 5V (Pin 2 or 4)          | Power             |
|                | GND (-)        | GND (via transistor)     | Ground            |
|                | CTRL           | PA3 (Pin 15)             | Control Signal    |
| **NAGRA Port** | *In development* | *Free GPIOs*           | Custom Bus        |

*\*Note on I2S: Pins 35 and 38 are not on the 26-pin header. You must verify the correct I2S pin locations for your specific board and Armbian overlay.*

*(The Digispark-based mouse connects to a free USB port on the Orange Pi).*

## III. Software

### Software Stack:
*   **OS:** Armbian (Debian-based) for Orange Pi Zero 2W
*   **Display Driver:** Custom build of [juj/fbcp-ili9341](https://github.com/juj/fbcp-ili9341).
*   **Keyboard Driver:** Custom C++ daemon `cardkb_daemon` (emulation via uinput).
*   **Cooling System:** Custom C++ daemon `fan_control_daemon`.
*   **Audio Driver:** Standard Linux ALSA driver configured for I2S.
*   **Graphical Environment:**
    *   **Window Manager:** Openbox
    *   **Taskbar:** tint2
    *   **Fonts:** Terminus (bitmap font for maximum clarity)
    *   **Application Launcher:** dmenu

---

## IV. Installation and Setup
(This section remains the same, as the commands are universal)

## V. Openbox Desktop Configuration
(This section also remains the same)

## VI. Troubleshooting
(This section also remains the same)

## VII. Acknowledgements

*   Thanks to **Google** for the **Gemini** language model, which served as an AI assistant and helped with the development and debugging of many software solutions.
*   Thanks to the **Armbian** community for the excellent Linux build for single-board computers.
*   Thanks to **juj** for the superbly optimized `fbcp-ili9341` driver.






**
      .--.
     |o_o |
     |:_/ |
    //   \ \
   (|     | )
   /`\_   _/`\
   \___)=(___/
   
**
Artwork by (R)(C)Gemini 2.5 pro lol