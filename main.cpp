/**
 * @file main.cpp
 * @author Кирилл К
 * @brief Главная функция приложения библиотеки
 * @version 1.0
 * @date 2024-12-19
 */

#include "mainwindow.h"
#include "startmenu.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QLabel>
#include <QMessageBox>

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
        // Если пользователь не прошел аутентификацию, завершаем приложение
        return 0;
    }

    // Создание и отображение главного окна приложения
    MainWindow w;
    w.show();

    // Запуск основного цикла обработки событий
    return a.exec();
}
