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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:


private slots:
    void act_save_file();
    void act_close_app();

    void act_add_book();
    void act_add_reader();

    void act_giveout_book();
    void act_edit_book();
    void act_edit_reader();

    void act_delete_book();
    void act_delete_reader();

    void act_search_book();
    void act_search_reader();
    void act_get_info();

    void act_UpdateActionStates(int index);

    void act_return_book();

    void act_export_books_pdf();
    void act_export_books_html();




    // Кнопки
    void on_pb_addBook_clicked();
    void on_pb_editBook_clicked();
    void on_pb_deleteBook_clicked();
    void on_pb_searchBook_clicked();

    void on_pb_addReader_clicked();
    void on_pb_editReader_clicked();
    void on_pb_deleteReader_clicked();
    void on_pb_searchReader_clicked();

    void on_pb_getBook_clicked();

    void on_pb_giveBook_clicked();

private:
    Ui::MainWindow *ui;
    BookModel *bookModel_;
    ReaderModel *readerModel_;

    void checkAddReader(const QString &surname, const QString &name);
    void checkBookName(const QString &name);
    void checkAddBook(const QString &name , const QString &author);
    void checkBookSearch(const QString &query);
    void checkGiveOutInput(const QString &code, const QString &readerID);
    void checkEditBook(const QString &name, const QString &author);

    QString buildFullReportHtml() const;
};

#endif // MAINWINDOW_H
