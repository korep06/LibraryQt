#ifndef INPUTVALID_H
#define INPUTVALID_H

#include <QString>
#include <optional>

class InputValid
{
public:
    // Проверки
    static void checkAddBook(const QString &name, const QString &author);
    static void checkEditBook(const QString &name, const QString &author);
    static void checkAddReader(const QString &surname,
                               const QString &name,
                               const std::optional<QString> &thname);
    static void checkGiveOutInput(const QString &code, const QString &readerID);
    static void checkBookSearch(const QString &query);

private:
    // Внутренние служебные функции
    static QString normalizeSpaces(const QString &s);
    static int countLetters(const QString &s);
    static int countDigits(const QString &s);
    static bool hasLongRepeatedPunct(const QString &s);
    static bool hasAdjacentInvalidHyphensOrApostrophes(const QString &s);
    static int firstAlnumIndex(const QString &s);
    static int lastAlnumIndex(const QString &s);
};

#endif // INPUTVALID_H
