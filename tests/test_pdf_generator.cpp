#include "domain/CardDeck.h"
#include "render/FontProvider.h"
#include "render/PdfGenerator.h"

#include <QPdfDocument>
#include <QTemporaryFile>
#include <QtTest>

class TestPdfGenerator : public QObject {
    Q_OBJECT
private slots:
    void rejectsEmptyDeck();
    void smokeProducesParsablePdf();
    void pageCountMatchesPaddedDeck();
};

void TestPdfGenerator::rejectsEmptyDeck() {
    CardDeck empty;
    QTemporaryFile out("XXXXXX.pdf");
    QVERIFY(out.open());
    auto r = PdfGenerator::generate(empty, out.fileName());
    QVERIFY(!r.ok);
    QVERIFY(!r.error.isEmpty());
}

void TestPdfGenerator::smokeProducesParsablePdf() {
    CardDeck d;
    d.append(Card{QStringLiteral("hello"), QStringLiteral("world")});

    QTemporaryFile out("XXXXXX.pdf");
    QVERIFY(out.open());
    const QString path = out.fileName();
    out.close();

    PdfGenerator::Options opts;
    opts.fontFamily = FontProvider::resolveFamily();
    auto r = PdfGenerator::generate(d, path, opts);
    QVERIFY2(r.ok, qPrintable(r.error));

    QFileInfo fi(path);
    QVERIFY(fi.exists());
    QVERIFY(fi.size() > 0);

    QPdfDocument pdf;
    QCOMPARE(pdf.load(path), QPdfDocument::Error::None);
    // 1 real card → padded to 18 → 1 logical page → 2 physical.
    QCOMPARE(pdf.pageCount(), 2);
}

void TestPdfGenerator::pageCountMatchesPaddedDeck() {
    CardDeck d;
    for (int i = 0; i < 19; ++i) {
        d.append(Card{QStringLiteral("a"), QStringLiteral("b")});
    }
    QTemporaryFile out("XXXXXX.pdf");
    QVERIFY(out.open());
    const QString path = out.fileName();
    out.close();

    PdfGenerator::Options opts;
    opts.fontFamily = FontProvider::resolveFamily();
    auto r = PdfGenerator::generate(d, path, opts);
    QVERIFY2(r.ok, qPrintable(r.error));

    QPdfDocument pdf;
    QCOMPARE(pdf.load(path), QPdfDocument::Error::None);
    // 19 → padded to 36 → 2 logical pages → 4 physical.
    QCOMPARE(pdf.pageCount(), 4);
}

QTEST_MAIN(TestPdfGenerator)
#include "test_pdf_generator.moc"
