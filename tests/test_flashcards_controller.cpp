#include "app/FlashcardsController.h"

#include <QPdfDocument>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QtTest>

class TestFlashcardsController : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void loadValidCsv();
    void loadMissingCsv();
};

void TestFlashcardsController::initTestCase() {
    // Make sure CacheLocation lives in a sandbox so we don't pollute the
    // user's real cache when tests run on a developer machine.
    static QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName("flashcards-test");
    QCoreApplication::setApplicationName("flashcards-test");
}

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
    QVERIFY(!c.currentPdfPath().isEmpty());
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
