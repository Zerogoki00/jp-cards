#include "PdfGenerator.h"

#include <QAbstractTextDocumentLayout>
#include <QFont>
#include <QMarginsF>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QPen>
#include <QRectF>
#include <QTextDocument>
#include <QTextOption>

#include <algorithm>

namespace {

constexpr int kCols = 3;
constexpr int kRows = 6;
constexpr double kFurigToMain = 9.0 / 14.0; // preserves the legacy ratio
constexpr double kMinFurigPt  = 7.0;

// Builds the CSS rules applied to every cell document.
QString cellStyleSheet(double mainPt, double furiPt, double lineSpacing) {
    return QStringLiteral(
        "body { text-align: center; font-size: %1pt; line-height: %2; color: #111; }"
        ".furigana { font-size: %3pt; color: #666; }"
    ).arg(mainPt, 0, 'f', 2)
     .arg(lineSpacing, 0, 'f', 2)
     .arg(furiPt, 0, 'f', 2);
}

// Configures `doc` against this paint device so geometry queries return
// values in device units (dots), not screen pixels.
void configureDocument(QTextDocument& doc, QPaintDevice* device,
                       const QString& family, double mainPt, double furiPt,
                       double lineSpacing, double maxWidthDots,
                       const QString& html)
{
    doc.documentLayout()->setPaintDevice(device);

    QFont f;
    if (!family.isEmpty()) f.setFamily(family);
    f.setPointSizeF(mainPt);
    doc.setDefaultFont(f);

    // body text-align in CSS doesn't propagate reliably; pin alignment per-paragraph.
    QTextOption opt = doc.defaultTextOption();
    opt.setAlignment(Qt::AlignHCenter);
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    doc.setDefaultTextOption(opt);

    doc.setDefaultStyleSheet(cellStyleSheet(mainPt, furiPt, lineSpacing));
    doc.setTextWidth(maxWidthDots);
    doc.setHtml(html);
}

void renderOneCell(QPainter& painter, QPaintDevice* device,
                   const QString& html, const QRectF& cellRect,
                   const PdfGenerator::Options& opts)
{
    if (opts.drawCellBorder) {
        QPen pen(QColor(170, 170, 170));
        pen.setWidthF(0.4 * device->logicalDpiX() / 72.0); // 0.4pt
        QVector<qreal> dashes{3.0, 3.0};
        pen.setDashPattern(dashes);
        painter.save();
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(cellRect);
        painter.restore();
    }

    if (html.trimmed().isEmpty()) return;

    const double mmToDots = device->logicalDpiX() / 25.4;
    const double pad      = opts.cellPaddingMm * mmToDots;
    const QRectF inner    = cellRect.adjusted(pad, pad, -pad, -pad);

    // Auto-fit: shrink main size 1pt at a time until the rendered block fits.
    QTextDocument doc;
    double chosenMain = opts.mainFontSizeMinPt;
    for (double sz = opts.mainFontSizeMaxPt; sz >= opts.mainFontSizeMinPt; sz -= 1.0) {
        const double furi = std::max(kMinFurigPt, sz * kFurigToMain);
        configureDocument(doc, device, opts.fontFamily, sz, furi, opts.lineSpacing,
                          inner.width(), html);
        if (doc.size().height() <= inner.height()) {
            chosenMain = sz;
            break;
        }
    }
    // Final layout at chosenMain (re-runs at min if nothing fit; overflow is clipped).
    {
        const double furi = std::max(kMinFurigPt, chosenMain * kFurigToMain);
        configureDocument(doc, device, opts.fontFamily, chosenMain, furi,
                          opts.lineSpacing, inner.width(), html);
    }

    const double docHeight = doc.size().height();
    const double yOffset = std::max(0.0, (inner.height() - docHeight) / 2.0);

    painter.save();
    painter.setClipRect(inner);
    painter.translate(inner.left(), inner.top() + yOffset);
    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.clip = QRectF(0, 0, inner.width(), inner.height() - yOffset);
    doc.documentLayout()->draw(&painter, ctx);
    painter.restore();
}

void renderPage(QPainter& painter, QPaintDevice* device,
                const QVector<Card>& cards, int pageStartIdx,
                bool isBack, const PdfGenerator::Options& opts)
{
    const double mmToDots = device->logicalDpiX() / 25.4;
    const double pageW    = device->width();
    const double pageH    = device->height();
    const double margin   = opts.pageMarginMm * mmToDots;
    const double cellW    = (pageW - 2 * margin) / kCols;
    const double cellH    = (pageH - 2 * margin) / kRows;

    for (int row = 0; row < kRows; ++row) {
        for (int col = 0; col < kCols; ++col) {
            const int sourceCol = isBack ? (kCols - 1 - col) : col;
            const int idx = pageStartIdx + row * kCols + sourceCol;
            const Card& card = cards[idx];
            const QString html = isBack ? card.back : card.front;

            const QRectF cellRect(
                margin + col * cellW,
                margin + row * cellH,
                cellW, cellH);
            renderOneCell(painter, device, html, cellRect, opts);
        }
    }
}

} // namespace

PdfGenerator::Result PdfGenerator::generate(const CardDeck& deckIn,
                                            const QString& outPath,
                                            const Options& opts)
{
    Result r;
    CardDeck deck = deckIn;
    if (deck.isEmpty()) { r.error = QStringLiteral("Empty deck."); return r; }
    if (deck.size() % CardDeck::kCardsPerPage != 0) deck.pad();

    QPdfWriter writer(outPath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(0, 0, 0, 0));
    writer.setResolution(opts.resolutionDpi);
    writer.setTitle(QStringLiteral("Flashcards"));

    QPainter painter;
    if (!painter.begin(&writer)) {
        r.error = QStringLiteral("QPainter::begin(QPdfWriter) failed for %1").arg(outPath);
        return r;
    }

    const auto& cards = deck.cards();
    const int totalLogicalPages = deck.pageCount();
    for (int p = 0; p < totalLogicalPages; ++p) {
        const int start = p * CardDeck::kCardsPerPage;
        renderPage(painter, &writer, cards, start, /*isBack=*/false, opts);
        writer.newPage();
        renderPage(painter, &writer, cards, start, /*isBack=*/true,  opts);
        if (p + 1 < totalLogicalPages) writer.newPage();
    }

    painter.end();
    r.ok = true;
    return r;
}
