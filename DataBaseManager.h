#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QString>

/**
 * @brief Одиночка для работы с SQLite-базой.
 */
class DatabaseManager
{
public:
    static DatabaseManager& instance();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool open(const QString& fileName);   // например "library.db"
    QSqlDatabase database() const;

private:
    DatabaseManager();
    ~DatabaseManager();

    bool initSchema();
    void closeConnection();

    QSqlDatabase db_;
};

#endif // DATABASEMANAGER_H
