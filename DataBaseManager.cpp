#include "DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

DatabaseManager::DatabaseManager() {}

DatabaseManager::~DatabaseManager()
{
    if (db_.isOpen())
        db_.close();
}

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager inst;
    return inst;
}

bool DatabaseManager::open(const QString& fileName)
{
    if (db_.isOpen())
        return true;

    db_ = QSqlDatabase::addDatabase("QSQLITE");
    db_.setDatabaseName(fileName);

    if (!db_.open()) {
        qDebug() << "DB open error:" << db_.lastError().text();
        return false;
    }
    return initSchema();
}

QSqlDatabase DatabaseManager::database() const
{
    return db_;
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
