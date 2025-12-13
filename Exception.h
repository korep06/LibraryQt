#pragma once
#include <stdexcept>
#include <QString>

// Базовый класс для наследования
class AppException : public std::runtime_error {
public:
    explicit AppException(const QString &msg)
        : std::runtime_error(msg.toStdString()) {}
};

// Исключения для книг
class EmptyBookNameException : public AppException {
public:
    explicit EmptyBookNameException(const QString &msg)
        : AppException(msg) {}
};

class EmptyAuthorException : public AppException {
public:
    explicit EmptyAuthorException(const QString &msg)
        : AppException(msg) {}
};

class InvalidBookNameException : public AppException {
public:
    explicit InvalidBookNameException(const QString &msg)
        : AppException(msg) {}
};

class InvalidAuthorException : public AppException {
public:
    explicit InvalidAuthorException(const QString &msg)
        : AppException(msg) {}
};

class BookNotFoundException : public AppException {
public:
    explicit BookNotFoundException(const QString &msg)
        : AppException(msg) {}
};

// Исключения для читателей
class EmptyReaderNameException : public AppException {
public:
    explicit EmptyReaderNameException(const QString &msg)
        : AppException(msg) {}
};

class InvalidReaderException : public AppException {
public:
    explicit InvalidReaderException(const QString &msg)
        : AppException(msg) {}
};

class EmptyReaderSurnameException : public AppException {
public:
    explicit EmptyReaderSurnameException(const QString &msg)
        : AppException(msg) {}
};

class ReaderNotFoundException : public AppException {
public:
    explicit ReaderNotFoundException(const QString &msg)
        : AppException(msg) {}
};

// Исключения для выдачи и взятия книг
class BookAlreadyTakenException : public AppException {
public:
    explicit BookAlreadyTakenException(const QString &msg)
        : AppException(msg) {}
};

class BookAlreadyAvailableException : public AppException {
public:
    explicit BookAlreadyAvailableException(const QString &msg)
        : AppException(msg) {}
};

class InvalidInputException : public AppException {
public:
    explicit InvalidInputException(const QString &msg)
        : AppException(msg) {}
};

//Исключения для редактирования книг и читателей
class BookDeleteForbiddenException : public AppException {
public:
    explicit BookDeleteForbiddenException(const QString &msg)
        : AppException(msg) {}
};

class ReaderDeleteForbiddenException : public AppException {
public:
    explicit ReaderDeleteForbiddenException(const QString &msg)
        : AppException(msg) {}
};

