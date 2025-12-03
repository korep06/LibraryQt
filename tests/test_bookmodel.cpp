// tests/tst_bookmodel.cpp
#include <QtTest>
#include "BookModel.h"
#include "Exception.h"

// Класс с тестами для BookModel
class BookModelTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Аналог @BeforeClass – вызывается один раз перед всеми тестами
        qDebug() << "== Начало тестов BookModel ==";
    }

    void cleanupTestCase() {
        // Аналог @AfterClass – вызывается один раз после всех тестов
        qDebug() << "== Конец тестов BookModel ==";
    }

    // 1) Тест добавления книги
    void testAddBookIncreasesRowCount() {
        BookModel model;
        QCOMPARE(model.rowCount(), 0);

        Book b{ "B001", "Война и мир", "Лев Толстой", false, std::nullopt };
        model.AddBook(b);

        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.data(model.index(0, 0)).toString(), QString("B001"));
        QCOMPARE(model.data(model.index(0, 1)).toString(), QString("Война и мир"));
    }

    // 2) Удаление существующей книги
    void testRemoveExistingBook() {
        BookModel model;
        model.AddBook({"B001", "Книга", "Автор", false, std::nullopt});

        QVERIFY(model.RemoveBook("B001"));
        QCOMPARE(model.rowCount(), 0);
    }

    // 3) Попытка удалить выданную книгу → должно бросить BookDeleteForbiddenException
    void testRemoveTakenBookThrows() {
        BookModel model;
        model.AddBook({"B001", "Книга", "Автор",
                       true, QDate::fromString("01/01/2024", "dd/MM/yyyy")});

        QVERIFY_EXCEPTION_THROWN(
            model.RemoveBook("B001"),
            BookDeleteForbiddenException
            );
    }

    // 4) Поиск несуществующей книги
    void testFindBookUnknownReturnsEmpty() {
        BookModel model;
        Book b = model.FindBook("NO_CODE");
        QVERIFY(b.code.isEmpty());
    }

    // 5) Установка статуса "выдана" + даты
    void testSetBookTakenUpdatesStatusAndDate() {
        BookModel model;
        model.AddBook({"B001", "Книга", "Автор", false, std::nullopt});

        std::optional<QDate> date = QDate(2024, 5, 10);
        bool ok = model.SetBookTaken("B001", true, date);
        QVERIFY(ok);

        Book updated = model.FindBook("B001");
        QVERIFY(updated.is_taken);
        QVERIFY(updated.date_taken.has_value());
        QCOMPARE(updated.date_taken.value(), date.value());
    }

    // 6) Генерация кода книги – код начинается с 'B' и не пересекается с существующими
    void testGenerateBookCodeIsUnique() {
        QList<Book> existing;
        existing.append({"B1234", "Книга1", "Автор1", false, std::nullopt});
        existing.append({"B5678", "Книга2", "Автор2", false, std::nullopt});

        QString code = BookModel::GenerateBookCode(existing);

        QVERIFY(code.startsWith("B"));

        bool collision = false;
        for (const auto &b : existing) {
            if (b.code == code) {
                collision = true;
                break;
            }
        }
        QVERIFY(!collision);
    }
};

QTEST_APPLESS_MAIN(BookModelTest)
#include "test_bookmodel.moc"
