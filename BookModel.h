/**
 * @file BookModel.h
 * @author Кирилл К
 * @brief Модель (Qt Model/View) для управления данными о книгах.
 * @version 1.1
 * @date 2025-12-08
 *
 * Содержит:
 * - структуру Book (данные одной книги);
 * - модель BookModel на базе QAbstractTableModel для отображения и CRUD-операций;
 * - загрузку/сохранение в файл/XML и операции с базой данных (SQLite через Qt SQL).
 */

#ifndef BOOKMODEL_H
#define BOOKMODEL_H

#pragma once

#include <QList>
#include <QString>
#include <QAbstractItemModel>
#include <QDate>
#include <optional>

/**
 * @struct Book
 * @brief Данные одной книги.
 *
 * Используется как элемент списка в BookModel.
 */
struct Book {
    QString code;                ///< Уникальный код (шифр) книги.
    QString name;                ///< Название книги.
    QString author;              ///< Автор книги.
    bool is_taken;               ///< Признак: книга выдана (true) / в наличии (false).
    std::optional<QDate> date_taken; ///< Дата выдачи (если книга выдана).

    /// Конструктор по умолчанию (книга "не выдана").
    Book() : is_taken(false) {}

    /**
     * @brief Конструктор с параметрами.
     * @param c Шифр книги.
     * @param n Название книги.
     * @param a Автор книги.
     * @param taken Признак выдачи.
     * @param dt Дата выдачи (если есть).
     */
    Book(const QString& c,
         const QString& n,
         const QString& a,
         bool taken = false,
         std::optional<QDate> dt = std::nullopt)
        : code(c)
        , name(n)
        , author(a)
        , is_taken(taken)
        , date_taken(dt)
    {}

    Book(const Book& other) = default; // копирующий конструктор
};

/**
 * @class BookModel
 * @brief Табличная модель книг для QTableView.
 *
 * Модель хранит список Book и предоставляет стандартный набор методов Qt:
 * rowCount/columnCount/data/headerData + прикладные методы добавления, удаления,
 * поиска, а также загрузки/сохранения и синхронизации с БД.
 */
class BookModel : public QAbstractTableModel {
    Q_OBJECT

public:
    /**
     * @brief Конструктор модели книг.
     * @param parent Родительский объект Qt.
     */
    explicit BookModel(QObject* parent = nullptr);

    /**
     * @brief Обновить книгу по индексу (замена записи в модели).
     * @param index Индекс элемента в списке books_.
     * @param book Новое значение книги.
     */
    void UpdateBookAt(int index, const Book& book);

    /**
     * @brief Возвращает количество строк в модели.
     * @param parent Родительский индекс (не используется для табличных моделей).
     * @return Количество книг в модели.
     */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief Возвращает количество столбцов в модели.
     * @param parent Родительский индекс (не используется для табличных моделей).
     * @return Количество столбцов (полей книги).
     */
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief Возвращает данные для отображения в указанной ячейке.
     * @param index Индекс ячейки (строка, столбец).
     * @param role Роль данных (отображение, редактирование и т.д.).
     * @return Данные для указанной ячейки и роли.
     */
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    /**
     * @brief Возвращает заголовки для столбцов и строк.
     * @param section Номер секции (столбца или строки).
     * @param orientation Ориентация заголовка (горизонтальный или вертикальный).
     * @param role Роль данных заголовка.
     * @return Данные заголовка для указанной секции.
     */
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /**
     * @brief Добавляет новую книгу в модель.
     * @param book Книга для добавления.
     */
    void AddBook(const Book& book);

    /**
     * @brief Удаляет книгу из модели по коду.
     * @param code Уникальный код книги для удаления.
     * @return true если книга найдена и удалена, иначе false.
     */
    bool RemoveBook(const QString& code);

    /**
     * @brief Находит книгу по коду.
     * @param code Уникальный код книги для поиска.
     * @return Найденная книга или "пустая" книга если не найдена.
     *
     * @note Формат "пустой книги" задаётся реализацией в .cpp.
     */
    Book FindBook(const QString& code) const;

    /**
     * @brief Найти индекс книги по коду.
     * @param code Уникальный код книги.
     * @return Индекс книги, если найдена; иначе std::nullopt.
     */
    std::optional<int> FindBookIndex(const QString &code) const;

    /**
     * @brief Установить состояние выдачи книги.
     * @param code Код книги.
     * @param isTaken true — выдана, false — возвращена.
     * @param date Дата выдачи (может меняться в зависимости от операции).
     * @return true если книга найдена и состояние изменено, иначе false.
     */
    bool SetBookTaken(const QString &code, bool isTaken, std::optional<QDate> &date);

    /**
     * @brief Возвращает константную ссылку на список книг.
     * @return Константная ссылка на список всех книг.
     */
    const QList<Book>& GetBooks() const;

    /**
     * @brief Сохраняет список книг в файл.
     * @param filePath Путь к файлу для сохранения.
     * @return true если сохранение успешно, false в случае ошибки.
     */
    bool SaveToFile(const QString& filePath) const;

    /**
     * @brief Загружает список книг из файла.
     * @param filePath Путь к файлу для загрузки.
     * @return true если загрузка успешна, false в случае ошибки.
     */
    bool LoadFromFile(const QString& filePath);

    /**
     * @brief Сохраняет список книг в XML.
     * @param filePath Путь к XML-файлу.
     * @return true если сохранение успешно.
     */
    bool SaveToXml(const QString& filePath) const;

    /**
     * @brief Загружает список книг из XML.
     * @param filePath Путь к XML-файлу.
     * @return true если загрузка успешна.
     */
    bool LoadFromXml(const QString& filePath);

    /**
     * @brief Генерация нового уникального кода книги.
     * @param existingBooks Список уже существующих книг (для проверки уникальности).
     * @return Новый код книги.
     */
    static QString GenerateBookCode(const QList<Book>& existingBooks);

    // --- Работа с базой данных ---

    /**
     * @brief Загрузить список книг из базы данных в модель.
     * @return true если загрузка успешна.
     */
    bool LoadFromDatabase();

    /**
     * @brief Добавить книгу в БД или обновить существующую (по коду).
     * @param book Книга для вставки/обновления.
     * @return true если операция успешна.
     */
    bool InsertOrUpdateInDatabase(const Book& book);

    /**
     * @brief Удалить книгу из БД по коду.
     * @param code Код книги.
     * @return true если удаление успешно.
     */
    bool DeleteFromDatabase(const QString& code);

    /**
     * @brief Обновить код книги в БД (смена шифра).
     * @param oldCode Старый код книги.
     * @param book Книга с новым кодом и актуальными данными.
     * @return true если обновление успешно.
     */
    bool UpdateBookCodeInDatabase(const QString& oldCode, const Book& book);

private:



private:
    QList<Book> books_; ///< Список книг, управляемых моделью.
};

#endif // BOOKMODEL_H
