#include <iostream>
#include <string>
#include <cstdlib>
#include <dbus/dbus.h>

// Функция обработки запроса на открытие файла
dbus_bool_t handleOpenFile(const char* filePath, const char* extension) {
    std::cout << "Opening file: " << filePath << " (extension: " << extension << ")" << std::endl;
    
    std::string command;
    
    // Выбираем приложение в зависимости от расширения
    if (std::string(extension) == ".cpp" || std::string(extension) == ".c") {
        command = "gedit \"" + std::string(filePath) + "\"";
    } else if (std::string(extension) == ".docx") {
        command = "libreoffice \"" + std::string(filePath) + "\"";
    } else if (std::string(extension) == ".pdf") {
        command = "evince \"" + std::string(filePath) + "\"";
    } else {
        // Для всех остальных типов - делегируем системе через xdg-open
        command = "xdg-open \"" + std::string(filePath) + "\"";
    }
    
    // Выполняем команду
    int result = std::system(command.c_str());
    
    // Возвращаем результат (TRUE - успех, FALSE - ошибка)
    return (result == 0) ? TRUE : FALSE;
}

// Главная функция - запуск D-Bus сервиса
int main() {
    DBusError error;
    DBusConnection* connection;
    
    // Инициализация структуры ошибки
    dbus_error_init(&error);
    
    // Подключение к сессионной шине D-Bus
    connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
    if (dbus_error_is_set(&error)) {
        std::cerr << "Connection Error: " << error.message << std::endl;
        dbus_error_free(&error);
        return EXIT_FAILURE;
    }
    
    if (!connection) {
        std::cerr << "Failed to connect to D-Bus" << std::endl;
        return EXIT_FAILURE;
    }
    
    // Регистрация имени сервиса
    int ret = dbus_bus_request_name(connection, "com.example.FileOpener", 
                                   DBUS_NAME_FLAG_REPLACE_EXISTING, &error);
    if (dbus_error_is_set(&error)) {
        std::cerr << "Name Error: " << error.message << std::endl;
        dbus_error_free(&error);
        return EXIT_FAILURE;
    }
    
    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        std::cerr << "Failed to acquire D-Bus name" << std::endl;
        return EXIT_FAILURE;
    }
    
    std::cout << "D-Bus service 'com.example.FileOpener' is running..." << std::endl;
    
    // Основной цикл обработки сообщений
    while (true) {
        dbus_connection_read_write(connection, 0);
        DBusMessage* message = dbus_connection_pop_message(connection);
        
        if (message == nullptr) {
            continue;
        }
        
        // Обработка вызова метода OpenFile
        if (dbus_message_is_method_call(message, "com.example.FileOpener", "OpenFile")) {
            const char* filePath = nullptr;
            const char* extension = nullptr;
            
            if (dbus_message_get_args(message, &error,
                                     DBUS_TYPE_STRING, &filePath,
                                     DBUS_TYPE_STRING, &extension,
                                     DBUS_TYPE_INVALID)) {
                
                dbus_bool_t success = handleOpenFile(filePath, extension);
                
                // Отправка ответа клиенту
                DBusMessage* reply = dbus_message_new_method_return(message);
                if (reply) {
                    dbus_message_append_args(reply, DBUS_TYPE_BOOLEAN, &success, DBUS_TYPE_INVALID);
                    dbus_connection_send(connection, reply, nullptr);
                    dbus_message_unref(reply);
                }
            } else {
                std::cerr << "Failed to parse message arguments: " << error.message << std::endl;
                dbus_error_free(&error);
            }
        }
        
        // Освобождение памяти сообщения
        dbus_message_unref(message);
    }
    
    return EXIT_SUCCESS;
}
