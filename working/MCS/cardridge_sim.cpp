// =========================================================================
// Эмулятор Картриджа для MCS
// Версия 1.0
//
// Описание:
// Эта простая программа имитирует вставку картриджа. Она записывает
// строку-"паспорт" в именованный канал (FIFO) и завершает работу.
// =========================================================================

#include <iostream>
#include <string>
#include <fstream>
#include <stdexcept>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

// --- НАСТРОЙКИ ---
const std::string FIFO_PATH = "/tmp/mcs_fifo";
const std::string PASSPORT_PRINTER = "TP=PRINTER;NM=SIG-MA v1;DRVID=00000001;CRDID=A1B2C3D4;";
const std::string PASSPORT_SENSOR = "TP=SENSOR;NM=Temp Sensor;DRVID=00000002;CRDID=BEEFCAFE;ODRVID=00000001;";


int main(int argc, char *argv[]) {
    std::cout << "Эмулятор Картриджа запущен..." << std::endl;

    // --- 1. Создаем FIFO, если он не существует ---
    // mkfifo вернет 0, если FIFO создан, или -1, если он уже существует или есть ошибка
    if (mkfifo(FIFO_PATH.c_str(), 0666) == -1) {
        if (errno != EEXIST) {
            perror("Ошибка при создании FIFO");
            return 1;
        }
        std::cout << "FIFO " << FIFO_PATH << " уже существует." << std::endl;
    } else {
        std::cout << "FIFO " << FIFO_PATH << " успешно создан." << std::endl;
    }

    // --- 2. Выбираем, какой паспорт "вставить" ---
    std::string passport_to_send = PASSPORT_PRINTER;
    if (argc > 1 && std::string(argv[1]) == "sensor") {
        passport_to_send = PASSPORT_SENSOR;
        std::cout << "Выбран 'сенсорный' картридж." << std::endl;
    } else {
        std::cout << "Выбран 'принтерный' картридж (по умолчанию)." << std::endl;
    }


    // --- 3. Открываем FIFO на запись ---
    // Важно: open "зависнет" до тех пор, пока кто-нибудь не откроет
    // этот же FIFO на чтение с другого конца!
    std::cout << "Ожидание, пока кто-нибудь откроет FIFO для чтения..." << std::endl;
    int fd = open(FIFO_PATH.c_str(), O_WRONLY);
    if (fd == -1) {
        perror("Ошибка при открытии FIFO для записи");
        return 1;
    }
    std::cout << "Читатель подключился! Отправка паспорта..." << std::endl;
    
    // --- 4. Записываем паспорт в FIFO ---
    ssize_t bytes_written = write(fd, passport_to_send.c_str(), passport_to_send.length());
    if (bytes_written == -1) {
        perror("Ошибка при записи в FIFO");
        close(fd);
        return 1;
    }

    close(fd);
    std::cout << "Паспорт '" << passport_to_send << "' успешно отправлен (" << bytes_written << " байт)." << std::endl;
    std::cout << "Эмулятор Картриджа завершает работу." << std::endl;

    return 0;
}