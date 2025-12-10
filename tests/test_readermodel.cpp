#include <QtTest>
#include "ReaderModel.h"
#include "Exception.h"

class ReaderModelTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        qDebug() << "== Начало тестов ReaderModel ==";
    }

    void cleanupTestCase() {
        qDebug() << "== Конец тестов ReaderModel ==";
    }




    // 2) Удаление читателя без книг
    void testRemoveReaderWithoutBooks() {
        ReaderModel model;
        model.AddReader({"R001", "Иванов", "Иван", "Иванович", Sex::Male, QDate::currentDate(),  {}});
        QVERIFY(model.RemoveReader("R001"));
        QCOMPARE(model.rowCount(), 0);
    }

    // 3) Попытка удалить читателя с книгами → исключение
    void testRemoveReaderWithBooksThrows() {
        ReaderModel model;
        Reader r{"R001", "Иванов", "Иван", "Иванович", Sex::Male,QDate::currentDate() , {"B001"}};
        model.AddReader(r);

        QVERIFY_EXCEPTION_THROWN(
            model.RemoveReader("R001"),
            ReaderDeleteForbiddenException
            );
    }

    // 4) Добавление связи читатель–книга
    void testAddLinkBook() {
        ReaderModel model;
        model.AddReader({"R001", "Иванов", "Иван", "Иванович", Sex::Male, QDate::currentDate(), {}});

        bool ok = model.AddLinkBook("R001", "B001");
        QVERIFY(ok);

        auto readers = model.GetReaders();
        QCOMPARE(readers[0].taken_books.size(), 1);
        QCOMPARE(readers[0].taken_books[0], QString("B001"));
    }

    // 5) Удаление связи читатель–книга
    void testRemoveLinkBook() {
        ReaderModel model;
        Reader r{"R001", "Иванов", "Иван", "Иванович",
                 Sex::Male, QDate::currentDate(), {"B001", "B002"}};
        model.AddReader(r);

        bool ok = model.RemoveLinkBook("R001", "B001");
        QVERIFY(ok);

        auto readers = model.GetReaders();
        QCOMPARE(readers[0].taken_books.size(), 1);
        QCOMPARE(readers[0].taken_books[0], QString("B002"));
    }

    // 6) Поиск читателя
    void testFindReader() {
        ReaderModel model;
        model.AddReader({"R001", "Иванов", "Иван", "Иванович", Sex::Male, QDate::currentDate() , {}});

        auto rOpt = model.FindReader("R001");
        QVERIFY(rOpt.has_value());
        QCOMPARE(rOpt->ID, QString("R001"));

        auto rOpt2 = model.FindReader("R999");
        QVERIFY(!rOpt2.has_value());
    }

    // 7) Генерация ID читателя
    void testGenerateReaderIdIsUnique() {
        QList<Reader> existing;
        existing.append({"R1234", "A", "B", "C", Sex::Male, QDate::currentDate() , {}});
        existing.append({"R5678", "D", "E", "F", Sex::Female, QDate::currentDate() , {}});

        QString id = ReaderModel::GenerateReaderID(existing);
        QVERIFY(id.startsWith("R"));

        bool collision = false;
        for (const auto &r : existing) {
            if (r.ID == id) {
                collision = true;
                break;
            }
        }
        QVERIFY(!collision);
    }
};

QTEST_APPLESS_MAIN(ReaderModelTest)
#include "test_readermodel.moc"




