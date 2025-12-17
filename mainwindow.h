/**
 * @file mainwindow.h
 * @author Кирилл К
 * @brief Объявление главного окна приложения библиотеки (Qt Widgets).
 * @version 1.1
 * @date 2025-12-04
 *
 * Главный экран приложения: работа с книгами и читателями, поиск, выдача/возврат,
 * формирование отчёта и экспорт в PDF/HTML.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "BookModel.h"
#include "ReaderModel.h"

#include <QMainWindow>
#include <QDateTime>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

/**
 * @class MainWindow
 * @brief Главное окно приложения.
 *
 * Окно объединяет:
 * - таблицу книг и таблицу читателей (Qt Model/View),
 * - действия меню (сохранение, поиск, добавление и т.п.),
 * - выдачу и возврат книг,
 * - формирование и экспорт отчёта (HTML/PDF).
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Конструктор главного окна.
     * @param parent Родительский виджет.
     */
    explicit MainWindow(QWidget *parent = nullptr);

    /**
     * @brief Деструктор главного окна.
     *
     * Обычно вызывается при закрытии приложения.
     */
    ~MainWindow();

private slots:
    /** @name Действия меню/основные действия
     *  @{
     */
    void act_save_file();     ///< Сохранить данные (JSON/XML).
    void act_close_app();     ///< Сохранить и закрыть приложение.

    void act_add_book();      ///< Добавить книгу (диалог).
    void act_add_reader();    ///< Добавить читателя (диалог).

    void act_giveout_book();  ///< Выдать книгу читателю.
    void act_edit_book();     ///< Редактировать выбранную книгу.
    void act_edit_reader();   ///< Редактировать выбранного читателя.

    void act_delete_book();   ///< Удалить выбранную книгу.
    void act_delete_reader(); ///< Удалить выбранного читателя.

    void act_search_book();   ///< Поиск книги (код/название).
    void act_search_reader(); ///< Поиск читателя (ID/ФИО).
    void act_get_info();      ///< Подробная информация о читателе.

    void act_UpdateActionStates(int index); ///< Обновление доступности действий по вкладке.
    void act_return_book();   ///< Возврат книги от читателя.

    void act_export_books_pdf();  ///< Экспорт отчёта в PDF.
    void act_export_books_html(); ///< Экспорт отчёта в HTML (многопоточно).
    /** @} */

    /** @name Слоты кнопок (auto-connect из UI)
     *  @{
     */
    void on_pb_addBook_clicked();
    void on_pb_editBook_clicked();
    void on_pb_deleteBook_clicked();
    void on_pb_searchBook_clicked();

    void on_pb_addReader_clicked();
    void on_pb_editReader_clicked();
    void on_pb_deleteReader_clicked();
    void on_pb_searchReader_clicked();

    void on_pb_getBook_clicked();           ///< Кнопка возврата книги.
    void on_pb_giveBook_clicked();          ///< Кнопка выдачи книги.
    void on_pb_get_info_reader_clicked();   ///< Кнопка информации о читателе.
    /** @} */

private:
    Ui::MainWindow *ui;          ///< UI, сгенерированный Qt Designer.
    BookModel *bookModel_;       ///< Модель книг (таблица книг).
    ReaderModel *readerModel_;   ///< Модель читателей (таблица читателей).

    /**
     * @brief Формирует полный HTML-отчёт по текущим данным моделей.
     * @return HTML-строка отчёта.
     */
    QString buildFullReportHtml() const;
};

#endif // MAINWINDOW_H
