/**
 * @file ReaderModel.h
 * @author Кирилл К
 * @brief Модель для управления данными о читателях
 * @version 1.0
 * @date 2024-12-19
 */

#ifndef READERMODEL_H
#define READERMODEL_H

#pragma once

#include <QString>
#include <QList>
#include <optional>
#include <QAbstractTableModel>

/**
 * @enum Sex
 * @brief Перечисление для обозначения пола читателя
 */
enum Sex {
    Female, ///< Женский пол
    Male,   ///< Мужской пол
};

/**
 * @struct Reader
 * @brief Структура для хранения информации о читателе
 */
struct Reader {
    QString ID;              ///< Уникальный идентификатор читателя
    QString first_name;      ///< Имя читателя
    QString second_name;     ///< Фамилия читателя
    QString third_name;      ///< Отчество читателя
    Sex gender;              ///< Пол читателя
    QList<QString> taken_books; ///< Список кодов взятых книг
};

/**
 * @class ReaderModel
 * @brief Модель для отображения и управления списком читателей в табличном виде
 *
 * Наследует QAbstractTableModel для интеграции с Qt View/Model архитектурой.
 * Обеспечивает работу с данными читателей библиотеки.
 */
class ReaderModel : public QAbstractTableModel {
    Q_OBJECT

public:
    /**
     * @brief Конструктор модели читателей
     * @param readers Ссылка на список читателей для инициализации модели
     * @param parent Родительский объект в иерархии Qt
     */
    explicit ReaderModel(QObject* parent = nullptr);

    void UpdateReaderAt(int index, const Reader& reader);

    /**
     * @brief Возвращает количество строк в модели
     * @param parent Родительский индекс (не используется для табличных моделей)
     * @return Количество читателей в модели
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * @brief Возвращает количество столбцов в модели
     * @param parent Родительский индекс (не используется для табличных моделей)
     * @return Количество столбцов (полей читателя)
     */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * @brief Возвращает данные для отображения в указанной ячейке
     * @param index Индекс ячейки (строка, столбец)
     * @param role Роль данных (отображение, редактирование и т.д.)
     * @return Данные для указанной ячейки и роли
     */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /**
     * @brief Возвращает заголовки для столбцов и строк
     * @param section Номер секции (столбца или строки)
     * @param orientation Ориентация заголовка (горизонтальный или вертикальный)
     * @param role Роль данных заголовка
     * @return Данные заголовка для указанной секции
     */
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /**
     * @brief Добавляет нового читателя в модель
     * @param reader Читатель для добавления
     */
    void AddReader(const Reader& reader);

    /**
     * @brief Удаляет читателя из модели по идентификатору
     * @param id Уникальный идентификатор читателя для удаления
     */
    bool RemoveReader(const QString& id);

    bool AddLinkBook(const QString& reader_id , const QString& book_code);

    bool RemoveLinkBook(const QString& reader_id , const QString& book_code);

    std::optional<int> FindReaderIndex(const QString &id) const;

    /**
     * @brief Находит читателя по идентификатору
     * @param id Уникальный идентификатор читателя для поиска
     * @return std::optional с найденным читателем или std::nullopt если не найден
     */
    std::optional<Reader> FindReader(const QString& id);

    /**
     * @brief Возвращает копию списка читателей
     * @return Копия списка всех читателей
     */
    const QList<Reader> GetReaders() const;

    /**
     * @brief Загружает список читателей из файла
     * @param filePath Путь к файлу для загрузки
     * @return true если загрузка успешна, false в случае ошибки
     */
    bool LoadFromFile(const QString& filePath);

    /**
     * @brief Сохраняет список читателей в файл
     * @param filePath Путь к файлу для сохранения
     * @return true если сохранение успешно, false в случае ошибки
     */
    bool SaveToFile(const QString& filePath);

    /**
     * @brief Загружает список читателей из XML
     */
    bool LoadFromXml(const QString& filePath);

    /**
     * @brief Сохраняет список читателей в XML
     */
    bool SaveToXml(const QString& filePath);


    static QString GenerateReaderID(const QList<Reader>& existingReaders);

private:


private:
    QList<Reader> readers_; ///< Список читателей, управляемых моделью
};

#endif // READERMODEL_H
