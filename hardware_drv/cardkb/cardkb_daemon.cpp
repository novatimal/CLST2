//============================================================================
// Название:      cardkb_daemon.cpp
// Версия:        1.5 (исправлена регистрация клавиш uinput и заполнение combo_map)
// Описание:      Демон для M5Stack CardKB.
//                Читает нажатия клавиш по I2C и эмулирует стандартную
//                USB-клавиатуру через uinput.
//============================================================================

#include <iostream>
#include <map>
#include <set>
#include <functional>
#include <cstdint>
#include <string>
#include <cctype> // для isprint, isupper, tolower

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include <linux/i2c-dev.h>
#include <linux/uinput.h>

// --- Константы ---
const int KEYBOARD_I2C_ADDR = 0x5F;
const char* UINPUT_DEVICE_PATH = "/dev/uinput";

// --- Перечисление с кодами Fn-клавиш ---
enum CardKeyCodes : uint8_t {
    UP = 181, DOWN = 182, LEFT = 180, RIGHT = 183,
    ESC = 27, BACKSPACE = 8, ENTER = 13,
    FN_SPACE = 175, FN_TAB = 140, FN_UP = 153, FN_DOWN = 164,
    FN_LEFT = 152, FN_RIGHT = 165, FN_ENTER = 163, FN_ESC = 128,
    FN_BACKSPACE = 139, FN_Z = 166, FN_X = 167, FN_C = 168,
    FN_V = 169, FN_D = 156, FN_A = 154, FN_S = 155, FN_N = 171,
    FN_F = 157, FN_1 = 129, FN_2 = 130, FN_3 = 131, FN_4 = 132,
    FN_5 = 133, FN_6 = 134, FN_7 = 135, FN_8 = 136, FN_9 = 137,
    FN_0 = 138
};

// --- Вспомогательные функции ---

void emit_event(int fd, int type, int code, int value) {
    struct input_event ie;
    memset(&ie, 0, sizeof(ie));
    ie.type = type;
    ie.code = code;
    ie.value = value;
    write(fd, &ie, sizeof(ie));
}

void perform_combo(int fd_uinput, int modifier_key, int key) {
    // Фаза 1: НАЖАТИЕ
    // Сначала нажимаем все нужные клавиши
    emit_event(fd_uinput, EV_KEY, modifier_key, 1); // Нажать модификатор
    emit_event(fd_uinput, EV_KEY, key, 1);         // Нажать основную клавишу

    // Теперь отправляем отчет, чтобы система обработала нажатия вместе
    emit_event(fd_uinput, EV_SYN, SYN_REPORT, 0);

    // Небольшая пауза для надежности, чтобы система точно успела
    usleep(20000); // 20 миллисекунд

    // Фаза 2: ОТПУСКАНИЕ
    // Теперь отпускаем все клавиши
    emit_event(fd_uinput, EV_KEY, key, 0);         // Отпустить основную клавишу
    emit_event(fd_uinput, EV_KEY, modifier_key, 0); // Отпустить модификатор

    // Снова отправляем отчет
    emit_event(fd_uinput, EV_SYN, SYN_REPORT, 0);
}

// --- Класс для обработки стандартных нажатий ---
class DefaultKeyHandler {
private:
    std::map<char, int> base_keys;
    std::map<char, char> shifted_to_base;

public:
    DefaultKeyHandler() {
        base_keys['a'] = KEY_A; base_keys['b'] = KEY_B; base_keys['c'] = KEY_C; base_keys['d'] = KEY_D;
        base_keys['e'] = KEY_E; base_keys['f'] = KEY_F; base_keys['g'] = KEY_G; base_keys['h'] = KEY_H;
        base_keys['i'] = KEY_I; base_keys['j'] = KEY_J; base_keys['k'] = KEY_K; base_keys['l'] = KEY_L;
        base_keys['m'] = KEY_M; base_keys['n'] = KEY_N; base_keys['o'] = KEY_O; base_keys['p'] = KEY_P;
        base_keys['q'] = KEY_Q; base_keys['r'] = KEY_R; base_keys['s'] = KEY_S; base_keys['t'] = KEY_T;
        base_keys['u'] = KEY_U; base_keys['v'] = KEY_V; base_keys['w'] = KEY_W; base_keys['x'] = KEY_X;
        base_keys['y'] = KEY_Y; base_keys['z'] = KEY_Z;
        base_keys['1'] = KEY_1; base_keys['2'] = KEY_2; base_keys['3'] = KEY_3; base_keys['4'] = KEY_4;
        base_keys['5'] = KEY_5; base_keys['6'] = KEY_6; base_keys['7'] = KEY_7; base_keys['8'] = KEY_8;
        base_keys['9'] = KEY_9; base_keys['0'] = KEY_0;
        base_keys[' '] = KEY_SPACE;
        shifted_to_base['!'] = '1'; shifted_to_base['@'] = '2'; shifted_to_base['#'] = '3';
        shifted_to_base['$'] = '4'; shifted_to_base['%'] = '5'; shifted_to_base['^'] = '6';
        shifted_to_base['&'] = '7'; shifted_to_base['*'] = '8'; shifted_to_base['('] = '9';
        shifted_to_base[')'] = '0';
    }

    // Внутри 'class DefaultKeyHandler'

	void type(int fd_uinput, uint8_t ascii_code) {
		if (!isprint(ascii_code)) return;
		bool use_shift = false;
		char base_char = ascii_code;
		int linux_keycode = 0;
	
		if (isupper(ascii_code)) {
			use_shift = true;
			base_char = tolower(ascii_code);
		} else if (shifted_to_base.count(ascii_code)) {
			use_shift = true;
			base_char = shifted_to_base[ascii_code];
		}
	
		if (base_keys.count(base_char)) {
			linux_keycode = base_keys[base_char];
		} else { return; }
	
		// --- ИЗМЕНЕННАЯ ЛОГИКА ---
		// Фаза 1: НАЖАТИЕ
		if (use_shift) {
			emit_event(fd_uinput, EV_KEY, KEY_LEFTSHIFT, 1); // Нажать Shift
		}
		emit_event(fd_uinput, EV_KEY, linux_keycode, 1);     // Нажать основную клавишу
		emit_event(fd_uinput, EV_SYN, SYN_REPORT, 0);        // Синхронизировать!
	
		usleep(20000); // Пауза
	
		// Фаза 2: ОТПУСКАНИЕ
		emit_event(fd_uinput, EV_KEY, linux_keycode, 0);     // Отпустить основную клавишу
		if (use_shift) {
			emit_event(fd_uinput, EV_KEY, KEY_LEFTSHIFT, 0); // Отпустить Shift
		}
		emit_event(fd_uinput, EV_SYN, SYN_REPORT, 0);        // Синхронизировать!
	}
};

void print_help(const char* prog_name) {
    std::cout << "Использование: " << prog_name << " [параметры]\n"
              << "Параметры:\n"
              << "  -i, --i2c-device <путь>   Путь к устройству I2C (или FIFO для симуляции).\n"
              << "                            По умолчанию: /dev/i2c-0\n"
              << "  -s, --simulate            Включить режим симуляции (не использовать ioctl).\n"
              << "  -v, --verbose             Включить подробный вывод для отладки.\n"
              << "  -h, --help                Показать эту справку и выйти.\n";
}

// --- Главная функция ---
int main(int argc, char *argv[]) {
    // --- 1. Парсинг аргументов командной строки ---
    std::string i2c_device_path = "/dev/i2c-0";
    bool verbose_mode = false;
    bool simulation_mode = false;

    const struct option long_options[] = {
        {"i2c-device", required_argument, 0, 'i'},
        {"simulate",   no_argument,       0, 's'},
        {"verbose",    no_argument,       0, 'v'},
        {"help",       no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "i:svh", long_options, nullptr)) != -1) {
        switch (opt) {
            case 'i': i2c_device_path = optarg; break;
            case 's': simulation_mode = true; break;
            case 'v': verbose_mode = true; break;
            case 'h': print_help(argv[0]); return 0;
            default: print_help(argv[0]); return 1;
        }
    }

    // --- 2. Определение карт специальных клавиш ---
    std::map<uint8_t, int> direct_keymap;
    std::map<uint8_t, std::function<void(int)>> combo_map;

    direct_keymap[ENTER] = KEY_ENTER;
    direct_keymap[BACKSPACE] = KEY_BACKSPACE;
    direct_keymap[ESC] = KEY_ESC;
    direct_keymap[UP] = KEY_UP;
    direct_keymap[DOWN] = KEY_DOWN;
    direct_keymap[LEFT] = KEY_LEFT;
    direct_keymap[RIGHT] = KEY_RIGHT;

    combo_map[FN_C] = [](int fd){ perform_combo(fd, KEY_LEFTCTRL, KEY_C); };
    combo_map[FN_X] = [](int fd){ perform_combo(fd, KEY_LEFTCTRL, KEY_X); };
    combo_map[FN_V] = [](int fd){ perform_combo(fd, KEY_LEFTCTRL, KEY_V); };
    combo_map[FN_A] = [](int fd){ perform_combo(fd, KEY_LEFTCTRL, KEY_A); };
    combo_map[FN_S] = [](int fd){ perform_combo(fd, KEY_LEFTCTRL, KEY_S); };
    combo_map[FN_Z] = [](int fd){ perform_combo(fd, KEY_LEFTCTRL, KEY_Z); };
    combo_map[FN_BACKSPACE] = [](int fd){ emit_event(fd, EV_KEY, KEY_DELETE, 1); emit_event(fd, EV_SYN, SYN_REPORT, 0); emit_event(fd, EV_KEY, KEY_DELETE, 0); emit_event(fd, EV_SYN, SYN_REPORT, 0); };

    // --- 3. Настройка виртуального устройства Uinput ---
    int fd_uinput = open(UINPUT_DEVICE_PATH, O_WRONLY | O_NONBLOCK);
    if (fd_uinput < 0) {
        perror("Ошибка: Не удалось открыть /dev/uinput. Запустите с sudo.");
        return 1;
    }
    ioctl(fd_uinput, UI_SET_EVBIT, EV_KEY);
    ioctl(fd_uinput, UI_SET_EVBIT, EV_SYN);
    
    // --- ИСПРАВЛЕНИЕ ЗДЕСЬ: Регистрируем все возможные клавиши ---
    // Это самый надежный способ, чтобы гарантировать, что все модификаторы (Shift, Ctrl)
    // и все целевые клавиши будут работать.
    for (int i = KEY_ESC; i < KEY_MAX; i++) {
        ioctl(fd_uinput, UI_SET_KEYBIT, i);
    }
    
    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1A2B;
    usetup.id.product = 0x3C4D;
    strcpy(usetup.name, "CLST2 CardKB");
    ioctl(fd_uinput, UI_DEV_SETUP, &usetup);
    ioctl(fd_uinput, UI_DEV_CREATE);
    sleep(1);

    // --- 4. Настройка I2C или симуляции ---
    int fd_i2c = open(i2c_device_path.c_str(), O_RDWR);
    if (fd_i2c < 0) {
        perror(("Ошибка: Не удалось открыть устройство: " + i2c_device_path).c_str());
        ioctl(fd_uinput, UI_DEV_DESTROY); close(fd_uinput);
        return 1;
    }

    if (!simulation_mode) {
        if (ioctl(fd_i2c, I2C_SLAVE, KEYBOARD_I2C_ADDR) < 0) {
            perror("Ошибка: Не удалось подключиться к I2C устройству (ioctl failed)");
            close(fd_i2c); 
            ioctl(fd_uinput, UI_DEV_DESTROY); 
            close(fd_uinput);
            return 1;
        }
    } else {
        if (verbose_mode) { 
            std::cout << "[DEBUG] Режим симуляции активен. Пропускаем ioctl для I2C." << std::endl; 
        }
    }

    std::cout << "Демон клавиатуры запущен." << std::endl;
    if (verbose_mode) { std::cout << "[DEBUG] Чтение с устройства: " << i2c_device_path << std::endl; }

    // --- 5. Основной цикл программы ---
    uint8_t current_key = 0, last_key = 0;
    DefaultKeyHandler default_handler;
    while (true) {
        if (read(fd_i2c, &current_key, 1) == 1) {
            if (current_key != 0x00 && last_key == 0x00) {
                if (verbose_mode) { std::cout << "[DEBUG] Получен код: " << (int)current_key << std::endl; }
                bool processed = false;
                if (combo_map.count(current_key)) {
                    combo_map[current_key](fd_uinput);
                    processed = true;
                } else if (direct_keymap.count(current_key)) {
                    int key_code = direct_keymap[current_key];
                    emit_event(fd_uinput, EV_KEY, key_code, 1);
                    emit_event(fd_uinput, EV_SYN, SYN_REPORT, 0);
                    emit_event(fd_uinput, EV_KEY, key_code, 0);
                    emit_event(fd_uinput, EV_SYN, SYN_REPORT, 0);
                    processed = true;
                }
                if (!processed) {
                    default_handler.type(fd_uinput, current_key);
                }
            }
            last_key = current_key;
        }
        usleep(15000);
    }

    // --- 6. Завершение работы ---
    ioctl(fd_uinput, UI_DEV_DESTROY);
    close(fd_uinput);
    close(fd_i2c);
    return 0;
}