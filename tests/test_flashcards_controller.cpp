#include "app/FlashcardsController.h"

#include <QPdfDocument>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QtTest>

class TestFlashcardsController : public QObject {
    Q_OBJECT
private slots:
    void loadValidCsv();
    void loadMissingCsv();
};

void TestFlashcardsController::loadValidCsv() {
    QTemporaryFile csv("XXXXXX.csv");
    QVERIFY(csv.open());
    csv.write(QString::fromUtf8("宴会,банкет\n飲み会,попойка\n").toUtf8());
    csv.flush();
    const QString path = csv.fileName();
    csv.close();

    FlashcardsController c;
    QSignalSpy loadedSpy(&c, &FlashcardsController::documentLoaded);
    QSignalSpy errorSpy (&c, &FlashcardsController::error);

    c.loadCsv(path);

    QCOMPARE(errorSpy.count(), 0);
    QCOMPARE(loadedSpy.count(), 1);
    QVERIFY(c.hasDocument());
    QCOMPARE(c.currentCsvPath(), path);
    QVERIFY(!c.pdfData().isEmpty());
    QVERIFY(c.pdfData().startsWith("%PDF-"));
    // 2 real cards → padded to 18 → 1 logical → 2 PDF pages.
    QCOMPARE(c.document()->pageCount(), 2);
}

void TestFlashcardsController::loadMissingCsv() {
    FlashcardsController c;
    QSignalSpy errorSpy(&c, &FlashcardsController::error);
    c.loadCsv(QStringLiteral("/nonexistent/path/to/x.csv"));
    QCOMPARE(errorSpy.count(), 1);
    QVERIFY(!c.hasDocument());
}

QTEST_MAIN(TestFlashcardsController)
#include "test_flashcards_controller.moc"
