#!/bin/bash

# ==========================================================
# Установочный скрипт для драйверов проекта CLST2
# Версия 2.1 - Файлы сервисов systemd генерируются "на лету"
# ==========================================================

# Немедленно завершить работу при любой ошибке
set -e

# --- Проверки и глобальные переменные ---
if [ "$EUID" -ne 0 ]; then
  echo "Ошибка: Пожалуйста, запустите этот скрипт с правами суперпользователя (sudo)."
  exit 1
fi

echo ">>> Начало полной установки и настройки системы для CLST2..."

# Определяем директории
INSTALL_DIR="/usr/local/bin"
SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
ARMBIAN_ENV_FILE="/boot/armbianEnv.txt"

# --- Функция для включения оверлеев в /boot/armbianEnv.txt ---
enable_overlay() {
  local overlay_name="$1"
  echo "Проверка оверлея: $overlay_name..."
  if [ ! -f "$ARMBIAN_ENV_FILE" ]; then
    echo "Предупреждение: Файл $ARMBIAN_ENV_FILE не найден. Создаем его."
    touch "$ARMBIAN_ENV_FILE"
  fi
  if grep -q "^overlays=" "$ARMBIAN_ENV_FILE"; then
    if ! grep "^overlays=" "$ARMBIAN_ENV_FILE" | grep -q "\b${overlay_name}\b"; then
      sed -i "/^overlays=/ s/$/ ${overlay_name}/" "$ARMBIAN_ENV_FILE"
      echo "Оверлей $overlay_name добавлен."
    else
      echo "Оверлей $overlay_name уже был включен."
    fi
  else
    echo -e "\noverlays=${overlay_name}" >> "$ARMBIAN_ENV_FILE"
    echo "Оверлей $overlay_name добавлен."
  fi
}

# --- Шаг 1: Установка системных зависимостей ---
echo ">>> Шаг 1: Установка необходимых пакетов..."
apt-get update
apt-get install -y build-essential cmake libi2c-dev git

# --- Шаг 2: Включение аппаратных интерфейсов ---
echo ">>> Шаг 2: Включение аппаратных интерфейсов I2C, SPI и I2S..."
enable_overlay "i2c0"
enable_overlay "spi-spidev"
enable_overlay "i2s-sound"

# --- Шаг 3: Компиляция и установка драйверов ---

## -- 3.1 УСТАНОВКА CARDKB DAEMON --
echo ">>> Шаг 3.1: Компиляция драйвера клавиатуры (cardkb)..."
g++ -std=c++17 -O2 "$SOURCE_DIR/hardware_drv/cardkb/cardkb_daemon.cpp" -o "$INSTALL_DIR/cardkb_daemon"
chmod +x "$INSTALL_DIR/cardkb_daemon"
echo "Драйвер cardkb успешно установлен в $INSTALL_DIR/cardkb_daemon"

## -- 3.2 УСТАНОВКА FAN CONTROL DAEMON --
echo ">>> Шаг 3.2: Компиляция драйвера вентилятора (fan_control)..."
g++ -std=c++17 -O2 "$SOURCE_DIR/hardware_drv/fan/fan_control_daemon.cpp" -o "$INSTALL_DIR/fan_control_daemon" -lpthread
chmod +x "$INSTALL_DIR/fan_control_daemon"
echo "Драйвер вентилятора успешно установлен в $INSTALL_DIR/fan_control_daemon"

## -- 3.3 УСТАНОВКА FBCP --
echo ">>> Шаг 3.3: Загрузка и компиляция Framebuffer Copy (fbcp)..."
if [ -d "$SOURCE_DIR/hardware_drv/fbcp/fbcp-ili9341" ]; then
    echo "Исходники fbcp уже существуют. Пропускаем загрузку."
else
    git clone https://github.com/juj/fbcp-ili9341.git "$SOURCE_DIR/hardware_drv/fbcp/fbcp-ili9341"
fi
cd "$SOURCE_DIR/hardware_drv/fbcp/fbcp-ili9341"
mkdir -p build
cd build
cmake -DST7789V=ON \
      -DGPIO_TFT_DATA_CONTROL=10 \
      -DGPIO_TFT_RESET_PIN=0 \
      -DGPIO_TFT_BACKLIGHT=2 \
      -DSPI_BUS_CLOCK_DIVISOR=6 \
      -DBACKLIGHT_CONTROL=ON \
      -DROTATE_DISPLAY=90 \
      ..
make -j$(nproc)
cp fbcp-ili9341 "$INSTALL_DIR/fbcp_daemon"
chmod +x "$INSTALL_DIR/fbcp_daemon"
echo "Драйвер fbcp успешно установлен в $INSTALL_DIR/fbcp_daemon"
cd "$SOURCE_DIR"

# --- Шаг 4: Настройка системных сервисов (systemd) ---
echo ">>> Шаг 4: Создание и включение системных сервисов..."

# Сервис для cardkb_daemon
echo "Создание cardkb.service..."
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

# Сервис для fan_control_daemon
echo "Создание fan_control.service..."
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

# Сервис для fbcp_daemon
echo "Создание fbcp.service..."
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

# Перезагружаем конфигурацию systemd и включаем сервисы
systemctl daemon-reload
systemctl enable cardkb.service
systemctl enable fan_control.service
systemctl enable fbcp.service

echo "Сервисы успешно настроены для автозапуска."

# --- Шаг 5: Завершение ---
echo ""
echo ">>> Установка успешно завершена! <<<"
echo ""
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!!! ВНИМАНИЕ: Для активации аппаратных интерфейсов I2C, SPI  !!!"
echo "!!! и I2S требуется полная перезагрузка системы.             !!!"
echo "!!!    Пожалуйста, выполните команду 'sudo reboot' сейчас.   !!!"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"

exit 0