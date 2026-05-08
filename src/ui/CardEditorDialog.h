#pragma once

#include "domain/Card.h"

#include <QDialog>
#include <QVector>

class QPushButton;
class QTableWidget;

// Modal editor for a deck of Cards. Caller seeds it with the current deck
// (may be empty), runs exec(), reads the result via cards() on accept.
class CardEditorDialog : public QDialog {
    Q_OBJECT
public:
    explicit CardEditorDialog(const QVector<Card>& initial,
                              QWidget* parent = nullptr);

    QVector<Card> cards() const;

private slots:
    void onAdd();
    void onRemove();
    void onMoveUp();
    void onMoveDown();
    void onSelectionChanged();

private:
    void appendRow(const Card& card);
    void swapRows(int a, int b);
    int  currentRow() const;

    QTableWidget* m_table     = nullptr;
    QPushButton*  m_btnAdd    = nullptr;
    QPushButton*  m_btnRemove = nullptr;
    QPushButton*  m_btnUp     = nullptr;
    QPushButton*  m_btnDown   = nullptr;
};
