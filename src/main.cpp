#include "ui/MainWindow.h"
#include "domain/CsvLoader.h"
#include "render/FontProvider.h"
#include "render/PdfGenerator.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>

namespace {

int runCli(const QStringList& positional) {
    QTextStream out(stdout);
    QTextStream err(stderr);

    if (positional.size() < 1) {
        err << "Usage: jp-cards --generate <input.csv> [output.pdf]\n";
        return 2;
    }
    const QString csvPath = positional.at(0);
    const QString pdfPath = positional.size() >= 2
        ? positional.at(1)
        : QFileInfo(csvPath).absoluteDir()
              .filePath(QFileInfo(csvPath).completeBaseName() + QStringLiteral(".pdf"));

    auto load = CsvLoader::loadFile(csvPath);
    for (const QString& w : load.warnings) err << "warning: " << w << "\n";
    if (!load.ok()) {
        err << "error: " << load.error << "\n";
        return 1;
    }

    PdfGenerator::Options opts;
    const QString family = FontProvider::resolveFamily();
    if (family.isEmpty()) {
        err << "warning: " << FontProvider::installHint() << "\n";
    } else {
        opts.fontFamily = family;
    }

    const auto res = PdfGenerator::generate(load.deck, pdfPath, opts);
    if (!res.ok) {
        err << "error: " << res.error << "\n";
        return 1;
    }
    const int real = load.deck.size();
    const int per  = CardDeck::kCardsPerPage;
    const int padded = ((real + per - 1) / per) * per;
    const int logical = padded / per;
    out << "Generated " << pdfPath
        << " (real cards: " << real
        << ", padded to: " << padded
        << ", logical pages: " << logical
        << ", physical pages: " << logical * 2 << ")\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("jp-cards");
    QCoreApplication::setApplicationName("jp-cards");
    QCoreApplication::setApplicationVersion("0.1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Double-sided flashcard generator");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption generateOpt(QStringList{"g", "generate"},
        "Headless mode: read CSV, write PDF, exit. "
        "Positional args: <input.csv> [output.pdf].");
    parser.addOption(generateOpt);
    parser.addPositionalArgument("paths", "CSV input and optional PDF output", "[paths...]");

    parser.process(app);

    if (parser.isSet(generateOpt)) {
        return runCli(parser.positionalArguments());
    }

    MainWindow window;
    window.show();
    return app.exec();
}
