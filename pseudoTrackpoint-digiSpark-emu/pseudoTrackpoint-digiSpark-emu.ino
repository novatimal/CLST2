/*
==========================================================================
 Скетч для кастомной мыши (джойстик) "pseudoTrackpoint"
 Версия 2.3: Уточненная инструкция по настройке Arduino IDE
 Проект: CLST2 Cyberdeck
 Платформа: DigiSpark (ATtiny85)
 Библиотека: EasyHID by AlexGyver
==========================================================================

ИНСТРУКЦИЯ ПО ПРОШИВКЕ И НАСТРОЙКЕ:

1. НАСТРОЙКА ARDUINO IDE ДЛЯ DIGISPARK:
   - Откройте "Файл" -> "Настройки".
   - В поле "Дополнительные ссылки для менеджера плат" вставьте следующую ссылку:
     http://digistump.com/package_digistump_index.json
   - Нажмите "ОК".

2. УСТАНОВКА ПАКЕТА ПЛАТ:
   - Откройте "Инструменты" -> "Плата" -> "Менеджер плат...".
   - В строке поиска введите "Digistump".
   - Найдите "Digistump AVR Boards" и нажмите кнопку "Установить".
   - Закройте Менеджер плат.

3. ВЫБОР ПЛАТЫ В МЕНЮ:
   - В меню "Инструменты" -> "Плата" теперь появится новый раздел.
   - Выберите "Digistump AVR Boards" -> "Digispark (Default - 16.5mhz)".

4. УСТАНОВКА БИБЛИОТЕКИ EasyHID:
   - Скачайте библиотеку с GitHub: https://github.com/AlexGyver/EasyHID (Code -> Download ZIP)
   - Установите ее в Arduino IDE через "Скетч" -> "Подключить библиотеку" -> "Добавить .ZIP библиотеку...".

5. НАСТРОЙКА ПОВОРОТА ДЖОЙСТИКА:
   - Ниже, в секции "КОНФИГУРАЦИЯ", раскомментируйте ОДНУ строку,
     соответствующую физической ориентации вашего джойстика.

6. ПРОШИВКА:
   - **НЕ ПОДКЛЮЧАЙТЕ DigiSpark к USB.**
   - Нажмите кнопку "Загрузка" (стрелка вправо) в Arduino IDE.
   - IDE скомпилирует скетч и напишет в нижней консоли:
     "Plug in device now... (will timeout in 60 seconds)"
   - **ТОЛЬКО ТЕПЕРЬ** вставьте DigiSpark в USB-порт.
   - Прошивка загрузится автоматически. После этого устройство сразу
     начнет работать как мышь.

==========================================================================
*/

#include <EasyHID.h>

// --- СЕКЦИЯ КОНФИГУРАЦИИ ---
// Определяем возможные ориентации
enum JoystickRotation {
  ROT_0,
  ROT_90,
  ROT_180,
  ROT_270
};

// ВЫБЕРИТЕ ОДНУ ОРИЕНТАЦИЮ ЗДЕСЬ
constexpr JoystickRotation CURRENT_ROTATION = ROT_0;
// ==========================================================


// --- НАСТРОЙКИ (без изменений) ---
const int X_PIN = 1;
const int Y_PIN = 2;
const int BUTTON_PIN = 1;
const int DEAD_ZONE = 50;
const int SENSITIVITY = 5;

// Создаем объект мыши
Mouse mouse;
bool lastButtonState = HIGH;

// --- ШАБЛОННАЯ ФУНКЦИЯ ДЛЯ ПОВОРОТА ---
// Это "общая" версия, которая никогда не будет вызвана,
// но нужна для синтаксиса.
template<JoystickRotation rot>
void applyRotation(int mapped_x, int mapped_y, int& dx, int& dy);

// А вот "специализации" для каждого конкретного значения.
// Компилятор выберет одну из них в зависимости от CURRENT_ROTATION.

template<> void applyRotation<ROT_0>(int mapped_x, int mapped_y, int& dx, int& dy) {
  dx = mapped_x;
  dy = mapped_y;
}

template<> void applyRotation<ROT_90>(int mapped_x, int mapped_y, int& dx, int& dy) {
  dx = -mapped_y;
  dy = mapped_x;
}

template<> void applyRotation<ROT_180>(int mapped_x, int mapped_y, int& dx, int& dy) {
  dx = -mapped_x;
  dy = -mapped_y;
}

template<> void applyRotation<ROT_270>(int mapped_x, int mapped_y, int& dx, int& dy) {
  dx = mapped_y;
  dy = -mapped_x;
}


void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  EasyHID.begin();
}

void loop() {
  // Читаем и преобразуем значения
  int mapped_x = map(analogRead(X_PIN), 0, 1023, -SENSITIVITY, SENSITIVITY);
  int mapped_y = map(analogRead(Y_PIN), 0, 1023, SENSITIVITY, -SENSITIVITY);

  int dx, dy;

  // --- ПРИМЕНЯЕМ ПОВОРОТ ---
  // Компилятор видит, что CURRENT_ROTATION - это константа,
  // и напрямую вставит сюда код ТОЛЬКО ОДНОЙ из applyRotation<...>
  applyRotation<CURRENT_ROTATION>(mapped_x, mapped_y, dx, dy);

  // --- Остальная часть loop() без изменений ---
  if (abs(dx) < DEAD_ZONE * SENSITIVITY / 100) dx = 0;
  if (abs(dy) < DEAD_ZONE * SENSITIVITY / 100) dy = 0;

  if (dx != 0 || dy != 0) {
    mouse.move(dx, dy);
  }

  bool currentButtonState = digitalRead(BUTTON_PIN);
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    mouse.press(MOUSE_LEFT);
  }
  else if (currentButtonState == HIGH && lastButtonState == LOW) {
    mouse.release(MOUSE_LEFT);
  }
  lastButtonState = currentButtonState;

  delay(10); 
}