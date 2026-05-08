#include "FlashcardsController.h"

#include "domain/CardDeck.h"
#include "domain/CsvLoader.h"
#include "render/FontProvider.h"
#include "render/PdfGenerator.h"

#include <QPdfDocument>

#include <utility>

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

void FlashcardsController::loadCsv(const QString& csvPath) {
    emit info(tr("Reading %1").arg(csvPath));

    auto load = CsvLoader::loadFile(csvPath);
    for (const QString& w : load.warnings) emit warning(w);
    if (!load.ok()) {
        emit error(load.error);
        return;
    }

    applyDeck(load.deck);
    if (hasDocument()) m_currentCsv = csvPath;
}

void FlashcardsController::applyDeck(CardDeck deck) {
    if (deck.isEmpty()) {
        emit error(tr("Deck is empty — nothing to render."));
        return;
    }

    PdfGenerator::Options opts;
    if (!m_fontFamily.isEmpty()) {
        opts.fontFamily = m_fontFamily;
    } else if (!m_fontWarned) {
        emit warning(FontProvider::installHint());
        m_fontWarned = true;
    }

    // QPdfDocument holds a non-owning pointer to the QIODevice; close before
    // the buffer is reset, then re-open after the new bytes are in place.
    m_document->close();
    m_pdfBuffer.close();
    m_pdfBytes.clear();
    m_currentCsv.clear();

    m_pdfBuffer.setBuffer(&m_pdfBytes);
    m_pdfBuffer.open(QIODevice::WriteOnly);
    const auto res = PdfGenerator::generate(deck, &m_pdfBuffer, opts);
    m_pdfBuffer.close();
    if (!res.ok) {
        emit error(res.error);
        return;
    }

    m_pdfBuffer.open(QIODevice::ReadOnly);
    m_document->load(&m_pdfBuffer);
    if (m_document->status() != QPdfDocument::Status::Ready) {
        emit error(tr("Failed to load generated PDF (error %1).")
                       .arg(int(m_document->error())));
        return;
    }

    m_deck = std::move(deck);

    const int real    = m_deck.size();
    const int per     = CardDeck::kCardsPerPage;
    const int padded  = ((real + per - 1) / per) * per;
    const int logical = padded / per;
    emit info(tr("Loaded %1 cards (padded to %2, %3 logical pages → %4 PDF pages).")
                  .arg(real).arg(padded).arg(logical).arg(logical * 2));
    emit documentLoaded();
}
