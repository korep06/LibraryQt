/**
 * @file BookModel.cpp
 * @author Кирилл К
 * @brief Реализация методов модели книг
 * @version 1.0
 * @date 2024-12-19
 */

#include "BookModel.h"
#include "Exception.h"
#include "spdlog/spdlog.h"
#include "DatabaseManager.h"


#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QList>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QSqlQuery>
#include <QSqlError>


//BookModel::BookModel() {}

/**
 * @brief Конструктор модели книг
 * @param books Ссылка на список книг для инициализации модели
 * @param parent Родительский объект в иерархии Qt
 */
BookModel::BookModel(QObject *parent)
    : QAbstractTableModel(parent) {}

void BookModel::UpdateBookAt(int index, const Book &book)
{
    if (index < 0 || index >= books_.size())
        return;

    const QString oldCode = books_[index].code;
    books_[index] = book;
    // Уведомляем представление, что изменились данные в строке index
    emit dataChanged(this->index(index, 0), this->index(index, columnCount() - 1));

    if (oldCode == book.code) {
        InsertOrUpdateInDatabase(book);
    } else {
        // Изменился первичный ключ — обновляем строку по старому коду
        UpdateBookCodeInDatabase(oldCode, book);
    }
}

/**
 * @brief Возвращает количество строк в модели
 * @param parent Родительский индекс (не используется для табличных моделей)
 * @return Количество книг в модели
 */
int BookModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : books_.size();
}

/**
 * @brief Возвращает количество столбцов в модели
 * @param parent Родительский индекс (не используется для табличных моделей)
 * @return Количество столбцов (полей книги)
 */
int BookModel::columnCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : 5; ///< Колонки: ID, Name, Author, is_taken, Date
}

/**
 * @brief Возвращает данные для отображения в указанной ячейке
 * @param index Индекс ячейки (строка, столбец)
 * @param role Роль данных (отображение, редактирование и т.д.)
 * @return Данные для указанной ячейки и роли
 */
QVariant BookModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }
    const Book& book = books_[index.row()];
    switch (index.column()) {
    case 0: return book.code;     ///< Код книги

    case 1: return book.name;     ///< Название книги

    case 2: return book.author;   ///< Автор книги

    case 3: return book.is_taken ? "Выдана" : "В наличии"; ///< Статус наличия

    case 4: return book.is_taken && book.date_taken ? book.date_taken->toString("dd/MM/yyyy") : ""; ///< Дата выдачи

    default:
        return QVariant();
    }
}

/**
 * @brief Возвращает заголовки для столбцов и строк
 * @param section Номер секции (столбца или строки)
 * @param orientation Ориентация заголовка (горизонтальный или вертикальный)
 * @param role Роль данных заголовка
 * @return Данные заголовка для указанной секции
 */
QVariant BookModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
        return QVariant();
    }
    switch (section) {
    case 0: return "Код книги";   ///< Заголовок для кода книги
    case 1: return "Название";    ///< Заголовок для названия книги
    case 2: return "Автор";       ///< Заголовок для автора книги
    case 3: return "Наличие";     ///< Заголовок для статуса наличия
    case 4: return "Дата выдачи"; ///< Заголовок для даты выдачи

    default:
        return QVariant();
    }
}

/**
 * @brief Добавляет новую книгу в модель
 * @param book Книга для добавления
 */
void BookModel::AddBook(const Book& book) {

    beginInsertRows(QModelIndex(), books_.size(), books_.size());
    books_.append(book);
    endInsertRows();

    InsertOrUpdateInDatabase(book);
}

/**
 * @brief Удаляет книгу из модели по коду
 * @param code Уникальный код книги для удаления
 */
bool BookModel::RemoveBook(const QString& code) {
    for (int i = 0; i < books_.size(); ++i) {
        if (books_[i].code == code) {
            if (books_[i].is_taken) {
                throw BookDeleteForbiddenException(
                    QStringLiteral("Невозможно удалить книгу: она сейчас выдана читателю")
                    );
            }
            beginRemoveRows(QModelIndex(), i, i);
            books_.removeAt(i);
            endRemoveRows();

            DeleteFromDatabase(code);
            return true;
        }
    }
    return false;
}

/**
 * @brief Находит книгу по коду
 * @param code Уникальный код книги для поиска
 * @return Найденная книга или пустая книга если не найдена
 */
Book BookModel::FindBook(const QString& code) const {
    for (const auto& book : books_) {
        if (book.code == code) {
            return book;
        }
    }
    return Book{};  ///< Пустая книга, если не найдено
}

std::optional<int> BookModel::FindBookIndex(const QString &code) const
{
    for (int i = 0; i < books_.size(); ++i) {
        if (books_[i].code == code) return i;
    }
    return std::nullopt;
}

bool BookModel::SetBookTaken(const QString &code, bool isTaken, std::optional<QDate> &date)
{
    if( !FindBookIndex(code).has_value() )  {
        return false;
    }
    int idx = FindBookIndex(code).value();

    books_[idx].is_taken = isTaken;
    if (isTaken) books_[idx].date_taken = std::optional<QDate>(date);
    else books_[idx].date_taken = std::nullopt;

    // уведомляем view об изменении строки
    QModelIndex topLeft = index(idx, 0);
    QModelIndex bottomRight = index(idx, columnCount() - 1);
    emit dataChanged(topLeft, bottomRight);

    InsertOrUpdateInDatabase(books_[idx]);
    return true;
}

/**
 * @brief Возвращает константную ссылку на список книг
 * @return Константная ссылка на список всех книг
 */
const QList<Book>& BookModel::GetBooks() const {
    return books_;
}

/**
 * @brief Загружает список книг из файла
 * @param filePath Путь к файлу для загрузки
 * @return true если загрузка успешна, false в случае ошибки
 */
bool BookModel::LoadFromFile(const QString& filePath) {
    spdlog::info("Загрузка книг из файла {}", filePath.toStdString());
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file for reading:" << filePath;
        spdlog::warn("Не удалось открыть файл для чтения: {}",
                     filePath.toStdString());
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isArray()) {
        qDebug() << "Invalid JSON format";
        return false;
    }

    books_.clear();
    QJsonArray array = doc.array();
    for (const auto& item : array) {
        QJsonObject obj = item.toObject();
        Book book;
        book.code = obj["code"].toString();
        book.name = obj["name"].toString();
        book.author = obj["author"].toString();
        book.is_taken = obj["is_taken"].toBool();
        QString dateStr = obj["date_taken"].toString();
        book.date_taken = dateStr.isEmpty() ? std::nullopt : std::optional<QDate>(QDate::fromString(dateStr , "dd/MM/yyyy"));
        books_.append(book);
    }
    spdlog::info("Успешная загрузка книг, всего: {}", books_.size());
    return true;
}

QString BookModel::GenerateBookCode(const QList<Book> &existingBooks)
{
    QString code;
    bool unique = false;

    while (!unique) {
        code = QString("B%1").arg(QRandomGenerator::global()->bounded(1000, 9999));
        unique = true;
        for (const auto& book : existingBooks) {
            if (book.code == code) {
                unique = false;
                break;
            }
        }
    }
    return code;
}

/**
 * @brief Сохраняет список книг в файл
 * @param filePath Путь к файлу для сохранения
 * @return true если сохранение успешно, false в случае ошибки
 */
bool BookModel::SaveToFile(const QString& filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open file for writing:" << filePath;
        return false;
    }

    QJsonArray array;
    for (const auto& book : books_) {
        QJsonObject obj;
        obj["code"] = book.code;
        obj["name"] = book.name;
        obj["author"] = book.author;
        obj["is_taken"] = book.is_taken;
        obj["date_taken"] = book.date_taken ? book.date_taken->toString("dd/MM/yyyy") : "";
        array.append(obj);
    }

    QJsonDocument doc(array);
    file.write(doc.toJson());
    return true;
}

bool BookModel::LoadFromXml(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open xml file for reading:" << filePath;
        return false;
    }

    QXmlStreamReader xml(&file);
    QList<Book> tmp;

    while (!xml.atEnd() && !xml.hasError()) {
        auto token = xml.readNext();
        if (token == QXmlStreamReader::StartElement && xml.name() == u"book") {
            Book book;

            while (!xml.atEnd()) {
                xml.readNext();

                if (xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == u"book")
                    break;

                if (xml.tokenType() != QXmlStreamReader::StartElement)
                    continue;

                const auto tag = xml.name();
                const QString text = xml.readElementText();

                if (tag == u"code")        book.code = text;
                else if (tag == u"name")   book.name = text;
                else if (tag == u"author") book.author = text;
                else if (tag == u"is_taken") {
                    const QString v = text.trimmed().toLower();
                    book.is_taken = (v == "1" || v == "true");
                } else if (tag == u"date_taken") {
                    QString v = text.trimmed();
                    book.date_taken = v.isEmpty()
                                          ? std::nullopt
                                          : std::optional<QDate>(QDate::fromString(v, "dd/MM/yyyy"));
                }
            }

            tmp.append(book);
        }
    }

    if (xml.hasError()) {
        qDebug() << "XML parse error (books):" << xml.errorString();
        return false;
    }

    books_.swap(tmp);
    return true;
}

bool BookModel::SaveToXml(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Failed to open xml file for writing:" << filePath;
        return false;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("books");

    for (const auto& book : books_) {
        xml.writeStartElement("book");
        xml.writeTextElement("code", book.code);
        xml.writeTextElement("name", book.name);
        xml.writeTextElement("author", book.author);
        xml.writeTextElement("is_taken", book.is_taken ? "1" : "0");
        xml.writeTextElement(
            "date_taken",
            book.date_taken ? book.date_taken->toString("dd/MM/yyyy") : ""
            );
        xml.writeEndElement(); // book
    }

    xml.writeEndElement(); // books
    xml.writeEndDocument();
    return !xml.hasError();
}


bool BookModel::LoadFromDatabase()
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen())
        return false;

    QSqlQuery q(db);
    if (!q.exec("SELECT code, name, author, is_taken, date_taken FROM books")) {
        qDebug() << "LoadFromDatabase books error:" << q.lastError().text();
        return false;
    }

    beginResetModel();
    books_.clear();

    while (q.next()) {
        const QString code = q.value(0).toString();
        const QString name = q.value(1).toString();
        const QString auth = q.value(2).toString();
        const bool taken   = q.value(3).toBool();
        const QString dateStr = q.value(4).toString();

        std::optional<QDate> dt;
        if (!dateStr.isEmpty())
            dt = QDate::fromString(dateStr, "dd/MM/yyyy");

        books_.append(Book(code, name, auth, taken, dt));
    }

    endResetModel();
    return true;
}

bool BookModel::InsertOrUpdateInDatabase(const Book& book)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen())
        return false;

    QSqlQuery q(db);
    q.prepare(
        "INSERT INTO books(code, name, author, is_taken, date_taken) "
        "VALUES (:code, :name, :author, :taken, :date) "
        "ON CONFLICT(code) DO UPDATE SET "
        " name       = excluded.name,"
        " author     = excluded.author,"
        " is_taken   = excluded.is_taken,"
        " date_taken = excluded.date_taken"
        );

    q.bindValue(":code", book.code);
    q.bindValue(":name", book.name);
    q.bindValue(":author", book.author);
    q.bindValue(":taken", book.is_taken ? 1 : 0);
    q.bindValue(":date",
                book.date_taken
                    ? book.date_taken->toString("dd/MM/yyyy")
                    : QString());

    if (!q.exec()) {
        qDebug() << "InsertOrUpdateInDatabase book error:" << q.lastError().text();
        return false;
    }
    return true;
}


bool BookModel::DeleteFromDatabase(const QString& code)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen())
        return false;

    QSqlQuery q(db);
    q.prepare("DELETE FROM books WHERE code = :code");
    q.bindValue(":code", code);

    if (!q.exec()) {
        qDebug() << "DeleteFromDatabase book error:" << q.lastError().text();
        return false;
    }
    return true;
}

bool BookModel::UpdateBookCodeInDatabase(const QString &oldCode, const Book &book)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen())
        return false;

    QSqlQuery q(db);
    q.prepare(
        "UPDATE books SET "
        " code = :newCode,"
        " name = :name,"
        " author = :author,"
        " is_taken = :taken,"
        " date_taken = :date "
        "WHERE code = :oldCode"
        );

    q.bindValue(":newCode", book.code);
    q.bindValue(":name", book.name);
    q.bindValue(":author", book.author);
    q.bindValue(":taken", book.is_taken ? 1 : 0);
    q.bindValue(":date",
                book.date_taken
                    ? book.date_taken->toString("dd/MM/yyyy")
                    : QString());
    q.bindValue(":oldCode", oldCode);

    if (!q.exec()) {
        qDebug() << "UpdateBookCodeInDatabase error:" << q.lastError().text();
        return false;
    }
    return true;
}

