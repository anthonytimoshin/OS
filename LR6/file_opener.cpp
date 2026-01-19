#include <iostream>
#include <string>
#include <cstring>
#include <dbus/dbus.h>

namespace fs = std::filesystem;

void showUsage() {
    std::cout << "Usage: file_opener <file_path>" << std::endl;
}

std::string getFileExtension(const std::string& filePath) {
    const char* dot = strrchr(filePath.c_str(), '.');
    if (!dot) return "";
    return std::string(dot);
}

bool fileExists(const std::string& filePath) {
    FILE* file = fopen(filePath.c_str(), "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

bool sendDbusMessage(const std::string& filePath, const std::string& extension) {
    DBusError error;
    dbus_error_init(&error);
    
    // Подключение к шине D-Bus
    DBusConnection* connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
    if (dbus_error_is_set(&error)) {
        std::cerr << "Connection Error: " << error.message << std::endl;
        dbus_error_free(&error);
        return false;
    }
    
    if (!connection) {
        std::cerr << "Failed to connect to D-Bus" << std::endl;
        return false;
    }
    
    // Создание сообщения
    DBusMessage* message = dbus_message_new_method_call(
        "com.example.FileOpener",      // Сервис
        "/com/example/FileOpener",     // Объект
        "com.example.FileOpener",      // Интерфейс
        "OpenFile");                   // Метод
    
    if (!message) {
        std::cerr << "Failed to create message" << std::endl;
        return false;
    }
    
    // Создание аргументов сообщения
    DBusMessageIter args;
    dbus_message_iter_init_append(message, &args);
    
    // Переменные для передачи строк
    const char* filePathCStr = filePath.c_str();
    const char* extensionCStr = extension.c_str();
    
    // Добавление аргументов в сообщение
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &filePathCStr) ||
        !dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &extensionCStr)) {
        std::cerr << "Out of memory!" << std::endl;
        dbus_message_unref(message);
        return false;
    }
    
    // Отправка сообщения и ожидание ответа
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection, message, -1, &error);
    dbus_message_unref(message);
    
    if (dbus_error_is_set(&error)) {
        std::cerr << "Error in communication: " << error.message << std::endl;
        dbus_error_free(&error);
        return false;
    }
    
    if (!reply) {
        std::cerr << "No reply received" << std::endl;
        return false;
    }
    
    // Получение ответа
    bool success = false;
    if (!dbus_message_get_args(reply, &error, DBUS_TYPE_BOOLEAN, &success, DBUS_TYPE_INVALID)) {
        std::cerr << "Failed to parse reply: " << error.message << std::endl;
        dbus_error_free(&error);
    }
    
    dbus_message_unref(reply);
    return success;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        showUsage();
        return EXIT_FAILURE;
    }
    
    std::string filePath = argv[1];
    if (!fs::exists(filePath)) {
        std::cerr << "File does not exist: " << filePath << std::endl;
        return EXIT_FAILURE;
    }
    
    std::string extension = getFileExtension(filePath);
    if (extension.empty()) {
        std::cerr << "Unable to determine file extension." << std::endl;
        return EXIT_FAILURE;
    }
    
    if (!sendDbusMessage(filePath, extension)) {
        std::cerr << "Failed to send message to D-Bus service." << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
