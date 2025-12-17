/**
 * @file ReaderModel.h
 * @author Кирилл К
 * @brief Модель (Qt Model/View) для управления данными о читателях библиотеки.
 * @version 1.1
 * @date 2025-12-10
 *
 * Содержит:
 * - абстрактный базовый класс PersonBase (полиморфизм);
 * - структуру Reader с данными читателя;
 * - модель ReaderModel на базе QAbstractTableModel для отображения и операций CRUD;
 * - связь читатель ↔ книги (список выданных кодов);
 * - загрузка/сохранение и синхронизация с БД.
 */

#ifndef READERMODEL_H
#define READERMODEL_H

#pragma once

#include <QString>
#include <QList>
#include <optional>
#include <QAbstractTableModel>
#include <QDate>

/**
 * @class PersonBase
 * @brief Абстрактная базовая сущность человека (пример ООП: наследование/полиморфизм).
 *
 * Используется как базовый тип для сущностей, у которых можно получить ФИО.
 */
class PersonBase {
public:
    virtual ~PersonBase() = default;

    /**
     * @brief Получить полное имя (ФИО) в строковом виде.
     * @return Строка с полным именем.
     */
    virtual QString fullName() const = 0;
};

/**
 * @enum Sex
 * @brief Перечисление пола читателя.
 */
enum Sex {
    Female, ///< Женский пол
    Male,   ///< Мужской пол
};

/**
 * @struct Reader
 * @brief Данные читателя библиотеки.
 *
 * Наследуется от PersonBase, реализуя метод fullName().
 */
struct Reader : public PersonBase {
    QString ID;                 ///< Уникальный идентификатор читателя (ключ).
    QString first_name;         ///< Имя.
    QString second_name;        ///< Фамилия.
    QString third_name;         ///< Отчество.
    Sex gender;                 ///< Пол.
    QDate reg_date;             ///< Дата регистрации в библиотеке.
    QList<QString> taken_books; ///< Список кодов книг, закреплённых за читателем.

    /// Конструктор по умолчанию.
    Reader() = default;

    /**
     * @brief Конструктор с параметрами.
     * @param id Идентификатор читателя.
     * @param first Имя.
     * @param second Фамилия.
     * @param third Отчество.
     * @param g Пол.
     * @param reg Дата регистрации.
     * @param books Список выданных книг (коды).
     */
    Reader(const QString& id,
           const QString& first,
           const QString& second,
           const QString& third,
           Sex g,
           const QDate& reg,
           const QList<QString>& books)
        : ID(id)
        , first_name(first)
        , second_name(second)
        , third_name(third)
        , gender(g)
        , reg_date(reg)
        , taken_books(books)
    {}

    /// Копирующий конструктор.
    Reader(const Reader& other) = default;

    /**
     * @brief Сформировать строку ФИО читателя.
     * @return "Имя Фамилия Отчество".
     */
    QString fullName() const override {
        return first_name + " " + second_name + " " + third_name;
    }
};

/**
 * @class ReaderModel
 * @brief Табличная модель читателей для QTableView.
 *
 * Модель хранит список Reader и предоставляет:
 * - стандартные методы QAbstractTableModel;
 * - добавление/удаление/поиск читателей;
 * - связь "читатель—книга" через список кодов taken_books;
 * - загрузку/сохранение и работу с БД.
 */
class ReaderModel : public QAbstractTableModel {
    Q_OBJECT

public:
    /**
     * @brief Конструктор модели читателей.
     * @param parent Родительский объект Qt.
     */
    explicit ReaderModel(QObject* parent = nullptr);

    /**
     * @brief Обновить данные читателя по индексу в модели.
     * @param index Индекс читателя в readers_.
     * @param reader Новые данные читателя.
     */
    void UpdateReaderAt(int index, const Reader& reader);

    /**
     * @brief Количество строк (читателей).
     * @param parent Родительский индекс (для таблицы не используется).
     * @return Число читателей в модели.
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * @brief Количество столбцов (полей читателя).
     * @param parent Родительский индекс (для таблицы не используется).
     * @return Число столбцов модели.
     */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * @brief Данные ячейки таблицы.
     * @param index Индекс (строка/столбец).
     * @param role Роль Qt (DisplayRole и т.п.).
     * @return QVariant со значением для отображения/использования.
     */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /**
     * @brief Заголовки столбцов таблицы.
     * @param section Номер секции.
     * @param orientation Горизонтально/вертикально.
     * @param role Роль Qt.
     * @return Текст заголовка.
     */
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /**
     * @brief Добавить нового читателя в модель.
     * @param reader Читатель для добавления.
     */
    void AddReader(const Reader& reader);

    /**
     * @brief Удалить читателя по идентификатору.
     * @param id ID читателя.
     * @return true если удалено успешно.
     */
    bool RemoveReader(const QString& id);

    /**
     * @brief Привязать книгу к читателю (добавить код в список taken_books).
     * @param reader_id ID читателя.
     * @param book_code Код книги.
     * @return true если связь добавлена успешно.
     */
    bool AddLinkBook(const QString& reader_id , const QString& book_code);

    /**
     * @brief Удалить привязку книги у читателя.
     * @param reader_id ID читателя.
     * @param book_code Код книги.
     * @return true если связь удалена успешно.
     */
    bool RemoveLinkBook(const QString& reader_id , const QString& book_code);

    /**
     * @brief Найти индекс читателя по ID.
     * @param id ID читателя.
     * @return Индекс читателя или std::nullopt.
     */
    std::optional<int> FindReaderIndex(const QString &id) const;

    /**
     * @brief Найти читателя по ID.
     * @param id Уникальный идентификатор читателя.
     * @return std::optional с читателем или std::nullopt если не найден.
     */
    std::optional<Reader> FindReader(const QString& id);

    /**
     * @brief Получить список читателей.
     * @return Константная ссылка на список readers_.
     */
    const QList<Reader>& GetReaders() const;

    /**
     * @brief Загрузить читателей из файла.
     * @param filePath Путь к файлу.
     * @return true если успешно.
     */
    bool LoadFromFile(const QString& filePath);

    /**
     * @brief Сохранить читателей в файл.
     * @param filePath Путь к файлу.
     * @return true если успешно.
     */
    bool SaveToFile(const QString& filePath);

    /**
     * @brief Загрузить читателей из XML.
     * @param filePath Путь к XML файлу.
     * @return true если успешно.
     */
    bool LoadFromXml(const QString& filePath);

    /**
     * @brief Сохранить читателей в XML.
     * @param filePath Путь к XML файлу.
     * @return true если успешно.
     */
    bool SaveToXml(const QString& filePath);

    /**
     * @brief Сгенерировать новый уникальный ID читателя.
     * @param existingReaders Список уже существующих читателей.
     * @return Новый ID.
     */
    static QString GenerateReaderID(const QList<Reader>& existingReaders);

    /**
     * @brief Обновить код книги у всех читателей (например, при смене шифра книги).
     * @param oldCode Старый код книги.
     * @param newCode Новый код книги.
     * @return true если обновление выполнено.
     */
    bool UpdateBookCodeForAllReaders(const QString &oldCode,
                                     const QString &newCode);

    /**
     * @brief Загрузить читателей из базы данных в модель.
     * @return true если успешно.
     */
    bool LoadFromDatabase();

    /**
     * @brief Вставить читателя в БД или обновить существующую запись.
     * @param reader Читатель для вставки/обновления.
     * @return true если успешно.
     */
    bool InsertOrUpdateInDatabase(const Reader& reader);

    /**
     * @brief Удалить читателя из БД по ID.
     * @param id ID читателя.
     * @return true если успешно.
     */
    bool DeleteFromDatabase(const QString& id);

private:

private:
    QList<Reader> readers_; ///< Список читателей, управляемых моделью.
};

#endif // READERMODEL_H
