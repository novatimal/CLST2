// ============================================================================
// Название:      cardkb_daemon.cpp
// Версия:        4.0 ("Умные" зажатые модификаторы и интеллектуальный Shift)
// Описание:      Демон для M5Stack CardKB с расширенным функционалом.
// ============================================================================

#include <iostream>
#include <map>
#include <set>
#include <functional>
#include <cstdint>
#include <string>
#include <cctype>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include <linux/i2c-dev.h>
#include <linux/uinput.h>

// --- Константы ---
const int KEYBOARD_I2C_ADDR = 0x5F;
const char* UINPUT_DEVICE_PATH = "/dev/uinput";

// --- Коды клавиш с CardKB ---
enum CardKeyCodes : uint8_t {
    UP = 181, DOWN = 182, LEFT = 180, RIGHT = 183,
    ESC = 27, BACKSPACE = 8, ENTER = 13, TAB = 9,
    SYM = 96,
    FN_UP = 153, FN_DOWN = 164, FN_LEFT = 152,
    FN_TAB = 140, FN_ESC = 128, FN_BACKSPACE = 139,
    FN_1 = 129, FN_2 = 130, FN_3 = 131, FN_4 = 132, FN_5 = 133,
    FN_6 = 134, FN_7 = 135, FN_8 = 136, FN_9 = 137, FN_0 = 138,
    FN_C = 168, FN_V = 169, FN_X = 167, FN_Z = 166, FN_A = 154, FN_S = 155
};

// --- Глобальные переменные состояния ---
bool g_toggle_mode_active = false;
std::set<int> g_toggled_keys;
bool g_latched_ctrl = false;
bool g_latched_alt = false;
bool g_latched_shift = false;

// --- Вспомогательные функции эмуляции ---
void emit_event(int fd, int type, int code, int value) {
    struct input_event ie;
    memset(&ie, 0, sizeof(ie));
    ie.type = type;
    ie.code = code;
    ie.value = value;
    if (write(fd, &ie, sizeof(ie)) < 0) {}
}
void sync_events(int fd) { emit_event(fd, EV_SYN, SYN_REPORT, 0); }
void press_key(int fd, int key_code) { emit_event(fd, EV_KEY, key_code, 1); sync_events(fd); }
void release_key(int fd, int key_code) { emit_event(fd, EV_KEY, key_code, 0); sync_events(fd); }
void tap_key(int fd, int key_code) { press_key(fd, key_code); usleep(20000); release_key(fd, key_code); }
void perform_combo(int fd, int modifier_key, int key) { press_key(fd, modifier_key); tap_key(fd, key); release_key(fd, modifier_key); }
void toggle_led(int fd, int led_code, bool state) { emit_event(fd, EV_LED, led_code, state ? 1 : 0); sync_events(fd); }
void handle_toggle_key(int fd, int key_code) {
    if (g_toggled_keys.count(key_code)) {
        release_key(fd, key_code);
        g_toggled_keys.erase(key_code);
    } else {
        press_key(fd, key_code);
        g_toggled_keys.insert(key_code);
    }
}

// --- Класс для обработки стандартных нажатий ---
class DefaultKeyHandler {
private:
    std::map<char, int> base_keys;
    std::map<char, char> shifted_to_base;
    std::set<int> toggle_target_keys;
public:
    DefaultKeyHandler() {
        base_keys = {
            {'a', KEY_A}, {'b', KEY_B}, {'c', KEY_C}, {'d', KEY_D}, {'e', KEY_E}, {'f', KEY_F}, {'g', KEY_G}, {'h', KEY_H},
            {'i', KEY_I}, {'j', KEY_J}, {'k', KEY_K}, {'l', KEY_L}, {'m', KEY_M}, {'n', KEY_N}, {'o', KEY_O}, {'p', KEY_P},
            {'q', KEY_Q}, {'r', KEY_R}, {'s', KEY_S}, {'t', KEY_T}, {'u', KEY_U}, {'v', KEY_V}, {'w', KEY_W}, {'x', KEY_X},
            {'y', KEY_Y}, {'z', KEY_Z}, {'1', KEY_1}, {'2', KEY_2}, {'3', KEY_3}, {'4', KEY_4}, {'5', KEY_5}, {'6', KEY_6},
            {'7', KEY_7}, {'8', KEY_8}, {'9', KEY_9}, {'0', KEY_0}, {' ', KEY_SPACE}, {'`', KEY_GRAVE}, {'=', KEY_EQUAL},
            {'[', KEY_LEFTBRACE}, {']', KEY_RIGHTBRACE}, {'\\', KEY_BACKSLASH}, {';', KEY_SEMICOLON}, {'\'', KEY_APOSTROPHE},
            {',', KEY_COMMA}, {'.', KEY_DOT}, {'/', KEY_SLASH}, {'-', KEY_MINUS}
        };
        shifted_to_base = {
            {'!', '1'}, {'@', '2'}, {'#', '3'}, {'$', '4'}, {'%', '5'}, {'^', '6'}, {'&', '7'}, {'*', '8'}, {'(', '9'},
            {')', '0'}, {'~', '`'}, {'+', '='}, {'{', '['}, {'}', ']'}, {'|', '\\'}, {':', ';'}, {'"', '\''},
            {'<', ','}, {'>', '.'}, {'?', '/'}, {'_', '-'}
        };
        toggle_target_keys = {KEY_W, KEY_A, KEY_S, KEY_D, KEY_LEFTSHIFT, KEY_LEFTCTRL, KEY_SPACE};
    }
    void type(int fd_uinput, uint8_t ascii_code) {
        if (!isprint(ascii_code)) return;
        bool use_shift = g_latched_shift || isupper(ascii_code);
        char base_char = tolower(ascii_code);

        if (!isupper(ascii_code) && shifted_to_base.count(base_char)) {
            use_shift = true;
            base_char = shifted_to_base[base_char];
        }

        if (!base_keys.count(base_char)) return;
        int key_code = base_keys[base_char];

        if (g_toggle_mode_active && toggle_target_keys.count(key_code)) {
            handle_toggle_key(fd_uinput, key_code);
        } else {
            if (use_shift && !g_latched_shift) press_key(fd_uinput, KEY_LEFTSHIFT);
            tap_key(fd_uinput, key_code);
            if (use_shift && !g_latched_shift) release_key(fd_uinput, KEY_LEFTSHIFT);
        }
    }
};

void print_help(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options]\n"
              << "Options:\n"
              << "  -i, --i2c-device <path>   Path to the I2C device. Default: /dev/i2c-0\n"
              << "  -v, --verbose             Enable verbose output for debugging.\n"
              << "  -h, --help                Show this help message and exit.\n";
}

// --- Главная функция ---
int main(int argc, char *argv[]) {
    std::string i2c_device_path = "/dev/i2c-1"; // Defaulting to i2c-1 as planned
    bool verbose_mode = false;

    const struct option long_options[] = {
        {"i2c-device", required_argument, 0, 'i'},
        {"verbose",    no_argument,       0, 'v'},
        {"help",       no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "i:vh", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'i': i2c_device_path = optarg; break;
            case 'v': verbose_mode = true; break;
            case 'h': print_help(argv[0]); return 0;
            default: print_help(argv[0]); return 1;
        }
    }

    int fd_uinput = open(UINPUT_DEVICE_PATH, O_WRONLY | O_NONBLOCK);
    if (fd_uinput < 0) { std::cerr << "Error: Cannot open uinput. Run with sudo." << std::endl; return 1; }
    
    ioctl(fd_uinput, UI_SET_EVBIT, EV_KEY);
    ioctl(fd_uinput, UI_SET_EVBIT, EV_SYN);
    ioctl(fd_uinput, UI_SET_EVBIT, EV_LED);
    for (int i = KEY_ESC; i < KEY_MAX; i++) ioctl(fd_uinput, UI_SET_KEYBIT, i);
    ioctl(fd_uinput, UI_SET_LEDBIT, LED_NUML);
    ioctl(fd_uinput, UI_SET_LEDBIT, LED_CAPSL);
    ioctl(fd_uinput, UI_SET_LEDBIT, LED_SCROLLL);
    
    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1A2B;
    usetup.id.product = 0x3C4D;
    strcpy(usetup.name, "CLST2 CardKB v4.0");
    ioctl(fd_uinput, UI_DEV_SETUP, &usetup);
    ioctl(fd_uinput, UI_DEV_CREATE);
    sleep(1);

    int fd_i2c = open(i2c_device_path.c_str(), O_RDWR);
    if (fd_i2c < 0) { std::cerr << "Error: Cannot open " << i2c_device_path << std::endl; return 1; }
    if (ioctl(fd_i2c, I2C_SLAVE, KEYBOARD_I2C_ADDR) < 0) { std::cerr << "Error: Cannot set i2c slave address." << std::endl; return 1; }

    std::cout << "Демон клавиатуры v4.0 запущен. Устройство: " << i2c_device_path << std::endl;

    uint8_t current_key = 0, last_key = 0;
    DefaultKeyHandler default_handler;
    while (true) {
        if (read(fd_i2c, &current_key, 1) == 1) {
            if (current_key != 0x00 && current_key != last_key) {
                if (verbose_mode) { std::cout << "Key code: " << (int)current_key << std::endl; }
                
                bool key_processed = true;
                switch(current_key) {
                    case FN_UP:
                        g_latched_alt = !g_latched_alt;
                        if (g_latched_alt) press_key(fd_uinput, KEY_LEFTALT); else release_key(fd_uinput, KEY_LEFTALT);
                        toggle_led(fd_uinput, LED_CAPSL, g_latched_alt);
                        break;
                    case FN_DOWN:
                        g_latched_ctrl = !g_latched_ctrl;
                        if (g_latched_ctrl) press_key(fd_uinput, KEY_LEFTCTRL); else release_key(fd_uinput, KEY_LEFTCTRL);
                        toggle_led(fd_uinput, LED_NUML, g_latched_ctrl);
                        break;
                    case FN_LEFT:
                        g_latched_shift = !g_latched_shift;
                        if (g_latched_shift) press_key(fd_uinput, KEY_LEFTSHIFT); else release_key(fd_uinput, KEY_LEFTSHIFT);
                        toggle_led(fd_uinput, LED_SCROLLL, g_latched_shift);
                        break;
                    case FN_TAB:
                        g_toggle_mode_active = !g_toggle_mode_active;
                        // Можно добавить отдельную индикацию, если потребуется
                        if (!g_toggle_mode_active) {
                            for (int key : g_toggled_keys) release_key(fd_uinput, key);
                            g_toggled_keys.clear();
                        }
                        break;
                    case FN_BACKSPACE: tap_key(fd_uinput, KEY_DELETE); break;
                    case FN_1: tap_key(fd_uinput, KEY_F1); break;
                    case FN_2: tap_key(fd_uinput, KEY_F2); break;
                    case FN_3: tap_key(fd_uinput, KEY_F3); break;
                    case FN_4: tap_key(fd_uinput, KEY_F4); break;
                    case FN_5: tap_key(fd_uinput, KEY_F5); break;
                    case FN_6: tap_key(fd_uinput, KEY_F6); break;
                    case FN_7: tap_key(fd_uinput, KEY_F7); break;
                    case FN_8: tap_key(fd_uinput, KEY_F8); break;
                    case FN_9: tap_key(fd_uinput, KEY_F9); break;
                    case FN_0: tap_key(fd_uinput, KEY_F10); break;
                    case FN_ESC: tap_key(fd_uinput, KEY_F11); break;
                    case SYM: {
                        uint8_t next_key = 0;
                        usleep(20000); // Короткая пауза для чтения следующего байта, если он есть
                        read(fd_i2c, &next_key, 1);
                        if (next_key == ESC) {
                             if (!g_latched_shift) press_key(fd_uinput, KEY_LEFTSHIFT);
                             tap_key(fd_uinput, KEY_F11);
                             if (!g_latched_shift) release_key(fd_uinput, KEY_LEFTSHIFT);
                        } else {
                            default_handler.type(fd_uinput, current_key);
                            if (next_key != 0) current_key = next_key; else { last_key = SYM; continue; }
                             key_processed = false;
                        }
                        break;
                    }
                    case ENTER: tap_key(fd_uinput, KEY_ENTER); break;
                    case BACKSPACE: tap_key(fd_uinput, KEY_BACKSPACE); break;
                    case ESC: tap_key(fd_uinput, KEY_ESC); break;
                    case TAB: tap_key(fd_uinput, KEY_TAB); break;
                    case UP: tap_key(fd_uinput, KEY_UP); break;
                    case DOWN: tap_key(fd_uinput, KEY_DOWN); break;
                    case LEFT: tap_key(fd_uinput, KEY_LEFT); break;
                    case RIGHT: tap_key(fd_uinput, KEY_RIGHT); break;
                    case FN_C: perform_combo(fd_uinput, KEY_LEFTCTRL, KEY_C); break;
                    case FN_V: perform_combo(fd_uinput, KEY_LEFTCTRL, KEY_V); break;
                    case FN_X: perform_combo(fd_uinput, KEY_LEFTCTRL, KEY_X); break;
                    case FN_A: perform_combo(fd_uinput, KEY_LEFTCTRL, KEY_A); break;
                    case FN_S: perform_combo(fd_uinput, KEY_LEFTCTRL, KEY_S); break;
                    case FN_Z: perform_combo(fd_uinput, KEY_LEFTCTRL, KEY_Z); break;
                    default:
                        key_processed = false;
                }
                
                if (!key_processed) {
                    default_handler.type(fd_uinput, current_key);
                }
            }
            last_key = current_key;
        } else {
            last_key = 0;
        }
        usleep(10000);
    }
    
    ioctl(fd_uinput, UI_DEV_DESTROY);
    close(fd_uinput);
    close(fd_i2c);
    return 0;
}