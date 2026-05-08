#include "domain/CsvLoader.h"

#include <QtTest>

class TestCsvLoader : public QObject {
    Q_OBJECT
private slots:
    void simpleThreeColumns();
    void emptyFuriganaAllowed();
    void bomStripped();
    void quotedCommaInside();
    void quotedNewlineInside();
    void escapedQuote();
    void emptyRowSkipped();
    void shortRowWarns();
    void crlfLineEndings();
    void emptyFileError();
    void utf8Cjk();
};

void TestCsvLoader::simpleThreeColumns() {
    auto r = CsvLoader::loadBytes("word1,furi1,trans1\nword2,furi2,trans2\n");
    QVERIFY(r.ok());
    QCOMPARE(r.deck.size(), 2);
    QCOMPARE(r.deck.cards()[0].word,        QStringLiteral("word1"));
    QCOMPARE(r.deck.cards()[0].furigana,    QStringLiteral("furi1"));
    QCOMPARE(r.deck.cards()[0].translation, QStringLiteral("trans1"));
    QCOMPARE(r.deck.cards()[1].translation, QStringLiteral("trans2"));
    QVERIFY(r.warnings.isEmpty());
}

void TestCsvLoader::emptyFuriganaAllowed() {
    auto r = CsvLoader::loadBytes("あふれる,,Перелив\n");
    QVERIFY(r.ok());
    QCOMPARE(r.deck.size(), 1);
    QCOMPARE(r.deck.cards()[0].furigana, QString());
    QCOMPARE(r.deck.cards()[0].word,
             QString::fromUtf8("あふれる"));
    QCOMPARE(r.deck.cards()[0].translation,
             QString::fromUtf8("Перелив"));
    QVERIFY(r.warnings.isEmpty());
}

void TestCsvLoader::bomStripped() {
    QByteArray data = QByteArray::fromHex("EFBBBF") + "a,b,c\n";
    auto r = CsvLoader::loadBytes(data);
    QVERIFY(r.ok());
    QCOMPARE(r.deck.size(), 1);
    QCOMPARE(r.deck.cards()[0].word, QStringLiteral("a"));
}

void TestCsvLoader::quotedCommaInside() {
    auto r = CsvLoader::loadBytes("word,furi,\"trans, with comma\"\n");
    QVERIFY(r.ok());
    QCOMPARE(r.deck.cards()[0].translation,
             QStringLiteral("trans, with comma"));
}

void TestCsvLoader::quotedNewlineInside() {
    auto r = CsvLoader::loadBytes("word,furi,\"line1\nline2\"\n");
    QVERIFY(r.ok());
    QCOMPARE(r.deck.size(), 1);
    QCOMPARE(r.deck.cards()[0].translation, QStringLiteral("line1\nline2"));
}

void TestCsvLoader::escapedQuote() {
    auto r = CsvLoader::loadBytes("word,furi,\"she said \"\"hi\"\"\"\n");
    QVERIFY(r.ok());
    QCOMPARE(r.deck.cards()[0].translation, QStringLiteral("she said \"hi\""));
}

void TestCsvLoader::emptyRowSkipped() {
    auto r = CsvLoader::loadBytes("a,b,c\n\n\nd,e,f\n");
    QVERIFY(r.ok());
    QCOMPARE(r.deck.size(), 2);
}

void TestCsvLoader::shortRowWarns() {
    auto r = CsvLoader::loadBytes("a,b,c\nlonelyfield\nd,e,f\n");
    QVERIFY(r.ok());
    QCOMPARE(r.deck.size(), 2);
    QCOMPARE(r.warnings.size(), 1);
    QVERIFY(r.warnings.first().contains(QStringLiteral("Row 2")));
}

void TestCsvLoader::crlfLineEndings() {
    auto r = CsvLoader::loadBytes("a,b,c\r\nd,e,f\r\n");
    QVERIFY(r.ok());
    QCOMPARE(r.deck.size(), 2);
    QCOMPARE(r.deck.cards()[1].word, QStringLiteral("d"));
}

void TestCsvLoader::emptyFileError() {
    auto r = CsvLoader::loadBytes("");
    QVERIFY(!r.ok());
    QVERIFY(!r.error.isEmpty());
}

void TestCsvLoader::utf8Cjk() {
    auto r = CsvLoader::loadBytes(
        QString::fromUtf8("宴会を開く,えんかいをひらく,банкет\n").toUtf8());
    QVERIFY(r.ok());
    QCOMPARE(r.deck.cards()[0].word,        QString::fromUtf8("宴会を開く"));
    QCOMPARE(r.deck.cards()[0].furigana,    QString::fromUtf8("えんかいをひらく"));
    QCOMPARE(r.deck.cards()[0].translation, QString::fromUtf8("банкет"));
}

QTEST_GUILESS_MAIN(TestCsvLoader)
#include "test_csv_loader.moc"
