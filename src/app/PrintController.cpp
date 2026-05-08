#include "PrintController.h"

#include <QAbstractPrintDialog>
#include <QImage>
#include <QPageRanges>
#include <QPainter>
#include <QPdfDocument>
#include <QPointF>
#include <QPrintDialog>
#include <QPrinter>

namespace {

QPageRanges rangesForMode(int totalPages, PrintController::Mode mode) {
    QPageRanges r;
    if (mode == PrintController::Mode::All) return r;
    for (int p = 1; p <= totalPages; ++p) {
        const bool odd = (p % 2 == 1);
        if (mode == PrintController::Mode::Odd  &&  odd) r.addRange(p, p);
        if (mode == PrintController::Mode::Even && !odd) r.addRange(p, p);
    }
    return r;
}

bool inRanges(const QPageRanges& r, int page) {
    for (const auto& range : r.toRangeList()) {
        if (page >= range.from && page <= range.to) return true;
    }
    return false;
}

} // namespace

PrintController::PrintController(QObject* parent) : QObject(parent) {}

bool PrintController::print(QPdfDocument* doc, Mode mode, QWidget* dialogParent) {
    if (!doc || doc->status() != QPdfDocument::Status::Ready) {
        emit error(tr("No document loaded."));
        return false;
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setDocName(QStringLiteral("Flashcards"));
    printer.setPageRanges(rangesForMode(doc->pageCount(), mode));

    QPrintDialog dialog(&printer, dialogParent);
    dialog.setWindowTitle(tr("Print"));
    // Pre-select the "Pages" radio so the pre-filled ranges are honoured
    // out of the box; if user picks "All", we still filter by mode below.
    if (mode != Mode::All) {
        dialog.setPrintRange(QAbstractPrintDialog::PageRange);
    }
    if (dialog.exec() != QDialog::Accepted) return false;

    if (!renderToPrinter(doc, &printer, mode)) return false;
    emit printFinished(mode);
    return true;
}

bool PrintController::printTo(QPdfDocument* doc, QPrinter* printer, Mode mode) {
    if (!doc || doc->status() != QPdfDocument::Status::Ready) {
        emit error(tr("No document loaded."));
        return false;
    }
    if (!printer) {
        emit error(tr("No printer."));
        return false;
    }
    // The renderToPrinter() filter intersects mode-derived ranges with
    // whatever pageRanges the caller has put on the printer, so this
    // entry point intentionally leaves printer->pageRanges() alone —
    // tests can pre-seed it to mimic dialog-driven user choices.
    if (!renderToPrinter(doc, printer, mode)) return false;
    emit printFinished(mode);
    return true;
}

bool PrintController::renderToPrinter(QPdfDocument* doc, QPrinter* printer, Mode mode) {
    // The mode is the source of truth for Odd/Even — the print dialog can
    // overwrite the printer's pageRanges (e.g. when the user keeps the
    // default "All pages" radio), so we re-derive the mode filter here and
    // intersect it with whatever range the user chose in the dialog.
    const QPageRanges modeRanges = rangesForMode(doc->pageCount(), mode);
    const QPageRanges userRanges = printer->pageRanges();
    const bool filterByMode = !modeRanges.isEmpty();
    const bool filterByUser = !userRanges.isEmpty();

    QPainter painter;
    if (!painter.begin(printer)) {
        emit error(tr("Cannot start printing — failed to acquire printer device."));
        return false;
    }

    const QSize raster = printer->pageLayout()
                             .fullRectPixels(printer->resolution())
                             .size();
    const int total = doc->pageCount();
    int printed = 0;
    for (int i = 0; i < total; ++i) {
        const int pageNum = i + 1;
        if (filterByMode && !inRanges(modeRanges, pageNum)) continue;
        if (filterByUser && !inRanges(userRanges, pageNum)) continue;

        if (printed > 0) printer->newPage();
        ++printed;

        const QImage img = doc->render(i, raster);
        if (img.isNull()) {
            emit error(tr("Failed to render page %1.").arg(pageNum));
            painter.end();
            return false;
        }
        painter.drawImage(QPointF(0, 0), img);
    }
    painter.end();

    if (printed == 0) {
        emit error(tr("No pages selected to print."));
        return false;
    }
    emit info(tr("Sent %1 page(s) to %2.").arg(printed)
              .arg(printer->printerName().isEmpty() ? tr("printer")
                                                    : printer->printerName()));
    return true;
}
