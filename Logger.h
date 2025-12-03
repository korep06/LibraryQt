#pragma once

#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>
#include <QThread>
#include <QSettings>

enum class LogLevel {
    Debug = 0,
    Info,
    Warn,
    Error,
    Off
};

class Logger
{
public:
    static Logger& instance();

    /// Инициализация логгера: читаем logging.ini и открываем файл
    bool init(const QString& configPath = "logging.ini");

    /// Основной метод логирования
    void log(LogLevel level,
             const QString& category,
             const QString& message);

    void setLevel(LogLevel level);
    LogLevel level() const;

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    LogLevel parseLevel(const QString& s) const;
    QString levelToString(LogLevel level) const;

private:
    QFile       file_;
    QTextStream stream_;
    LogLevel    level_ = LogLevel::Info;
    QMutex      mutex_;
};

// Удобные макросы, как в Log4j
#define LOG_DEBUG(cat, msg) Logger::instance().log(LogLevel::Debug, (cat), (msg))
#define LOG_INFO(cat, msg)  Logger::instance().log(LogLevel::Info,  (cat), (msg))
#define LOG_WARN(cat, msg)  Logger::instance().log(LogLevel::Warn,  (cat), (msg))
#define LOG_ERROR(cat, msg) Logger::instance().log(LogLevel::Error, (cat), (msg))
