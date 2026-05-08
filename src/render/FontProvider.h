#pragma once

#include <QString>

class FontProvider {
public:
    // Resolves a font family suitable for rendering Japanese (kanji + kana)
    // and the surrounding Cyrillic / Latin text in the cells.
    //
    // Resolution order:
    //   1. $FLASHCARDS_FONT_PATH if it points to an existing TTF/OTF file —
    //      registered in QFontDatabase via addApplicationFont, the resulting
    //      family is returned.
    //   2. Bundled Qt resource :/fonts/NotoSansCJK-Regular.ttf, if present.
    //   3. First family from a curated list of system CJK fonts that exists
    //      in QFontDatabase::families().
    //
    // Returns an empty string if nothing usable is found; callers may then
    // surface installHint() to the user.
    static QString resolveFamily();

    // Human-readable hint for missing-font situations.
    static QString installHint();
};
