/**
 * @file BookModel.cpp
 * @author Кирилл К
 * @brief Реализация методов модели книг
 * @version 1.1
 * @date 2025-12-09
 */

#include "BookModel.h"
#include "Exception.h"
#include "spdlog/spdlog.h"
#include "DatabaseManager.h"


#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

/**
 * @brief Конструктор модели книг
 * @param books Ссылка на список книг для инициализации модели
 * @param parent Родительский объект в иерархии Qt
 */
BookModel::BookModel(QObject *parent)
    : QAbstractTableModel(parent) {}

/**
 * @brief Обновляет книгу по индексу модели и синхронизирует изменения с БД.
 * @param index Индекс книги в списке books_.
 * @param book Новое значение книги.
 *
 * Если код (первичный ключ) не изменился — выполняется обычное обновление записи.
 * Если код изменился — используется отдельный запрос обновления по старому коду.
 */
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

    const Book& book = books_.at(index.row());
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
                    QString("Нельзя удалить книгу '%1': она выдана читателю.").arg(code)
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
 * @return Найденная книга или "пустая" книга если не найдена
 */
Book BookModel::FindBook(const QString& code) const {
    for (const auto& book : books_) {
        if (book.code == code) {
            return book;
        }
    }
    return Book{};  ///< Пустая книга, если не найдено
}

/**
 * @brief Ищет индекс книги по её коду.
 * @param code Код (шифр) книги.
 * @return Индекс книги в books_, либо std::nullopt если не найдено.
 */
std::optional<int> BookModel::FindBookIndex(const QString &code) const
{
    for (int i = 0; i < books_.size(); ++i) {
        if (books_[i].code == code) return i;
    }
    return std::nullopt;
}

/**
 * @brief Устанавливает признак выдачи книги и дату выдачи.
 * @param code Код книги.
 * @param isTaken true — выдана, false — возвращена.
 * @param date Дата выдачи (может быть задана/сброшена).
 * @return true если книга найдена и обновлена, иначе false.
 */
bool BookModel::SetBookTaken(const QString &code, bool isTaken, std::optional<QDate> &date)
{
    for (int i = 0; i < books_.size(); ++i) {
        if (books_[i].code == code) {
            books_[i].is_taken = isTaken;
            books_[i].date_taken = date;

            emit dataChanged(index(i, 0), index(i, columnCount() - 1));

            InsertOrUpdateInDatabase(books_[i]);
            return true;
        }
    }
    return false;
}

/**
 * @brief Возвращает список всех книг
 * @return Константная ссылка на список книг
 */
const QList<Book>& BookModel::GetBooks() const {
    return books_;
}

/**
 * @brief Сохраняет список книг в JSON файл
 * @param filePath Путь к файлу для сохранения
 * @return true если сохранение успешно, иначе false
 */
bool BookModel::SaveToFile(const QString& filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
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

/**
 * @brief Загружает список книг из JSON файла
 * @param filePath Путь к файлу для загрузки
 * @return true если загрузка успешна, иначе false
 */
bool BookModel::LoadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        return false;
    }

    QJsonArray array = doc.array();
    books_.clear();
    for (const auto& value : array) {
        QJsonObject obj = value.toObject();
        Book book;
        book.code = obj["code"].toString();
        book.name = obj["name"].toString();
        book.author = obj["author"].toString();
        book.is_taken = obj["is_taken"].toBool();
        QString dateStr = obj["date_taken"].toString();
        book.date_taken = dateStr.isEmpty() ? std::nullopt  : std::optional<QDate>(QDate::fromString(dateStr , "dd/MM/yyyy"));
        books_.append(book);
    }
    spdlog::info("Успешная загрузка книг, всего: {}", books_.size());
    return true;
}

/**
 * @brief Генерирует новый уникальный код книги.
 * @param existingBooks Список существующих книг (для проверки уникальности).
 * @return Сгенерированный код, который отсутствует в existingBooks.
 */
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
 * @brief Загружает список книг из XML-файла.
 * @param filePath Путь к XML-файлу.
 * @return true если загрузка успешна.
 */
bool BookModel::LoadFromXml(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open xml file:" << filePath;
        return false;
    }

    QXmlStreamReader xml(&file);
    QList<Book> tmp;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == "book") {
            Book book;
            while (!(xml.isEndElement() && xml.name() == "book") && !xml.atEnd()) {
                xml.readNext();
                if (xml.isStartElement()) {
                    if (xml.name() == "code") book.code = xml.readElementText();
                    else if (xml.name() == "name") book.name = xml.readElementText();
                    else if (xml.name() == "author") book.author = xml.readElementText();
                    else if (xml.name() == "is_taken") book.is_taken = (xml.readElementText() == "1");
                    else if (xml.name() == "date_taken") {
                        QString ds = xml.readElementText();
                        book.date_taken = ds.isEmpty() ? std::nullopt : std::optional<QDate>(QDate::fromString(ds, "dd/MM/yyyy"));
                    }
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

/**
 * @brief Сохраняет список книг в XML-файл.
 * @param filePath Путь к XML-файлу.
 * @return true если сохранение успешно.
 */
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
        xml.writeTextElement("date_taken", book.date_taken ? book.date_taken->toString("dd/MM/yyyy") : "");

        xml.writeEndElement(); // book
    }

    xml.writeEndElement(); // books
    xml.writeEndDocument();

    return true;
}

/**
 * @brief Загружает список книг из базы данных в модель.
 * @return true если загрузка успешна.
 */
bool BookModel::LoadFromDatabase()
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen())
        return false;

    QSqlQuery q(db);
    if (!q.exec("SELECT code, name, author, is_taken, date_taken FROM books")) {
        qDebug() << "LoadFromDatabase error:" << q.lastError().text();
        return false;
    }

    QList<Book> tmp;
    while (q.next()) {
        Book b;
        b.code = q.value(0).toString();
        b.name = q.value(1).toString();
        b.author = q.value(2).toString();
        b.is_taken = q.value(3).toInt() != 0;

        const QString ds = q.value(4).toString();
        b.date_taken = ds.isEmpty() ? std::nullopt : std::optional<QDate>(QDate::fromString(ds, Qt::ISODate));

        tmp.append(b);
    }

    beginResetModel();
    books_.swap(tmp);
    endResetModel();

    return true;
}

/**
 * @brief Вставляет книгу в БД или обновляет существующую запись (по коду).
 * @param book Книга для вставки/обновления.
 * @return true если операция выполнена успешно.
 */
bool BookModel::InsertOrUpdateInDatabase(const Book& book)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen())
        return false;

    QSqlQuery q(db);
    q.prepare(
        "INSERT INTO books(code, name, author, is_taken, date_taken) "
        "VALUES(:code, :name, :author, :is_taken, :date_taken) "
        "ON CONFLICT(code) DO UPDATE SET "
        "name=excluded.name, author=excluded.author, is_taken=excluded.is_taken, date_taken=excluded.date_taken"
        );

    q.bindValue(":code", book.code);
    q.bindValue(":name", book.name);
    q.bindValue(":author", book.author);
    q.bindValue(":is_taken", book.is_taken ? 1 : 0);

    if (book.date_taken.has_value())
        q.bindValue(":date_taken", book.date_taken->toString(Qt::ISODate));
    else
        q.bindValue(":date_taken", "");

    if (!q.exec()) {
        qDebug() << "InsertOrUpdate error:" << q.lastError().text();
        return false;
    }

    return true;
}

/**
 * @brief Удаляет книгу из базы данных по коду.
 * @param code Код (шифр) книги.
 * @return true если удаление успешно.
 */
bool BookModel::DeleteFromDatabase(const QString& code)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen())
        return false;

    QSqlQuery q(db);
    q.prepare("DELETE FROM books WHERE code=:code");
    q.bindValue(":code", code);

    if (!q.exec()) {
        qDebug() << "DeleteFromDatabase error:" << q.lastError().text();
        return false;
    }

    return true;
}

/**
 * @brief Обновляет код (первичный ключ) книги в базе данных.
 * @param oldCode Старый код книги.
 * @param book Книга с новым кодом и актуальными данными.
 * @return true если обновление успешно.
 */
bool BookModel::UpdateBookCodeInDatabase(const QString &oldCode, const Book &book)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen())
        return false;

    QSqlQuery q(db);
    q.prepare(
        "UPDATE books SET code=:newCode, name=:name, author=:author, is_taken=:is_taken, date_taken=:date_taken "
        "WHERE code=:oldCode"
        );

    q.bindValue(":newCode", book.code);
    q.bindValue(":name", book.name);
    q.bindValue(":author", book.author);
    q.bindValue(":is_taken", book.is_taken ? 1 : 0);
    q.bindValue(":date_taken", book.date_taken ? book.date_taken->toString(Qt::ISODate) : "");
    q.bindValue(":oldCode", oldCode);

    if (!q.exec()) {
        qDebug() << "UpdateBookCodeInDatabase error:" << q.lastError().text();
        return false;
    }

    return true;
}
