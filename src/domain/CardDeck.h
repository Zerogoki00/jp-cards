#pragma once

#include "Card.h"

#include <QVector>

class CardDeck {
public:
    static constexpr int kCardsPerPage = 18; // 3 cols x 6 rows

    void append(const Card& card) { m_cards.append(card); }

    const QVector<Card>& cards() const { return m_cards; }
    int size() const { return m_cards.size(); }
    bool isEmpty() const { return m_cards.isEmpty(); }

    // Number of real (non-padding) cards before pad() was last called.
    int realCount() const { return m_realCount; }

    // Pads with empty cards so size() is a multiple of `multiple`.
    // Records realCount() as the size before padding.
    void pad(int multiple = kCardsPerPage);

    // Number of logical pages (each materialises into 2 physical pages).
    int pageCount() const { return m_cards.size() / kCardsPerPage; }

private:
    QVector<Card> m_cards;
    int m_realCount = 0;
};
