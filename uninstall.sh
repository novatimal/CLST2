#!/bin/bash

# ==========================================================
# Скрипт УДАЛЕНИЯ для проекта CLST2
# Версия 1.0 - Мультиязычный интерфейс (EN/RU)
# ==========================================================

set -e

# --- Языковой пакет ---
LANG_PACK="RU"

_print() {
    if [ "$LANG_PACK" = "EN" ]; then echo -e "$1"; else echo -e "$2"; fi
}

clear
echo "========================================="
echo " CLST2 Project Uninstaller / Деинсталлятор"
echo "========================================="
echo
_print "This script will remove all custom drivers and services for the CLST2 project." "Этот скрипт удалит все кастомные драйверы и сервисы проекта CLST2."
_print "System packages (like cmake, git) will NOT be removed." "Системные пакеты (cmake, git) НЕ будут удалены."
echo

read -p "Are you sure you want to continue? (y/N) / Вы уверены, что хотите продолжить? (y/N): " confirm
if [[ ! "$confirm" =~ ^[Yy]$ ]]; then
    _print "Aborted." "Отменено."
    exit 1
fi

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

_print "\n>>> Starting uninstallation process for CLST2..." "\n>>> Начало процесса удаления CLST2..."

INSTALL_DIR="/usr/local/bin"

# --- Шаг 1: Остановка и отключение системных сервисов ---
_print "\n>>> Step 1: Stopping and disabling system services..." "\n>>> Шаг 1: Остановка и отключение системных сервисов..."

_print "Disabling cardkb.service..." "Отключение cardkb.service..."
systemctl disable --now cardkb.service || true
_print "Disabling fan_control.service..." "Отключение fan_control.service..."
systemctl disable --now fan_control.service || true
_print "Disabling fbcp.service..." "Отключение fbcp.service..."
systemctl disable --now fbcp.service || true

# --- Шаг 2: Удаление файлов сервисов ---
_print "\n>>> Step 2: Removing systemd service files..." "\n>>> Шаг 2: Удаление файлов сервисов..."

rm -f /etc/systemd/system/cardkb.service
rm -f /etc/systemd/system/fan_control.service
rm -f /etc/systemd/system/fbcp.service

systemctl daemon-reload
_print "Service files removed." "Файлы сервисов удалены."

# --- Шаг 3: Удаление скомпилированных драйверов ---
_print "\n>>> Step 3: Removing compiled driver binaries..." "\n>>> Шаг 3: Удаление скомпилированных драйверов..."

rm -f "$INSTALL_DIR/cardkb_daemon"
rm -f "$INSTALL_DIR/fan_control_daemon"
rm -f "$INSTALL_DIR/fbcp_daemon"

_print "Driver binaries removed from $INSTALL_DIR" "Бинарные файлы драйверов удалены из $INSTALL_DIR"

# --- Шаг 4: Завершение ---
_print "\n>>> Uninstallation successful! <<<" "\n>>> Удаление успешно завершено! <<<"
_print "Note: Configuration changes in /boot/armbianEnv.txt have NOT been reverted." "Примечание: Изменения в файле /boot/armbianEnv.txt НЕ были отменены."
_print "You may need to manually remove overlays and kernel parameters if you wish." "При желании вы можете вручную удалить оверлеи и параметры ядра."
echo

exit 0