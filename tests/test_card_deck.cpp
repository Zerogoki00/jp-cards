#include "domain/CardDeck.h"

#include <QtTest>

class TestCardDeck : public QObject {
    Q_OBJECT
private slots:
    void padFromZero();
    void padFromSeventeen();
    void padFromExactlyOnePage();
    void padFromNineteen();
    void pageCountAfterPad();
};

namespace {
CardDeck make(int n) {
    CardDeck d;
    for (int i = 0; i < n; ++i) d.append(Card{QStringLiteral("f"), QStringLiteral("b")});
    return d;
}
}

void TestCardDeck::padFromZero() {
    CardDeck d;
    d.pad();
    QCOMPARE(d.size(), 0);
    QCOMPARE(d.realCount(), 0);
}

void TestCardDeck::padFromSeventeen() {
    CardDeck d = make(17);
    d.pad();
    QCOMPARE(d.size(), 18);
    QCOMPARE(d.realCount(), 17);
}

void TestCardDeck::padFromExactlyOnePage() {
    CardDeck d = make(18);
    d.pad();
    QCOMPARE(d.size(), 18);
    QCOMPARE(d.realCount(), 18);
}

void TestCardDeck::padFromNineteen() {
    CardDeck d = make(19);
    d.pad();
    QCOMPARE(d.size(), 36);
    QCOMPARE(d.realCount(), 19);
}

void TestCardDeck::pageCountAfterPad() {
    CardDeck d = make(37);
    d.pad();
    QCOMPARE(d.size(), 54);
    QCOMPARE(d.pageCount(), 3);
}

QTEST_GUILESS_MAIN(TestCardDeck)
#include "test_card_deck.moc"
