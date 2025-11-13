/**
 * @file mainwindow.cpp
 * @brief Реализация главного окна приложения библиотеки (Qt/C++/CMake).
 */

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "Exception.h"

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

    // Инициализация моделей
    bookModel_ = new BookModel(this);
    readerModel_ = new ReaderModel(this);

    // --- КНИГИ: сначала JSON, потом XML, потом тестовые данные ---
    bool booksLoadedJson = bookModel_->LoadFromFile("books.json");
    if (!booksLoadedJson || bookModel_->GetBooks().isEmpty()) {
        bool booksLoadedXml = bookModel_->LoadFromXml("books.xml");
        if (!booksLoadedXml || bookModel_->GetBooks().isEmpty()) {
            bookModel_->AddBook({"B001", "Война и мир", "Лев Толстой",
                                 true, QDate::fromString("14/04/2006", "dd/MM/yyyy")});
            bookModel_->AddBook({"B002", "Преступление и наказание",
                                 "Фёдор Достоевский", false, std::nullopt});
        }
    }

    // --- ЧИТАТЕЛИ: сначала JSON, потом XML, потом тестовые данные ---
    bool readersLoadedJson = readerModel_->LoadFromFile("readers.json");
    if (!readersLoadedJson || readerModel_->GetReaders().isEmpty()) {
        bool readersLoadedXml = readerModel_->LoadFromXml("readers.xml");
        if (!readersLoadedXml || readerModel_->GetReaders().isEmpty()) {
            readerModel_->AddReader({"R001", "Иванов", "Иван", "Иванович",
                                     Sex::Male, {"B002"}});
            readerModel_->AddReader({"R002", "Петрова", "Анна", "Сергеевна",
                                     Sex::Female, {}});
        }
    }


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

    // Соединение действий меню с методами
    connect(ui->mb_act_save_file, &QAction::triggered, this, &MainWindow::act_save_file);
    connect(ui->mb_act_close_app, &QAction::triggered, this, &MainWindow::act_close_app);
    connect(ui->mb_act_add_book, &QAction::triggered, this, &MainWindow::act_add_book);
    connect(ui->mb_act_add_reader, &QAction::triggered, this, &MainWindow::act_add_reader);
    connect(ui->mb_act_delete_book, &QAction::triggered, this, &MainWindow::act_delete_book);
    connect(ui->mb_act_delete_reader, &QAction::triggered, this, &MainWindow::act_delete_reader);
    connect(ui->mb_act_search_book, &QAction::triggered, this, &MainWindow::act_search_book);
    connect(ui->mb_act_search_reader, &QAction::triggered, this, &MainWindow::act_search_reader);

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
    if (name.trimmed().isEmpty())
        throw EmptyBookNameException("Введите название книги!");
    if (author.trimmed().isEmpty())
        throw EmptyAuthorException("Введите автора книги!");
    if (name.trimmed().length() < 3)
        throw InvalidBookNameException("Слишком короткое название книги!");
    if (author.trimmed().length() < 3)
        throw InvalidAuthorException("Некорректное имя автора!");
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
void MainWindow::checkAddReader(const QString &surname, const QString &name)
{
    QRegularExpression validNameRegex("^[A-Za-zА-Яа-яЁё\\- ]+$");

    if (surname.trimmed().isEmpty())
        throw EmptyReaderSurnameException("Введите фамилию читателя!");
    if (!validNameRegex.match(surname.trimmed()).hasMatch())
        throw EmptyReaderSurnameException("Фамилия может содержать только буквы, пробел или дефис!");
    if (name.trimmed().isEmpty())
        throw EmptyReaderNameException("Введите имя читателя!");
    if (!validNameRegex.match(name.trimmed()).hasMatch())
        throw EmptyReaderNameException("Имя может содержать только буквы, пробел или дефис!");
}

/**
 * @brief Слот: добавление книги через диалог.
 */
void MainWindow::act_add_book()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Добавить книгу");

    QFormLayout form(&dialog);
    QLineEdit nameEdit, authorEdit;
    QComboBox availableCombo;
    availableCombo.addItem("В наличии", false);
    availableCombo.addItem("Выдана", true);

    QPushButton okButton("Добавить"), cancelButton("Отмена");
    form.addRow("Название:", &nameEdit);
    form.addRow("Автор:", &authorEdit);
    form.addRow("Статус:", &availableCombo);
    form.addRow(&okButton, &cancelButton);

    connect(&okButton, &QPushButton::clicked, [&]() {
        try {
            checkAddBook(nameEdit.text(), authorEdit.text());
            dialog.accept();
        } catch (const AppException &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
        } catch (const std::exception &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
        }
    });

    connect(&cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        Book book;
        book.code = BookModel::GenerateBookCode(bookModel_->GetBooks());
        book.name = nameEdit.text().trimmed();
        book.author = authorEdit.text().trimmed();
        book.is_taken = availableCombo.currentData().toBool();
        book.date_taken = std::nullopt;
        bookModel_->AddBook(book);

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
            checkAddReader(firstEdit.text(), secondEdit.text());
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
    QLineEdit nameEdit(book.name);
    QLineEdit authorEdit(book.author);
    QComboBox statusCombo;
    statusCombo.addItem("В наличии", false);
    statusCombo.addItem("Выдана", true);
    statusCombo.setCurrentIndex(book.is_taken ? 1 : 0);

    QPushButton okButton("Сохранить"), cancelButton("Отмена");
    form.addRow("Название:", &nameEdit);
    form.addRow("Автор:", &authorEdit);
    form.addRow("Статус:", &statusCombo);
    form.addRow(&okButton, &cancelButton);

    connect(&cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(&okButton, &QPushButton::clicked, [&]() {
        try {
            checkEditBook(nameEdit.text(), authorEdit.text());
            dialog.accept();
        } catch (const AppException &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
        } catch (const std::exception &ex) {
            QMessageBox::warning(&dialog, "Ошибка", ex.what());
        }
    });

    if (dialog.exec() == QDialog::Accepted) {
        book.name = nameEdit.text().trimmed();
        book.author = authorEdit.text().trimmed();
        book.is_taken = statusCombo.currentData().toBool();
        book.date_taken = book.is_taken ? book.date_taken : std::optional<QDate>(std::nullopt);
        bookModel_->UpdateBookAt(row, book);

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
            checkAddReader(firstEdit.text(), secondEdit.text());
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
    if (code.trimmed().isEmpty() || readerID.trimmed().isEmpty())
        throw InvalidInputException("Введите код книги и ID читателя!");
    if (code.trimmed().length() < 2)
        throw InvalidInputException("Неверный код книги!");
    if (readerID.trimmed().length() < 2)
        throw InvalidInputException("Неверный ID читателя!");
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
        QMessageBox::information(this, "Удаление книги", "Книга успешно удалена");
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
        QMessageBox::information(this, "Удаление читателя", "Читатель успешно удален");
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

    connect(&cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);
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

    if (dialog.exec() == QDialog::Accepted) {
        QString query = searchEdit.text().trimmed();
        try {
            Book book = bookModel_->FindBook(query);
            if (book.code.isEmpty())
                throw BookNotFoundException("Книга не найдена!");

            QMessageBox::information(this, "Найдено",
                                     QString("Код %1\nНазвание: %2\nАвтор: %3\nСостояние: %4")
                                         .arg(book.code, book.name, book.author, book.is_taken ? "Выдана" : "Свободна"));
        } catch (const AppException &ex) {
            QMessageBox::warning(this, "Ошибка", ex.what());
        } catch (const std::exception &ex) {
            QMessageBox::warning(this, "Ошибка", ex.what());
        }
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
                    QString fullName = r.first_name + " " + r.second_name + " " + r.third_name;
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
    QMessageBox::information(this, "Информация", "Функция пока не реализована");
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
