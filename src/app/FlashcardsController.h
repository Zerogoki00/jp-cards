#pragma once

#include <QObject>
#include <QString>

class QPdfDocument;

// Orchestrates the CSV → CardDeck → PDF → QPdfDocument flow.
// Owns the QPdfDocument used by the preview view; the rendered PDF is
// kept on disk in the application cache directory and reused by Save.
class FlashcardsController : public QObject {
    Q_OBJECT
public:
    explicit FlashcardsController(QObject* parent = nullptr);
    ~FlashcardsController() override;

    QPdfDocument* document() const { return m_document; }

    // Empty until a CSV has been successfully loaded.
    QString currentCsvPath() const { return m_currentCsv; }
    QString currentPdfPath() const { return m_currentPdf; }
    bool    hasDocument()    const;

public slots:
    // Loads the given CSV, regenerates the preview PDF, swaps it into the
    // owned QPdfDocument. Emits info/warning/error along the way.
    void loadCsv(const QString& csvPath);

signals:
    void documentLoaded();
    void info   (const QString& message);
    void warning(const QString& message);
    void error  (const QString& message);

private:
    QString cachePdfPath() const;

    QPdfDocument* m_document = nullptr;
    QString       m_currentCsv;
    QString       m_currentPdf;
    QString       m_fontFamily;
    bool          m_fontWarned = false;
};
