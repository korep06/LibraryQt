/**
 * @file ReaderModel.cpp
 * @author Кирилл К
 * @brief Реализация методов модели читателей
 * @version 1.1
 * @date 2025-12-11
 *
 * Реализация Qt-модели ReaderModel:
 * - хранение читателей в памяти (QList<Reader>);
 * - отображение в таблице (QAbstractTableModel);
 * - CRUD операции;
 * - привязка/отвязка книг к читателю;
 * - загрузка/сохранение в JSON/XML;
 * - синхронизация с SQLite через DatabaseManager.
 */

#include "ReaderModel.h"
#include "Exception.h"
#include "DatabaseManager.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QList>
#include <QJsonObject>
#include <optional>
#include <QRandomGenerator>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QSqlQuery>
#include <QSqlError>

/**
 * @brief Конструктор модели читателей.
 * @param parent Родительский объект Qt.
 */
ReaderModel::ReaderModel(QObject *parent)
    : QAbstractTableModel(parent) {
}

/**
 * @brief Обновляет данные читателя по индексу и сохраняет изменения в БД.
 * @param index Индекс читателя в списке readers_.
 * @param reader Новые данные читателя.
 *
 * @note После замены записи уведомляет View через dataChanged().
 */
void ReaderModel::UpdateReaderAt(int index, const Reader &reader) {
    if (index < 0 || index >= readers_.size()) {
        return;
    }
    readers_[index] = reader;
    emit dataChanged(this->index(index, 0), this->index(index, columnCount() - 1));

    InsertOrUpdateInDatabase(reader);
}

/**
 * @brief Возвращает количество строк в модели
 * @param parent Родительский индекс (не используется для табличных моделей)
 * @return Количество читателей в модели
 */
int ReaderModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : readers_.size();
}

/**
 * @brief Возвращает количество столбцов в модели
 * @param parent Родительский индекс (не используется для табличных моделей)
 * @return Количество столбцов (полей читателя)
 */
int ReaderModel::columnCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : 4; ///< Колонки: ID, Name, count_taken_books, taken_books
}

/**
 * @brief Возвращает данные для отображения в указанной ячейке
 * @param index Индекс ячейки (строка, столбец)
 * @param role Роль данных (отображение, редактирование и т.д.)
 * @return Данные для указанной ячейки и роли
 */
QVariant ReaderModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }
    const Reader& reader = readers_[index.row()];
    switch (index.column()) {
    case 0: return reader.ID; ///< ID читателя

    case 1: return reader.first_name + " " + reader.second_name + " " + reader.third_name; ///< Полное ФИО

    case 2: return reader.taken_books.size(); ///< Количество взятых книг

    case 3: return reader.taken_books.join(", "); ///< Список кодов взятых книг

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
QVariant ReaderModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
        return QVariant();
    }
    switch (section) {
    case 0: return "ID"; ///< Заголовок для идентификатора
    case 1: return "ФИО"; ///< Заголовок для полного имени
    case 2: return "количество взятых книг"; ///< Заголовок для количества книг
    case 3: return "Коды взятых книг"; ///< Заголовок для списка книг
    default:
        return QVariant();
    }
}

/**
 * @brief Добавляет нового читателя в модель.
 * @param reader Читатель для добавления.
 *
 * Добавляет запись в память + синхронизирует с БД.
 */
void ReaderModel::AddReader(const Reader &reader)
{
    beginInsertRows(QModelIndex(), readers_.size(), readers_.size());
    readers_.append(reader);
    endInsertRows();

    InsertOrUpdateInDatabase(reader);
}

/**
 * @brief Удаляет читателя из модели по идентификатору.
 * @param id Уникальный идентификатор читателя для удаления.
 * @return true если читатель найден и удалён.
 *
 * @throw ReaderDeleteForbiddenException если у читателя есть невозвращённые книги.
 */
bool ReaderModel::RemoveReader(const QString &id)
{
    for (int i = 0; i < readers_.size(); ++i) {
        if (readers_[i].ID == id) {

            if (!readers_[i].taken_books.isEmpty()) {
                throw ReaderDeleteForbiddenException("Невозможно удалить читателя: у него есть невозвращённые книги");
            }

            beginRemoveRows(QModelIndex(), i, i);
            readers_.removeAt(i);
            endRemoveRows();

            DeleteFromDatabase(id);
            return true;
        }
    }
    return false;
}

/**
 * @brief Добавляет связь "читатель → книга" (закрепляет книгу за читателем).
 * @param readerID ID читателя.
 * @param bookCode Код книги.
 * @return true если связь добавлена, false если читатель не найден или книга уже закреплена.
 */
bool ReaderModel::AddLinkBook(const QString &readerID, const QString &bookCode)
{
    auto optIdx = FindReaderIndex(readerID);
    if (!optIdx.has_value())
        return false;

    const int idx = optIdx.value();

    if (readers_[idx].taken_books.contains(bookCode))
        return false;

    readers_[idx].taken_books.append(bookCode);

    emit dataChanged(index(idx, 0), index(idx, columnCount() - 1));
    InsertOrUpdateInDatabase(readers_[idx]);
    return true;
}

/**
 * @brief Удаляет связь "читатель → книга" (убирает книгу из списка читателя).
 * @param readerID ID читателя.
 * @param bookCode Код книги.
 * @return true если связь удалена, false если читатель/книга не найдены.
 */
bool ReaderModel::RemoveLinkBook(const QString &readerID, const QString &bookCode)
{
    auto optIdx = FindReaderIndex(readerID);
    if (!optIdx.has_value()) {
        return false;
    }

    int idx = optIdx.value();

    // Проверяем, есть ли книга у читателя
    if (!readers_[idx].taken_books.contains(bookCode))
        return false;

    // Удаляем книгу
    readers_[idx].taken_books.removeAll(bookCode);

    // Уведомляем view об изменении строки читателя
    QModelIndex topLeft = index(idx, 0);
    QModelIndex bottomRight = index(idx, columnCount() - 1);
    emit dataChanged(topLeft, bottomRight);

    InsertOrUpdateInDatabase(readers_[idx]);
    return true;
}

/**
 * @brief Ищет индекс читателя по ID.
 * @param id ID читателя.
 * @return std::optional<int> с индексом, либо std::nullopt если не найден.
 */
std::optional<int> ReaderModel::FindReaderIndex(const QString &id) const
{
    for (int i = 0; i < readers_.size(); ++i) {
        if (readers_[i].ID == id) return i;
    }
    return std::nullopt;
}

/**
 * @brief Находит читателя по идентификатору
 * @param id Уникальный идентификатор читателя для поиска
 * @return std::optional с найденным читателем или std::nullopt если не найден
 */
std::optional<Reader> ReaderModel::FindReader(const QString &id)
{
    for (const auto& reader : readers_) {
        if (reader.ID == id) {
            return reader;
        }
    }
    return std::nullopt;  ///< Пустое значение, если читатель не найден
}

/**
 * @brief Возвращает список читателей.
 * @return Константная ссылка на список readers_.
 */
const QList<Reader>& ReaderModel::GetReaders() const
{
    return readers_;
}

/**
 * @brief Загружает список читателей из файла
 * @param filePath Путь к файлу для загрузки
 * @return true если загрузка успешна, false в случае ошибки
 */
bool ReaderModel::LoadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file for reading:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isArray()) {
        qDebug() << "Invalid JSON format";
        return false;
    }

    readers_.clear();
    QJsonArray array = doc.array();
    for (const auto& item : array) {
        QJsonObject obj = item.toObject();
        Reader reader;
        reader.ID = obj["ID"].toString();
        reader.first_name = obj["first_name"].toString();
        reader.second_name = obj["second_name"].toString();
        reader.third_name = obj["third_name"].toString();
        reader.gender = (obj["gender"].toInt() == 0 ? Sex::Female : Sex::Male);
        QString regStr = obj["reg_date"].toString();
        if (!regStr.isEmpty())
            reader.reg_date = QDate::fromString(regStr, "dd/MM/yyyy");
        else
            reader.reg_date = QDate();

        // Загрузка списка взятых книг
        auto arr = obj["taken_books"].toArray();
        for (const auto& item : arr) {
            reader.taken_books.append(item.toString());
        }
        readers_.append(reader);
    }
    return true;
}

/**
 * @brief Сохраняет список читателей в файл
 * @param filePath Путь к файлу для сохранения
 * @return true если сохранение успешно, false в случае ошибки
 */
bool ReaderModel::SaveToFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open file for writing:" << filePath;
        return false;
    }

    QJsonArray array;
    for (const auto& reader : readers_) {
        QJsonObject obj;
        obj["ID"] = reader.ID;
        obj["first_name"] = reader.first_name;
        obj["second_name"] = reader.second_name;
        obj["third_name"] = reader.third_name;
        obj["gender"] = reader.gender;
        obj["reg_date"] =
            reader.reg_date.isValid()
                ? reader.reg_date.toString("dd/MM/yyyy")
                : "";

        // Преобразуем QList<QString> в QJsonArray
        QJsonArray booksArray;
        for (const auto& bookId : reader.taken_books) {
            booksArray.append(bookId); ///< Добавляем каждый ID книги в массив
        }
        obj["taken_books"] = booksArray; ///< Присваиваем массив книг объекту

        array.append(obj);
    }

    QJsonDocument doc(array);
    file.write(doc.toJson(QJsonDocument::Indented)); ///< Indented для читаемого формата
    file.close(); ///< Закрываем файл
    return true;
}

/**
 * @brief Генерирует новый уникальный ID читателя.
 * @param existingReaders Список существующих читателей.
 * @return Новый ID в формате "Rxxxx".
 */
QString ReaderModel::GenerateReaderID(const QList<Reader> &existingReaders)
{
    QString id;
    bool unique = false;

    while (!unique) {
        id = QString("R%1").arg(QRandomGenerator::global()->bounded(1000, 9999));
        unique = true;
        for (const auto& r : existingReaders) {
            if (r.ID == id) {
                unique = false;
                break;
            }
        }
    }
    return id;
}

/**
 * @brief Загружает читателей из XML.
 * @param filePath Путь к XML-файлу.
 * @return true если загрузка успешна.
 *
 * Поддерживает структуру:
 * - <readers>
 *   - <reader> ... </reader>
 *     - <taken_books>
 *       - <book>CODE</book>
 *       ...
 */
bool ReaderModel::LoadFromXml(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open xml file for reading:" << filePath;
        return false;
    }

    QXmlStreamReader xml(&file);
    QList<Reader> tmp;

    while (!xml.atEnd() && !xml.hasError()) {
        auto token = xml.readNext();

        // Ищем <reader>
        if (token == QXmlStreamReader::StartElement && xml.name() == u"reader") {
            Reader reader;

            // Разбираем содержимое <reader> ... </reader>
            while (!xml.atEnd()) {
                xml.readNext();

                // Закрывающий тег </reader> — выходим из внутреннего цикла
                if (xml.tokenType() == QXmlStreamReader::EndElement
                    && xml.name() == u"reader")
                {
                    break;
                }

                if (xml.tokenType() != QXmlStreamReader::StartElement)
                    continue;

                const auto tag = xml.name();

                // ОСОБЫЙ СЛУЧАЙ: <taken_books> с вложенными <book>
                if (tag == u"taken_books") {
                    while (!xml.atEnd()) {
                        xml.readNext();

                        if (xml.tokenType() == QXmlStreamReader::EndElement
                            && xml.name() == u"taken_books")
                        {
                            break;
                        }

                        if (xml.tokenType() == QXmlStreamReader::StartElement
                            && xml.name() == u"book")
                        {
                            QString code = xml.readElementText().trimmed();
                            if (!code.isEmpty())
                                reader.taken_books.append(code);
                        }
                    }
                } else {
                    // Обычные простые поля: только текст внутри тега
                    const QString text = xml.readElementText();

                    if (tag == u"ID")              reader.ID = text;
                    else if (tag == u"first_name")  reader.first_name = text;
                    else if (tag == u"second_name") reader.second_name = text;
                    else if (tag == u"third_name")  reader.third_name = text;
                    else if (tag == u"gender") {
                        reader.gender =
                            (text.trimmed() == "1" ? Sex::Male : Sex::Female);
                    }
                    else if (tag == u"reg_date") {
                        QString t = text.trimmed();
                        if (t.isEmpty())
                            reader.reg_date = QDate();
                        else
                            reader.reg_date = QDate::fromString(t, "dd/MM/yyyy");
                    }
                }
            }

            tmp.append(reader);
        }
    }

    if (xml.hasError()) {
        qDebug() << "XML parse error (readers):" << xml.errorString();
        return false;
    }

    readers_.swap(tmp);
    return true;
}

/**
 * @brief Сохраняет читателей в XML.
 * @param filePath Путь к XML-файлу.
 * @return true если сохранение успешно.
 */
bool ReaderModel::SaveToXml(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Failed to open xml file for writing:" << filePath;
        return false;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("readers");

    for (const auto& reader : readers_) {
        xml.writeStartElement("reader");
        xml.writeTextElement("ID", reader.ID);
        xml.writeTextElement("first_name", reader.first_name);
        xml.writeTextElement("second_name", reader.second_name);
        xml.writeTextElement("third_name", reader.third_name);
        xml.writeTextElement("gender", reader.gender == Sex::Male ? "1" : "0");
        xml.writeTextElement(
            "reg_date",
            reader.reg_date.isValid()
                ? reader.reg_date.toString("dd/MM/yyyy")
                : ""
            );

        xml.writeStartElement("taken_books");
        for (const auto& bookCode : reader.taken_books) {
            xml.writeTextElement("book", bookCode);
        }
        xml.writeEndElement(); // taken_books

        xml.writeEndElement(); // reader
    }

    xml.writeEndElement(); // readers
    xml.writeEndDocument();
    return !xml.hasError();
}

/**
 * @brief Заменяет код книги у всех читателей (например, при смене шифра книги).
 * @param oldCode Старый код книги.
 * @param newCode Новый код книги.
 * @return true если хотя бы у одного читателя были изменения.
 */
bool ReaderModel::UpdateBookCodeForAllReaders(const QString &oldCode,
                                              const QString &newCode)
{
    bool changedAny = false;

    for (int i = 0; i < readers_.size(); ++i) {
        bool rowChanged = false;

        for (QString &code : readers_[i].taken_books) {
            if (code == oldCode) {
                code = newCode;
                rowChanged = true;
                changedAny = true;
            }
        }

        if (rowChanged) {
            QModelIndex topLeft = index(i, 0);
            QModelIndex bottomRight = index(i, columnCount() - 1);
            emit dataChanged(topLeft, bottomRight);

            InsertOrUpdateInDatabase(readers_[i]);
        }
    }

    return changedAny;
}

/**
 * @brief Загружает читателей из базы данных (SQLite).
 * @return true если загрузка успешна.
 */
bool ReaderModel::LoadFromDatabase()
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen())
        return false;

    QSqlQuery q(db);
    if (!q.exec("SELECT id, first_name, second_name, third_name, "
                "gender, reg_date, taken_books FROM readers")) {
        qDebug() << "LoadFromDatabase readers error:" << q.lastError().text();
        return false;
    }

    beginResetModel();
    readers_.clear();

    while (q.next()) {
        Reader r;
        r.ID          = q.value(0).toString();
        r.first_name  = q.value(1).toString();
        r.second_name = q.value(2).toString();
        r.third_name  = q.value(3).toString();
        r.gender      = (q.value(4).toInt() == 1 ? Sex::Male : Sex::Female);

        const QString regStr = q.value(5).toString();
        if (!regStr.isEmpty())
            r.reg_date = QDate::fromString(regStr, "dd/MM/yyyy");
        else
            r.reg_date = QDate();

        const QString takenStr = q.value(6).toString();
        r.taken_books.clear();
        for (const QString& part : takenStr.split(",", Qt::SkipEmptyParts)) {
            r.taken_books.append(part.trimmed());
        }

        readers_.append(r);
    }

    endResetModel();
    return true;
}

/**
 * @brief Вставляет читателя в БД или обновляет существующую запись (по ID).
 * @param reader Читатель для вставки/обновления.
 * @return true если операция выполнена успешно.
 */
bool ReaderModel::InsertOrUpdateInDatabase(const Reader& reader)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen())
        return false;

    const QString taken = reader.taken_books.join(",");

    QSqlQuery q(db);
    q.prepare(
        "INSERT INTO readers(id, first_name, second_name, third_name, "
        " gender, reg_date, taken_books) "
        "VALUES (:id, :first, :second, :third, :gender, :reg, :books) "
        "ON CONFLICT(id) DO UPDATE SET "
        " first_name  = excluded.first_name,"
        " second_name = excluded.second_name,"
        " third_name  = excluded.third_name,"
        " gender      = excluded.gender,"
        " reg_date    = excluded.reg_date,"
        " taken_books = excluded.taken_books"
        );

    q.bindValue(":id", reader.ID);
    q.bindValue(":first", reader.first_name);
    q.bindValue(":second", reader.second_name);
    q.bindValue(":third", reader.third_name);
    q.bindValue(":gender", reader.gender == Sex::Male ? 1 : 0);
    q.bindValue(":reg",
                reader.reg_date.isValid()
                    ? reader.reg_date.toString("dd/MM/yyyy")
                    : QString());
    q.bindValue(":books", taken);

    if (!q.exec()) {
        qDebug() << "InsertOrUpdateInDatabase reader error:" << q.lastError().text();
        return false;
    }
    return true;
}

/**
 * @brief Удаляет читателя из БД по ID.
 * @param id ID читателя.
 * @return true если удаление успешно.
 */
bool ReaderModel::DeleteFromDatabase(const QString& id)
{
    QSqlDatabase db = DatabaseManager::instance().database();
    if (!db.isOpen())
        return false;

    QSqlQuery q(db);
    q.prepare("DELETE FROM readers WHERE id = :id");
    q.bindValue(":id", id);

    if (!q.exec()) {
        qDebug() << "DeleteFromDatabase reader error:" << q.lastError().text();
        return false;
    }
    return true;
}
