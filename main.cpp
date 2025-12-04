/**
 * @file main.cpp
 * @author Кирилл К
 * @brief Главная функция приложения библиотеки
 * @version 1.0
 * @date 2024-12-19
 */

#include "mainwindow.h"
#include "startmenu.h"

#include "spdlog/details/null_mutex.h"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"   // простой файловый sink

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QLabel>
#include <QMessageBox>
#include <iostream>

/**
 * @brief Точка входа в приложение библиотеки
 *
 * Функция инициализирует приложение, настраивает переводы, отображает
 * стартовое меню для аутентификации и запускает главное окно приложения.
 *
 * @param argc Количество аргументов командной строки
 * @param argv Массив аргументов командной строки
 * @return Код завершения приложения
 */
int main(int argc, char *argv[])
{
    // Инициализация Qt приложения
    QApplication a(argc, argv);

    // создаём файловый логгер, который пишет в library.log
    auto logger = spdlog::basic_logger_mt("main", "library.log");
    spdlog::set_default_logger(logger);

    // формат вида: [2025-12-04 22:10:01] [info] [main] сообщение
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");

    spdlog::set_level(spdlog::level::debug); // глобальный уровень

    spdlog::info("Приложение запущено");

    // Настройка системы переводов
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "LibraryQt_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    // Отображение стартового меню для аутентификации
    StartMenu login;
    if (login.exec() != QDialog::Accepted) {
         spdlog::info("Пользователь закрыл окно логина");
        // Если пользователь не прошел аутентификацию, завершаем приложение
        return 0;
    }

    // Создание и отображение главного окна приложения
    MainWindow w;
    w.show();
    spdlog::info("Главное окно показано");

    int rc = a.exec();
    spdlog::info("Цикл событий завершён, код = {}", rc);
    return rc;
}
