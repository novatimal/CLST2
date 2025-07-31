#!/bin/bash

set -e

if [ "$EUID" -ne 0 ]; then
  echo "Ошибка: Пожалуйста, запустите этот скрипт с правами суперпользователя (sudo)."
  exit 1
fi

echo ">>> Начало удаления драйверов CLST2..."

# 1. Останавливаем и отключаем сервисы
echo "Остановка и отключение сервисов..."
systemctl stop cardkb.service || true # || true - чтобы не было ошибки, если сервис уже остановлен
systemctl disable cardkb.service || true

systemctl stop fbcp.service || true
systemctl disable fbcp.service || true

# 2. Удаляем файлы сервисов
echo "Удаление файлов сервисов..."
rm -f /etc/systemd/system/cardkb.service
systemctl daemon-reload

rm -f /etc/systemd/system/fbcp.service
systemctl daemon-reload

# 3. Удаляем исполняемые файлы
echo "Удаление исполняемых файлов..."
rm -f /usr/local/bin/cardkb_daemon

rm -f /usr/local/bin/fbcp_daemon

echo ">>> Удаление успешно завершено! <<<"

exit 0