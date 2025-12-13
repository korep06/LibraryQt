/**
 * @file BookModel.h
 * @author Кирилл К
 * @brief Модель для управления данными о книгах
 * @version 1.0
 * @date 2024-12-19
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
 * @brief Структура для хранения информации о книге
 */
struct Book {
    QString code;                ///< Уникальный код книги
    QString name;                ///< Название книги
    QString author;              ///< Автор книги
    bool is_taken;               ///< Флаг взятия книги (true если книга взята)
    std::optional<QDate> date_taken; ///< Дата взятия книги (если книга взята)

    Book() : is_taken(false) {}

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
 * @brief Модель для отображения и управления списком книг в табличном виде
 *
 * Наследует QAbstractTableModel для интеграции с Qt View/Model архитектурой
 */
class BookModel : public QAbstractTableModel {
    Q_OBJECT

public:
    /**
     * @brief Конструктор модели книг
     * @param books Ссылка на список книг для инициализации модели
     * @param parent Родительский объект в иерархии Qt
     */
    explicit BookModel(QObject* parent = nullptr);

    void UpdateBookAt(int index, const Book& book);

    /**
     * @brief Возвращает количество строк в модели
     * @param parent Родительский индекс (не используется для табличных моделей)
     * @return Количество книг в модели
     */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief Возвращает количество столбцов в модели
     * @param parent Родительский индекс (не используется для табличных моделей)
     * @return Количество столбцов (полей книги)
     */
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief Возвращает данные для отображения в указанной ячейке
     * @param index Индекс ячейки (строка, столбец)
     * @param role Роль данных (отображение, редактирование и т.д.)
     * @return Данные для указанной ячейки и роли
     */
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    /**
     * @brief Возвращает заголовки для столбцов и строк
     * @param section Номер секции (столбца или строки)
     * @param orientation Ориентация заголовка (горизонтальный или вертикальный)
     * @param role Роль данных заголовка
     * @return Данные заголовка для указанной секции
     */
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /**
     * @brief Добавляет новую книгу в модель
     * @param book Книга для добавления
     */
    void AddBook(const Book& book);

    /**
     * @brief Удаляет книгу из модели по коду
     * @param code Уникальный код книги для удаления
     */
    bool RemoveBook(const QString& code);

    /**
     * @brief Находит книгу по коду
     * @param code Уникальный код книги для поиска
     * @return Найденная книга или пустая книга если не найдена
     */
    Book FindBook(const QString& code) const;

    std::optional<int> FindBookIndex(const QString &code) const;

    bool SetBookTaken(const QString &code, bool isTaken, std::optional<QDate> &date);

    /**
     * @brief Возвращает константную ссылку на список книг
     * @return Константная ссылка на список всех книг
     */
    const QList<Book>& GetBooks() const;

    /**
     * @brief Сохраняет список книг в файл
     * @param filePath Путь к файлу для сохранения
     * @return true если сохранение успешно, false в случае ошибки
     */
    bool SaveToFile(const QString& filePath) const;

    /**
     * @brief Загружает список книг из файла
     * @param filePath Путь к файлу для загрузки
     * @return true если загрузка успешна, false в случае ошибки
     */
    bool LoadFromFile(const QString& filePath);

    /**
     * @brief Сохраняет список книг в XML
     */
    bool SaveToXml(const QString& filePath) const;

    /**
     * @brief Загружает список книг из XML
     */
    bool LoadFromXml(const QString& filePath);

    static QString GenerateBookCode(const QList<Book>& existingBooks);

    // --- Работа с базой данных ---
    bool LoadFromDatabase();
    bool InsertOrUpdateInDatabase(const Book& book);
    bool DeleteFromDatabase(const QString& code);
    bool UpdateBookCodeInDatabase(const QString& oldCode, const Book& book);

private:



private:
    QList<Book> books_; ///< Список книг, управляемых моделью
};

#endif // BOOKMODEL_H
