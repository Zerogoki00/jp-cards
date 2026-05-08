#include "CardDeck.h"

void CardDeck::pad(int multiple) {
    m_realCount = m_cards.size();
    if (multiple <= 0) return;
    while (m_cards.size() % multiple != 0) {
        m_cards.append(Card{});
    }
}
