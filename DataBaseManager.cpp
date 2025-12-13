#include "DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

namespace {
// Чтобы Qt не ругался на "duplicate connection name" и не плодил дефолтные подключения.
const char kConnName[] = "LibraryQtConnection";

// Сейчас используем SQLite. Для ODBC можно заменить на "QODBC" (см. TODO в .h).
const char kDriverName[] = "QSQLITE";
} // namespace

DatabaseManager::DatabaseManager() = default;

DatabaseManager::~DatabaseManager()
{
    closeConnection();
}

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager inst;
    return inst;
}

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

QSqlDatabase DatabaseManager::database() const
{
    // QSqlDatabase — это "handle", копирование дешёвое.
    return db_;
}

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
