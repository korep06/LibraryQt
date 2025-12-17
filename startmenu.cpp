/**
 * @file startmenu.cpp
 * @author Кирилл К
 * @brief Реализация стартового меню приложения
 * @version 1.0
 * @date 2025-11-25
 */

#include "startmenu.h"
#include "ui_startmenu.h"

#include <QMessageBox>

/**
 * @brief Конструктор стартового меню
 * @param parent Родительский виджет
 *
 * Инициализирует пользовательский интерфейс и настраивает диалоговое окно.
 */
StartMenu::StartMenu(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::StartMenu)
{

    ui->setupUi(this);
}

/**
 * @brief Деструктор стартового меню
 *
 * Освобождает ресурсы, занятые пользовательским интерфейсом.
 */
StartMenu::~StartMenu()
{
    delete ui;
}

/**
 * @brief Обработчик нажатия кнопки входа в аккаунт
 *
 * Выполняет проверку введенных учетных данных пользователя.
 * Проверяет логин и пароль, отображает соответствующие сообщения
 * и в случае успешной аутентификации завершает диалог с результатом Accepted.
 */
void StartMenu::on_pb_enter_acc_clicked()
{
    this->setWindowTitle("Вход");
    QString login = ui->le_login->text();
    QString password = ui->le_password->text();

    // Проверка учетных данных
    if (login == "Kirill" && password == "12345678") {
        QMessageBox::information(this, "Вход", "Удачно!");
        accept(); ///< Завершаем диалог с положительным результатом
    }
    else if (login.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите Логин");
    }
    else if (password.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите пароль");
    }
    else {
        QMessageBox::warning(this, "Ошибка", "Неверный логин или пароль. Попробуйте снова");
        ui->le_password->clear(); ///< Очищаем поле пароля для повторного ввода
    }
}
