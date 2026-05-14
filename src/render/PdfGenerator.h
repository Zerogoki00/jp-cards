#pragma once

#include "domain/CardDeck.h"

#include <QString>

class QIODevice;

class PdfGenerator {
public:
    struct Options {
        // Empty => Qt picks its default sans-serif (no Japanese coverage).
        // Real callers resolve this via FontProvider::resolveFamily().
        QString fontFamily;

        // Geometry (millimetres) and font sizes (PDF points).
        double pageMarginMm      = 10.0;
        double cellPaddingMm     = 3.0;
        double mainFontSizeMaxPt = 14.0;
        double mainFontSizeMinPt = 8.0;
        // Preferred maximum for front-side text. <= 0 uses mainFontSizeMaxPt.
        double frontFontSizePt   = 0.0;
        double lineSpacing       = 1.3;
        bool   drawCellBorder    = true;

        // Output resolution. 1200 dpi is Qt's default for QPdfWriter and
        // gives plenty of precision for vector text.
        int    resolutionDpi     = 1200;
    };

    struct Result {
        bool ok = false;
        QString error;
    };

    static Result generate(const CardDeck& deck,
                           QIODevice* device,
                           const Options& opts);
    static Result generate(const CardDeck& deck,
                           const QString& outPath,
                           const Options& opts);
    static Result generate(const CardDeck& deck, const QString& outPath) {
        return generate(deck, outPath, Options{});
    }
};
