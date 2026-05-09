#pragma once

#include <QString>

class FontProvider {
public:
    // Resolves a font family suitable for rendering Japanese (kanji + kana)
    // and the surrounding Cyrillic / Latin text in the cells.
    //
    // The font must be the bundled Qt resource embedded at build time.
    //
    // Returns an empty string if nothing usable is found; callers may then
    // surface installHint() to the user.
    static QString resolveFamily();

    // Human-readable hint for missing-font situations.
    static QString installHint();
};
