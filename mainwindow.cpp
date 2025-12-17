/**
 * @file mainwindow.cpp
 * @brief Реализация главного окна приложения библиотеки (Qt/C++/CMake).
 */

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "Exception.h"
#include "spdlog/spdlog.h"
#include "DatabaseManager.h"
#include "InputValid.h"

#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QDate>
#include <QSortFilterProxyModel>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QTextDocument>
#include <QPrinter>
#include <QDateTime>
#include <QMutex>
#include <QWaitCondition>
#include <thread>
#include <optional>


static auto logMainWindow = spdlog::get("main");

namespace {

// Чтобы не копипастить 100 раз одно и то же (proxy -> source)
QModelIndex mapToSourceIndex(const QAbstractItemView* view, const QModelIndex& idx)
{
    if (!view || !idx.isValid())
        return idx;

    if (auto proxy = qobject_cast<const QSortFilterProxyModel*>(view->model()))
        return proxy->mapToSource(idx);

    return idx;
}

void showExceptionBox(QWidget* parent, const QString& title, const std::exception& ex)
{
    QMessageBox::warning(parent, title, ex.what());
    spdlog::warn("{}: {}", title.toStdString(), ex.what());
}

static bool saveAllData(BookModel* books, ReaderModel* readers)
{
    if (!books || !readers)
        return false;

    const bool okBooksJson   = books->SaveToFile("books.json");
    const bool okReadersJson = readers->SaveToFile("readers.json");
    const bool okBooksXml    = books->SaveToXml("books.xml");
    const bool okReadersXml  = readers->SaveToXml("readers.xml");

    return okBooksJson && okReadersJson && okBooksXml && okReadersXml;
}

// Узкий helper: выставить ошибку и разбудить потоки (не меняет логику)
template <typename SharedT>
void setThreadError(SharedT& shared, const QString& msg)
{
    QMutexLocker locker(&shared.mutex);
    shared.error = msg;
    shared.cond.wakeAll();
}

} // namespace


namespace {
QString buildReportHtmlFromData(const QList<Book> &books,
                                const QList<Reader> &readers)

{

    QDate today      = QDate::currentDate();
    QDate monthStart = QDate(today.year(), today.month(), 1);
    QDate monthEnd   = monthStart.addMonths(1).addDays(-1);

    auto isInCurrentMonth = [&](const QDate &d) {
        return d >= monthStart && d <= monthEnd;
    };

    QHash<QString, const Book*> bookByCode;
    for (const Book &b : books) {
        bookByCode.insert(b.code, &b);
    }

    int totalBooks          = books.size();
    int totalReaders        = readers.size();
    int totalTakenNow       = 0;
    int booksTakenThisMonth = 0;
    int newReadersThisMonth = 0;

    for (const Book &b : books) {
        if (b.is_taken)
            ++totalTakenNow;

        if (b.date_taken.has_value() && isInCurrentMonth(*b.date_taken))
            ++booksTakenThisMonth;
    }

    for (const Reader &r : readers) {
        if (r.reg_date.isValid() && isInCurrentMonth(r.reg_date))
            ++newReadersThisMonth;
    }

    QString html;
    html += "<!DOCTYPE html><html><head><meta charset=\"utf-8\">";
    html += "<title>Отчёт по библиотеке</title>";
    html += "<style>"
            "body{font-family:'DejaVu Sans',sans-serif;font-size:10pt;}"
            "h1{margin-bottom:4px;}"
            "h2{margin-top:14px;margin-bottom:4px;font-size:11pt;}"
            "table{border-collapse:collapse;width:100%;margin-bottom:10px;}"
            "th,td{border:1px solid #444;padding:4px;}"
            "th{background:#f0f0f0;text-align:left;}"
            ".meta{color:#555;font-size:9pt;margin-bottom:8px;}"
            ".badge{display:inline-block;padding:2px 6px;border-radius:4px;font-size:8pt;}"
            ".badge-ok{background:#d4edda;border:1px solid #c3e6cb;}"
            ".badge-warn{background:#f8d7da;border:1px solid #f5c6cb;}"
            "</style>";
    html += "</head><body>";

    html += "<h1>Отчёт по библиотеке</h1>";
    html += "<div class=\"meta\">";
    html += "Дата формирования: "
            + QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm")
            + "<br/>";
    html += "Отчётный период: "
            + monthStart.toString("dd.MM.yyyy")
            + " – "
            + monthEnd.toString("dd.MM.yyyy");
    html += "</div>";

    html += "<h2>Общая статистика</h2>";
    html += "<table>";
    html += "<tr><th>Показатель</th><th>Значение</th></tr>";
    html += "<tr><td>Всего книг</td><td>"
            + QString::number(totalBooks) + "</td></tr>";
    html += "<tr><td>Всего читателей</td><td>"
            + QString::number(totalReaders) + "</td></tr>";
    html += "<tr><td>Книг сейчас на руках</td><td>"
            + QString::number(totalTakenNow) + "</td></tr>";
    html += "<tr><td>Книг выдано в текущем месяце</td><td>"
            + QString::number(booksTakenThisMonth) + "</td></tr>";
    html += "<tr><td>Читателей записалось в текущем месяце</td><td>"
            + QString::number(newReadersThisMonth) + "</td></tr>";
    html += "</table>";

    // --- Таблица книг ---
    html += "<h2>Список книг</h2>";
    html += "<table>";
    html += "<tr>"
            "<th>Код</th>"
            "<th>Название</th>"
            "<th>Автор</th>"
            "<th>Статус</th>"
            "<th>Дата выдачи</th>"
            "</tr>";
    for (const Book &b : books) {
        html += "<tr>";
        html += "<td>" + b.code.toHtmlEscaped() + "</td>";
        html += "<td>" + b.name.toHtmlEscaped() + "</td>";
        html += "<td>" + b.author.toHtmlEscaped() + "</td>";

        QString statusHtml = b.is_taken
                                 ? "<span class=\"badge badge-warn\">Выдана</span>"
                                 : "<span class=\"badge badge-ok\">В наличии</span>";
        html += "<td>" + statusHtml + "</td>";

        QString dateStr;
        if (b.date_taken.has_value())
            dateStr = b.date_taken->toString("dd.MM.yyyy");
        html += "<td>" + dateStr.toHtmlEscaped() + "</td>";

        html += "</tr>";
    }
    html += "</table>";

    // --- Таблица читателей ---
    html += "<h2>Список читателей</h2>";
    html += "<table>";
    html += "<tr>"
            "<th>ID</th>"
            "<th>ФИО</th>"
            "<th>Пол</th>"
            "<th>Книг на руках</th>"
            "</tr>";

    for (const Reader &r : readers) {
        html += "<tr>";
        html += "<td>" + r.ID.toHtmlEscaped() + "</td>";

        QString fio = r.first_name + " " + r.second_name + " " + r.third_name;
        html += "<td>" + fio.toHtmlEscaped() + "</td>";

        QString genderStr = (r.gender == Sex::Male) ? "Мужской" : "Женский";
        html += "<td>" + genderStr.toHtmlEscaped() + "</td>";

        html += "<td>" + QString::number(r.taken_books.size()) + "</td>";
        html += "</tr>";
    }
    html += "</table>";

    // --- Должники ---
    html += "<h2>Должники (книги на руках)</h2>";

    bool hasDebtors = false;
    QString debtorTable;
    debtorTable += "<table>";
    debtorTable += "<tr>"
                   "<th>ID читателя</th>"
                   "<th>ФИО</th>"
                   "<th>Код книги</th>"
                   "<th>Название книги</th>"
                   "<th>Дата выдачи</th>"
                   "</tr>";

    for (const Reader &r : readers) {
        QString fio = r.first_name + " " + r.second_name + " " + r.third_name;
        for (const QString &code : r.taken_books) {
            const Book *b = bookByCode.value(code, nullptr);
            if (!b || !b->is_taken)
                continue;

            hasDebtors = true;
            debtorTable += "<tr>";
            debtorTable += "<td>" + r.ID.toHtmlEscaped() + "</td>";
            debtorTable += "<td>" + fio.toHtmlEscaped() + "</td>";
            debtorTable += "<td>" + b->code.toHtmlEscaped() + "</td>";
            debtorTable += "<td>" + b->name.toHtmlEscaped() + "</td>";

            QString dateStr;
            if (b->date_taken.has_value())
                dateStr = b->date_taken->toString("dd.MM.yyyy");
            debtorTable += "<td>" + dateStr.toHtmlEscaped() + "</td>";
            debtorTable += "</tr>";
        }
    }
    debtorTable += "</table>";

    if (hasDebtors) {
        html += debtorTable;
    } else {
        html += "<p>На данный момент все книги возвращены. Должников нет </p>";
    }

    html += "</body></html>";
    return html;
}

} // namespace

/**
 * @class MainWindow
 * @brief Главное окно приложения библиотеки.
 *
 * Предоставляет интерфейс для работы с книгами и читателями:
 * - отображение таблиц
 * - добавление/редактирование/удаление
 * - поиск и фильтрация
 */
/**
 * @brief Конструктор главного окна.
 * @param parent Родительский виджет.
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    if (!DatabaseManager::instance().open("library.db")) {
        QMessageBox::warning(this, "База данных",
                             "Не удалось открыть файл базы данных. "
                             "Данные будут храниться только в JSON/XML.");
    }

    // Инициализация моделей
    bookModel_ = new BookModel(this);
    readerModel_ = new ReaderModel(this);

    bool booksLoadedDb   = bookModel_->LoadFromDatabase();
    bool readersLoadedDb = readerModel_->LoadFromDatabase();

    /*
    // --- КНИГИ: сначала JSON, потом XML, потом тестовые данные ---
    bool booksLoadedJson = bookModel_->LoadFromFile("books.json");
    if (!booksLoadedJson || bookModel_->GetBooks().isEmpty()) {
        bool booksLoadedXml = bookModel_->LoadFromXml("books.xml");
    }

    // --- ЧИТАТЕЛИ: сначала JSON, потом XML, потом тестовые данные ---
    bool readersLoadedJson = readerModel_->LoadFromFile("readers.json");
    if (!readersLoadedJson || readerModel_->GetReaders().isEmpty()) {
        bool readersLoadedXml = readerModel_->LoadFromXml("readers.xml");
    }
    */

    // Установка моделей в таблицы
    ui->tbl_view_book->setModel(bookModel_);
    ui->tbl_view_reader->setModel(readerModel_);

    // Настройка таблиц
    ui->tbl_view_book->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tbl_view_reader->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tbl_view_book->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tbl_view_reader->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tbl_view_book->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tbl_view_reader->setSelectionMode(QAbstractItemView::SingleSelection);

    // Настройка подсказок для кнопок
    ui->pb_addBook->setToolTip("Добавить Книгу");
    ui->pb_editBook->setToolTip("Редактировать Книгу");
    ui->pb_deleteBook->setToolTip("Удалить Книгу");
    ui->pb_searchBook->setToolTip("Найти Книгу");
    ui->pb_getBook->setToolTip("Принять Книгу");
    ui->pb_giveBook->setToolTip("Выдать Книгу");
    ui->pb_addReader->setToolTip("Добавить читателя");
    ui->pb_editReader->setToolTip("Редактировать читателя");
    ui->pb_deleteReader->setToolTip("Удалить читателя");
    ui->pb_get_info_reader->setToolTip("Информация по читателю");

    // Соединение действий меню с методами
    connect(ui->mb_act_save_file, &QAction::triggered, this, &MainWindow::act_save_file);
    connect(ui->mb_act_close_app, &QAction::triggered, this, &MainWindow::act_close_app);
    connect(ui->mb_act_add_book, &QAction::triggered, this, &MainWindow::act_add_book);
    connect(ui->mb_act_add_reader, &QAction::triggered, this, &MainWindow::act_add_reader);
    connect(ui->mb_act_delete_book, &QAction::triggered, this, &MainWindow::act_delete_book);
    connect(ui->mb_act_delete_reader, &QAction::triggered, this, &MainWindow::act_delete_reader);
    connect(ui->mb_act_search_book, &QAction::triggered, this, &MainWindow::act_search_book);
    connect(ui->mb_act_search_reader, &QAction::triggered, this, &MainWindow::act_search_reader);
    connect(ui->mb_act_HTML, &QAction::triggered, this , &MainWindow::act_export_books_html);
    connect(ui->mb_act_PDF, &QAction::triggered, this , &MainWindow::act_export_books_pdf);

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::act_UpdateActionStates);
    act_UpdateActionStates(ui->tabWidget->currentIndex());

    // Фильтры поиска
    QSortFilterProxyModel *proxy_book = new QSortFilterProxyModel(this);
    proxy_book->setSourceModel(bookModel_);
    proxy_book->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy_book->setFilterKeyColumn(-1);
    ui->tbl_view_book->setModel(proxy_book);
    ui->le_filterBook->setPlaceholderText("Поиск по коду или названию");

    QSortFilterProxyModel *proxy_reader = new QSortFilterProxyModel(this);
    proxy_reader->setSourceModel(readerModel_);
    proxy_reader->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy_reader->setFilterKeyColumn(-1);
    ui->tbl_view_reader->setModel(proxy_reader);
    ui->le_filterReader->setPlaceholderText("Поиск по айди или названию");

    connect(ui->le_filterReader, &QLineEdit::textChanged, proxy_reader, &QSortFilterProxyModel::setFilterFixedString);
    connect(ui->le_filterBook, &QLineEdit::textChanged, proxy_book, &QSortFilterProxyModel::setFilterFixedString);
}

/**
 * @brief Деструктор MainWindow. Сохраняет данные перед закрытием.
 */
MainWindow::~MainWindow()
{
    const bool ok = saveAllData(bookModel_, readerModel_);
    if (!ok) {
        spdlog::warn("Не удалось сохранить данные при закрытии приложения");
    }

    delete ui;
}


/**
 * @brief Слоты для кнопок (делегируют вызов соответствующих действий)
 */
void MainWindow::on_pb_addBook_clicked() { act_add_book(); }
/**
 * @brief Обработчик кнопки «Редактировать книгу».
 */
void MainWindow::on_pb_editBook_clicked() { act_edit_book(); }
/**
 * @brief Обработчик кнопки «Удалить книгу».
 */
void MainWindow::on_pb_deleteBook_clicked() { act_delete_book(); }
/**
 * @brief Обработчик кнопки «Поиск книги».
 */
void MainWindow::on_pb_searchBook_clicked() { act_search_book(); }
/**
 * @brief Обработчик кнопки «Добавить читателя».
 */
void MainWindow::on_pb_addReader_clicked() { act_add_reader(); }
/**
 * @brief Обработчик кнопки «Редактировать читателя».
 */
void MainWindow::on_pb_editReader_clicked() { act_edit_reader(); }
/**
 * @brief Обработчик кнопки «Удалить читателя».
 */
void MainWindow::on_pb_deleteReader_clicked() { act_delete_reader(); }
/**
 * @brief Обработчик кнопки «Поиск читателя».
 */
void MainWindow::on_pb_searchReader_clicked() { act_search_reader(); }
/**
 * @brief Обработчик кнопки «Выдать книгу».
 */
void MainWindow::on_pb_giveBook_clicked() { act_giveout_book(); }

/**
 * @brief Сохраняет книги и читателей в файл.
 */
void MainWindow::act_save_file()
{
    const bool ok = saveAllData(bookModel_, readerModel_);

    if (ok) {
        QMessageBox::information(this, "Успех", "Данные сохранены (JSON и XML)");
    } else {
        QMessageBox::warning(this, "Ошибка",
                             "Не удалось сохранить данные во все файлы (JSON/XML)");
    }
}


/**
 * @brief Закрывает приложение с сохранением данных.
 */
void MainWindow::act_close_app()
{
    act_save_file();
    close();
}

/**
 * @brief Сформировать HTML-отчёт по данным книг и читателей.
 */
QString MainWindow::buildFullReportHtml() const
{
    return buildReportHtmlFromData(bookModel_->GetBooks(),
                                   readerModel_->GetReaders());
}


/**
 * @brief Экспорт отчёта в PDF.
 */
void MainWindow::act_export_books_pdf()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Сохранить отчёт в PDF",
        "LibraryReport.pdf",
        "PDF файлы (*.pdf)");

    if (fileName.isEmpty())
        return;

    const QString html = buildFullReportHtml();

    QTextDocument doc;
    doc.setHtml(html);

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);

    doc.print(&printer);

    QMessageBox::information(this, "Отчёт",
                             "PDF-отчёт успешно сохранён ");
}

/**
 * @brief Экспорт отчёта в HTML (возможна генерация в потоках).
 */
void MainWindow::act_export_books_html()
{
    spdlog::info("Старт многопоточного экспорта HTML-отчёта");

    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Сохранить отчёт в HTML",
        "LibraryReport.html",
        "HTML файлы (*.html)");

    if (fileName.isEmpty())
        return;

    const QList<Book>   booksSnapshot   = bookModel_->GetBooks();
    const QList<Reader> readersSnapshot = readerModel_->GetReaders();

    struct SharedData {
        QList<Book>   books;
        QList<Reader> readers;

        QList<Book>   sortedBooks;
        QList<Reader> sortedReaders;

        bool loaded = false;
        bool edited = false;

        QString error;

        QMutex mutex;
        QWaitCondition cond;
    };

    SharedData shared;

    std::thread t1([&shared, booksSnapshot, readersSnapshot]() {
        spdlog::debug("t1: начало копирования данных");
        {
            QMutexLocker lock(&shared.mutex);
            shared.books   = booksSnapshot;
            shared.readers = readersSnapshot;
            shared.loaded  = true;
        }
        shared.cond.wakeAll();
        spdlog::debug("t1: копирование завершено, книг={}, читателей={}",
                      booksSnapshot.size(), readersSnapshot.size());
    });

    std::thread t2([&shared]() {
        spdlog::debug("t2: ожидание данных для сортировки");

        QList<Book>   books;
        QList<Reader> readers;

        {
            QMutexLocker lock(&shared.mutex);
            while (!shared.loaded && shared.error.isEmpty())
                shared.cond.wait(&shared.mutex);

            if (!shared.error.isEmpty())
                return;

            books   = shared.books;
            readers = shared.readers;
        }

        std::sort(books.begin(), books.end(),
                  [](const Book &a, const Book &b) { return a.name < b.name; });

        std::sort(readers.begin(), readers.end(),
                  [](const Reader &a, const Reader &b) {
                      if (a.second_name != b.second_name)
                          return a.second_name < b.second_name;
                      return a.first_name < b.first_name;
                  });

        {
            QMutexLocker lock(&shared.mutex);
            shared.sortedBooks   = books;
            shared.sortedReaders = readers;
            shared.edited = true;
        }
        shared.cond.wakeAll();
        spdlog::debug("t2: сортировка завершена");
    });

    std::thread t3([&shared, fileName]() {
        spdlog::debug("t3: ожидание отсортированных данных");

        QList<Book>   books;
        QList<Reader> readers;

        {
            QMutexLocker lock(&shared.mutex);
            while (!shared.edited && shared.error.isEmpty())
                shared.cond.wait(&shared.mutex);

            if (!shared.error.isEmpty())
                return;

            books   = shared.sortedBooks;
            readers = shared.sortedReaders;
        }

        const QString html = buildReportHtmlFromData(books, readers);

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            setThreadError(shared, "Не удалось открыть файл для записи HTML-отчёта");
            return;
        }

        QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        out.setEncoding(QStringConverter::Utf8);
#else
        out.setCodec("UTF-8");
#endif
        out << html;
        file.close();

        spdlog::info("t3: отчёт записан в файл {}", fileName.toStdString());
    });

    t1.join();
    t2.join();
    t3.join();

    if (!shared.error.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", shared.error);
        spdlog::warn("Экспорт завершился с ошибкой: {}", shared.error.toStdString());
    } else {
        spdlog::info("Экспорт HTML-отчёта успешно завершён");
        QMessageBox::information(this, "Отчёт",
                                 "HTML-отчёт (многопоточный) успешно сохранён");
    }
}






/**
 * @brief Слот: добавление книги через диалог.
 */
void MainWindow::act_add_book()
{
    spdlog::debug("Открыт диалог добавления книги");
    QDialog dialog(this);
    dialog.setWindowTitle("Добавить книгу");

    QFormLayout form(&dialog);
    QLineEdit nameEdit, authorEdit;

    QPushButton okButton("Добавить"), cancelButton("Отмена");
    form.addRow("Название:", &nameEdit);
    form.addRow("Автор:", &authorEdit);
    form.addRow(&okButton, &cancelButton);

    connect(&okButton, &QPushButton::clicked, [&]() {
        try {
            InputValid::checkAddBook(nameEdit.text(), authorEdit.text());
            dialog.accept();

        } catch (const AppException &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
            spdlog::warn("Ошибка при добавлении книги: {}", ex.what());
        } catch (const std::exception &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
            spdlog::warn("Ошибка при добавлении книги: {}", ex.what());
        }
    });

    connect(&cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        Book book;
        book.code = BookModel::GenerateBookCode(bookModel_->GetBooks());
        book.name = nameEdit.text().trimmed();
        book.author = authorEdit.text().trimmed();
        book.is_taken = false;
        book.date_taken = std::nullopt;
        bookModel_->AddBook(book);

        spdlog::info("Добавлена книга: код={}, имя={}, автор={}",
                     book.code.toStdString(),
                     book.name.toStdString(),
                     book.author.toStdString());

        QMessageBox::information(this, "Добавление книги", "Книга успешно создана");
    }
}

/**
 * @brief Слот: добавление читателя через диалог.
 */
void MainWindow::act_add_reader()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Добавить читателя");

    QFormLayout form(&dialog);
    QLineEdit firstEdit, secondEdit, thirdEdit;
    QComboBox sexCombo;
    sexCombo.addItem("Мужчина", true);
    sexCombo.addItem("Женщина", false);

    QPushButton okButton("Добавить"), cancelButton("Отмена");
    form.addRow("Фамилия:", &firstEdit);
    form.addRow("Имя:", &secondEdit);
    form.addRow("Отчество:", &thirdEdit);
    form.addRow("Пол:", &sexCombo);
    form.addRow(&okButton, &cancelButton);

    connect(&cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    connect(&okButton, &QPushButton::clicked, [&]() {
        try {
            const QString th = thirdEdit.text().trimmed();
            const std::optional<QString> thOpt = th.isEmpty()
                                                     ? std::nullopt
                                                     : std::optional<QString>(th);

            InputValid::checkAddReader(firstEdit.text(), secondEdit.text(), thOpt);
            dialog.accept();
        } catch (const AppException &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
        } catch (const std::exception &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
        }
    });

    if (dialog.exec() == QDialog::Accepted) {
        Reader reader;
        reader.ID = ReaderModel::GenerateReaderID(readerModel_->GetReaders());
        reader.first_name  = firstEdit.text().trimmed();
        reader.second_name = secondEdit.text().trimmed();
        reader.third_name  = thirdEdit.text().trimmed();
        reader.gender = sexCombo.currentData().toBool() ? Sex::Male : Sex::Female;
        reader.reg_date = QDate::currentDate();
        readerModel_->AddReader(reader);

        QMessageBox::information(this, "Добавление", "Читатель успешно добавлен!");
    }
}

/**
 * @brief Слот: редактирование книги через диалог.
 */
void MainWindow::act_edit_book()
{
    QModelIndex index = ui->tbl_view_book->currentIndex();
    if (!index.isValid()) {
        QMessageBox::warning(this, "Ошибка", "Выберите книгу для редактирования");
        return;
    }

    if (auto proxy = qobject_cast<QSortFilterProxyModel*>(ui->tbl_view_book->model()))
        index = proxy->mapToSource(index);

    const int row = index.row();
    const QList<Book> &books = bookModel_->GetBooks();
    if (row < 0 || row >= books.size())
        return;

    Book book = books[row];

    QDialog dialog(this);
    dialog.setWindowTitle("Редактировать книгу");

    QFormLayout form(&dialog);
    QLineEdit codeEdit(book.code);
    QLineEdit nameEdit(book.name);
    QLineEdit authorEdit(book.author);
    QLabel statusLabel(book.is_taken ? "Выдана" : "В наличии");

    QPushButton okButton("Сохранить"), cancelButton("Отмена");
    form.addRow("Шифр:", &codeEdit);
    form.addRow("Название:", &nameEdit);
    form.addRow("Автор:", &authorEdit);
    form.addRow("Статус:", &statusLabel);
    form.addRow(&okButton, &cancelButton);

    connect(&cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    connect(&okButton, &QPushButton::clicked, [&]() {
        try {
            InputValid::checkEditBook(nameEdit.text(), authorEdit.text());

            QString newCode = codeEdit.text().trimmed().toUpper();
            if (newCode.isEmpty())
                throw InvalidInputException("Код книги не может быть пустым");

            static const QRegularExpression codeRe("^B[0-9]{3,5}$");
            if (!codeRe.match(newCode).hasMatch())
                throw InvalidInputException("Неверный формат кода книги (ожидается BXXXX)");

            if (newCode != book.code) {
                auto existingIdx = bookModel_->FindBookIndex(newCode);
                if (existingIdx.has_value())
                    throw InvalidInputException("Книга с таким кодом уже существует");
            }

            dialog.accept();
        } catch (const AppException &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
        } catch (const std::exception &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
        }
    });

    if (dialog.exec() == QDialog::Accepted) {
        const QString oldCode = book.code;
        const QString newCode = codeEdit.text().trimmed().toUpper();

        book.code   = newCode;
        book.name   = nameEdit.text().trimmed();
        book.author = authorEdit.text().trimmed();

        bookModel_->UpdateBookAt(row, book);

        if (newCode != oldCode)
            readerModel_->UpdateBookCodeForAllReaders(oldCode, newCode);

        QMessageBox::information(this, "Редактирование книги", "Изменения применены");
    }
}


/**
 * @brief Слот: редактирование читателя через диалог.
 */
void MainWindow::act_edit_reader()
{
    QModelIndex index = ui->tbl_view_reader->currentIndex();
    if (!index.isValid()) {
        QMessageBox::warning(this, "Ошибка", "Выберите читателя для редактирования");
        return;
    }

    if (auto proxy = qobject_cast<QSortFilterProxyModel*>(ui->tbl_view_reader->model()))
        index = proxy->mapToSource(index);

    const int row = index.row();
    const QList<Reader> &readers = readerModel_->GetReaders();
    if (row < 0 || row >= readers.size())
        return;

    Reader reader = readers[row];

    QDialog dialog(this);
    dialog.setWindowTitle("Редактировать читателя");

    QFormLayout form(&dialog);
    QLineEdit firstEdit(reader.second_name); // фамилия
    QLineEdit secondEdit(reader.first_name); // имя
    QLineEdit thirdEdit(reader.third_name);

    QComboBox sexCombo;
    sexCombo.addItem("Мужчина", true);
    sexCombo.addItem("Женщина", false);
    sexCombo.setCurrentIndex(reader.gender == Sex::Male ? 0 : 1);

    QPushButton okButton("Сохранить"), cancelButton("Отмена");
    form.addRow("Фамилия:", &firstEdit);
    form.addRow("Имя:", &secondEdit);
    form.addRow("Отчество:", &thirdEdit);
    form.addRow("Пол:", &sexCombo);
    form.addRow(&okButton, &cancelButton);

    connect(&cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    connect(&okButton, &QPushButton::clicked, [&]() {
        try {
            const QString th = thirdEdit.text().trimmed();
            const std::optional<QString> thOpt = th.isEmpty()
                                                     ? std::nullopt
                                                     : std::optional<QString>(th);

            InputValid::checkAddReader(firstEdit.text(), secondEdit.text(), thOpt);
            dialog.accept();
        } catch (const AppException &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
        } catch (const std::exception &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
        }
    });

    if (dialog.exec() == QDialog::Accepted) {
        reader.second_name = firstEdit.text().trimmed(); // фамилия
        reader.first_name  = secondEdit.text().trimmed(); // имя
        reader.third_name  = thirdEdit.text().trimmed();
        reader.gender = sexCombo.currentData().toBool() ? Sex::Male : Sex::Female;

        readerModel_->UpdateReaderAt(row, reader);

        QMessageBox::information(this, "Редактирование читателя", "Изменения применены!");
    }
}



/**
 * @brief Слот: выдача книги читателю.
 */
void MainWindow::act_giveout_book()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Выдать книгу");

    QFormLayout form(&dialog);
    QLineEdit bookCodeEdit;
    QLineEdit readerIdEdit;
    QPushButton okButton("Выдать"), cancelButton("Отмена");

    form.addRow("Код книги:", &bookCodeEdit);
    form.addRow("ID читателя:", &readerIdEdit);
    form.addRow(&okButton, &cancelButton);

    connect(&cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    connect(&okButton, &QPushButton::clicked, [&]() {
        try {
            InputValid::checkGiveOutInput(bookCodeEdit.text(), readerIdEdit.text());
            dialog.accept();
        } catch (const AppException &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
        } catch (const std::exception &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
        }
    });

    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString code     = bookCodeEdit.text().trimmed().toUpper();
    const QString readerID = readerIdEdit.text().trimmed().toUpper();

    try {
        auto bookIndexOpt = bookModel_->FindBookIndex(code);
        if (!bookIndexOpt.has_value())
            throw BookNotFoundException("Книга не найдена");

        const Book book = bookModel_->GetBooks().at(bookIndexOpt.value());
        if (book.is_taken)
            throw BookAlreadyTakenException("Эта книга уже выдана");

        auto readerIndexOpt = readerModel_->FindReaderIndex(readerID);
        if (!readerIndexOpt.has_value())
            throw ReaderNotFoundException("Читатель не найден");

        if (!readerModel_->AddLinkBook(readerID, code))
            throw InvalidInputException("Не удалось закрепить книгу за читателем");

        std::optional<QDate> now_date = QDate::currentDate();
        if (!bookModel_->SetBookTaken(code, true, now_date))
            throw InvalidInputException("Не удалось обновить статус книги");

        QMessageBox::information(this, "Успех", "Книга успешно выдана!");
    } catch (const AppException &ex) {
        QMessageBox::warning(this, "Ошибка", ex.what());
    } catch (const std::exception &ex) {
        QMessageBox::warning(this, "Ошибка", ex.what());
    }
}


/**
 * @brief Слот: удаление книги с подтверждением.
 */
void MainWindow::act_delete_book()
{
    QModelIndex index = ui->tbl_view_book->currentIndex();
    if (!index.isValid()) {
        QMessageBox::warning(this, "Ошибка", "Выберите книгу для удаления");
        return;
    }

    index = mapToSourceIndex(ui->tbl_view_book, index);

    const QString code = bookModel_->data(index.siblingAtColumn(0), Qt::DisplayRole).toString().trimmed();

    const Book book = bookModel_->FindBook(code); // может быть пустая, если не найдена
    const QString title = book.name.isEmpty() ? QString("(название неизвестно)") : book.name;

    const int ret = QMessageBox::question(
        this,
        "Подтвердите",
        QString("Удалить книгу %1 - %2 ?").arg(code, title),
        QMessageBox::Yes | QMessageBox::No
        );

    if (ret != QMessageBox::Yes)
        return;

    try {
        const bool ok = bookModel_->RemoveBook(code);
        if (!ok) {
            QMessageBox::warning(this, "Ошибка",
                                 "Не удалось удалить книгу (возможно, книга не найдена или привязана к читателю)");
        } else {
            QMessageBox::information(this, "Удаление книги", "Книга успешно удалена");
        }
    } catch (const AppException &ex) {
        QMessageBox::warning(this, "Ошибка", ex.what());
    } catch (const std::exception &ex) {
        QMessageBox::warning(this, "Ошибка", ex.what());
    }
}



/**
 * @brief Слот: удаление читателя с подтверждением.
 */
void MainWindow::act_delete_reader()
{
    QModelIndex index = ui->tbl_view_reader->currentIndex();
    if (!index.isValid()) {
        QMessageBox::warning(this, "Ошибка", "Выберите читателя для удаления");
        return;
    }

    index = mapToSourceIndex(ui->tbl_view_reader, index);

    const QString id = readerModel_->data(index.siblingAtColumn(0), Qt::DisplayRole).toString();

    // Было опасно: FindReader(id)->... (если не найдено — краш)
    const auto readerOpt = readerModel_->FindReader(id);

    QString prompt;
    if (readerOpt.has_value()) {
        prompt = QString("Удалить читателя %1 - %2 %3 ?")
                     .arg(id)
                     .arg(readerOpt->second_name)
                     .arg(readerOpt->first_name);
    } else {
        // На всякий случай: пусть удаление всё равно возможно, но без ФИО
        prompt = QString("Удалить читателя %1 ?").arg(id);
    }

    int ret = QMessageBox::question(this, "Подтвердите", prompt,
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes)
        return;

    try {
        bool ok = readerModel_->RemoveReader(id);
        if (!ok) {
            QMessageBox::warning(this, "Ошибка",
                                 "Не удалось удалить читателя (возможно, читатель не найден или есть выданные книги)");
        } else {
            QMessageBox::information(this, "Удаление читателя", "Читатель успешно удален");
        }
    } catch (const AppException &ex) {
        QMessageBox::warning(this, "Ошибка", ex.what());
    } catch (const std::exception &ex) {
        QMessageBox::warning(this, "Ошибка", ex.what());
    }
}



/**
 * @brief Слот: поиск книги по коду или названию.
 */
void MainWindow::act_search_book()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Поиск книги");

    QFormLayout form(&dialog);
    QLineEdit searchEdit;
    QPushButton okButton("Найти"), cancelButton("Отмена");

    form.addRow("Код или название:", &searchEdit);
    form.addRow(&okButton, &cancelButton);

    connect(&cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    connect(&okButton, &QPushButton::clicked, [&]() {
        try {
            InputValid::checkBookSearch(searchEdit.text());
            dialog.accept();
        } catch (const AppException &ex) {
            QMessageBox::warning(&dialog, "Ошибка ввода", ex.what());
        } catch (const std::exception &ex) {
            showExceptionBox(&dialog, "Ошибка ввода", ex);
        }
    });

    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString queryRaw = searchEdit.text().trimmed();
    const QString queryCode = queryRaw.toUpper(); // для поиска по коду устойчивее
    const QString queryName = queryRaw;           // для поиска по названию оставим как есть

    try {
        // 1) Поиск по коду (точное совпадение)
        const Book bookByCode = bookModel_->FindBook(queryCode);
        if (!bookByCode.code.isEmpty()) {
            QMessageBox::information(
                this, "Найдено",
                QString("Код: %1\nНазвание: %2\nАвтор: %3\nСостояние: %4")
                    .arg(bookByCode.code,
                         bookByCode.name,
                         bookByCode.author,
                         bookByCode.is_taken ? "Выдана" : "Свободна"));
            return;
        }

        // 2) Поиск по названию (подстрока, без учёта регистра)
        const QList<Book> &books = bookModel_->GetBooks();
        QList<Book> matches;
        matches.reserve(4);

        for (const Book &b : books) {
            if (b.name.contains(queryName, Qt::CaseInsensitive))
                matches.append(b);
        }

        if (matches.isEmpty())
            throw BookNotFoundException("Книга не найдена!");

        if (matches.size() == 1) {
            const Book &b = matches.first();
            QMessageBox::information(
                this, "Найдена книга",
                QString("Код: %1\nНазвание: %2\nАвтор: %3\nСостояние: %4")
                    .arg(b.code, b.name, b.author, b.is_taken ? "Выдана" : "Свободна"));
            return;
        }

        // 3) Несколько книг — список
        QStringList lines;
        for (const Book &b : matches) {
            lines << QString("%1 — %2 (%3) [%4]")
                         .arg(b.code, b.name, b.author,
                              b.is_taken ? "выдана" : "в наличии");
        }

        QMessageBox::information(
            this, "Найдено несколько книг",
            "Под ваш запрос найдено несколько книг:\n\n" + lines.join("\n"));
    } catch (const AppException &ex) {
        QMessageBox::warning(this, "Ошибка", ex.what());
    } catch (const std::exception &ex) {
        showExceptionBox(this, "Ошибка", ex);
    }
}


/**
 * @brief Слот: поиск читателя по ID или ФИО.
 */
void MainWindow::act_search_reader()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Поиск читателя");

    QFormLayout form(&dialog);
    QLineEdit searchEdit;
    QPushButton okButton("Найти"), cancelButton("Отмена");

    form.addRow("ID или ФИО:", &searchEdit);
    form.addRow(&okButton, &cancelButton);

    connect(&cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    connect(&okButton, &QPushButton::clicked, [&]() {
        if (searchEdit.text().trimmed().isEmpty()) {
            QMessageBox::warning(&dialog, "Ошибка", "Введите ID или ФИО для поиска");
            return;
        }
        dialog.accept();
    });

    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString query = searchEdit.text().trimmed();
    const QString queryUp = query.toUpper();

    try {
        // 1) Пробуем найти по ID (точно)
        auto readerOpt = readerModel_->FindReader(queryUp);

        // 2) Если не нашли — ищем по ФИО (подстрока)
        if (!readerOpt.has_value()) {
            for (const Reader &r : readerModel_->GetReaders()) {
                if (r.ID.compare(queryUp, Qt::CaseInsensitive) == 0 ||
                    r.fullName().contains(query, Qt::CaseInsensitive)) {
                    readerOpt = r;
                    break;
                }
            }
        }

        if (!readerOpt.has_value())
            throw ReaderNotFoundException("Читатель не найден");

        const Reader &r = readerOpt.value();
        QMessageBox::information(this, "Найдено",
                                 QString("ID: %1\nФИО: %2 %3 %4")
                                     .arg(r.ID, r.first_name, r.second_name, r.third_name));

        ui->tabWidget->setCurrentIndex(1);
    } catch (const AppException &ex) {
        QMessageBox::warning(this, "Не найдено", ex.what());
    } catch (const std::exception &ex) {
        showExceptionBox(this, "Не найдено", ex);
    }
}


/**
 * @brief Слот: получение информации (пока не реализовано).
 */
void MainWindow::act_get_info()
{
    QModelIndex index = ui->tbl_view_reader->currentIndex();
    if (!index.isValid()) {
        QMessageBox::warning(this, "Информация",
                             "Сначала выберите читателя в таблице");
        return;
    }

    index = mapToSourceIndex(ui->tbl_view_reader, index);

    const QList<Reader> &readers = readerModel_->GetReaders();
    const int row = index.row();
    if (row < 0 || row >= readers.size())
        return;

    const Reader &r = readers[row];

    const QString fio = r.fullName();
    const QString genderStr = (r.gender == Sex::Male) ? "Мужской" : "Женский";

    QString msg;
    msg += "ID: " + r.ID + "\n";
    msg += "ФИО: " + fio + "\n";
    msg += "Пол: " + genderStr + "\n";

    if (r.reg_date.isValid())
        msg += "Дата регистрации: " + r.reg_date.toString("dd.MM.yyyy") + "\n";

    msg += "\nКниги на руках:\n";

    if (r.taken_books.isEmpty()) {
        msg += "— нет выданных книг";
    } else {
        QStringList lines;
        for (const QString &code : r.taken_books) {
            const Book b = bookModel_->FindBook(code);

            if (b.code.isEmpty()) {
                lines << QString("%1 — (книга не найдена в каталоге)").arg(code);
            } else {
                lines << QString("%1 — %2 (%3)").arg(b.code, b.name, b.author);
            }
        }
        msg += lines.join("\n");
    }

    QMessageBox::information(this, "Информация о читателе", msg);
}



/**
 * @brief Обновление состояния действий в зависимости от выбранной вкладки.
 * @param index Индекс текущей вкладки
 */
void MainWindow::act_UpdateActionStates(int index) {
    bool isBooksTab = (index == 0);
    ui->mb_act_add_book->setEnabled(isBooksTab);
    ui->mb_act_delete_book->setEnabled(isBooksTab);
    ui->mb_act_add_reader->setEnabled(!isBooksTab);
    ui->mb_act_delete_reader->setEnabled(!isBooksTab && ui->tbl_view_reader->currentIndex().isValid());
}

/**
 * @brief Слот: возврат книги от читателя.
 */
void MainWindow::act_return_book()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Возврат книги");

    QFormLayout form(&dialog);
    QLineEdit bookCodeEdit;
    QLineEdit readerIdEdit;
    QPushButton okButton("Вернуть"), cancelButton("Отмена");

    form.addRow("Код книги:", &bookCodeEdit);
    form.addRow("ID читателя:", &readerIdEdit);
    form.addRow(&okButton, &cancelButton);

    connect(&cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    connect(&okButton, &QPushButton::clicked, [&]() {
        const QString code = bookCodeEdit.text().trimmed();
        const QString id   = readerIdEdit.text().trimmed();

        // Сохраняем твой текст сообщения "Введите код книги и ID читателя"
        if (code.isEmpty() || id.isEmpty()) {
            QMessageBox::warning(&dialog, "Ошибка", "Введите код книги и ID читателя");
            return;
        }

        try {
            InputValid::checkGiveOutInput(code, id);
            dialog.accept();
        } catch (const AppException &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
        } catch (const std::exception &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
        }
    });

    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString code     = bookCodeEdit.text().trimmed().toUpper();
    const QString readerID = readerIdEdit.text().trimmed().toUpper();

    try {
        auto bookIndexOpt = bookModel_->FindBookIndex(code);
        if (!bookIndexOpt.has_value())
            throw BookNotFoundException("Книга с таким кодом не найдена");

        const Book book = bookModel_->GetBooks().at(bookIndexOpt.value());
        if (!book.is_taken)
            throw BookAlreadyAvailableException("Эта книга уже в наличии");

        auto readerIndexOpt = readerModel_->FindReaderIndex(readerID);
        if (!readerIndexOpt.has_value())
            throw ReaderNotFoundException("Читатель с таким ID не найден");

        if (!readerModel_->RemoveLinkBook(readerID, code))
            throw InvalidInputException("Не удалось удалить книгу у читателя");

        std::optional<QDate> date_null = std::nullopt;
        if (!bookModel_->SetBookTaken(code, false, date_null))
            throw InvalidInputException("Не удалось обновить статус книги");

        QMessageBox::information(this, "Успех", "Книга успешно возвращена!");
    } catch (const AppException &ex) {
        QMessageBox::warning(this, "Ошибка", ex.what());
    } catch (const std::exception &ex) {
        QMessageBox::warning(this, "Ошибка", ex.what());
    }
}


/**
 * @brief Слот: кнопка "Получить книгу" вызывает возврат книги.
 */
void MainWindow::on_pb_getBook_clicked() {
    act_return_book();
}

/**
 * @brief Обработчик кнопки «Информация о читателе».
 */
void MainWindow::on_pb_get_info_reader_clicked()
{
    act_get_info();
}
