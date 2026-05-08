#include "CardEditorDialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {
constexpr int kColWord  = 0;
constexpr int kColFuri  = 1;
constexpr int kColTrans = 2;
}

CardEditorDialog::CardEditorDialog(const QVector<Card>& initial, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Card Editor"));
    resize(820, 560);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels(
        {tr("Word"), tr("Furigana"), tr("Translation")});
    m_table->horizontalHeader()->setSectionResizeMode(kColWord,  QHeaderView::Interactive);
    m_table->horizontalHeader()->setSectionResizeMode(kColFuri,  QHeaderView::Interactive);
    m_table->horizontalHeader()->setSectionResizeMode(kColTrans, QHeaderView::Stretch);
    m_table->setColumnWidth(kColWord, 200);
    m_table->setColumnWidth(kColFuri, 200);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked
                           | QAbstractItemView::EditKeyPressed
                           | QAbstractItemView::AnyKeyPressed
                           | QAbstractItemView::SelectedClicked);
    m_table->verticalHeader()->setDefaultSectionSize(
        m_table->fontMetrics().lineSpacing() + 8);

    for (const Card& c : initial) appendRow(c);

    m_btnAdd    = new QPushButton(tr("Add Card"), this);
    m_btnRemove = new QPushButton(tr("Remove"),   this);
    m_btnUp     = new QPushButton(tr("Move Up"),  this);
    m_btnDown   = new QPushButton(tr("Move Down"),this);

    auto* sideBox = new QVBoxLayout;
    sideBox->addWidget(m_btnAdd);
    sideBox->addWidget(m_btnRemove);
    sideBox->addSpacing(12);
    sideBox->addWidget(m_btnUp);
    sideBox->addWidget(m_btnDown);
    sideBox->addStretch();

    auto* topRow = new QHBoxLayout;
    topRow->addWidget(m_table, 1);
    topRow->addLayout(sideBox);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    auto* root = new QVBoxLayout(this);
    root->addLayout(topRow, 1);
    root->addWidget(buttons);

    connect(m_btnAdd,    &QPushButton::clicked, this, &CardEditorDialog::onAdd);
    connect(m_btnRemove, &QPushButton::clicked, this, &CardEditorDialog::onRemove);
    connect(m_btnUp,     &QPushButton::clicked, this, &CardEditorDialog::onMoveUp);
    connect(m_btnDown,   &QPushButton::clicked, this, &CardEditorDialog::onMoveDown);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &CardEditorDialog::onSelectionChanged);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    onSelectionChanged();
}

void CardEditorDialog::appendRow(const Card& card) {
    const int row = m_table->rowCount();
    m_table->insertRow(row);
    m_table->setItem(row, kColWord,  new QTableWidgetItem(card.word));
    m_table->setItem(row, kColFuri,  new QTableWidgetItem(card.furigana));
    m_table->setItem(row, kColTrans, new QTableWidgetItem(card.translation));
}

void CardEditorDialog::swapRows(int a, int b) {
    if (a == b) return;
    for (int c = 0; c < m_table->columnCount(); ++c) {
        QTableWidgetItem* itemA = m_table->takeItem(a, c);
        QTableWidgetItem* itemB = m_table->takeItem(b, c);
        m_table->setItem(a, c, itemB);
        m_table->setItem(b, c, itemA);
    }
}

int CardEditorDialog::currentRow() const {
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) return -1;
    return rows.first().row();
}

void CardEditorDialog::onAdd() {
    appendRow(Card{});
    const int row = m_table->rowCount() - 1;
    m_table->setCurrentCell(row, kColWord);
    m_table->editItem(m_table->item(row, kColWord));
}

void CardEditorDialog::onRemove() {
    const int row = currentRow();
    if (row < 0) return;
    m_table->removeRow(row);
    if (m_table->rowCount() > 0) {
        m_table->selectRow(std::min(row, m_table->rowCount() - 1));
    }
}

void CardEditorDialog::onMoveUp() {
    const int row = currentRow();
    if (row <= 0) return;
    swapRows(row - 1, row);
    m_table->selectRow(row - 1);
}

void CardEditorDialog::onMoveDown() {
    const int row = currentRow();
    if (row < 0 || row >= m_table->rowCount() - 1) return;
    swapRows(row, row + 1);
    m_table->selectRow(row + 1);
}

void CardEditorDialog::onSelectionChanged() {
    const int row = currentRow();
    const bool hasRow = row >= 0;
    m_btnRemove->setEnabled(hasRow);
    m_btnUp    ->setEnabled(hasRow && row > 0);
    m_btnDown  ->setEnabled(hasRow && row < m_table->rowCount() - 1);
}

QVector<Card> CardEditorDialog::cards() const {
    auto cellText = [this](int r, int c) {
        const QTableWidgetItem* it = m_table->item(r, c);
        return it ? it->text() : QString();
    };
    QVector<Card> out;
    out.reserve(m_table->rowCount());
    for (int r = 0; r < m_table->rowCount(); ++r) {
        Card c{ cellText(r, kColWord),
                cellText(r, kColFuri),
                cellText(r, kColTrans) };
        if (!c.isEmpty()) out.append(c);
    }
    return out;
}
