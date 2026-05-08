#include "CsvLoader.h"

#include <QFile>
#include <QString>
#include <QStringDecoder>

namespace {

// RFC 4180 streaming parser. State machine: handles quoted fields, embedded
// quotes (""), embedded newlines, and both LF / CRLF line endings.
QVector<QStringList> parseRows(const QString& text) {
    QVector<QStringList> rows;
    QStringList row;
    QString field;

    enum class State { FieldStart, Unquoted, Quoted, QuotedAfterQuote };
    State state = State::FieldStart;

    auto endField = [&]() {
        row.append(field);
        field.clear();
    };
    auto endRow = [&]() {
        endField();
        rows.append(row);
        row.clear();
    };

    const int n = text.size();
    for (int i = 0; i < n; ++i) {
        const QChar c = text[i];
        switch (state) {
        case State::FieldStart:
            if (c == QLatin1Char('"')) {
                state = State::Quoted;
            } else if (c == QLatin1Char(',')) {
                endField();
            } else if (c == QLatin1Char('\n')) {
                endRow();
                state = State::FieldStart;
            } else if (c == QLatin1Char('\r')) {
                if (i + 1 < n && text[i + 1] == QLatin1Char('\n')) ++i;
                endRow();
                state = State::FieldStart;
            } else {
                field.append(c);
                state = State::Unquoted;
            }
            break;
        case State::Unquoted:
            if (c == QLatin1Char(',')) {
                endField();
                state = State::FieldStart;
            } else if (c == QLatin1Char('\n')) {
                endRow();
                state = State::FieldStart;
            } else if (c == QLatin1Char('\r')) {
                if (i + 1 < n && text[i + 1] == QLatin1Char('\n')) ++i;
                endRow();
                state = State::FieldStart;
            } else {
                field.append(c);
            }
            break;
        case State::Quoted:
            if (c == QLatin1Char('"')) {
                state = State::QuotedAfterQuote;
            } else {
                field.append(c);
            }
            break;
        case State::QuotedAfterQuote:
            if (c == QLatin1Char('"')) {
                field.append(QLatin1Char('"'));
                state = State::Quoted;
            } else if (c == QLatin1Char(',')) {
                endField();
                state = State::FieldStart;
            } else if (c == QLatin1Char('\n')) {
                endRow();
                state = State::FieldStart;
            } else if (c == QLatin1Char('\r')) {
                if (i + 1 < n && text[i + 1] == QLatin1Char('\n')) ++i;
                endRow();
                state = State::FieldStart;
            } else {
                // Stray content after a closing quote — treat as part of the field.
                field.append(c);
                state = State::Unquoted;
            }
            break;
        }
    }

    // Flush trailing field/row if the file does not end with a newline.
    if (state != State::FieldStart || !field.isEmpty() || !row.isEmpty()) {
        endRow();
    }

    return rows;
}

} // namespace

CsvLoader::Result CsvLoader::loadFile(const QString& path) {
    Result r;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        r.error = QStringLiteral("Cannot open file: %1").arg(path);
        return r;
    }
    return loadBytes(f.readAll());
}

CsvLoader::Result CsvLoader::loadBytes(const QByteArray& data) {
    Result r;

    // Decode as UTF-8; QStringDecoder strips BOM automatically.
    QStringDecoder dec(QStringDecoder::Utf8, QStringDecoder::Flag::ConvertInvalidToNull);
    QString text = dec.decode(data);

    auto rows = parseRows(text);

    int rowNum = 0;
    for (const auto& row : rows) {
        ++rowNum;
        // Skip empty rows (no fields, or single empty field — common from trailing newline).
        if (row.isEmpty() || (row.size() == 1 && row.first().isEmpty())) {
            continue;
        }
        if (row.size() < 3) {
            r.warnings.append(QStringLiteral("Row %1 skipped: needs 3 columns, got %2.")
                                  .arg(rowNum).arg(row.size()));
            continue;
        }
        Card card{ row[0], row[1], row[2] };
        if (card.isEmpty()) {
            r.warnings.append(QStringLiteral("Row %1 skipped: word and translation are empty.")
                                  .arg(rowNum));
            continue;
        }
        r.deck.append(card);
    }

    if (r.deck.isEmpty()) {
        r.error = QStringLiteral("No cards found in CSV.");
    }
    return r;
}
