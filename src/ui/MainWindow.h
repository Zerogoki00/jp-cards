#pragma once

#include <QMainWindow>

class FlashcardsController;
class PrintController;
class QAction;
class QLabel;
class QPdfView;
class QPlainTextEdit;
class QSpinBox;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onOpenCsv();
    void onCardEditor();
    void onSavePdf();
    void onPrintAll();
    void onPrintOdd();
    void onPrintEven();
    void onZoomIn();
    void onZoomOut();
    void onZoomFit();
    void onPrevPage();
    void onNextPage();
    void onAbout();

    void onDocumentLoaded();
    void onPageChanged();

private:
    void buildActions();
    void buildMenu();
    void buildToolbar();
    void buildCentralLayout();

    void log(const QString& message, const QString& severity = QStringLiteral("info"));
    void updateActionStates();
    void updatePageLabel();

    QString lastCsvDir() const;
    void    setLastCsvDir(const QString& dir);
    QString lastSaveDir() const;
    void    setLastSaveDir(const QString& dir);
    int     frontFontSizePt() const;
    void    setFrontFontSizePt(int pointSize);

    QAction* m_openAction       = nullptr;
    QAction* m_cardEditorAction = nullptr;
    QAction* m_saveAction       = nullptr;
    QAction* m_printAllAction  = nullptr;
    QAction* m_printOddAction  = nullptr;
    QAction* m_printEvenAction = nullptr;
    QAction* m_zoomInAction    = nullptr;
    QAction* m_zoomOutAction   = nullptr;
    QAction* m_zoomFitAction   = nullptr;
    QAction* m_prevPageAction  = nullptr;
    QAction* m_nextPageAction  = nullptr;
    QAction* m_aboutAction     = nullptr;

    QSpinBox* m_frontFontSizeSpin = nullptr;

    QPdfView*       m_pdfView   = nullptr;
    QPlainTextEdit* m_logPane   = nullptr;
    QLabel*         m_pageLabel = nullptr;

    FlashcardsController* m_controller      = nullptr;
    PrintController*      m_printController = nullptr;
};
