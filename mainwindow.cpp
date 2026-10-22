#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "filemanager.h"
#include "AutocompleteEngineAdapter.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QTextEdit>
#include <QTextDocument>
#include <QCloseEvent>
#include <QKeySequence>
#include <QAction>
#include <QStatusBar>
#include <QLabel>
#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "textedit.h"
// #include "dummyautocompleteengine.h"
// #include "dummyspellchecker.h"
#include "SpellChecker/SpellChecker.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Autocomplete engine
    static AutocompleteEngineAdapter ac;
    if (ac.initialize("dictionary.txt", "frequencies.txt")) {
        ui->editor->setAutocompleteEngine(&ac);
        statusBar()->showMessage("Autocomplete engine loaded.", 3500);
    } else {
        statusBar()->showMessage(
            "Failed to load autocomplete dictionary.", 5000);
    }

    // Spell checker
    static SpellChecker sp;
    if (sp.initialize("dictionary.txt")) {
        ui->editor->setSpellChecker(&sp);
        statusBar()->showMessage("SpellChecker engine loaded.", 3500);
    } else {
        statusBar()->showMessage("Failed to load dictionary.", 5000);
    }

    // static DummyAutocompleteEngine ac;
    // static DummySpellChecker sp;
    // ui->editor->setAutocompleteEngine(&ac);
    // ui->editor->setSpellChecker(&sp);

    setupStatusBar();
    setupMenusAndShortcuts();

    connect(ui->editor, &QTextEdit::textChanged,
            this, &MainWindow::updateWindowTitle);

    connect(ui->editor, &TextEdit::statusMessage,
            this, [this](const QString &msg) {
                statusBar()->showMessage(msg, 3500);
            });

    connect(ui->editor, &TextEdit::zoomChanged,
            this, &MainWindow::updateZoomLabel);

    connect(ui->editor, &TextEdit::cursorInfoChanged,
            this, &MainWindow::updateCursorInfo);

    updateWindowTitle();
    updateCursorInfo(1, 1, 0, 0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupStatusBar()
{
    // Clean Notepad-like status bar styling
    statusBar()->setStyleSheet(
        "QStatusBar {"
        "  background: #f8fafc;"
        "  border-top: 1px solid #e2e8f0;"
        "  font-family: 'Segoe UI', sans-serif;"
        "  font-size: 11px;"
        "  color: #475569;"
        "}"
        "QLabel {"
        "  padding: 0px 8px;"
        "  color: #475569;"
        "}"
    );

    m_cursorPosLabel = new QLabel("Ln 1, Col 1", this);
    m_cursorPosLabel->setMinimumWidth(85);

    m_docStatsLabel = new QLabel("0 words, 0 chars", this);
    m_docStatsLabel->setMinimumWidth(110);

    m_zoomLabel = new QLabel("100%", this);
    m_zoomLabel->setMinimumWidth(50);
    m_zoomLabel->setAlignment(Qt::AlignCenter);

    m_lineEndingLabel = new QLabel("Windows (CRLF)", this);
    m_lineEndingLabel->setMinimumWidth(95);

    m_encodingLabel = new QLabel("UTF-8", this);
    m_encodingLabel->setMinimumWidth(55);

    statusBar()->addPermanentWidget(m_cursorPosLabel);
    statusBar()->addPermanentWidget(m_docStatsLabel);
    statusBar()->addPermanentWidget(m_zoomLabel);
    statusBar()->addPermanentWidget(m_lineEndingLabel);
    statusBar()->addPermanentWidget(m_encodingLabel);

    statusBar()->showMessage("Ready", 2000);
}

void MainWindow::setupMenusAndShortcuts()
{
    // File Actions
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newFile);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveFile);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::saveFileAs);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

    ui->actionNew->setShortcut(QKeySequence::New);
    ui->actionOpen->setShortcut(QKeySequence::Open);
    ui->actionSave->setShortcut(QKeySequence::Save);
    ui->actionSaveAs->setShortcut(QKeySequence::SaveAs);
    ui->actionExit->setShortcut(QKeySequence::Quit);

    // Edit Actions
    connect(ui->actionUndo, &QAction::triggered, ui->editor, &QTextEdit::undo);
    connect(ui->actionRedo, &QAction::triggered, ui->editor, &QTextEdit::redo);
    connect(ui->actionCut, &QAction::triggered, ui->editor, &QTextEdit::cut);
    connect(ui->actionCopy, &QAction::triggered, ui->editor, &QTextEdit::copy);
    connect(ui->actionPaste, &QAction::triggered, ui->editor, &QTextEdit::paste);
    connect(ui->actionDelete, &QAction::triggered, this, [this]() {
        ui->editor->textCursor().removeSelectedText();
    });
    connect(ui->actionFind, &QAction::triggered, this, &MainWindow::showFindDialog);
    connect(ui->actionSelectAll, &QAction::triggered, ui->editor, &QTextEdit::selectAll);

    ui->actionUndo->setShortcut(QKeySequence::Undo);
    ui->actionRedo->setShortcut(QKeySequence::Redo);
    ui->actionCut->setShortcut(QKeySequence::Cut);
    ui->actionCopy->setShortcut(QKeySequence::Copy);
    ui->actionPaste->setShortcut(QKeySequence::Paste);
    ui->actionDelete->setShortcut(QKeySequence::Delete);
    ui->actionFind->setShortcut(QKeySequence::Find);
    ui->actionSelectAll->setShortcut(QKeySequence::SelectAll);

    // View Actions & Zoom Shortcuts
    connect(ui->actionZoomIn, &QAction::triggered, ui->editor, &TextEdit::zoomInStep);
    connect(ui->actionZoomOut, &QAction::triggered, ui->editor, &TextEdit::zoomOutStep);
    connect(ui->actionRestoreDefaultZoom, &QAction::triggered, ui->editor, &TextEdit::resetZoomStep);
    connect(ui->actionWordWrap, &QAction::toggled, this, &MainWindow::toggleWordWrap);
    connect(ui->actionStatusBar, &QAction::toggled, ui->statusbar, &QWidget::setVisible);

    ui->actionZoomIn->setShortcutContext(Qt::ApplicationShortcut);
    ui->actionZoomOut->setShortcutContext(Qt::ApplicationShortcut);
    ui->actionRestoreDefaultZoom->setShortcutContext(Qt::ApplicationShortcut);

    ui->actionZoomIn->setShortcuts({
        QKeySequence::ZoomIn,
        QKeySequence("Ctrl+="),
        QKeySequence("Ctrl+Shift+="),
        QKeySequence("Ctrl++")
    });
    ui->actionZoomOut->setShortcuts({
        QKeySequence::ZoomOut,
        QKeySequence("Ctrl+-"),
        QKeySequence("Ctrl+_")
    });
    ui->actionRestoreDefaultZoom->setShortcuts({
        QKeySequence("Ctrl+0"),
        QKeySequence(Qt::CTRL | Qt::Key_0)
    });
}

void MainWindow::updateZoomLabel(int percent)
{
    if (m_zoomLabel) {
        m_zoomLabel->setText(QString("%1%").arg(percent));
    }
}

void MainWindow::updateCursorInfo(int line, int col, int wordCount, int charCount)
{
    if (m_cursorPosLabel) {
        m_cursorPosLabel->setText(QString("Ln %1, Col %2").arg(line).arg(col));
    }
    if (m_docStatsLabel) {
        m_docStatsLabel->setText(QString("%1 words, %2 chars").arg(wordCount).arg(charCount));
    }
}

void MainWindow::toggleWordWrap(bool enable)
{
    ui->editor->setLineWrapMode(enable ? QTextEdit::WidgetWidth : QTextEdit::NoWrap);
}

void MainWindow::showFindDialog()
{
    if (!m_findDialog) {
        m_findDialog = new QDialog(this);
        m_findDialog->setWindowTitle("Find");
        m_findDialog->setWindowFlags(m_findDialog->windowFlags() & ~Qt::WindowContextHelpButtonHint);
        m_findDialog->setFixedSize(360, 130);

        m_findLineEdit = new QLineEdit(m_findDialog);
        m_findLineEdit->setPlaceholderText("Search text...");

        m_matchCaseBox = new QCheckBox("Match &case", m_findDialog);

        auto *findNextBtn = new QPushButton("Find &Next", m_findDialog);
        auto *findPrevBtn = new QPushButton("Find &Prev", m_findDialog);
        auto *closeBtn = new QPushButton("Cancel", m_findDialog);

        connect(findNextBtn, &QPushButton::clicked, this, &MainWindow::findNext);
        connect(findPrevBtn, &QPushButton::clicked, this, &MainWindow::findPrevious);
        connect(closeBtn, &QPushButton::clicked, m_findDialog, &QDialog::close);
        connect(m_findLineEdit, &QLineEdit::returnPressed, this, &MainWindow::findNext);

        auto *btnLayout = new QVBoxLayout();
        btnLayout->addWidget(findNextBtn);
        btnLayout->addWidget(findPrevBtn);
        btnLayout->addWidget(closeBtn);
        btnLayout->addStretch();

        auto *leftLayout = new QVBoxLayout();
        leftLayout->addWidget(new QLabel("Find what:", m_findDialog));
        leftLayout->addWidget(m_findLineEdit);
        leftLayout->addWidget(m_matchCaseBox);
        leftLayout->addStretch();

        auto *mainLayout = new QHBoxLayout(m_findDialog);
        mainLayout->addLayout(leftLayout);
        mainLayout->addLayout(btnLayout);
    }

    // Prepopulate with selected text if any
    QString selected = ui->editor->textCursor().selectedText();
    if (!selected.isEmpty() && !selected.contains('\n')) {
        m_findLineEdit->setText(selected);
    }
    m_findLineEdit->selectAll();
    m_findLineEdit->setFocus();
    m_findDialog->show();
    m_findDialog->raise();
    m_findDialog->activateWindow();
}

void MainWindow::findNext()
{
    if (!m_findLineEdit) return;
    QString query = m_findLineEdit->text();
    if (query.isEmpty()) return;

    QTextDocument::FindFlags flags;
    if (m_matchCaseBox && m_matchCaseBox->isChecked()) {
        flags |= QTextDocument::FindCaseSensitively;
    }

    bool found = ui->editor->find(query, flags);
    if (!found) {
        // Wrap around to beginning of document
        QTextCursor c = ui->editor->textCursor();
        c.movePosition(QTextCursor::Start);
        ui->editor->setTextCursor(c);
        found = ui->editor->find(query, flags);
        if (!found) {
            statusBar()->showMessage(QString("Cannot find \"%1\"").arg(query), 3000);
        }
    }
}

void MainWindow::findPrevious()
{
    if (!m_findLineEdit) return;
    QString query = m_findLineEdit->text();
    if (query.isEmpty()) return;

    QTextDocument::FindFlags flags = QTextDocument::FindBackward;
    if (m_matchCaseBox && m_matchCaseBox->isChecked()) {
        flags |= QTextDocument::FindCaseSensitively;
    }

    bool found = ui->editor->find(query, flags);
    if (!found) {
        // Wrap around to end of document
        QTextCursor c = ui->editor->textCursor();
        c.movePosition(QTextCursor::End);
        ui->editor->setTextCursor(c);
        found = ui->editor->find(query, flags);
        if (!found) {
            statusBar()->showMessage(QString("Cannot find \"%1\"").arg(query), 3000);
        }
    }
}

void MainWindow::newFile()
{
    if (!maybeSave())
        return;

    ui->editor->clear();
    currentFile.clear();

    ui->editor->document()->setModified(false);
    updateWindowTitle();
    statusBar()->showMessage("New file created.", 2000);
}

void MainWindow::openFile()
{
    if (!maybeSave())
        return;

    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Open File",
        QString(),
        "Text Files (*.txt);;All Files (*)"
    );

    if (fileName.isEmpty())
        return;

    QString content;
    if (!FileManager::openFile(fileName, content)) {
        QMessageBox::warning(this, "Error", "Could not open the file.");
        return;
    }

    ui->editor->setPlainText(content);
    currentFile = fileName;

    ui->editor->document()->setModified(false);
    updateWindowTitle();
    statusBar()->showMessage(QString("Opened %1").arg(QFileInfo(fileName).fileName()), 2500);
}

void MainWindow::saveFile()
{
    saveCurrentFile();
}

void MainWindow::saveFileAs()
{
    saveAs();
}

bool MainWindow::saveCurrentFile()
{
    if (currentFile.isEmpty())
        return saveAs();

    if (!FileManager::saveFile(currentFile, ui->editor->toPlainText())) {
        QMessageBox::warning(this, "Error", "Could not save the file.");
        return false;
    }

    ui->editor->document()->setModified(false);
    updateWindowTitle();
    statusBar()->showMessage("File saved.", 2500);
    return true;
}

bool MainWindow::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save File",
        currentFile,
        "Text Files (*.txt);;All Files (*)"
    );

    if (fileName.isEmpty())
        return false;

    if (!fileName.endsWith(".txt", Qt::CaseInsensitive))
        fileName += ".txt";

    if (!FileManager::saveFile(fileName, ui->editor->toPlainText())) {
        QMessageBox::warning(this, "Error", "Could not save the file.");
        return false;
    }

    currentFile = fileName;
    ui->editor->document()->setModified(false);
    updateWindowTitle();
    statusBar()->showMessage("File saved.", 2500);
    return true;
}

bool MainWindow::maybeSave()
{
    if (!ui->editor->document()->isModified())
        return true;

    QMessageBox::StandardButton answer = QMessageBox::warning(
        this,
        "Unsaved Changes",
        "Do you want to save your changes?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );

    if (answer == QMessageBox::Save)
        return saveCurrentFile();

    if (answer == QMessageBox::Cancel)
        return false;

    return true;
}

void MainWindow::updateWindowTitle()
{
    QString name = currentFile.isEmpty() ? "Untitled" : QFileInfo(currentFile).fileName();
    if (ui->editor->document()->isModified())
        name += " *";

    setWindowTitle(name + " - SmartWrite");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}
