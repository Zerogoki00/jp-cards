#include "FlashcardsController.h"

#include "domain/CardDeck.h"
#include "domain/CsvLoader.h"
#include "render/FontProvider.h"
#include "render/PdfGenerator.h"

#include <QDir>
#include <QPdfDocument>
#include <QStandardPaths>

FlashcardsController::FlashcardsController(QObject* parent)
    : QObject(parent),
      m_document(new QPdfDocument(this))
{
    m_fontFamily = FontProvider::resolveFamily();
}

FlashcardsController::~FlashcardsController() {
    if (m_document) m_document->close();
}

bool FlashcardsController::hasDocument() const {
    return m_document && m_document->status() == QPdfDocument::Status::Ready;
}

QString FlashcardsController::cachePdfPath() const {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/preview.pdf");
}

void FlashcardsController::loadCsv(const QString& csvPath) {
    emit info(tr("Reading %1").arg(csvPath));

    auto load = CsvLoader::loadFile(csvPath);
    for (const QString& w : load.warnings) emit warning(w);
    if (!load.ok()) {
        emit error(load.error);
        return;
    }

    PdfGenerator::Options opts;
    if (!m_fontFamily.isEmpty()) {
        opts.fontFamily = m_fontFamily;
    } else if (!m_fontWarned) {
        emit warning(FontProvider::installHint());
        m_fontWarned = true;
    }

    const QString outPath = cachePdfPath();
    // QPdfDocument keeps the file open on Windows; close before overwriting.
    m_document->close();

    auto res = PdfGenerator::generate(load.deck, outPath, opts);
    if (!res.ok) {
        emit error(res.error);
        return;
    }

    const auto status = m_document->load(outPath);
    if (status != QPdfDocument::Error::None) {
        emit error(tr("Failed to load generated PDF (status %1).").arg(int(status)));
        return;
    }

    m_currentCsv = csvPath;
    m_currentPdf = outPath;

    const int real    = load.deck.size();
    const int per     = CardDeck::kCardsPerPage;
    const int padded  = ((real + per - 1) / per) * per;
    const int logical = padded / per;
    emit info(tr("Loaded %1 cards (padded to %2, %3 logical pages → %4 PDF pages).")
                  .arg(real).arg(padded).arg(logical).arg(logical * 2));
    emit documentLoaded();
}
