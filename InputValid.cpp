#include "InputValid.h"
#include "Exception.h"

#include <QRegularExpression>

// ----------------------------------
// Вспомогательные функции
// ----------------------------------

QString InputValid::normalizeSpaces(const QString &s) {
    return s.simplified();
}

int InputValid::countLetters(const QString &s) {
    int cnt = 0;
    for (QChar c : s) if (c.isLetter()) ++cnt;
    return cnt;
}

int InputValid::countDigits(const QString &s) {
    int cnt = 0;
    for (QChar c : s) if (c.isDigit()) ++cnt;
    return cnt;
}

bool InputValid::hasLongRepeatedPunct(const QString &s) {
    QRegularExpression re("([\\p{P}\\p{S}])\\1{2,}");
    return re.match(s).hasMatch();
}

bool InputValid::hasAdjacentInvalidHyphensOrApostrophes(const QString &s) {
    QRegularExpression re("(-{2,}|'{2,}|`{2,})");
    return re.match(s).hasMatch();
}

int InputValid::firstAlnumIndex(const QString &s) {
    for (int i = 0; i < s.size(); ++i)
        if (s[i].isLetterOrNumber()) return i;
    return -1;
}

int InputValid::lastAlnumIndex(const QString &s) {
    for (int i = s.size() - 1; i >= 0; --i)
        if (s[i].isLetterOrNumber()) return i;
    return -1;
}

// ----------------------------------
// Проверки логики
// ----------------------------------

void InputValid::checkAddBook(const QString &name, const QString &author)
{
    QString nameNorm = normalizeSpaces(name);
    QString authorNorm = normalizeSpaces(author);

    if (nameNorm.isEmpty())
        throw EmptyBookNameException("Введите название книги!");

    QRegularExpression startRepeat("^[\\p{P}\\p{S}]{1,}");
    if (startRepeat.match(nameNorm).hasMatch()) {
        throw InvalidBookNameException("Название не может начинаться несколькими знаками пунктуации.");
    }

    QRegularExpression allowedTitleRe(
        "^[\\p{L}\\p{N}\\s\\.,:;!\\?()\\[\\]{}'\"«»\\-–—/\\\\+&%#@]+$");
    if (!allowedTitleRe.match(nameNorm).hasMatch())
        throw InvalidBookNameException("Название содержит недопустимые символы.");

    int letters = countLetters(nameNorm);
    int digits = countDigits(nameNorm);

    if (letters + digits < 1)
        throw InvalidBookNameException("Название должно содержать буквы или цифры.");

    if (hasLongRepeatedPunct(nameNorm))
        throw InvalidBookNameException("Обнаружены повторяющиеся знаки пунктуации.");

    if (hasAdjacentInvalidHyphensOrApostrophes(nameNorm))
        throw InvalidBookNameException("Повторяющиеся дефисы/апострофы недопустимы.");

    int firstAlnum = firstAlnumIndex(nameNorm);
    int lastAlnum = lastAlnumIndex(nameNorm);

    if (firstAlnum < 0 || lastAlnum < 0)
        throw InvalidBookNameException("Нет буквенно-цифровых символов.");

    if (firstAlnum > 3)
        throw InvalidBookNameException("Название начинается с большого количества пунктуации.");

    if ((int)nameNorm.size() - 1 - lastAlnum > 3)
        throw InvalidBookNameException("Название заканчивается большим количеством пунктуации.");

    if (letters > 0 && letters < 2 && digits == 0) {
        if (!nameNorm.contains('+') && !nameNorm.contains('#'))
            throw InvalidBookNameException("Название слишком короткое.");
    }

    // ---------- Автор ----------
    if (authorNorm.isEmpty())
        throw EmptyAuthorException("Введите автора!");

    QRegularExpression authorRe("^[\\p{L}\\-\\'\\.\\s]+$");
    if (!authorRe.match(authorNorm).hasMatch())
        throw InvalidAuthorException("Недопустимые символы в имени автора.");

    if (hasAdjacentInvalidHyphensOrApostrophes(authorNorm))
        throw InvalidAuthorException("Повторяющиеся дефисы/апострофы в имени автора.");

    QChar firstA = authorNorm.front();
    QChar lastA  = authorNorm.back();

    if (firstA == '-' || firstA == '\'' || firstA == '.' ||
        lastA  == '-' || lastA  == '\'' || lastA  == '.')
        throw InvalidAuthorException("Имя автора не может начинаться/заканчиваться пунктуацией.");

    if (countLetters(authorNorm) < 2)
        throw InvalidAuthorException("В имени автора минимум 2 буквы.");
}

void InputValid::checkEditBook(const QString &name, const QString &author)
{
    checkAddBook(name, author);
}

void InputValid::checkAddReader(const QString &surname,
                                const QString &name,
                                const std::optional<QString> &thname)
{
    auto validatePersonName = [&](const QString &value, const QString &fieldName) {
        if (value.isEmpty())
            throw InvalidReaderException(fieldName + " не может быть пустым.");

        QRegularExpression re("^[\\p{L}\\-\\'\\.\\s]+$");
        if (!re.match(value).hasMatch())
            throw InvalidReaderException(fieldName + " содержит недопустимые символы.");

        if (hasAdjacentInvalidHyphensOrApostrophes(value))
            throw InvalidReaderException(fieldName + " содержит повторяющиеся символы.");

        QChar first = value.front();
        QChar last  = value.back();
        if (first == '-' || first == '\'' || first == '.' ||
            last  == '-' || last  == '\'' || last  == '.')
            throw InvalidReaderException(fieldName + " начинается/заканчивается пунктуацией.");

        if (countLetters(value) < 2)
            throw InvalidReaderException(fieldName + " должно содержать минимум 2 буквы.");
    };

    QString sTrim = normalizeSpaces(surname);
    QString nTrim = normalizeSpaces(name);

    if (sTrim.isEmpty())
        throw EmptyReaderSurnameException("Введите фамилию!");
    validatePersonName(sTrim, "Фамилия");

    if (nTrim.isEmpty())
        throw EmptyReaderNameException("Введите имя!");
    validatePersonName(nTrim, "Имя");

    if (thname.has_value()) {
        QString t = normalizeSpaces(thname.value());
        if (!t.isEmpty()) {
            if (t.size() <= 2 && !t.contains('.'))
                throw InvalidReaderException("Отчество слишком короткое.");
            validatePersonName(t, "Отчество");
        }
    }
}

void InputValid::checkGiveOutInput(const QString &code, const QString &readerID)
{
    QString c = code.trimmed().toUpper();
    QString r = readerID.trimmed().toUpper();

    if (c.isEmpty() || r.isEmpty())
        throw InvalidInputException("Введите код книги и ID читателя!");

    QRegularExpression codeRe("^B[0-9]{3,5}$", QRegularExpression::CaseInsensitiveOption);
    if (!codeRe.match(c).hasMatch())
        throw InvalidInputException("Формат кода книги: B + 3–5 цифр.");

    QRegularExpression idRe("^R[0-9]{4}$", QRegularExpression::CaseInsensitiveOption);
    if (!idRe.match(r).hasMatch())
        throw InvalidInputException("Формат ID читателя: R + 4 цифры.");
}

void InputValid::checkBookSearch(const QString &query)
{
    QString q = query.trimmed();
    if (q.isEmpty())
        throw InvalidInputException("Введите название или код книги!");

    QRegularExpression searchRe(
        "^[\\p{L}\\p{N}\\s\\.,:;!\\?()\\[\\]{}'\"«»\\-–—/\\\\+&%#@]+$");
    if (!searchRe.match(q).hasMatch())
        throw InvalidInputException("Недопустимые символы в запросе.");
}
