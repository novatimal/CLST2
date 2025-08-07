/*
==========================================================================
 Скетч для "pseudoTrackpoint" - эмулятор мыши на джойстике
 Версия 2.2: Добавлена эмуляция правого клика по долгому нажатию ЛКМ.
 Проект: CLST2 Cyberdeck
 Платформа: DigiSpark (ATtiny85)
 Библиотека: EasyHID by AlexGyver
==========================================================================

ИНСТРУКЦИЯ ПО ПРОШИВКЕ И НАСТРОЙКЕ:

--- ШАГ 1: НАСТРОЙКА ARDUINO IDE ДЛЯ DIGISPARK ---
1. Добавьте в "Настройки" -> "Доп. ссылки...":
   https://raw.githubusercontent.com/ArminJo/DigistumpArduino/master/package_digistump_index.json
2. Установите "Digistump AVR Boards (by ArminJo)" через "Менеджер плат".

--- ШАГ 2: УСТАНОВКА БИБЛИОТЕКИ EasyHID ---
1. Скачайте ZIP-архив с GitHub: https://github.com/GyverLibs/EasyHID
2. Установите его через "Скетч" -> "Подключить библиотеку" -> "Добавить .ZIP..."

--- ШАГ 3: ВЫБОР ПЛАТЫ ---
1. "Инструменты" -> "Плата" -> "Digispark (Default - 16.5mhz)".

--- ШАГ 4: НАСТРОЙКА ПОВОРОТА ---
1. Ниже, в секции "КОНФИГУРАЦИЯ", раскомментируйте ОДНУ строку,
   соответствующую ориентации вашего джойстика.

--- ШАГ 5: ПРОШИВКА ---
1. НЕ ПОДКЛЮЧАЙТЕ DigiSpark.
2. Нажмите "Загрузка".
3. Когда появится сообщение "Plug in device now...", вставьте DigiSpark в USB.
==========================================================================
*/

#include <EasyHID.h> 

// ===================================================================
// --- СЕКЦИЯ КОНФИГУРАЦИИ ---
#define ROTATION_0_DEGREES
// #define ROTATION_90_DEGREES
// #define ROTATION_180_DEGREES
// #define ROTATION_270_DEGREES
// ===================================================================

// --- НАСТРОЙКИ ПИНОВ И ПАРАМЕТРОВ ---
const int X_PIN = 1;      // Аналоговый вход для оси X (P2 = Analog In 1)
const int Y_PIN = 2;      // Аналоговый вход для оси Y (P4 = Analog In 2)
const int BUTTON_PIN = 1; // Цифровой пин для кнопки (P1 = Digital In 1)

const int DEAD_ZONE_PERCENT = 10;
const int SENSITIVITY = 5;
constexpr int LONG_PRESS_TIME = 500; // 500 мс для долгого нажатия

// --- Глобальные переменные ---
bool lastButtonState = HIGH;
unsigned long pressStartTime = 0; // Время, когда кнопка была нажата
bool longPressTriggered = false;  // Флаг, что долгое нажатие уже сработало

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  HID.begin();
}

void loop() {
  // --- 1. Чтение и обработка джойстика ---
  int x_val = analogRead(X_PIN);
  int y_val = analogRead(Y_PIN);
  int mapped_x = map(x_val, 0, 1023, -SENSITIVITY, SENSITIVITY);
  int mapped_y = map(y_val, 0, 1023, SENSITIVITY, -SENSITIVITY);
  
  int dx, dy;

  #if defined(ROTATION_0_DEGREES)
    dx = mapped_x; dy = mapped_y;
  #elif defined(ROTATION_90_DEGREES)
    dx = -mapped_y; dy = mapped_x;
  #elif defined(ROTATION_180_DEGREES)
    dx = -mapped_x; dy = -mapped_y;
  #elif defined(ROTATION_270_DEGREES)
    dx = mapped_y; dy = -mapped_x;
  #else
    #error "Не выбрана ориентация джойстика!"
  #endif

  constexpr int DEAD_ZONE_THRESHOLD = (DEAD_ZONE_PERCENT * SENSITIVITY) / 100;
  if (abs(dx) <= DEAD_ZONE_THRESHOLD) dx = 0;
  if (abs(dy) <= DEAD_ZONE_THRESHOLD) dy = 0;
  
  Mouse.move(dx, dy);

  // --- 2. НОВАЯ ЛОГИКА ОБРАБОТКИ НАЖАТИЯ КНОПКИ ---
  bool currentButtonState = digitalRead(BUTTON_PIN);

  // --- Событие: Кнопка только что НАЖАТА ---
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    pressStartTime = millis();    // Засекаем время
    longPressTriggered = false;   // Сбрасываем флаг
  }
  
  // --- Событие: Кнопка УДЕРЖИВАЕТСЯ ---
  if (currentButtonState == LOW) {
    // Если кнопка удерживается дольше порога И мы еще не отправили правый клик
    if (millis() - pressStartTime > LONG_PRESS_TIME && !longPressTriggered) {
      Mouse.click(MOUSE_RIGHT); // Отправляем правый клик
      longPressTriggered = true;  // Ставим флаг, чтобы не отправлять его снова и снова
    }
  }

  // --- Событие: Кнопка только что ОТПУЩЕНА ---
  if (currentButtonState == HIGH && lastButtonState == LOW) {
    // Если это был короткий клик (правый клик не успел сработать)
    if (!longPressTriggered) {
      Mouse.click(MOUSE_LEFT); // Отправляем левый клик
    }
  }
  
  lastButtonState = currentButtonState;
  
  // Обязательный вызов для работы библиотеки
  HID.tick();
}