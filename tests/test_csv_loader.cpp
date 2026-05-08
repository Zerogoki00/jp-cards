#include "domain/CsvLoader.h"

#include <QtTest>

class TestCsvLoader : public QObject {
    Q_OBJECT
private slots:
    void simpleTwoColumns();
    void bomStripped();
    void quotedCommaInside();
    void quotedNewlineInside();
    void escapedQuote();
    void emptyRowSkipped();
    void singleColumnRowWarns();
    void crlfLineEndings();
    void emptyFileError();
    void utf8Cjk();
};

void TestCsvLoader::simpleTwoColumns() {
    auto r = CsvLoader::loadBytes("front1,back1\nfront2,back2\n");
    QVERIFY(r.ok());
    QCOMPARE(r.deck.size(), 2);
    QCOMPARE(r.deck.cards()[0].front, QStringLiteral("front1"));
    QCOMPARE(r.deck.cards()[1].back,  QStringLiteral("back2"));
    QVERIFY(r.warnings.isEmpty());
}

void TestCsvLoader::bomStripped() {
    QByteArray data = QByteArray::fromHex("EFBBBF") + "a,b\n";
    auto r = CsvLoader::loadBytes(data);
    QVERIFY(r.ok());
    QCOMPARE(r.deck.size(), 1);
    QCOMPARE(r.deck.cards()[0].front, QStringLiteral("a"));
}

void TestCsvLoader::quotedCommaInside() {
    auto r = CsvLoader::loadBytes("\"a,b\",c\n");
    QVERIFY(r.ok());
    QCOMPARE(r.deck.cards()[0].front, QStringLiteral("a,b"));
    QCOMPARE(r.deck.cards()[0].back,  QStringLiteral("c"));
}

void TestCsvLoader::quotedNewlineInside() {
    auto r = CsvLoader::loadBytes("\"line1\nline2\",back\n");
    QVERIFY(r.ok());
    QCOMPARE(r.deck.size(), 1);
    QCOMPARE(r.deck.cards()[0].front, QStringLiteral("line1\nline2"));
}

void TestCsvLoader::escapedQuote() {
    auto r = CsvLoader::loadBytes("\"she said \"\"hi\"\"\",ok\n");
    QVERIFY(r.ok());
    QCOMPARE(r.deck.cards()[0].front, QStringLiteral("she said \"hi\""));
}

void TestCsvLoader::emptyRowSkipped() {
    auto r = CsvLoader::loadBytes("a,b\n\n\nc,d\n");
    QVERIFY(r.ok());
    QCOMPARE(r.deck.size(), 2);
}

void TestCsvLoader::singleColumnRowWarns() {
    auto r = CsvLoader::loadBytes("a,b\nlonelyfield\nc,d\n");
    QVERIFY(r.ok());
    QCOMPARE(r.deck.size(), 2);
    QCOMPARE(r.warnings.size(), 1);
    QVERIFY(r.warnings.first().contains(QStringLiteral("Row 2")));
}

void TestCsvLoader::crlfLineEndings() {
    auto r = CsvLoader::loadBytes("a,b\r\nc,d\r\n");
    QVERIFY(r.ok());
    QCOMPARE(r.deck.size(), 2);
    QCOMPARE(r.deck.cards()[1].front, QStringLiteral("c"));
}

void TestCsvLoader::emptyFileError() {
    auto r = CsvLoader::loadBytes("");
    QVERIFY(!r.ok());
    QVERIFY(!r.error.isEmpty());
}

void TestCsvLoader::utf8Cjk() {
    auto r = CsvLoader::loadBytes(QString::fromUtf8("宴会,банкет\n").toUtf8());
    QVERIFY(r.ok());
    QCOMPARE(r.deck.cards()[0].front, QString::fromUtf8("宴会"));
    QCOMPARE(r.deck.cards()[0].back,  QString::fromUtf8("банкет"));
}

QTEST_GUILESS_MAIN(TestCsvLoader)
#include "test_csv_loader.moc"
