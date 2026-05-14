#include "MainWindow.h"

#include "CardEditorDialog.h"
#include "app/FlashcardsController.h"
#include "app/PrintController.h"
#include "domain/CardDeck.h"

#include <QAction>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPdfDocument>
#include <QPdfPageNavigator>
#include <QPdfView>
#include <QPlainTextEdit>
#include <QSettings>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QSpinBox>
#include <QToolBar>

#include <algorithm>

namespace {
constexpr double kZoomStep = 1.25;
constexpr double kZoomMin  = 0.25;
constexpr double kZoomMax  = 8.0;
constexpr int kFrontFontMinPt = 8;
constexpr int kFrontFontMaxPt = 48;

constexpr const char* kKeyCsvDir  = "lastCsvDir";
constexpr const char* kKeySaveDir = "lastSaveDir";
constexpr const char* kKeyFrontFontSize = "frontFontSizePt";
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("Flashcards"));
    resize(1100, 800);

    m_controller      = new FlashcardsController(this);
    m_printController = new PrintController(this);
    m_controller->setFrontFontSizePt(frontFontSizePt());

    buildActions();
    buildMenu();
    buildToolbar();
    buildCentralLayout();

    connect(m_controller, &FlashcardsController::info,
            this, [this](const QString& m){ log(m); });
    connect(m_controller, &FlashcardsController::warning,
            this, [this](const QString& m){ log(m, QStringLiteral("warn")); });
    connect(m_controller, &FlashcardsController::error,
            this, [this](const QString& m){ log(m, QStringLiteral("error")); });
    connect(m_controller, &FlashcardsController::documentLoaded,
            this, &MainWindow::onDocumentLoaded);

    connect(m_printController, &PrintController::info,
            this, [this](const QString& m){ log(m); });
    connect(m_printController, &PrintController::error,
            this, [this](const QString& m){ log(m, QStringLiteral("error")); });
    connect(m_printController, &PrintController::printFinished,
            this, [this](PrintController::Mode mode) {
                if (mode == PrintController::Mode::Odd) {
                    QMessageBox::information(this, tr("Flip the stack"),
                        tr("Front side printed.\n\n"
                           "Flip the stack along the long edge and load it back into "
                           "the printer, then click \"Print even (back)\"."));
                }
            });

    statusBar()->showMessage(tr("Ready"));
    log(tr("Application started. Open a CSV to begin."));
    updateActionStates();
}

MainWindow::~MainWindow() = default;

void MainWindow::buildActions() {
    m_openAction       = new QAction(tr("Open CSV…"), this);
    m_cardEditorAction = new QAction(tr("Card Editor…"), this);
    m_saveAction       = new QAction(tr("Save PDF…"), this);
    m_printAllAction  = new QAction(tr("Print all"), this);
    m_printOddAction  = new QAction(tr("Print odd (front)"), this);
    m_printEvenAction = new QAction(tr("Print even (back)"), this);
    m_zoomInAction    = new QAction(tr("Zoom +"), this);
    m_zoomOutAction   = new QAction(tr("Zoom −"), this);
    m_zoomFitAction   = new QAction(tr("Fit"), this);
    m_prevPageAction  = new QAction(tr("◀ Page"), this);
    m_nextPageAction  = new QAction(tr("Page ▶"), this);
    m_aboutAction     = new QAction(tr("&About"), this);
    m_aboutAction->setMenuRole(QAction::AboutRole);

    m_openAction       ->setShortcut(QKeySequence::Open);
    m_cardEditorAction ->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
    m_saveAction       ->setShortcut(QKeySequence::Save);
    m_printAllAction->setShortcut(QKeySequence::Print);
    m_zoomInAction  ->setShortcut(QKeySequence::ZoomIn);
    m_zoomOutAction ->setShortcut(QKeySequence::ZoomOut);
    m_zoomFitAction ->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    m_prevPageAction->setShortcut(QKeySequence::MoveToPreviousPage);
    m_nextPageAction->setShortcut(QKeySequence::MoveToNextPage);

    connect(m_openAction,       &QAction::triggered, this, &MainWindow::onOpenCsv);
    connect(m_cardEditorAction, &QAction::triggered, this, &MainWindow::onCardEditor);
    connect(m_saveAction,       &QAction::triggered, this, &MainWindow::onSavePdf);
    connect(m_printAllAction,  &QAction::triggered, this, &MainWindow::onPrintAll);
    connect(m_printOddAction,  &QAction::triggered, this, &MainWindow::onPrintOdd);
    connect(m_printEvenAction, &QAction::triggered, this, &MainWindow::onPrintEven);
    connect(m_zoomInAction,    &QAction::triggered, this, &MainWindow::onZoomIn);
    connect(m_zoomOutAction,   &QAction::triggered, this, &MainWindow::onZoomOut);
    connect(m_zoomFitAction,   &QAction::triggered, this, &MainWindow::onZoomFit);
    connect(m_prevPageAction,  &QAction::triggered, this, &MainWindow::onPrevPage);
    connect(m_nextPageAction,  &QAction::triggered, this, &MainWindow::onNextPage);
    connect(m_aboutAction,     &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::buildMenu() {
    auto* helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(m_aboutAction);
}

void MainWindow::buildToolbar() {
    QToolBar* tb = addToolBar(tr("Main"));
    tb->setMovable(false);
    tb->setToolButtonStyle(Qt::ToolButtonTextOnly);

    tb->addAction(m_openAction);
    tb->addAction(m_cardEditorAction);
    tb->addAction(m_saveAction);
    tb->addSeparator();
    tb->addAction(m_printAllAction);
    tb->addAction(m_printOddAction);
    tb->addAction(m_printEvenAction);
    tb->addSeparator();
    tb->addAction(m_prevPageAction);
    tb->addAction(m_nextPageAction);
    tb->addAction(m_zoomOutAction);
    tb->addAction(m_zoomInAction);
    tb->addAction(m_zoomFitAction);
    tb->addSeparator();

    auto* frontFontLabel = new QLabel(tr("Front font"), tb);
    tb->addWidget(frontFontLabel);

    m_frontFontSizeSpin = new QSpinBox(tb);
    m_frontFontSizeSpin->setRange(kFrontFontMinPt, kFrontFontMaxPt);
    m_frontFontSizeSpin->setValue(static_cast<int>(m_controller->frontFontSizePt()));
    m_frontFontSizeSpin->setSuffix(tr(" pt"));
    m_frontFontSizeSpin->setToolTip(tr("Front-side font size"));
    m_frontFontSizeSpin->setMinimumWidth(72);
    tb->addWidget(m_frontFontSizeSpin);

    connect(m_frontFontSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int pointSize) {
                setFrontFontSizePt(pointSize);
                m_controller->setFrontFontSizePt(pointSize);
            });
}

void MainWindow::buildCentralLayout() {
    m_pdfView = new QPdfView(this);
    m_pdfView->setDocument(m_controller->document());
    m_pdfView->setPageMode(QPdfView::PageMode::MultiPage);
    m_pdfView->setZoomMode(QPdfView::ZoomMode::FitInView);

    connect(m_pdfView->pageNavigator(), &QPdfPageNavigator::currentPageChanged,
            this, &MainWindow::onPageChanged);

    m_logPane = new QPlainTextEdit(this);
    m_logPane->setReadOnly(true);
    m_logPane->setMaximumBlockCount(500);
    m_logPane->setPlaceholderText(tr("Operation log"));
    QFontMetrics fm(m_logPane->font());
    m_logPane->setFixedHeight(fm.lineSpacing() * 6 + 8);

    auto* splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(m_pdfView);
    splitter->addWidget(m_logPane);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    splitter->setCollapsible(0, false);
    splitter->setCollapsible(1, true);

    setCentralWidget(splitter);

    m_pageLabel = new QLabel(this);
    m_pageLabel->setMinimumWidth(80);
    statusBar()->addPermanentWidget(m_pageLabel);
}

// ---------------------------------------------------------------------------

void MainWindow::onOpenCsv() {
    const QString chosen = QFileDialog::getOpenFileName(
        this, tr("Open CSV"), lastCsvDir(),
        tr("CSV files (*.csv);;All files (*)"));
    if (chosen.isEmpty()) return;

    setLastCsvDir(QFileInfo(chosen).absolutePath());
    m_controller->loadCsv(chosen);
}

void MainWindow::onCardEditor() {
    CardEditorDialog dlg(m_controller->deck().cards(), this);
    if (dlg.exec() != QDialog::Accepted) return;

    const QVector<Card> edited = dlg.cards();
    if (edited.isEmpty()) {
        log(tr("Card editor: no cards — preview unchanged."),
            QStringLiteral("warn"));
        return;
    }
    CardDeck deck;
    for (const Card& c : edited) deck.append(c);
    m_controller->applyDeck(deck);
}

void MainWindow::onSavePdf() {
    if (!m_controller->hasDocument()) return;

    const QString stem = QFileInfo(m_controller->currentCsvPath()).completeBaseName();
    const QString defaultName = (stem.isEmpty()
            ? QStringLiteral("jp-cards-")
                  + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-hhmm"))
            : stem)
        + QStringLiteral(".pdf");
    const QString defaultPath = lastSaveDir() + QLatin1Char('/') + defaultName;

    const QString chosen = QFileDialog::getSaveFileName(
        this, tr("Save PDF"), defaultPath,
        tr("PDF files (*.pdf);;All files (*)"));
    if (chosen.isEmpty()) return;

    setLastSaveDir(QFileInfo(chosen).absolutePath());

    QFile out(chosen);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        log(tr("Save failed: %1 (%2)").arg(chosen, out.errorString()),
            QStringLiteral("error"));
        QMessageBox::warning(this, tr("Save PDF"),
                             tr("Could not write the PDF to the chosen location."));
        return;
    }
    const QByteArray& bytes = m_controller->pdfData();
    if (out.write(bytes) != bytes.size()) {
        log(tr("Save failed: %1 (%2)").arg(chosen, out.errorString()),
            QStringLiteral("error"));
        QMessageBox::warning(this, tr("Save PDF"),
                             tr("Could not write the PDF to the chosen location."));
        return;
    }
    log(tr("Saved %1").arg(chosen));
}

void MainWindow::onPrintAll() {
    if (!m_controller->hasDocument()) return;
    m_printController->print(m_controller->document(),
                             PrintController::Mode::All, this);
}
void MainWindow::onPrintOdd() {
    if (!m_controller->hasDocument()) return;
    m_printController->print(m_controller->document(),
                             PrintController::Mode::Odd, this);
}
void MainWindow::onPrintEven() {
    if (!m_controller->hasDocument()) return;
    m_printController->print(m_controller->document(),
                             PrintController::Mode::Even, this);
}

void MainWindow::onZoomIn() {
    if (m_pdfView->zoomMode() != QPdfView::ZoomMode::Custom) {
        m_pdfView->setZoomMode(QPdfView::ZoomMode::Custom);
        m_pdfView->setZoomFactor(1.0);
    }
    const double next = std::min(kZoomMax, m_pdfView->zoomFactor() * kZoomStep);
    m_pdfView->setZoomFactor(next);
}

void MainWindow::onZoomOut() {
    if (m_pdfView->zoomMode() != QPdfView::ZoomMode::Custom) {
        m_pdfView->setZoomMode(QPdfView::ZoomMode::Custom);
        m_pdfView->setZoomFactor(1.0);
    }
    const double next = std::max(kZoomMin, m_pdfView->zoomFactor() / kZoomStep);
    m_pdfView->setZoomFactor(next);
}

void MainWindow::onZoomFit() {
    m_pdfView->setZoomMode(QPdfView::ZoomMode::FitInView);
}

void MainWindow::onPrevPage() {
    auto* nav = m_pdfView->pageNavigator();
    nav->jump(std::max(0, nav->currentPage() - 1), QPointF{}, nav->currentZoom());
}

void MainWindow::onNextPage() {
    auto* nav = m_pdfView->pageNavigator();
    const int last = m_controller->document()->pageCount() - 1;
    nav->jump(std::min(last, nav->currentPage() + 1), QPointF{}, nav->currentZoom());
}

void MainWindow::onAbout() {
    QMessageBox::about(this, tr("About JP Cards"),
        tr("<h3>JP Cards %1</h3>"
           "<p>Build: %2</p>"
           "<p>Qt %3</p>"
           "<p><a href=\"https://github.com/Zerogoki00/jp-cards\">"
           "github.com/Zerogoki00/jp-cards</a></p>")
            .arg(QStringLiteral(JP_CARDS_VERSION),
                 QStringLiteral(JP_CARDS_BUILD_DATE),
                 QString::fromLatin1(qVersion())));
}

// ---------------------------------------------------------------------------

void MainWindow::onDocumentLoaded() {
    // Re-attach so QPdfView refreshes scrollbars / page count.
    m_pdfView->setDocument(m_controller->document());
    m_pdfView->setZoomMode(QPdfView::ZoomMode::FitInView);
    m_pdfView->pageNavigator()->jump(0, QPointF{}, 0.0);
    updateActionStates();
    updatePageLabel();
}

void MainWindow::onPageChanged() {
    updatePageLabel();
}

void MainWindow::updateActionStates() {
    const bool ready = m_controller->hasDocument();
    m_saveAction     ->setEnabled(ready);
    m_printAllAction ->setEnabled(ready);
    m_printOddAction ->setEnabled(ready);
    m_printEvenAction->setEnabled(ready);
    m_zoomInAction   ->setEnabled(ready);
    m_zoomOutAction  ->setEnabled(ready);
    m_zoomFitAction  ->setEnabled(ready);
    m_prevPageAction ->setEnabled(ready);
    m_nextPageAction ->setEnabled(ready);
}

void MainWindow::updatePageLabel() {
    if (!m_controller->hasDocument()) {
        m_pageLabel->setText(QString());
        return;
    }
    const int cur  = m_pdfView->pageNavigator()->currentPage() + 1;
    const int last = m_controller->document()->pageCount();
    m_pageLabel->setText(tr("Page %1 / %2").arg(cur).arg(last));
}

void MainWindow::log(const QString& message, const QString& severity) {
    if (!m_logPane) return;
    const QString prefix =
        severity == QStringLiteral("error") ? QStringLiteral("[error] ") :
        severity == QStringLiteral("warn")  ? QStringLiteral("[warn]  ") :
                                              QString();
    m_logPane->appendPlainText(prefix + message);
}

// --- Settings ---------------------------------------------------------------

QString MainWindow::lastCsvDir() const {
    QSettings s;
    return s.value(QString::fromLatin1(kKeyCsvDir),
                   QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
            .toString();
}
void MainWindow::setLastCsvDir(const QString& dir) {
    QSettings().setValue(QString::fromLatin1(kKeyCsvDir), dir);
}
QString MainWindow::lastSaveDir() const {
    QSettings s;
    return s.value(QString::fromLatin1(kKeySaveDir),
                   QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
            .toString();
}
void MainWindow::setLastSaveDir(const QString& dir) {
    QSettings().setValue(QString::fromLatin1(kKeySaveDir), dir);
}
int MainWindow::frontFontSizePt() const {
    QSettings s;
    bool ok = false;
    const int fallback = static_cast<int>(m_controller->frontFontSizePt());
    const int pointSize = s.value(QString::fromLatin1(kKeyFrontFontSize), fallback)
                              .toInt(&ok);
    return std::clamp(ok ? pointSize : fallback, kFrontFontMinPt, kFrontFontMaxPt);
}
void MainWindow::setFrontFontSizePt(int pointSize) {
    QSettings().setValue(QString::fromLatin1(kKeyFrontFontSize),
                         std::clamp(pointSize, kFrontFontMinPt, kFrontFontMaxPt));
}
