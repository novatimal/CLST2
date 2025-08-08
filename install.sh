#!/bin/bash

# ==========================================================
# Установочный скрипт для драйверов проекта CLST2
# Версия 3.0 - Мультиязычный интерфейс (EN/RU)
# ==========================================================

set -e

# --- Языковой пакет ---
LANG_PACK="RU"

_print() {
    if [ "$LANG_PACK" = "EN" ]; then echo -e "$1"; else echo -e "$2"; fi
}

clear
echo "========================================="
echo " CLST2 Project Installer / Установщик"
echo "========================================="
echo
echo "1. English"
echo "2. Русский"
echo

read -p "Select language / Выберите язык (1/2): " lang_choice
case $lang_choice in
    1) LANG_PACK="EN";;
    *) LANG_PACK="RU";;
esac

# --- Проверки и глобальные переменные ---
if [ "$EUID" -ne 0 ]; then
  _print "\nError: Please run this script with superuser privileges (sudo)." "\nОшибка: Пожалуйста, запустите этот скрипт с правами суперпользователя (sudo)."
  exit 1
fi

_print "\n>>> Starting full system installation and setup for CLST2..." "\n>>> Начало полной установки и настройки системы для CLST2..."

INSTALL_DIR="/usr/local/bin"
SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
ARMBIAN_ENV_FILE="/boot/armbianEnv.txt"

# --- Функции-помощники ---
enable_overlay() {
  local overlay_name="$1"
  _print "Checking overlay: $overlay_name..." "Проверка оверлея: $overlay_name..."
  if [ ! -f "$ARMBIAN_ENV_FILE" ]; then touch "$ARMBIAN_ENV_FILE"; fi
  if grep -q "^overlays=" "$ARMBIAN_ENV_FILE"; then
    if ! grep "^overlays=" "$ARMBIAN_ENV_FILE" | grep -q "\b${overlay_name}\b"; then
      sed -i "/^overlays=/ s/$/ ${overlay_name}/" "$ARMBIAN_ENV_FILE"
      _print "Overlay $overlay_name has been added." "Оверлей $overlay_name добавлен."
    else
      _print "Overlay $overlay_name was already enabled." "Оверлей $overlay_name уже был включен."
    fi
  else
    echo -e "\noverlays=${overlay_name}" >> "$ARMBIAN_ENV_FILE"
    _print "Overlay $overlay_name has been added." "Оверлей $overlay_name добавлен."
  fi
}

set_extra_args() {
  local new_arg="$1"
  _print "Checking kernel parameter: $new_arg..." "Проверка параметра ядра: $new_arg..."
  if [ ! -f "$ARMBIAN_ENV_FILE" ]; then touch "$ARMBIAN_ENV_FILE"; fi
  if grep -q "^extraargs=" "$ARMBIAN_ENV_FILE"; then
    if ! grep "^extraargs=" "$ARMBIAN_ENV_FILE" | grep -q "${new_arg}"; then
      sed -i "/^extraargs=/ s/$/ ${new_arg}/" "$ARMBIAN_ENV_FILE"
      _print "Parameter '${new_arg}' added." "Параметр '${new_arg}' добавлен."
    else
      _print "Parameter '${new_arg}' was already set." "Параметр '${new_arg}' уже был установлен."
    fi
  else
    echo -e "\nextraargs=${new_arg}" >> "$ARMBIAN_ENV_FILE"
    _print "Parameter '${new_arg}' added." "Параметр '${new_arg}' добавлен."
  fi
}

set_disp_mode() {
  local mode="$1"
  _print "Setting display mode: $mode..." "Установка режима дисплея: $mode..."
  if [ ! -f "$ARMBIAN_ENV_FILE" ]; then touch "$ARMBIAN_ENV_FILE"; fi
  if grep -q "^disp_mode=" "$ARMBIAN_ENV_FILE"; then
    sed -i "s/^disp_mode=.*/disp_mode=${mode}/" "$ARMBIAN_ENV_FILE"
  else
    echo "disp_mode=${mode}" >> "$ARMBIAN_ENV_FILE"
  fi
  _print "Display mode set to '${mode}'." "Режим дисплея установлен на '${mode}'."
}

# --- Шаг 1: Установка системных зависимостей ---
_print "\n>>> Step 1: Installing required packages..." "\n>>> Шаг 1: Установка необходимых пакетов..."
apt-get update
apt-get install -y build-essential cmake libi2c-dev git

# --- Шаг 2: Настройка загрузчика и аппаратных интерфейсов ---
_print "\n>>> Step 2: Configuring bootloader and hardware interfaces..." "\n>>> Шаг 2: Настройка параметров загрузки и аппаратных интерфейсов..."
set_disp_mode "320x240p60"
set_extra_args "video=HDMI-A-1:320x240@60"
enable_overlay "i2c0"
enable_overlay "spi-spidev"
enable_overlay "i2s-sound"

# --- Шаг 3: Компиляция и установка драйверов ---

## -- 3.1 УСТАНОВКА CARDKB DAEMON --
_print "\n>>> Step 3.1: Compiling keyboard driver (cardkb)..." "\n>>> Шаг 3.1: Компиляция драйвера клавиатуры (cardkb)..."
g++ -std=c++17 -O2 "$SOURCE_DIR/hardware_drv/cardkb/cardkb_daemon.cpp" -o "$INSTALL_DIR/cardkb_daemon" -lpthread
chmod +x "$INSTALL_DIR/cardkb_daemon"
_print "cardkb driver installed successfully." "Драйвер cardkb успешно установлен."

## -- 3.2 УСТАНОВКА FAN CONTROL DAEMON --
_print "\n>>> Step 3.2: Compiling fan control driver..." "\n>>> Шаг 3.2: Компиляция драйвера вентилятора..."
g++ -std=c++17 -O2 "$SOURCE_DIR/hardware_drv/fan/fan_control_daemon.cpp" -o "$INSTALL_DIR/fan_control_daemon" -lpthread
chmod +x "$INSTALL_DIR/fan_control_daemon"
_print "Fan control driver installed successfully." "Драйвер вентилятора успешно установлен."

## -- 3.3 УСТАНОВКА FBCP --
_print "\n>>> Step 3.3: Compiling Framebuffer Copy (fbcp) from local sources..." "\n>>> Шаг 3.3: Компиляция Framebuffer Copy (fbcp) из локальных исходников..."
cd "$SOURCE_DIR/hardware_drv/fbcp/fbcp-ili9341-master"
mkdir -p build && cd build
cmake -DST7789V=ON -DGPIO_TFT_DATA_CONTROL=10 -DGPIO_TTFT_RESET_PIN=0 -DGPIO_TFT_BACKLIGHT=2 -DSPI_BUS_CLOCK_DIVISOR=6 -DBACKLIGHT_CONTROL=ON -DROTATE_DISPLAY=90 ..
make -j$(nproc)
cp fbcp-ili9341 "$INSTALL_DIR/fbcp_daemon"
chmod +x "$INSTALL_DIR/fbcp_daemon"
_print "fbcp driver installed successfully." "Драйвер fbcp успешно установлен."
cd "$SOURCE_DIR"

# --- Шаг 4: Настройка системных сервисов (systemd) ---
_print "\n>>> Step 4: Creating and enabling system services..." "\n>>> Шаг 4: Создание и включение системных сервисов..."

_print "Creating cardkb.service..." "Создание cardkb.service..."
cat > /etc/systemd/system/cardkb.service << EOL
[Unit]
Description=CardKB I2C Keyboard Daemon for CLST2
After=multi-user.target
[Service]
Type=simple
ExecStart=${INSTALL_DIR}/cardkb_daemon -i /dev/i2c-0
Restart=always
RestartSec=5
[Install]
WantedBy=multi-user.target
EOL

_print "Creating fan_control.service..." "Создание fan_control.service..."
cat > /etc/systemd/system/fan_control.service << EOL
[Unit]
Description=Fan Control Daemon for CLST2
After=multi-user.target
[Service]
Type=simple
ExecStart=${INSTALL_DIR}/fan_control_daemon
Restart=always
RestartSec=10
[Install]
WantedBy=multi-user.target
EOL

_print "Creating fbcp.service..." "Создание fbcp.service..."
cat > /etc/systemd/system/fbcp.service << EOL
[Unit]
Description=Framebuffer Copy Daemon for CLST2
After=graphical.target
[Service]
Type=simple
ExecStart=${INSTALL_DIR}/fbcp_daemon
Restart=always
RestartSec=5
[Install]
WantedBy=graphical.target
EOL

systemctl daemon-reload
systemctl enable cardkb.service
systemctl enable fan_control.service
systemctl enable fbcp.service

_print "Services have been configured for auto-start." "Сервисы успешно настроены для автозапуска."

# --- Шаг 5: Завершение ---
_print "\n>>> Installation successful! <<<" "\n>>> Установка успешно завершена! <<<"
_print "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
_print "!!! ATTENTION: A system reboot is required to activate       !!!" "!!! ВНИМАНИЕ: Для активации аппаратных интерфейсов I2C, SPI  !!!"
_print "!!! the new hardware interfaces (I2C, SPI, I2S).             !!!" "!!! и I2S требуется полная перезагрузка системы.             !!!"
_print "!!! Please run 'sudo reboot' now.                            !!!" "!!!    Пожалуйста, выполните команду 'sudo reboot' сейчас.   !!!"
_print "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"

exit 0