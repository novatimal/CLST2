#!/bin/bash

# ==========================================================
# Скрипт для проигрывания ASCII-анимации при загрузке
# Проект: CLST2
# Версия: 1.1 (с взмахами и встряхиванием)
# ==========================================================

# Путь к директории с кадрами анимации
FRAME_DIR="/etc/boot-animation"

# Задержка для плавного взмаха крыльями (в секундах)
WAVE_DELAY=0.3

# Задержка для быстрого встряхивания (в секундах)
SHAKE_DELAY=0.08

# Проверяем, существует ли директория с кадрами, чтобы избежать ошибок
if [ ! -d "$FRAME_DIR" ]; then
    # Если директории нет, просто выходим.
    # Это делает скрипт более отказоустойчивым.
    exit 0
fi

# Бесконечный цикл, который будет прерван systemd,
# когда загрузка перейдет к экрану входа.
while true; do
    
    # --- ФАЗА 1: ВЗМАХ КРЫЛЬЯМИ ---
    # Проверяем наличие каждого файла перед использованием
    
    if [ -f "$FRAME_DIR/frame1" ]; then
        clear
        cat "$FRAME_DIR/frame1"
        sleep $WAVE_DELAY
    fi

    if [ -f "$FRAME_DIR/frame2" ]; then
        clear
        cat "$FRAME_DIR/frame2"
        sleep $WAVE_DELAY
    fi

    if [ -f "$FRAME_DIR/frame1" ]; then
        clear
        cat "$FRAME_DIR/frame1" # Возвращаемся в исходное положение
        sleep $WAVE_DELAY
    fi
    
    if [ -f "$FRAME_DIR/frame3" ]; then
        clear
        cat "$FRAME_DIR/frame3"
        sleep $WAVE_DELAY
    fi

    # --- ФАЗА 2: ВСТРЯХИВАНИЕ ---
    # Повторяем два раза для 4 кадров
    if [ -f "$FRAME_DIR/frame4" ] && [ -f "$FRAME_DIR/frame5" ]; then
        for i in 1 2; do
            clear
            cat "$FRAME_DIR/frame4" # Сдвиг влево-вверх
            sleep $SHAKE_DELAY

            clear
            cat "$FRAME_DIR/frame5" # Сдвиг вправо-вниз
            sleep $SHAKE_DELAY
        done
    fi
    
    # Возвращаемся в исходное положение перед новым циклом
    if [ -f "$FRAME_DIR/frame1" ]; then
        clear
        cat "$FRAME_DIR/frame1"
        sleep $WAVE_DELAY
    fi
    
done