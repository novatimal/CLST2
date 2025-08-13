// ==========================================================
// Название:      fan_control_daemon.cpp
// Версия:        1.0
// Описание:      Демон для управления вентилятором проекта CLST2.
//                Читает температуру CPU и управляет GPIO-пином.
//==========================================================

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <unistd.h>
#include <sys/stat.h>

// --- НАСТРОЙКИ ---
const int FAN_GPIO_PIN = 3; // PA3 -> GPIO 3
const std::string TEMP_FILE_PATH = "/sys/class/thermal/thermal_zone0/temp";
const float TEMP_OFF = 45.0f;
const float TEMP_ON = 50.0f;

// --- Класс для простого управления GPIO через sysfs ---
class GPIOManager {
private:
    int pin_number;
    std::string gpio_path;

    bool write_to_sysfs(const std::string& path, const std::string& value) {
        std::ofstream fs(path);
        if (!fs.is_open()) {
            std::cerr << "Ошибка: Не удалось открыть sysfs-файл: " << path << std::endl;
            return false;
        }
        fs << value;
        fs.close();
        return !fs.fail();
    }

public:
    GPIOManager(int pin) : pin_number(pin) {
        gpio_path = "/sys/class/gpio/gpio" + std::to_string(pin_number);
    }

    bool setup() {
        struct stat buffer;
        if (stat(gpio_path.c_str(), &buffer) != 0) {
            std::cout << "Экспортируем GPIO " << pin_number << "..." << std::endl;
            if (!write_to_sysfs("/sys/class/gpio/export", std::to_string(pin_number))) return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        std::cout << "Устанавливаем направление 'out' для GPIO " << pin_number << "..." << std::endl;
        return write_to_sysfs(gpio_path + "/direction", "out");
    }

    void set_value(int value) {
        write_to_sysfs(gpio_path + "/value", std::to_string(value));
    }
};

float get_cpu_temp() {
    std::ifstream temp_file(TEMP_FILE_PATH);
    if (!temp_file.is_open()) {
        return 50.0f;
    }
    int temp_milli_celsius;
    temp_file >> temp_milli_celsius;
    return static_cast<float>(temp_milli_celsius) / 1000.0f;
}

int main() {
    std::cout << "Запуск демона управления вентилятором..." << std::endl;

    GPIOManager fan_gpio(FAN_GPIO_PIN);
    if (!fan_gpio.setup()) {
        std::cerr << "Критическая ошибка: Не удалось настроить GPIO. Запустите с sudo." << std::endl;
        return 1;
    }

    std::cout << "Демон запущен. Управление GPIO " << FAN_GPIO_PIN << " активно." << std::endl;
    
    bool is_fan_on = false;

    while (true) {
        float temp = get_cpu_temp();

        if (temp >= TEMP_ON && !is_fan_on) {
            std::cout << "Температура: " << temp << "°C. Включаем вентилятор." << std::endl;
            fan_gpio.set_value(1);
            is_fan_on = true;
        } else if (temp < TEMP_OFF && is_fan_on) {
            std::cout << "Температура: " << temp << "°C. Выключаем вентилятор." << std::endl;
            fan_gpio.set_value(0);
            is_fan_on = false;
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    return 0;
}