#pragma once

#include "domain/CardDeck.h"
#include "render/PdfGenerator.h"

#include <QBuffer>
#include <QByteArray>
#include <QObject>
#include <QString>

class QPdfDocument;

// Orchestrates the CSV → CardDeck → PDF → QPdfDocument flow.
// Owns the QPdfDocument used by the preview view; the rendered PDF is
// kept in memory and reused by Save.
class FlashcardsController : public QObject {
    Q_OBJECT
public:
    explicit FlashcardsController(QObject* parent = nullptr);
    ~FlashcardsController() override;

    QPdfDocument* document() const { return m_document; }

    // Empty until a CSV / deck has been successfully loaded.
    QString            currentCsvPath() const { return m_currentCsv; }
    const QByteArray&  pdfData()        const { return m_pdfBytes; }
    const CardDeck&    deck()           const { return m_deck; }
    double             frontFontSizePt() const { return m_frontFontSizePt; }
    bool               hasDocument()    const;

public slots:
    // Parse CSV into a deck, then forward to applyDeck().
    void loadCsv(const QString& csvPath);
    // Take a deck directly (e.g. from the Card Editor), regenerate the PDF,
    // swap it into the owned QPdfDocument. Clears `currentCsvPath()` because
    // the deck no longer corresponds to the on-disk CSV.
    void applyDeck(CardDeck deck);
    void setFrontFontSizePt(double pointSize);

signals:
    void documentLoaded();
    void info   (const QString& message);
    void warning(const QString& message);
    void error  (const QString& message);

private:
    PdfGenerator::Options makePdfOptions() const;
    bool renderDeck(CardDeck deck, bool preserveCurrentCsv);

    QPdfDocument* m_document = nullptr;
    QString       m_currentCsv;
    CardDeck      m_deck;       // unpadded; the renderer pads its own copy
    QByteArray    m_pdfBytes;   // backing storage for m_pdfBuffer
    QBuffer       m_pdfBuffer;  // QPdfDocument keeps a non-owning pointer to this
    QString       m_fontFamily;
    double        m_frontFontSizePt = 14.0;
    bool          m_fontWarned = false;
};
