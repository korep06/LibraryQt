/**
 * @file StartMenu.h
 * @author Кирилл К
 * @brief Стартовое меню приложения библиотеки
 * @version 1.0
 * @date 2024-12-19
 */

#ifndef STARTMENU_H
#define STARTMENU_H

#include <QDialog>

namespace Ui {
class StartMenu;
}

/**
 * @class StartMenu
 * @brief Диалоговое окно стартового меню приложения
 *
 * Класс представляет начальное окно приложения, которое появляется
 * при запуске и предоставляет доступ к основной функциональности.
 */
class StartMenu : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Конструктор стартового меню
     * @param parent Родительский виджет
     */
    explicit StartMenu(QWidget *parent = nullptr);

    /**
     * @brief Деструктор стартового меню
     */
    ~StartMenu();

private slots:
    /**
     * @brief Обработчик нажатия кнопки входа в аккаунт
     *
     * Выполняет переход к основному интерфейсу приложения
     * после аутентификации пользователя.
     */
    void on_pb_enter_acc_clicked();

private:
    Ui::StartMenu *ui; ///< Указатель на пользовательский интерфейс стартового меню
};

#endif // STARTMENU_H
