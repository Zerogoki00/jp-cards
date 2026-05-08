#pragma once

#include "CardDeck.h"

#include <QString>
#include <QStringList>

class CsvLoader {
public:
    struct Result {
        CardDeck deck;
        QStringList warnings;
        QString error;
        bool ok() const { return error.isEmpty(); }
    };

    // Loads a CSV file from disk. Expected format: 3 columns
    // (word, furigana, translation). Furigana column may be empty.
    // Encoding: UTF-8, with or without BOM.
    static Result loadFile(const QString& path);

    // Same parser, but takes raw bytes. Used by tests and for future paste-from-clipboard.
    static Result loadBytes(const QByteArray& data);
};
