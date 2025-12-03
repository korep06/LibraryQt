#include "Logger.h"

Logger& Logger::instance()
{
    static Logger inst;
    return inst;
}

Logger::Logger() = default;

Logger::~Logger()
{
    if (file_.isOpen()) {
        file_.flush();
        file_.close();
    }
}

bool Logger::init(const QString& configPath)
{
    // Читаем конфиг вида:
    // [logging]
    // level = DEBUG
    // file  = library.log
    QSettings settings(configPath, QSettings::IniFormat);
    const QString levelStr = settings
                                 .value("logging/level", "INFO")
                                 .toString()
                                 .toUpper();
    const QString fileName = settings
                                 .value("logging/file", "library.log")
                                 .toString();

    level_ = parseLevel(levelStr);

    file_.setFileName(fileName);
    if (!file_.open(QIODevice::Append | QIODevice::Text)) {
        // если не смогли открыть файл — логгер не инициализирован
        return false;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream_.setEncoding(QStringConverter::Utf8);
#else
    stream_.setCodec("UTF-8");
#endif

    return true;
}

void Logger::log(LogLevel level,
                 const QString& category,
                 const QString& message)
{
    if (level_ == LogLevel::Off || level < level_) {
        return; // уровень ниже текущего — не логируем
    }

    const QString dt   = QDateTime::currentDateTime()
                           .toString("yyyy-MM-dd hh:mm:ss.zzz");
    const QString lvl  = levelToString(level);
    const QString thId = QString::number(
        (qulonglong)QThread::currentThreadId(),
        16);

    QString line = QString("%1 [%2] (thr:%3) {%4} %5\n")
                       .arg(dt, lvl, thId, category, message);

    QMutexLocker locker(&mutex_);
    if (file_.isOpen()) {
        stream_ << line;
        stream_.flush();
    }
}

void Logger::setLevel(LogLevel level)
{
    QMutexLocker locker(&mutex_);
    level_ = level;
}

LogLevel Logger::level() const
{
    return level_;
}

LogLevel Logger::parseLevel(const QString& s) const
{
    if (s == "DEBUG") return LogLevel::Debug;
    if (s == "INFO")  return LogLevel::Info;
    if (s == "WARN")  return LogLevel::Warn;
    if (s == "ERROR") return LogLevel::Error;
    if (s == "OFF")   return LogLevel::Off;
    return LogLevel::Info;
}

QString Logger::levelToString(LogLevel level) const
{
    switch (level) {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info:  return "INFO";
    case LogLevel::Warn:  return "WARN";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Off:   return "OFF";
    }
    return "UNK";
}
