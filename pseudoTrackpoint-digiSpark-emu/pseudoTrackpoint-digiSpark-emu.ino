/*
==========================================================================
 Sketch for "pseudoTrackpoint" - a joystick-based mouse emulator
 Version 2.2: Added Right-Click emulation via long press.
 Project: CLST2 Cyberdeck
 Platform: DigiSpark (ATtiny85)
 Library: EasyHID by AlexGyver
 
 ---
 
 Скетч для "pseudoTrackpoint" - эмулятор мыши на джойстике
 Версия 2.2: Добавлена эмуляция ПКМ по долгому нажатию ЛКМ.
 Проект: CLST2 Cyberdeck
 Платформа: DigiSpark (ATtiny85)
 Библиотека: EasyHID by AlexGyver
==========================================================================

INSTRUCTIONS / ИНСТРУКЦИЯ ПО ПРОШИВКЕ:

--- STEP 1: CONFIGURE ARDUINO IDE FOR DIGISPARK ---
--- ШАГ 1: НАСТРОЙКА ARDUINO IDE ДЛЯ DIGISPARK ---
1. Go to "File" -> "Preferences". / Откройте "Файл" -> "Настройки".
2. In "Additional Board Manager URLs", paste:
   https://raw.githubusercontent.com/ArminJo/DigistumpArduino/master/package_digistump_index.json
3. Install "Digistump AVR Boards (by ArminJo)" via "Tools" -> "Board" -> "Boards Manager...".
   Установите "Digistump AVR Boards (by ArminJo)" через "Инструменты" -> "Плата" -> "Менеджер плат...".

--- STEP 2: INSTALL EasyHID LIBRARY ---
--- ШАГ 2: УСТАНОВКА БИБЛИОТЕКИ EasyHID ---
1. Download ZIP from GitHub: https://github.com/GyverLibs/EasyHID
2. Install it via "Sketch" -> "Include Library" -> "Add .ZIP Library...".
   Установите его через "Скетч" -> "Подключить библиотеку" -> "Добавить .ZIP..."

--- STEP 3: SELECT BOARD ---
--- ШАГ 3: ВЫБОР ПЛАТЫ ---
1. "Tools" -> "Board" -> "Digispark (Default - 16.5mhz)".

--- STEP 4: CONFIGURE ROTATION ---
--- ШАГ 4: НАСТРОЙКА ПОВОРОТА ---
1. Below, in the CONFIGURATION section, uncomment ONLY ONE line
   that matches your joystick's physical orientation.
   Ниже, в секции КОНФИГУРАЦИЯ, раскомментируйте ОДНУ строку,
   соответствующую ориентации вашего джойстика.

--- STEP 5: FLASHING ---
--- ШАГ 5: ПРОШИВКА ---
1. DO NOT connect the DigiSpark to USB yet. / НЕ ПОДКЛЮЧАЙТЕ DigiSpark.
2. Press the "Upload" button. / Нажмите "Загрузка".
3. Wait for the "Plug in device now..." message in the console.
   Дождитесь сообщения "Plug in device now..." в консоли.
4. ONLY NOW, plug the DigiSpark into a USB port.
   ТОЛЬКО ТЕПЕРЬ вставьте DigiSpark в USB-порт.
==========================================================================
*/

#include <EasyHID.h> 

// ===================================================================
// --- CONFIGURATION / КОНФИГУРАЦИЯ ---
#define ROTATION_0_DEGREES    // Default position / Стандартное положение
// #define ROTATION_90_DEGREES   // Rotated 90° clockwise / Повернут на 90° по часовой
// #define ROTATION_180_DEGREES  // Flipped upside down / Перевернут
// #define ROTATION_270_DEGREES  // Rotated 90° counter-clockwise / Повернут на 90° против часовой
// ===================================================================

// --- PIN & PARAMETER SETTINGS / НАСТРОЙКИ ПИНОВ И ПАРАМЕТРОВ ---
const int X_PIN = 1;      // Analog input for X-axis / Аналоговый вход для оси X (P2 = A1)
const int Y_PIN = 2;      // Analog input for Y-axis / Аналоговый вход для оси Y (P4 = A2)
const int BUTTON_PIN = 1; // Digital pin for the button / Цифровой пин для кнопки (P1)

const int DEAD_ZONE_PERCENT = 10;
const int SENSITIVITY = 5;
constexpr int LONG_PRESS_TIME = 500; // 500 ms for a long press / 500 мс для долгого нажатия

// --- GLOBAL VARIABLES / ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ---
bool lastButtonState = HIGH;
unsigned long pressStartTime = 0; // Time when the button was pressed / Время нажатия кнопки
bool longPressTriggered = false;  // Flag to indicate that a long press has been triggered / Флаг, что долгое нажатие сработало

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  HID.begin();
}

void loop() {
  // --- 1. Read and process joystick input / Чтение и обработка джойстика ---
  int x_val = analogRead(X_PIN);
  int y_val = analogRead(Y_PIN);
  int mapped_x = map(x_val, 0, 1023, -SENSITIVITY, SENSITIVITY);
  int mapped_y = map(y_val, 0, 1023, SENSITIVITY, -SENSITIVITY);
  
  int dx, dy;

  // Apply rotation / Применение поворота
  #if defined(ROTATION_0_DEGREES)
    dx = mapped_x; dy = mapped_y;
  #elif defined(ROTATION_90_DEGREES)
    dx = -mapped_y; dy = mapped_x;
  #elif defined(ROTATION_180_DEGREES)
    dx = -mapped_x; dy = -mapped_y;
  #elif defined(ROTATION_270_DEGREES)
    dx = mapped_y; dy = -mapped_x;
  #else
    #error "Joystick rotation is not selected! Uncomment one of the ROTATION_..._DEGREES lines."
  #endif

  constexpr int DEAD_ZONE_THRESHOLD = (DEAD_ZONE_PERCENT * SENSITIVITY) / 100;
  if (abs(dx) <= DEAD_ZONE_THRESHOLD) dx = 0;
  if (abs(dy) <= DEAD_ZONE_THRESHOLD) dy = 0;
  
  Mouse.move(dx, dy);

  // --- 2. Button press logic with long press for Right-Click / Логика кнопки с долгим нажатием для ПКМ ---
  bool currentButtonState = digitalRead(BUTTON_PIN);

  // Event: Button just PRESSED / Событие: Кнопка только что НАЖАТА
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    pressStartTime = millis();    // Start the timer / Засекаем время
    longPressTriggered = false;   // Reset the flag / Сбрасываем флаг
  }
  
  // Event: Button is HELD DOWN / Событие: Кнопка УДЕРЖИВАЕТСЯ
  if (currentButtonState == LOW) {
    // If held longer than the threshold AND the right-click hasn't been sent yet
    // Если удерживается дольше порога И мы еще не отправили правый клик
    if (millis() - pressStartTime > LONG_PRESS_TIME && !longPressTriggered) {
      Mouse.click(MOUSE_RIGHT); // Send a right-click / Отправляем правый клик
      longPressTriggered = true;  // Set the flag to prevent repeats / Ставим флаг, чтобы не повторять
    }
  }

  // Event: Button just RELEASED / Событие: Кнопка только что ОТПУЩЕНА
  if (currentButtonState == HIGH && lastButtonState == LOW) {
    // If it was a short press (the long press was not triggered)
    // Если это был короткий клик (правый клик не успел сработать)
    if (!longPressTriggered) {
      Mouse.click(MOUSE_LEFT); // Send a left-click / Отправляем левый клик
    }
  }
  
  lastButtonState = currentButtonState;
  
  // Mandatory call for the library to work / Обязательный вызов для работы библиотеки
  HID.tick();
}