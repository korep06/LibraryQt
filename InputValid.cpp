#include "InputValid.h"
#include "Exception.h"

#include <QRegularExpression>

// ----------------------------------
// Регулярки (чтобы не создавать их на каждый вызов)
// ЛОГИКА НЕ МЕНЯЕТСЯ — только вынесено в static const
// ----------------------------------
namespace {

const QRegularExpression kLongRepeatedPunctRe(QStringLiteral("([\\p{P}\\p{S}])\\1{2,}"));
const QRegularExpression kAdjacentInvalidHyphensOrAposRe(QStringLiteral("(-{2,}|'{2,}|`{2,})"));

const QRegularExpression kStartRepeatPunctRe(QStringLiteral("^[\\p{P}\\p{S}]{1,}"));
const QRegularExpression kAllowedTitleRe(QStringLiteral(
    "^[\\p{L}\\p{N}\\s\\.,:;!\\?()\\[\\]{}'\"«»\\-–—/\\\\+&%#@]+$"));

const QRegularExpression kAuthorRe(QStringLiteral("^[\\p{L}\\-\\'\\.\\s]+$"));
const QRegularExpression kPersonNameRe(QStringLiteral("^[\\p{L}\\-\\'\\.\\s]+$"));

const QRegularExpression kBookCodeRe(QStringLiteral("^B[0-9]{3,5}$"),
                                     QRegularExpression::CaseInsensitiveOption);
const QRegularExpression kReaderIdRe(QStringLiteral("^R[0-9]{4}$"),
                                     QRegularExpression::CaseInsensitiveOption);

const QRegularExpression kSearchRe(QStringLiteral(
    "^[\\p{L}\\p{N}\\s\\.,:;!\\?()\\[\\]{}'\"«»\\-–—/\\\\+&%#@]+$"));

} // namespace

// ----------------------------------
// Вспомогательные функции
// ----------------------------------

QString InputValid::normalizeSpaces(const QString &s)
{
    return s.simplified();
}

int InputValid::countLetters(const QString &s)
{
    int cnt = 0;
    for (const QChar c : s)
        if (c.isLetter()) ++cnt;
    return cnt;
}

int InputValid::countDigits(const QString &s)
{
    int cnt = 0;
    for (const QChar c : s)
        if (c.isDigit()) ++cnt;
    return cnt;
}

bool InputValid::hasLongRepeatedPunct(const QString &s)
{
    return kLongRepeatedPunctRe.match(s).hasMatch();
}

bool InputValid::hasAdjacentInvalidHyphensOrApostrophes(const QString &s)
{
    return kAdjacentInvalidHyphensOrAposRe.match(s).hasMatch();
}

int InputValid::firstAlnumIndex(const QString &s)
{
    for (int i = 0; i < s.size(); ++i)
        if (s[i].isLetterOrNumber())
            return i;
    return -1;
}

int InputValid::lastAlnumIndex(const QString &s)
{
    for (int i = s.size() - 1; i >= 0; --i)
        if (s[i].isLetterOrNumber())
            return i;
    return -1;
}

void InputValid::validatePersonNameField(const QString &value, const QString &fieldName)
{
    // Сообщения и логика сохранены (как было в лямбде)
    if (value.isEmpty())
        throw InvalidReaderException(fieldName + " не может быть пустым.");

    if (!kPersonNameRe.match(value).hasMatch())
        throw InvalidReaderException(fieldName + " содержит недопустимые символы.");

    if (hasAdjacentInvalidHyphensOrApostrophes(value))
        throw InvalidReaderException(fieldName + " содержит повторяющиеся символы.");

    const QChar first = value.front();
    const QChar last  = value.back();
    if (first == '-' || first == '\'' || first == '.' ||
        last  == '-' || last  == '\'' || last  == '.')
        throw InvalidReaderException(fieldName + " начинается/заканчивается пунктуацией.");

    if (countLetters(value) < 2)
        throw InvalidReaderException(fieldName + " должно содержать минимум 2 буквы.");
}

// ----------------------------------
// Проверки логики
// ----------------------------------

void InputValid::checkAddBook(const QString &name, const QString &author)
{
    const QString nameNorm = normalizeSpaces(name);
    const QString authorNorm = normalizeSpaces(author);

    if (nameNorm.isEmpty())
        throw EmptyBookNameException("Введите название книги!");

    if (kStartRepeatPunctRe.match(nameNorm).hasMatch()) {
        throw InvalidBookNameException("Название не может начинаться несколькими знаками пунктуации.");
    }

    if (!kAllowedTitleRe.match(nameNorm).hasMatch())
        throw InvalidBookNameException("Название содержит недопустимые символы.");

    const int letters = countLetters(nameNorm);
    const int digits = countDigits(nameNorm);

    if (letters + digits < 1)
        throw InvalidBookNameException("Название должно содержать буквы или цифры.");

    if (hasLongRepeatedPunct(nameNorm))
        throw InvalidBookNameException("Обнаружены повторяющиеся знаки пунктуации.");

    if (hasAdjacentInvalidHyphensOrApostrophes(nameNorm))
        throw InvalidBookNameException("Повторяющиеся дефисы/апострофы недопустимы.");

    const int firstAlnum = firstAlnumIndex(nameNorm);
    const int lastAlnum = lastAlnumIndex(nameNorm);

    if (firstAlnum < 0 || lastAlnum < 0)
        throw InvalidBookNameException("Нет буквенно-цифровых символов.");

    if (firstAlnum > 3)
        throw InvalidBookNameException("Название начинается с большого количества пунктуации.");

    const int nameLen = nameNorm.size();
    if (nameLen - 1 - lastAlnum > 3)
        throw InvalidBookNameException("Название заканчивается большим количеством пунктуации.");

    if (letters > 0 && letters < 2 && digits == 0) {
        if (!nameNorm.contains('+') && !nameNorm.contains('#'))
            throw InvalidBookNameException("Название слишком короткое.");
    }

    // ---------- Автор ----------
    if (authorNorm.isEmpty())
        throw EmptyAuthorException("Введите автора!");

    if (!kAuthorRe.match(authorNorm).hasMatch())
        throw InvalidAuthorException("Недопустимые символы в имени автора.");

    if (hasAdjacentInvalidHyphensOrApostrophes(authorNorm))
        throw InvalidAuthorException("Повторяющиеся дефисы/апострофы в имени автора.");

    const QChar firstA = authorNorm.front();
    const QChar lastA  = authorNorm.back();

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
    const QString sTrim = normalizeSpaces(surname);
    const QString nTrim = normalizeSpaces(name);

    if (sTrim.isEmpty())
        throw EmptyReaderSurnameException("Введите фамилию!");
    validatePersonNameField(sTrim, "Фамилия");

    if (nTrim.isEmpty())
        throw EmptyReaderNameException("Введите имя!");
    validatePersonNameField(nTrim, "Имя");

    if (thname.has_value()) {
        const QString t = normalizeSpaces(thname.value());
        if (!t.isEmpty()) {
            if (t.size() <= 2 && !t.contains('.'))
                throw InvalidReaderException("Отчество слишком короткое.");
            validatePersonNameField(t, "Отчество");
        }
    }
}

void InputValid::checkGiveOutInput(const QString &code, const QString &readerID)
{
    const QString c = code.trimmed().toUpper();
    const QString r = readerID.trimmed().toUpper();

    if (c.isEmpty() || r.isEmpty())
        throw InvalidInputException("Введите код книги и ID читателя!");

    if (!kBookCodeRe.match(c).hasMatch())
        throw InvalidInputException("Формат кода книги: B + 3–5 цифр.");

    if (!kReaderIdRe.match(r).hasMatch())
        throw InvalidInputException("Формат ID читателя: R + 4 цифры.");
}

void InputValid::checkBookSearch(const QString &query)
{
    const QString q = query.trimmed();
    if (q.isEmpty())
        throw InvalidInputException("Введите название или код книги!");

    if (!kSearchRe.match(q).hasMatch())
        throw InvalidInputException("Недопустимые символы в запросе.");
}
