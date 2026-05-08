#pragma once

#include <QString>

struct Card {
    QString word;
    QString furigana;     // optional reading; may be empty
    QString translation;

    bool isEmpty() const {
        return word.isEmpty() && translation.isEmpty();
    }
};
