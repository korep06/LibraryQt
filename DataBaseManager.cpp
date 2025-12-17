/**
 * @file DataBaseManager.cpp
 * @author Кирилл К
 * @brief Реализация менеджера базы данных (SQLite через Qt SQL).
 * @version 1.1
 * @date 2025-12-07
 *
 * В этом файле реализованы:
 * - открытие/переиспользование подключения QSqlDatabase;
 * - закрытие подключения;
 * - создание таблиц при первом запуске (initSchema).
 */

#include "DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

namespace {
/**
 * @brief Имя подключения QSqlDatabase внутри Qt.
 *
 * Нужен фиксированный connection name, чтобы Qt не создавал дубликаты подключений
 * и не появлялись предупреждения про "duplicate connection name".
 */
const char kConnName[] = "LibraryQtConnection";

/**
 * @brief Имя драйвера базы данных.
 *
 * Сейчас используется SQLite ("QSQLITE"). При необходимости можно заменить на ODBC
 * ("QODBC") и настроить строку подключения.
 */
const char kDriverName[] = "QSQLITE";
} // namespace

/**
 * @brief Конструктор менеджера БД.
 *
 * Закрытый (Singleton). Реальная инициализация соединения выполняется в open().
 */
DatabaseManager::DatabaseManager() = default;

/**
 * @brief Деструктор менеджера БД.
 *
 * Корректно закрывает соединение и удаляет его из Qt connection pool.
 */
DatabaseManager::~DatabaseManager()
{
    closeConnection();
}

/**
 * @brief Доступ к единственному экземпляру DatabaseManager.
 * @return Ссылка на статический объект-одиночку.
 */
DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager inst;
    return inst;
}

/**
 * @brief Открывает (или создаёт) SQLite базу данных.
 *
 * Если соединение уже открыто — просто возвращает true.
 * Если соединение ранее создавалось, переиспользует его по имени kConnName.
 *
 * @param fileName Имя файла базы данных (например, "library.db").
 * @return true при успешном открытии и успешной инициализации схемы.
 */
bool DatabaseManager::open(const QString& fileName)
{
    // Уже открыто — просто ок.
    if (db_.isValid() && db_.isOpen())
        return true;

    // Переиспользуем одно и то же подключение по имени.
    if (QSqlDatabase::contains(kConnName))
        db_ = QSqlDatabase::database(kConnName);
    else
        db_ = QSqlDatabase::addDatabase(kDriverName, kConnName);

    db_.setDatabaseName(fileName);

    if (!db_.open()) {
        qDebug() << "DB open error:" << db_.lastError().text();
        return false;
    }

    return initSchema();
}

/**
 * @brief Возвращает объект подключения QSqlDatabase.
 *
 * @return "handle" QSqlDatabase (копирование дешёвое, это не реальная копия базы).
 */
QSqlDatabase DatabaseManager::database() const
{
    // QSqlDatabase — это "handle", копирование дешёвое.
    return db_;
}

/**
 * @brief Закрывает соединение и удаляет его из менеджера подключений Qt.
 *
 * @note В Qt важно вызывать removeDatabase() после того как все объекты,
 *       использующие это соединение, перестали его использовать.
 */
void DatabaseManager::closeConnection()
{
    if (!db_.isValid())
        return;

    const QString name = db_.connectionName();

    if (db_.isOpen())
        db_.close();

    db_ = QSqlDatabase();
    QSqlDatabase::removeDatabase(name);
}

/**
 * @brief Создаёт таблицы БД, если они ещё не существуют.
 *
 * Создаются:
 * - books (книги)
 * - readers (читатели)
 *
 * @return true если таблицы созданы успешно (или уже существовали).
 */
bool DatabaseManager::initSchema()
{
    QSqlQuery q(db_);

    // Таблица книг
    if (!q.exec(
            "CREATE TABLE IF NOT EXISTS books ("
            " code TEXT PRIMARY KEY,"
            " name TEXT NOT NULL,"
            " author TEXT NOT NULL,"
            " is_taken INTEGER NOT NULL,"
            " date_taken TEXT"
            ")")) {
        qDebug() << "Create books table error:" << q.lastError().text();
        return false;
    }

    // Таблица читателей
    if (!q.exec(
            "CREATE TABLE IF NOT EXISTS readers ("
            " id TEXT PRIMARY KEY,"
            " first_name  TEXT NOT NULL,"
            " second_name TEXT NOT NULL,"
            " third_name  TEXT,"
            " gender INTEGER NOT NULL,"
            " reg_date TEXT,"
            " taken_books TEXT"
            ")")) {
        qDebug() << "Create readers table error:" << q.lastError().text();
        return false;
    }

    return true;
}
