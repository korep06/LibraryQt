/**
 * @file ReaderModel.cpp
 * @author Кирилл К
 * @brief Реализация методов модели читателей
 * @version 1.0
 * @date 2024-12-19
 */

#include "ReaderModel.h"
#include "Exception.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QList>
#include <QJsonObject>
#include <optional>
#include <QRandomGenerator>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>


/**
 * @brief Конструктор модели читателей
 * @param readers Ссылка на список читателей для инициализации модели
 * @param parent Родительский объект в иерархии Qt
 */
ReaderModel::ReaderModel( QObject *parent)
    : QAbstractTableModel(parent) {
}

void ReaderModel::ReaderModel::UpdateReaderAt(int index, const Reader &reader) {
    if (index < 0 || index >= readers_.size()) {
        return;
    }
    readers_[index] = reader;
    emit dataChanged(this->index(index, 0), this->index(index, columnCount() - 1));
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
 * @brief Добавляет нового читателя в модель
 * @param reader Читатель для добавления
 */
void ReaderModel::AddReader(const Reader &reader)
{
    beginInsertRows(QModelIndex(), readers_.size(), readers_.size());
    readers_.append(reader);
    endInsertRows();
}

/**
 * @brief Удаляет читателя из модели по идентификатору
 * @param id Уникальный идентификатор читателя для удаления
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
            return true;
        }
    }
    return false;
}

bool ReaderModel::AddLinkBook(const QString &readerID, const QString &bookCode)
{
    if (!FindReaderIndex(readerID).has_value()) {
        return false;
    }
    int idx = FindReaderIndex(readerID).value();

    // Если книга уже есть в списке — не дублируем
    if (readers_[idx].taken_books.contains(bookCode))
        return false;

    readers_[idx].taken_books.append(bookCode);

    // уведомляем view об изменении строки читателя
    QModelIndex topLeft = index(idx, 0);
    QModelIndex bottomRight = index(idx, columnCount() - 1);
    emit dataChanged(topLeft, bottomRight);
    return true;
}

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

    return true;
}


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
 * @brief Возвращает копию списка читателей
 * @return Копия списка всех читателей
 */
const QList<Reader> ReaderModel::GetReaders() const
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
        if (token == QXmlStreamReader::StartElement && xml.name() == "reader") {
            Reader reader;

            // Разбираем содержимое <reader> ... </reader>
            while (!xml.atEnd()) {
                xml.readNext();

                // Закрывающий тег </reader> — выходим из внутреннего цикла
                if (xml.tokenType() == QXmlStreamReader::EndElement
                    && xml.name() == "reader")
                {
                    break;
                }

                if (xml.tokenType() != QXmlStreamReader::StartElement)
                    continue;

                const auto tag = xml.name();

                // ОСОБЫЙ СЛУЧАЙ: <taken_books> с вложенными <book>
                if (tag == "taken_books") {
                    while (!xml.atEnd()) {
                        xml.readNext();

                        if (xml.tokenType() == QXmlStreamReader::EndElement
                            && xml.name() == "taken_books")
                        {
                            break;
                        }

                        if (xml.tokenType() == QXmlStreamReader::StartElement
                            && xml.name() == "book")
                        {
                            QString code = xml.readElementText().trimmed();
                            if (!code.isEmpty())
                                reader.taken_books.append(code);
                        }
                    }
                } else {
                    // Обычные простые поля: только текст внутри тега
                    const QString text = xml.readElementText();

                    if (tag == "ID")              reader.ID = text;
                    else if (tag == "first_name")  reader.first_name = text;
                    else if (tag == "second_name") reader.second_name = text;
                    else if (tag == "third_name")  reader.third_name = text;
                    else if (tag == "gender") {
                        reader.gender =
                            (text.trimmed() == "1" ? Sex::Male : Sex::Female);
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
