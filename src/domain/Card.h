#pragma once

#include <QString>

struct Card {
    QString front;
    QString back;

    bool isEmpty() const { return front.isEmpty() && back.isEmpty(); }
};
