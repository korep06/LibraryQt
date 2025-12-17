/**
 * @file DataBaseManager.h
 * @author Кирилл К
 * @brief Объявление менеджера базы данных (SQLite через Qt SQL).
 * @version 1.1
 * @date 2025-12-06
 *
 * Класс DatabaseManager реализует простой доступ к SQLite базе данных через
 * QSqlDatabase. Используется как одиночка (Singleton), хранит подключение и
 * инициализирует структуру БД при первом запуске.
 */

#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QString>

/**
 * @class DatabaseManager
 * @brief Одиночка для работы с SQLite-базой.
 *
 * Основные задачи:
 * - открыть подключение к файлу БД;
 * - предоставить объект QSqlDatabase моделям/сервисам;
 * - при необходимости создать таблицы (schema).
 *
 * @note В рамках курсового проекта обычно используется из GUI-потока.
 */
class DatabaseManager
{
public:
    /**
     * @brief Получить единственный экземпляр менеджера.
     * @return Ссылка на DatabaseManager.
     */
    static DatabaseManager& instance();

    /// Запрещаем копирование (одиночка).
    DatabaseManager(const DatabaseManager&) = delete;
    /// Запрещаем присваивание (одиночка).
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    /**
     * @brief Открыть (или создать) базу данных по имени файла.
     * @param fileName Имя файла SQLite (например: "library.db").
     * @return true если подключение успешно установлено.
     */
    bool open(const QString& fileName);   // например "library.db"

    /**
     * @brief Получить объект базы данных Qt.
     * @return Копия QSqlDatabase (указатель на то же подключение Qt).
     */
    QSqlDatabase database() const;

private:
    /**
     * @brief Закрытый конструктор (одиночка).
     */
    DatabaseManager();

    /**
     * @brief Деструктор (закрывает соединение).
     */
    ~DatabaseManager();

    /**
     * @brief Создать структуру БД (таблицы), если они отсутствуют.
     * @return true если схема создана/уже существует.
     */
    bool initSchema();

    /**
     * @brief Закрыть подключение к базе данных.
     */
    void closeConnection();

    QSqlDatabase db_; ///< Храним подключение к SQLite.
};

#endif // DATABASEMANAGER_H
