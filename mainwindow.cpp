/**
 * @file mainwindow.cpp
 * @brief Реализация главного окна приложения библиотеки (Qt/C++/CMake).
 */

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "Exception.h"
#include "spdlog/spdlog.h"
#include "DatabaseManager.h"

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

static auto logMainWindow = spdlog::get("main");


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
    bookModel_->SaveToFile("books.json");
    readerModel_->SaveToFile("readers.json");

    bookModel_->SaveToXml("books.xml");
    readerModel_->SaveToXml("readers.xml");

    delete ui;
}

/**
 * @brief Слоты для кнопок (делегируют вызов соответствующих действий)
 */
void MainWindow::on_pb_addBook_clicked() { act_add_book(); }
void MainWindow::on_pb_editBook_clicked() { act_edit_book(); }
void MainWindow::on_pb_deleteBook_clicked() { act_delete_book(); }
void MainWindow::on_pb_searchBook_clicked() { act_search_book(); }
void MainWindow::on_pb_addReader_clicked() { act_add_reader(); }
void MainWindow::on_pb_editReader_clicked() { act_edit_reader(); }
void MainWindow::on_pb_deleteReader_clicked() { act_delete_reader(); }
void MainWindow::on_pb_searchReader_clicked() { act_search_reader(); }
void MainWindow::on_pb_giveBook_clicked() { act_giveout_book(); }

/**
 * @brief Сохраняет книги и читателей в файл.
 */
void MainWindow::act_save_file()
{
    bool okBooksJson   = bookModel_->SaveToFile("books.json");
    bool okReadersJson = readerModel_->SaveToFile("readers.json");

    bool okBooksXml    = bookModel_->SaveToXml("books.xml");
    bool okReadersXml  = readerModel_->SaveToXml("readers.xml");

    if (okBooksJson && okReadersJson && okBooksXml && okReadersXml) {
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

QString MainWindow::buildFullReportHtml() const
{
    return buildReportHtmlFromData(bookModel_->GetBooks(),
                                   readerModel_->GetReaders());
}


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
        QMutexLocker lock(&shared.mutex);

        shared.books   = booksSnapshot;
        shared.readers = readersSnapshot;
        shared.loaded  = true;

        shared.cond.wakeAll();
        spdlog::debug("t1: копирование завершено, книг={}, читателей={}",
                      booksSnapshot.size(), readersSnapshot.size());
    });


    std::thread t2([&shared]() {
        spdlog::debug("t2: ожидание данных для сортировки");
        shared.mutex.lock();
        while (!shared.loaded && shared.error.isEmpty()) {
            shared.cond.wait(&shared.mutex);
        }

        if (!shared.error.isEmpty()) {
            shared.mutex.unlock();
            return;
        }


        QList<Book>   books   = shared.books;
        QList<Reader> readers = shared.readers;
        shared.mutex.unlock();


        std::sort(books.begin(), books.end(),
                  [](const Book &a, const Book &b) {
                      return a.name < b.name;
                  });

        std::sort(readers.begin(), readers.end(),
                  [](const Reader &a, const Reader &b) {
                      // пример: по фамилии, потом по имени
                      if (a.second_name != b.second_name)
                          return a.second_name < b.second_name;
                      return a.first_name < b.first_name;
                  });

        shared.mutex.lock();
        shared.sortedBooks   = books;
        shared.sortedReaders = readers;
        shared.edited = true;
        shared.cond.wakeAll();
        shared.mutex.unlock();
        spdlog::debug("t2: сортировка завершена");
    });


    std::thread t3([&shared, fileName]() {
        spdlog::debug("t3: ожидание отсортированных данных");
        QList<Book>   books;
        QList<Reader> readers;

        shared.mutex.lock();
        while (!shared.edited && shared.error.isEmpty()) {
            shared.cond.wait(&shared.mutex);
        }

        if (!shared.error.isEmpty()) {
            shared.mutex.unlock();
            return;
        }

        books   = shared.sortedBooks;
        readers = shared.sortedReaders;
        shared.mutex.unlock();


        BookModel   reportBooksModel;
        ReaderModel reportReadersModel;

        QString html = buildReportHtmlFromData(books, readers);


        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            shared.mutex.lock();
            shared.error = "Не удалось открыть файл для записи HTML-отчёта";
            shared.mutex.unlock();
            return;
        }

        QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        out.setEncoding(QStringConverter::Utf8);
#else
        out.setCodec("UTF-8");
#endif
        out << html;
        spdlog::info("t3: отчёт записан в файл {}", fileName.toStdString());
        file.close();
    });


    t1.join();
    t2.join();
    t3.join();


    if (!shared.error.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", shared.error);
        spdlog::warn("Экспорт завершился с ошибкой: {}",
                     shared.error.toStdString());
    } else {
        spdlog::info("Экспорт HTML-отчёта успешно завершён");
        QMessageBox::information(this, "Отчёт",
                                 "HTML-отчёт (многопоточный) успешно сохранён");
    }
}


static int countLetters(const QString &s) {
    int cnt = 0;
    for (QChar ch : s) {
        if (ch.isLetter())
            ++cnt;
    }
    return cnt;
}

/**
 * @brief Проверка корректности данных при добавлении книги.
 * @param name Название книги
 * @param author Автор книги
 * @throws EmptyBookNameException если название пустое
 * @throws EmptyAuthorException если автор пустой
 * @throws InvalidBookNameException если название слишком короткое
 * @throws InvalidAuthorException если имя автора некорректно
 */
void MainWindow::checkAddBook(const QString &name, const QString &author) {
    QString nameTrim = name.trimmed();
    QString authorTrim = author.trimmed();

    // Разрешаем буквы, цифры, пробел, запятую, точку и дефис
    QRegularExpression allowedRe("^[A-Za-zА-Яа-яЁё0-9 ,\\.\\-]+$");

    if (nameTrim.isEmpty())
        throw EmptyBookNameException("Введите название книги!");
    if (!allowedRe.match(nameTrim).hasMatch())
        throw InvalidBookNameException("Название может содержать только буквы, цифры, пробелы и знаки , . -");
    // Минимум 3 буквы, чтобы не было ",,," или "---"
    if (countLetters(nameTrim) < 3)
        throw InvalidBookNameException("В названии должно быть не меньше 3 букв!");
    if (authorTrim.isEmpty())
        throw EmptyAuthorException("Введите автора книги!");
    if (!allowedRe.match(authorTrim).hasMatch())
        throw InvalidAuthorException("Имя автора может содержать только буквы, цифры, пробелы и знаки , . -");
    if (countLetters(authorTrim) < 3)
        throw InvalidAuthorException("В имени автора должно быть не меньше 3 букв!");
}

/**
 * @brief Проверка корректности данных при редактировании книги.
 * @param name Название книги
 * @param author Автор книги
 */
void MainWindow::checkEditBook(const QString &name, const QString &author) {
    checkAddBook(name, author);
}

/**
 * @brief Проверка корректности данных при добавлении читателя.
 * @param surname Фамилия читателя
 * @param name Имя читателя
 * @throws EmptyReaderSurnameException если фамилия пустая или некорректная
 * @throws EmptyReaderNameException если имя пустое или некорректное
 */
void MainWindow::checkAddReader(const QString &surname, const QString &name , const std::optional<QString> & thname)
{
    QRegularExpression validNameRegex("^[A-Za-zА-Яа-яЁё\\- ]+$");

    QString sTrim = surname.trimmed();
    QString nTrim = name.trimmed();
    if (thname.has_value() && thname.value().trimmed().length() > 0) {
       QString thTrim = thname.value().trimmed();
        if (!validNameRegex.match(thTrim).hasMatch())
            throw EmptyReaderSurnameException("Отчество может содержать только буквы, пробел или дефис!");
        if (countLetters(thTrim) < 3)
            throw EmptyReaderSurnameException("Отчество должно содержать минимум 3 буквы!");
    }


    if (sTrim.isEmpty())
        throw EmptyReaderSurnameException("Введите фамилию читателя!");
    if (!validNameRegex.match(sTrim).hasMatch())
        throw EmptyReaderSurnameException("Фамилия может содержать только буквы, пробел или дефис!");
    if (countLetters(sTrim) < 3)
        throw EmptyReaderSurnameException("Фамилия должна содержать минимум 3 буквы!");

    if (nTrim.isEmpty())
        throw EmptyReaderNameException("Введите имя читателя!");
    if (!validNameRegex.match(nTrim).hasMatch())
        throw EmptyReaderNameException("Имя может содержать только буквы, пробел или дефис!");
    if (countLetters(nTrim) < 3)
        throw EmptyReaderNameException("Имя должно содержать минимум 3 буквы!");
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
            checkAddBook(nameEdit.text(), authorEdit.text());
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
            checkAddReader(firstEdit.text(), secondEdit.text() , thirdEdit.text() );
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
        reader.first_name = firstEdit.text().trimmed();
        reader.second_name = secondEdit.text().trimmed();
        reader.third_name = thirdEdit.text().trimmed();
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

    // Маппим индекс из proxy в исходную модель для корректности отображения
    if (auto proxy = qobject_cast<QSortFilterProxyModel*>(ui->tbl_view_book->model())) {
        index = proxy->mapToSource(index);
    }

    int row = index.row();
    const QList<Book> &books = bookModel_->GetBooks();
    if (row < 0 || row >= books.size()) return;

    Book book = books[row];

    QDialog dialog(this);
    dialog.setWindowTitle("Редактировать книгу");

    QFormLayout form(&dialog);
    QLineEdit codeEdit(book.code);
    QLineEdit nameEdit(book.name);
    QLineEdit authorEdit(book.author);
    QLabel statusLabel(book.is_taken ? "Выдана" : "В наличии");

    QPushButton okButton("Сохранить"), cancelButton("Отмена");
    form.addRow("Шифр:" , &codeEdit);
    form.addRow("Название:", &nameEdit);
    form.addRow("Автор:", &authorEdit);
    form.addRow("Статус:", &statusLabel);
    form.addRow(&okButton, &cancelButton);

    connect(&cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(&okButton, &QPushButton::clicked, [&]() {
        try {
            checkEditBook(nameEdit.text(), authorEdit.text());

            QString newCode = codeEdit.text().trimmed();
            if (newCode.isEmpty())
                throw InvalidInputException("Код книги не может быть пустым");


            QRegularExpression codeRe("^B[0-9]{3,5}$");
            if (!codeRe.match(newCode).hasMatch()) {
                 throw InvalidInputException("Неверный формат кода книги (ожидается BXXXX)");
            }

            // Проверка уникальности кода
            if (newCode != book.code) {
                auto existingIdx = bookModel_->FindBookIndex(newCode);
                if (existingIdx.has_value()) {
                    throw InvalidInputException("Книга с таким кодом уже существует");
                }
            }

            dialog.accept();
        } catch (const AppException &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
        } catch (const std::exception &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
        }
    });

    if (dialog.exec() == QDialog::Accepted) {
        QString oldCode = book.code;
        QString newCode = codeEdit.text().trimmed();

        book.code = newCode;
        book.name = nameEdit.text().trimmed();
        book.author = authorEdit.text().trimmed();
        book.is_taken = book.is_taken;
        book.date_taken = book.date_taken;
        bookModel_->UpdateBookAt(row, book);

        if (newCode != oldCode) {
            readerModel_->UpdateBookCodeForAllReaders(oldCode, newCode);
        }

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

    // Маппим индекс из proxy в исходную модель для корректности отображения
    if (auto proxy = qobject_cast<QSortFilterProxyModel*>(ui->tbl_view_reader->model())) {
        index = proxy->mapToSource(index);
    }

    int row = index.row();
    const QList<Reader> &readers = readerModel_->GetReaders();
    if (row < 0 || row >= readers.size()) return;

    Reader reader = readers[row];

    QDialog dialog(this);
    dialog.setWindowTitle("Редактировать читателя");

    QFormLayout form(&dialog);
    QLineEdit firstEdit(reader.first_name);
    QLineEdit secondEdit(reader.second_name);
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
            checkAddReader(firstEdit.text(), secondEdit.text() , thirdEdit.text());
            dialog.accept();
        } catch (const AppException &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
        } catch (const std::exception &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
        }
    });

    if (dialog.exec() == QDialog::Accepted) {
        reader.first_name = firstEdit.text().trimmed();
        reader.second_name = secondEdit.text().trimmed();
        reader.third_name = thirdEdit.text().trimmed();
        reader.gender = sexCombo.currentData().toBool() ? Sex::Male : Sex::Female;
        readerModel_->UpdateReaderAt(row, reader);

        QMessageBox::information(this, "Редактирование читателя", "Изменения применены!");
    }
}


/**
 * @brief Проверка корректности данных при выдаче книги.
 * @param code Код книги
 * @param readerID ID читателя
 * @throws InvalidInputException если поля пустые или слишком короткие
 */
void MainWindow::checkGiveOutInput(const QString &code, const QString &readerID) {
    QString c = code.trimmed();
    QString r = readerID.trimmed();

    if (c.isEmpty() || r.isEmpty())
        throw InvalidInputException("Введите код книги и ID читателя!");

    QRegularExpression codeRe("^B[0-9]{3,5}$");
    if (!codeRe.match(c).hasMatch())
        throw InvalidInputException("Неверный формат кода книги (ожидается BXXX)");

    QRegularExpression idRe("^R[0-9]{4}$");
    if (!idRe.match(r).hasMatch())
        throw InvalidInputException("Неверный формат ID читателя (ожидается RXXXX)");
}

/**
 * @brief Слот: выдача книги читателю.
 */
void MainWindow::act_giveout_book() {
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
            checkGiveOutInput(bookCodeEdit.text(), readerIdEdit.text());
            dialog.accept();
        } catch (const AppException &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
        } catch (const std::exception &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
        }
    });

    if (dialog.exec() != QDialog::Accepted) return;

    QString code = bookCodeEdit.text().trimmed();
    QString readerID = readerIdEdit.text().trimmed();

    try {
        auto bookIndexOpt = bookModel_->FindBookIndex(code);
        if (!bookIndexOpt.has_value())
            throw BookNotFoundException("Книга не найдена");

        int bookIndex = bookIndexOpt.value();
        Book book = bookModel_->GetBooks().at(bookIndex);
        if (book.is_taken)
            throw BookAlreadyTakenException("Эта книга уже выдана");

        auto readerIndexOpt = readerModel_->FindReaderIndex(readerID);
        if (!readerIndexOpt.has_value())
            throw ReaderNotFoundException("Читатель не найден");

        readerModel_->AddLinkBook(readerID, code);
        std::optional<QDate> now_date = QDate::currentDate();
        bookModel_->SetBookTaken(code, true, now_date);

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
void MainWindow::act_delete_book() {
    QModelIndex index = ui->tbl_view_book->currentIndex();
    if (!index.isValid()) {
        QMessageBox::warning(this, "Ошибка", "Выберите книгу для удаления");
        return;
    }
        // Маппим индекс из proxy в исходную модель для корректности отображения
    if (auto proxy = qobject_cast<QSortFilterProxyModel*>(ui->tbl_view_book->model())) {
        index = proxy->mapToSource(index);
    }

    QString code = bookModel_->data(index.siblingAtColumn(0), Qt::DisplayRole).toString();

    int ret = QMessageBox::question(this, "Подтвердите", QString("Удалить книгу %1 - %2 ?").arg(code).arg(bookModel_->FindBook(code).name),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    try {
        bool ok = bookModel_->RemoveBook(code);
        if (!ok) {
            QMessageBox::warning(this, "Ошибка", "Не удалось удалить книгу (возможно, книга не найдена или привязана к читателю)");
        }
        else {
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
void MainWindow::act_delete_reader() {
    QModelIndex index = ui->tbl_view_reader->currentIndex();
    if (!index.isValid()) {
        QMessageBox::warning(this, "Ошибка", "Выберите читателя для удаления");
        return;
    }

    // Маппим индекс из proxy в исходную модель для корректности отображения
    if (auto proxy = qobject_cast<QSortFilterProxyModel*>(ui->tbl_view_reader->model())) {
        index = proxy->mapToSource(index);
    }

    QString id = readerModel_->data(index.siblingAtColumn(0), Qt::DisplayRole).toString();

    int ret = QMessageBox::question(this, "Подтвердите", QString("Удалить читателя %1 - %2 %3 ?").arg(id).arg(readerModel_->FindReader(id)->second_name ).arg(readerModel_->FindReader(id)->first_name),
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    try {
        bool ok = readerModel_->RemoveReader(id);
        if (!ok) {
            QMessageBox::warning(this, "Ошибка", "Не удалось удалить читателя (возможно, читатель не найден или есть выданные книги)");
        }
        else {
           QMessageBox::information(this, "Удаление читателя", "Читатель успешно удален");
        }

    } catch (const AppException &ex) {
        QMessageBox::warning(this, "Ошибка", ex.what());
    } catch (const std::exception &ex) {
        QMessageBox::warning(this, "Ошибка", ex.what());
    }
}

/**
 * @brief Проверка ввода при поиске книги.
 * @param query Строка запроса
 * @throws InvalidInputException если строка пустая
 */
void MainWindow::checkBookSearch(const QString &query) {
    if (query.trimmed().isEmpty())
        throw InvalidInputException("Введите название или код книги!");
}

/**
 * @brief Слот: поиск книги по коду или названию.
 */
void MainWindow::act_search_book() {
    QDialog dialog(this);
    dialog.setWindowTitle("Поиск книги");

    QFormLayout form(&dialog);
    QLineEdit searchEdit;
    QPushButton okButton("Найти"), cancelButton("Отмена");
    form.addRow("Код или название:", &searchEdit);
    form.addRow(&okButton, &cancelButton);

    connect(&cancelButton, &QPushButton::clicked,
            &dialog, &QDialog::reject);
    connect(&okButton, &QPushButton::clicked, [&]() {
        try {
            checkBookSearch(searchEdit.text());
            dialog.accept();
        } catch (const AppException &ex) {
            QMessageBox::warning(&dialog, "Ошибка ввода", ex.what());
        } catch (const std::exception &ex) {
            QMessageBox::warning(&dialog, "Ошибка ввода", ex.what());
        }
    });

    if (dialog.exec() != QDialog::Accepted)
        return;

    QString query = searchEdit.text().trimmed();

    try {
        // 1️⃣ Сначала пробуем найти по коду (точное совпадение)
        Book book = bookModel_->FindBook(query);
        if (!book.code.isEmpty()) {
            QMessageBox::information(
                this, "Найдено",
                QString("Код: %1\nНазвание: %2\nАвтор: %3\nСостояние: %4")
                    .arg(book.code,
                         book.name,
                         book.author,
                         book.is_taken ? "Выдана" : "Свободна"));
            return;
        }

        // 2️⃣ Если по коду не нашли — ищем по названию (подстрока, без учёта регистра)
        const QList<Book> &books = bookModel_->GetBooks();
        QList<Book> matches;
        for (const Book &b : books) {
            if (b.name.contains(query, Qt::CaseInsensitive)) {
                matches.append(b);
            }
        }

        if (matches.isEmpty())
            throw BookNotFoundException("Книга не найдена!");

        // 3️⃣ Одна книга по названию
        if (matches.size() == 1) {
            const Book &b = matches.first();
            QMessageBox::information(
                this, "Найдена книга",
                QString("Код: %1\nНазвание: %2\nАвтор: %3\nСостояние: %4")
                    .arg(b.code,
                         b.name,
                         b.author,
                         b.is_taken ? "Выдана" : "Свободна"));
        } else {
            // 4️⃣ Несколько книг — показываем список
            QStringList lines;
            for (const Book &b : matches) {
                lines << QString("%1 — %2 (%3) [%4]")
                             .arg(b.code,
                                  b.name,
                                  b.author,
                                  b.is_taken ? "выдана" : "в наличии");
            }

            QMessageBox::information(
                this, "Найдено несколько книг",
                "Под ваш запрос найдено несколько книг:\n\n"
                    + lines.join("\n"));
        }
    } catch (const AppException &ex) {
        QMessageBox::warning(this, "Ошибка", ex.what());
    } catch (const std::exception &ex) {
        QMessageBox::warning(this, "Ошибка", ex.what());
    }
}

/**
 * @brief Слот: поиск читателя по ID или ФИО.
 */
void MainWindow::act_search_reader() {
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

    if (dialog.exec() == QDialog::Accepted) {
        QString query = searchEdit.text().trimmed();
        try {
            auto readerOpt = readerModel_->FindReader(query);
            if (!readerOpt) {
                for (const auto& r : readerModel_->GetReaders()) {
                    QString fullName = r.fullName();
                    if (fullName.contains(query, Qt::CaseInsensitive) || r.ID.compare(query, Qt::CaseInsensitive) == 0) {
                        readerOpt = r;
                        break;
                    }
                }
            }
            if (readerOpt) {
                Reader r = *readerOpt;
                QMessageBox::information(this, "Найдено", QString("ID: %1\nФИО: %2 %3 %4")
                                                              .arg(r.ID, r.first_name, r.second_name, r.third_name));
                ui->tabWidget->setCurrentIndex(1);
            } else {
                throw ReaderNotFoundException("Читатель не найден");
            }
        } catch (const AppException &ex) {
            QMessageBox::warning(this, "Не найдено", ex.what());
        } catch (const std::exception &ex) {
            QMessageBox::warning(this, "Не найдено", ex.what());
        }
    }
}

/**
 * @brief Слот: получение информации (пока не реализовано).
 */
void MainWindow::act_get_info() {
    QModelIndex index = ui->tbl_view_reader->currentIndex();
    if (!index.isValid()) {
        QMessageBox::warning(this, "Информация",
                             "Сначала выберите читателя в таблице");
        return;
    }

    // Если таблица завернута в proxy — размэппим
    if (auto proxy = qobject_cast<QSortFilterProxyModel*>(ui->tbl_view_reader->model())) {
        index = proxy->mapToSource(index);
    }

    const QList<Reader> &readers = readerModel_->GetReaders();
    int row = index.row();
    if (row < 0 || row >= readers.size())
        return;

    const Reader &r = readers[row];

    QString fio = r.fullName();
    QString genderStr = (r.gender == Sex::Male) ? "Мужской" : "Женский";

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
            Book b = bookModel_->FindBook(code);
            if (b.code.isEmpty()) {
                lines << QString("%1 — (книга не найдена в каталоге)").arg(code);
            } else {
                lines << QString("%1 — %2 (%3)")
                             .arg(b.code, b.name, b.author);
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
void MainWindow::act_return_book() {
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
        if (bookCodeEdit.text().trimmed().isEmpty() || readerIdEdit.text().trimmed().isEmpty()) {
            QMessageBox::warning(&dialog, "Ошибка", "Введите код книги и ID читателя");
            return;
        }
        dialog.accept();
    });

    if (dialog.exec() != QDialog::Accepted) return;

    QString code = bookCodeEdit.text().trimmed();
    QString readerID = readerIdEdit.text().trimmed();

    try {
        auto bookIndexOpt = bookModel_->FindBookIndex(code);
        if (!bookIndexOpt.has_value())
            throw BookNotFoundException("Книга с таким кодом не найдена");

        int bookIndex = bookIndexOpt.value();
        Book book = bookModel_->GetBooks().at(bookIndex);

        if (!book.is_taken)
            throw BookAlreadyAvailableException("Эта книга уже в наличии");

        auto readerIndexOpt = readerModel_->FindReaderIndex(readerID);
        if (!readerIndexOpt.has_value())
            throw ReaderNotFoundException("Читатель с таким ID не найден");

        if (!readerModel_->RemoveLinkBook(readerID, code))
            throw InvalidInputException("Не удалось удалить книгу у читателя");

        std::optional<QDate> date_null = (std::nullopt);
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

void MainWindow::on_pb_get_info_reader_clicked()
{
    act_get_info();
}

