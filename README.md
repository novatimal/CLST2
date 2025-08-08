[English](README.md) | [Русский](README.ru.md)

# Project CLST2: Building a Cyberdeck based on Orange Pi Zero 2W

[![Status](https://img.shields.io/badge/status-early%20beta-orange)](https://shields.io)

> **Note:** 99% of this project (including almost all the names like "cipher-slot") was created by Google's Gemini language model, this project is in early beta. Expect bugs, missing features, and frequent changes. Contributions and feedback are welcome!

This is the repository for a project to build a retro-style handheld computer (PDA/Cyberdeck). The device is based on the Orange Pi Zero 2W single-board computer and uses custom drivers and a finely-tuned Linux environment for a comfortable user experience on a small screen.

**This README contains detailed instructions for assembly, software installation, and solutions to problems encountered during development.**

<p align="center">
  <img src="images/openBench.jpg" alt="CLST2 open bench setup" width="600">
  <br>
  <em>Fig. 1 - Photo of my open bench setup.</em>
</p>

## I. Concept and Philosophy

*   **Goal:** To create a fully functional, pocket-sized Linux computer with a physical keyboard for development, system administration, and retro gaming, featuring a modular expansion system.
*   **Inspiration:** Classic 90s PDAs (Psion, Sharp Zaurus), the Toshiba T1100 laptop, and cyberpunk aesthetics.
*   **Interface:** A full-featured, customizable desktop environment adapted for a 320x240 screen, without artificial limitations or shells.
## II. Hardware Components

### Core Components:
*   **Processing Module:** Orange Pi Zero 2W
*   **Display:** 2.4" SPI 320x240 LCD (ST7789V controller)
*   **Keyboard:** M5Stack CardKB (I2C connection)
*   **Input Device (Mouse):** Analog joystick connected via Digispark ATTiny85 (emulates a USB mouse), dubbed the **"pseudoTrackpoint"**.
*   **Expansion Port:** The **"Cipher-Slot"**, a core component of the custom **Multi-Cartridge System (MCS)**, built upon a card reader from a NAGRA Syster unit.
*   **Audio System:** **MAX98357A** I2S amplifier with an external speaker.
*   **Cooling System:** **4010 5V** Blower Fan with copper heatsinks.
*   **Power Supply:** A single-cell Li-Pol battery with a **TP4056** charging and protection board.
*   **Case:** Custom 3D-printed enclosure (design in progress).

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
| **Cipher-Slot**| *In development* | *Free GPIOs*           | Custom Bus        |

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

Follow these steps to set up the system from scratch.

### Step 1: Prepare the SD Card and OS

1.  Flash a fresh Armbian image (Debian-based) for the Orange Pi Zero 2W to a microSD card.
2.  Perform the initial system setup: create a user, connect to Wi-Fi, etc.

### Step 2: Configure Video Output (Critical Step!)

For the `fbcp` driver to work correctly, the Linux kernel must render the desktop to a "virtual" low-resolution monitor.

1.  Open the `/boot/armbianEnv.txt` file for editing:
    ```bash
    sudo nano /boot/armbianEnv.txt
    ```
2.  Add or modify the `extraargs` line to include the following parameter:
    ```
    extraargs=video=HDMI-A-1:320x240@60
    ```
3.  Save the file (`Ctrl+O`, `Enter`) and **reboot the device immediately**: `sudo reboot`.

### Step 3: Flash the pseudoTrackpoint Firmware

The mouse device is based on a custom-programmed DigiSpark board. You need to compile and upload the firmware to it.

1.  Navigate to the `hardware_drv/pseudoTrackpoint-digiSpark-emu/` directory in the project.
2.  Open the `pseudoTrackpoint-digiSpark-emu.ino` file in the Arduino IDE.
3.  **Follow the detailed instructions inside the sketch file (`.ino`)** to set up the Arduino IDE, configure the joystick orientation, and flash the firmware.
### Step 4: Run the Automatic Installer

The `install.sh` script in this repository will perform all necessary actions: install dependencies, enable hardware interfaces, compile drivers, and set them up to run automatically.

1.  Clone the repository:
    ```bash
    git clone https://github.com/your_username/CLST2.git
    cd CLST2
    ```
2.  Make the installation script executable:
    ```bash
    chmod +x install.sh
    ```
3.  Run the script with superuser privileges:
    ```bash
    sudo ./install.sh
    ```
4.  The script will automatically enable I2C, SPI, and I2S overlays, compile and install `cardkb_daemon`, `fan_control_daemon`, and `fbcp`, and configure their respective `systemd` services.

### Step 5: Final Reboot

After the script finishes, it will prompt you to reboot the device. This is necessary to activate the hardware interfaces (I2C, SPI, I2S).
```bash
sudo reboot
```
---

## V. Openbox Desktop Configuration

After the first login, you will see a basic XDM login screen. Log in to your Openbox session to get a minimal desktop. Let's make it more comfortable and functional.

1.  **Install Essential Desktop Components:**
    First, let's install a better terminal, a file manager, a wallpaper manager (`feh`), and the network manager applet.
    ```bash
    sudo apt install -y sakura pcmanfm feh network-manager-gnome
    ```
    *   `sakura`: A lightweight and pleasant terminal emulator.
    *   `pcmanfm`: A fast and simple file manager.
    *   `feh`: A tool to set the desktop wallpaper.
    *   `network-manager-gnome`: Provides the `nm-applet` for a Wi-Fi icon in the system tray.

2.  **Copy Default Configurations:**
    To start customizing, copy the default Openbox configuration files to your home directory.
    ```bash
    mkdir -p ~/.config/openbox
    cp /etc/xdg/openbox/* ~/.config/openbox/
    ```

3.  **Configure Autostart:**
    We need to tell Openbox what programs to launch at startup.
    ```bash
    # Open the autostart file with nano
    nano ~/.config/openbox/autostart
    ```
    Add the following lines to the file:
    ```bash
    # Set a black background color as a fallback
    xsetroot -solid black &

    # (Optional) Set your wallpaper. Create a Pictures folder and place your image there.
    # feh --bg-scale ~/Pictures/wallpaper.png &

    # Launch the Network Manager applet for the Wi-Fi icon
    nm-applet &

    # Launch the tint2 panel
    tint2 &
    ```
    Save the file (`Ctrl+O`, `Enter`) and exit (`Ctrl+X`). Now, make it executable:
    ```bash
    chmod +x ~/.config/openbox/autostart
    ```

4.  **Configure tint2 for a System Tray:**
    We need to enable the notification area (system tray) in our `tint2` panel.
    ```bash
    # Open the tint2 configuration file
    nano ~/.config/tint2/tint2rc
    ```
    Find the line that starts with `panel_items = ...` and add the letter **S** to it (which stands for System Tray). It's usually best placed before the clock (`C`).
    
    *   Example: Change `panel_items = LTSBC` to `panel_items = LTS**S**BC`

5.  **Re-login or Reboot:**
    To apply all changes, either log out and log back into your Openbox session, or simply reboot the device. You should now see a functional desktop with a wallpaper and a Wi-Fi icon on your `tint2` panel!


<p align="center">
  <img src="images/desktop.jpg" alt="Openbox desktop with tint2" width="600">
  <br>
  <em>Fig. 2 - A screenshot of the custom Openbox environment.</em>
</p>

<p align="center">
  <img src="images/login.jpg" alt="XDM login window" width="600">
  <br>
  <em>Fig. 3 - A screenshot of XDM login window.</em>
</p>

---
## VI. Troubleshooting

This section describes problems encountered during development and their solutions.

### Issue: Cannot change screen resolution in VirtualBox

*   **Symptom:** Attempting to change the resolution with `xrandr` results in a `BadMatch (invalid parameter attributes)` error.
*   **Cause:** The guest OS video driver in VirtualBox cannot correctly handle the custom video mode.
*   **Solution:**
    1.  **Install Guest Additions.** This is the most reliable method.
    2.  **Use `VBoxManage`.** Turn off the VM and run `VBoxManage setextradata "VM Name" "CustomVideoMode1" "320x240x16"` on your host machine.
    3.  **Manual Resize.** After installing Guest Additions, you can often just manually resize the VM window to a small size, and the internal resolution will adjust automatically.

### Issue: `tint2` does not autostart with Openbox

*   **Symptom:** The `tint2` panel does not appear after logging into Openbox, but it starts manually from the terminal.
*   **Cause:** The `~/.config/openbox/autostart` file is not executable or is missing a shebang (`#!/bin/bash`).
*   **Solution:** Ensure the file is executable (`chmod +x ...`) and starts with the shebang. A full reboot often resolves session-related issues.

---
## VII. Acknowledgements

*   Thanks to **Google** for the **Gemini** language model, which served as an AI assistant and helped with the development and debugging of many software solutions.
*   Thanks to the **Armbian** community for the excellent Linux build for single-board computers.
*   Thanks to **juj** for the superbly optimized `fbcp-ili9341` driver.
*   Thanks to **AlexGyver** for the simple and powerful **EasyHID** library for Arduino.

**
      .--.
     |o_o |
     |:_/ |
    //   \ \
   (|     | )
   /`\_   _/`\
   \___)=(___/
   
**
Artwork by (R)(C)Gemini 2.5 pro