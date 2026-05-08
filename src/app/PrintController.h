#pragma once

#include <QObject>
#include <QString>

class QPdfDocument;
class QPrinter;
class QWidget;

// Drives the printing flow: pre-fills page selection per mode, surfaces
// the system QPrintDialog, then rasterises the loaded QPdfDocument page
// by page onto the printer.
class PrintController : public QObject {
    Q_OBJECT
public:
    enum class Mode { All, Odd, Even };
    Q_ENUM(Mode)

    explicit PrintController(QObject* parent = nullptr);

    // Shows the system print dialog with mode-specific page ranges
    // pre-populated. Returns true if the user accepted the dialog and
    // printing began (no guarantee about completion at the printer).
    bool print(QPdfDocument* doc, Mode mode, QWidget* dialogParent);

    // Headless variant used by tests (no dialog). Caller sets up the
    // QPrinter (e.g. PdfFormat for tests) and the page ranges from `mode`
    // are applied automatically.
    bool printTo(QPdfDocument* doc, QPrinter* printer, Mode mode);

signals:
    void info (const QString& message);
    void error(const QString& message);
    void printFinished(Mode mode);

private:
    bool renderToPrinter(QPdfDocument* doc, QPrinter* printer, Mode mode);
};
