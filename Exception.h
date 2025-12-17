/**
 * @file Exception.h
 * @author Кирилл К
 * @brief Набор пользовательских исключений приложения библиотеки.
 * @version 1.1
 * @date 2025-12-14
 *
 * В этом файле определены исключения для:
 * - книг (валидация, поиск, удаление);
 * - читателей (валидация, поиск, удаление);
 * - операций выдачи/возврата;
 * - некорректного пользовательского ввода.
 *
 * Все исключения наследуются от AppException.
 */

#pragma once
#include <stdexcept>
#include <QString>

/**
 * @class AppException
 * @brief Базовый класс всех исключений приложения.
 *
 * Наследуется от std::runtime_error, принимает сообщение в QString.
 */
class AppException : public std::runtime_error {
public:
    /**
     * @brief Конструктор исключения.
     * @param msg Сообщение об ошибке.
     */
    explicit AppException(const QString &msg)
        : std::runtime_error(msg.toStdString()) {}
};

/// @name Исключения для книг
/// @{

/**
 * @class EmptyBookNameException
 * @brief Пустое название книги.
 */
class EmptyBookNameException : public AppException {
public:
    explicit EmptyBookNameException(const QString &msg)
        : AppException(msg) {}
};

/**
 * @class EmptyAuthorException
 * @brief Пустое поле автора книги.
 */
class EmptyAuthorException : public AppException {
public:
    explicit EmptyAuthorException(const QString &msg)
        : AppException(msg) {}
};

/**
 * @class InvalidBookNameException
 * @brief Некорректное название книги (мусорный ввод / неверный формат).
 */
class InvalidBookNameException : public AppException {
public:
    explicit InvalidBookNameException(const QString &msg)
        : AppException(msg) {}
};

/**
 * @class InvalidAuthorException
 * @brief Некорректное имя автора (мусорный ввод / неверный формат).
 */
class InvalidAuthorException : public AppException {
public:
    explicit InvalidAuthorException(const QString &msg)
        : AppException(msg) {}
};

/**
 * @class BookNotFoundException
 * @brief Книга не найдена.
 */
class BookNotFoundException : public AppException {
public:
    explicit BookNotFoundException(const QString &msg)
        : AppException(msg) {}
};

/// @}

/// @name Исключения для читателей
/// @{

/**
 * @class EmptyReaderNameException
 * @brief Пустое имя читателя.
 */
class EmptyReaderNameException : public AppException {
public:
    explicit EmptyReaderNameException(const QString &msg)
        : AppException(msg) {}
};

/**
 * @class InvalidReaderException
 * @brief Некорректные данные читателя (ФИО/формат).
 */
class InvalidReaderException : public AppException {
public:
    explicit InvalidReaderException(const QString &msg)
        : AppException(msg) {}
};

/**
 * @class EmptyReaderSurnameException
 * @brief Пустая фамилия читателя.
 */
class EmptyReaderSurnameException : public AppException {
public:
    explicit EmptyReaderSurnameException(const QString &msg)
        : AppException(msg) {}
};

/**
 * @class ReaderNotFoundException
 * @brief Читатель не найден.
 */
class ReaderNotFoundException : public AppException {
public:
    explicit ReaderNotFoundException(const QString &msg)
        : AppException(msg) {}
};

/// @}

/// @name Исключения для выдачи и возврата книг
/// @{

/**
 * @class BookAlreadyTakenException
 * @brief Книга уже выдана (нельзя выдать повторно).
 */
class BookAlreadyTakenException : public AppException {
public:
    explicit BookAlreadyTakenException(const QString &msg)
        : AppException(msg) {}
};

/**
 * @class BookAlreadyAvailableException
 * @brief Книга уже в наличии (нельзя "вернуть" то, что не выдано).
 */
class BookAlreadyAvailableException : public AppException {
public:
    explicit BookAlreadyAvailableException(const QString &msg)
        : AppException(msg) {}
};

/**
 * @class InvalidInputException
 * @brief Некорректный ввод (общий случай).
 */
class InvalidInputException : public AppException {
public:
    explicit InvalidInputException(const QString &msg)
        : AppException(msg) {}
};

/// @}

/// @name Исключения ограничений (удаление/правила предметной области)
/// @{

/**
 * @class BookDeleteForbiddenException
 * @brief Нельзя удалить книгу из-за ограничений (например, книга выдана).
 */
class BookDeleteForbiddenException : public AppException {
public:
    explicit BookDeleteForbiddenException(const QString &msg)
        : AppException(msg) {}
};

/**
 * @class ReaderDeleteForbiddenException
 * @brief Нельзя удалить читателя из-за ограничений (например, есть книги).
 */
class ReaderDeleteForbiddenException : public AppException {
public:
    explicit ReaderDeleteForbiddenException(const QString &msg)
        : AppException(msg) {}
};

/// @}
