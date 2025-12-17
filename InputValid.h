/**
 * @file InputValid.h
 * @author Кирилл К
 * @brief Валидатор пользовательского ввода (книги/читатели/поиск/выдача).
 * @version 1.1
 * @date 2025-12-12
 *
 * Класс InputValid содержит статические методы проверки строковых полей,
 * вводимых пользователем в интерфейсе. При ошибке выбрасывает исключения
 * из Exception.h.
 */

#ifndef INPUTVALID_H
#define INPUTVALID_H

#include <QString>
#include <optional>

/**
 * @class InputValid
 * @brief Набор статических проверок корректности ввода.
 *
 * Используется перед созданием/редактированием сущностей и выполнением действий,
 * чтобы отсеивать мусорный ввод и выводить понятные сообщения об ошибках.
 *
 * @note Методы бросают исключения (например InvalidInputException и др.).
 */
class InputValid
{
public:
    /** @name Публичные проверки
     *  @{
     */

    /**
     * @brief Проверка полей при добавлении книги.
     * @param name Название книги.
     * @param author Автор книги.
     *
     * @throw EmptyBookNameException если название пустое
     * @throw InvalidBookNameException если название содержит недопустимые символы/формат
     * @throw EmptyAuthorException если автор пустой
     * @throw InvalidAuthorException если автор содержит недопустимые символы/формат
     */
    static void checkAddBook(const QString &name, const QString &author);

    /**
     * @brief Проверка полей при редактировании книги.
     * @param name Новое название.
     * @param author Новый автор.
     *
     * Логика аналогична checkAddBook().
     */
    static void checkEditBook(const QString &name, const QString &author);

    /**
     * @brief Проверка полей при добавлении читателя.
     * @param surname Фамилия.
     * @param name Имя.
     * @param thname Отчество (может отсутствовать).
     *
     * @throw EmptyReaderSurnameException если фамилия пустая
     * @throw EmptyReaderNameException если имя пустое
     * @throw InvalidReaderException если поля содержат недопустимые символы/формат
     */
    static void checkAddReader(const QString &surname,
                               const QString &name,
                               const std::optional<QString> &thname);

    /**
     * @brief Проверка ввода для выдачи книги.
     * @param code Код (шифр) книги.
     * @param readerID ID читателя.
     *
     * @throw InvalidInputException если формат/поля неверные
     */
    static void checkGiveOutInput(const QString &code, const QString &readerID);

    /**
     * @brief Проверка строки поиска книги.
     * @param query Запрос (может быть кодом или названием).
     *
     * @throw InvalidInputException если запрос пустой или содержит недопустимые символы
     */
    static void checkBookSearch(const QString &query);

    /** @} */

private:
    /** @name Внутренние служебные функции
     *  @{
     */

    /**
     * @brief Нормализация пробелов (сжатие подряд идущих пробелов).
     * @param s Исходная строка.
     * @return Нормализованная строка.
     */
    static QString normalizeSpaces(const QString &s);

    /**
     * @brief Подсчёт количества букв (Unicode).
     * @param s Строка.
     * @return Количество букв в строке.
     */
    static int countLetters(const QString &s);

    /**
     * @brief Подсчёт количества цифр.
     * @param s Строка.
     * @return Количество цифр в строке.
     */
    static int countDigits(const QString &s);

    /**
     * @brief Проверка на длинные повторы пунктуации/символов.
     * @param s Строка.
     * @return true если есть подозрительные повторы.
     */
    static bool hasLongRepeatedPunct(const QString &s);

    /**
     * @brief Проверка на подряд идущие дефисы/апострофы.
     * @param s Строка.
     * @return true если обнаружены нежелательные последовательности.
     */
    static bool hasAdjacentInvalidHyphensOrApostrophes(const QString &s);

    /**
     * @brief Индекс первого буквенно-цифрового символа.
     * @param s Строка.
     * @return Индекс или -1 если не найдено.
     */
    static int firstAlnumIndex(const QString &s);

    /**
     * @brief Индекс последнего буквенно-цифрового символа.
     * @param s Строка.
     * @return Индекс или -1 если не найдено.
     */
    static int lastAlnumIndex(const QString &s);

    /**
     * @brief Общая проверка поля ФИО (имя/фамилия/отчество).
     * @param value Значение поля.
     * @param fieldName Название поля для сообщения об ошибке.
     *
     * @throw InvalidReaderException при ошибке формата
     */
    static void validatePersonNameField(const QString &value, const QString &fieldName);

    /** @} */
};

#endif // INPUTVALID_H
