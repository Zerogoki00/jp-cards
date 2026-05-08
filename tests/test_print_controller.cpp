#include "app/FlashcardsController.h"
#include "app/PrintController.h"

#include <QPdfDocument>
#include <QPrinter>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QtTest>

class TestPrintController : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void printsAllPagesToPdf();
    void printsOnlyOddPages();
    void printsOnlyEvenPages();
    void emitsFinishedSignal();
    void modeWinsOverEmptyUserRanges();
    void modeAndUserRangesIntersect();
};

namespace {

// Builds a 2-logical-page deck (4 PDF pages) via FlashcardsController.
// Returns it through `outController` (caller owns lifetime).
void buildFourPageDocument(FlashcardsController& c) {
    QTemporaryFile csv("XXXXXX.csv");
    QVERIFY(csv.open());
    // 19 cards → padded to 36 → 2 logical → 4 PDF pages.
    QByteArray data;
    for (int i = 0; i < 19; ++i) {
        data += QString::fromUtf8("front%1,back%1\n").arg(i).toUtf8();
    }
    csv.write(data);
    csv.flush();
    const QString path = csv.fileName();
    csv.close();

    QSignalSpy loaded(&c, &FlashcardsController::documentLoaded);
    c.loadCsv(path);
    QCOMPARE(loaded.count(), 1);
    QCOMPARE(c.document()->pageCount(), 4);
}

// Wires up a QPrinter that "prints" to a PDF file. Caller owns the
// QTemporaryFile so the file outlives the QPrinter.
QPrinter* makePdfPrinter(const QString& outPath) {
    auto* p = new QPrinter(QPrinter::HighResolution);
    p->setOutputFormat(QPrinter::PdfFormat);
    p->setOutputFileName(outPath);
    p->setResolution(150); // keep test fast — full HighRes raster is huge
    return p;
}

} // namespace

void TestPrintController::initTestCase() {
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName("flashcards-test");
    QCoreApplication::setApplicationName("flashcards-test");
}

void TestPrintController::printsAllPagesToPdf() {
    FlashcardsController c;
    buildFourPageDocument(c);

    QTemporaryFile out("XXXXXX.pdf");
    QVERIFY(out.open());
    const QString path = out.fileName();
    out.close();

    QScopedPointer<QPrinter> printer(makePdfPrinter(path));
    PrintController p;
    QVERIFY(p.printTo(c.document(), printer.data(), PrintController::Mode::All));
    printer.reset(); // flushes the PDF

    QPdfDocument verify;
    QCOMPARE(verify.load(path), QPdfDocument::Error::None);
    QCOMPARE(verify.pageCount(), 4);
}

void TestPrintController::printsOnlyOddPages() {
    FlashcardsController c;
    buildFourPageDocument(c);

    QTemporaryFile out("XXXXXX.pdf");
    QVERIFY(out.open());
    const QString path = out.fileName();
    out.close();

    QScopedPointer<QPrinter> printer(makePdfPrinter(path));
    PrintController p;
    QVERIFY(p.printTo(c.document(), printer.data(), PrintController::Mode::Odd));
    printer.reset();

    QPdfDocument verify;
    QCOMPARE(verify.load(path), QPdfDocument::Error::None);
    QCOMPARE(verify.pageCount(), 2); // pages 1 and 3
}

void TestPrintController::printsOnlyEvenPages() {
    FlashcardsController c;
    buildFourPageDocument(c);

    QTemporaryFile out("XXXXXX.pdf");
    QVERIFY(out.open());
    const QString path = out.fileName();
    out.close();

    QScopedPointer<QPrinter> printer(makePdfPrinter(path));
    PrintController p;
    QVERIFY(p.printTo(c.document(), printer.data(), PrintController::Mode::Even));
    printer.reset();

    QPdfDocument verify;
    QCOMPARE(verify.load(path), QPdfDocument::Error::None);
    QCOMPARE(verify.pageCount(), 2); // pages 2 and 4
}

void TestPrintController::emitsFinishedSignal() {
    FlashcardsController c;
    buildFourPageDocument(c);

    QTemporaryFile out("XXXXXX.pdf");
    QVERIFY(out.open());
    const QString path = out.fileName();
    out.close();

    QScopedPointer<QPrinter> printer(makePdfPrinter(path));
    PrintController p;
    QSignalSpy finished(&p, &PrintController::printFinished);
    QVERIFY(p.printTo(c.document(), printer.data(), PrintController::Mode::Odd));
    QCOMPARE(finished.count(), 1);
    QCOMPARE(finished.first().first().value<PrintController::Mode>(),
             PrintController::Mode::Odd);
}

// Regression: simulating the QPrintDialog "All pages" radio (empty ranges
// on QPrinter after dialog accept) used to print ALL pages for Odd/Even.
// The mode filter must still kick in.
void TestPrintController::modeWinsOverEmptyUserRanges() {
    FlashcardsController c;
    buildFourPageDocument(c);

    QTemporaryFile out("XXXXXX.pdf");
    QVERIFY(out.open());
    const QString path = out.fileName();
    out.close();

    QScopedPointer<QPrinter> printer(makePdfPrinter(path));
    // Mimic the dialog clearing the pre-fill: explicitly empty ranges.
    printer->setPageRanges(QPageRanges());

    PrintController p;
    QVERIFY(p.printTo(c.document(), printer.data(), PrintController::Mode::Odd));
    printer.reset();

    QPdfDocument verify;
    QCOMPARE(verify.load(path), QPdfDocument::Error::None);
    QCOMPARE(verify.pageCount(), 2); // pages 1, 3 — not all 4
}

void TestPrintController::modeAndUserRangesIntersect() {
    FlashcardsController c;
    buildFourPageDocument(c);

    QTemporaryFile out("XXXXXX.pdf");
    QVERIFY(out.open());
    const QString path = out.fileName();
    out.close();

    QScopedPointer<QPrinter> printer(makePdfPrinter(path));
    // User chose "Pages: 1-2" in the dialog; mode is Odd.
    // Intersection: only page 1.
    QPageRanges userRanges;
    userRanges.addRange(1, 2);
    printer->setPageRanges(userRanges);

    PrintController p;
    QVERIFY(p.printTo(c.document(), printer.data(), PrintController::Mode::Odd));
    printer.reset();

    QPdfDocument verify;
    QCOMPARE(verify.load(path), QPdfDocument::Error::None);
    QCOMPARE(verify.pageCount(), 1);
}

QTEST_MAIN(TestPrintController)
#include "test_print_controller.moc"
