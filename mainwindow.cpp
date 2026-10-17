#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "filemanager.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QTextEdit>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QAction>
#include <QListWidget>
#include <QCoreApplication>
#include <QFile>
#include <vector>
#include <string>

static bool isAsciiLetter(QChar c)
{
    const ushort u = c.unicode();
    return (u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z');
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->editor->setAcceptRichText(false);

    connect(ui->actionNew, &QAction::triggered,
            this, &MainWindow::newFile);

    connect(ui->actionOpen, &QAction::triggered,
            this, &MainWindow::openFile);

    connect(ui->actionSave, &QAction::triggered,
            this, &MainWindow::saveFile);

    connect(ui->actionSaveAs, &QAction::triggered,
            this, &MainWindow::saveFileAs);

    connect(ui->actionExit, &QAction::triggered,
            this, &QWidget::close);

    ui->actionNew->setShortcut(QKeySequence::New);
    ui->actionOpen->setShortcut(QKeySequence::Open);
    ui->actionSave->setShortcut(QKeySequence::Save);
    ui->actionSaveAs->setShortcut(QKeySequence::SaveAs);
    ui->actionExit->setShortcut(QKeySequence::Quit);

    connect(ui->editor, &QTextEdit::textChanged,
            this, &MainWindow::updateWindowTitle);

    setupAutocomplete();

    updateWindowTitle();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::newFile()
{
    if (!maybeSave())
        return;

    ui->editor->clear();
    currentFile.clear();

    ui->editor->document()->setModified(false);
    updateWindowTitle();
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

    if (!FileManager::openFile(fileName, content))
    {
        QMessageBox::warning(
            this, "Error", "Could not open the file."
        );
        return;
    }

    ui->editor->setPlainText(content);
    currentFile = fileName;

    ui->editor->document()->setModified(false);
    updateWindowTitle();
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

    if (!FileManager::saveFile(
            currentFile,
            ui->editor->toPlainText()))
    {
        QMessageBox::warning(
            this, "Error", "Could not save the file."
        );
        return false;
    }

    ui->editor->document()->setModified(false);
    updateWindowTitle();

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

    if (!FileManager::saveFile(
            fileName,
            ui->editor->toPlainText()))
    {
        QMessageBox::warning(
            this, "Error", "Could not save the file."
        );
        return false;
    }

    currentFile = fileName;

    ui->editor->document()->setModified(false);
    updateWindowTitle();

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
        QMessageBox::Save |
        QMessageBox::Discard |
        QMessageBox::Cancel
    );

    if (answer == QMessageBox::Save)
        return saveCurrentFile();

    if (answer == QMessageBox::Cancel)
        return false;

    return true;
}

void MainWindow::updateWindowTitle()
{
    QString name;

    if (currentFile.isEmpty())
        name = "Untitled";
    else
        name = QFileInfo(currentFile).fileName();

    if (ui->editor->document()->isModified())
        name += " *";

    setWindowTitle(name + " - SmartWrite");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        hidePopup();
        if (autocompleteReady &&
            !autocomplete.saveFrequencies(freqPath.toStdString()))
        {
            QMessageBox::warning(this, "Warning",
                                 "Could not save frequencies.txt");
        }
        event->accept();
    } else {
        event->ignore();
    }
}

// ===================== Autocomplete =====================

void MainWindow::setupAutocomplete()
{
    // dictionary.txt: working directory first, then next to the executable.
    QString dictPath = "dictionary.txt";
    if (!QFile::exists(dictPath)) {
        dictPath = QCoreApplication::applicationDirPath() + "/dictionary.txt";
    }

    if (!autocomplete.loadDictionary(dictPath.toStdString())) {
        QMessageBox::warning(this, "Autocomplete",
            "Could not load dictionary.txt.\n"
            "The editor will work, but without suggestions.");
        return;
    }

    // frequencies.txt lives next to the dictionary.
    freqPath = QFileInfo(dictPath).absolutePath() + "/frequencies.txt";

    // A missing frequency file is expected on the first run.
    autocomplete.loadFrequencies(freqPath.toStdString());
    autocomplete.rebuildTopK();
    autocompleteReady = true;

    // Suggestion popup (does not take keyboard focus from the editor).
    popup = new QListWidget(this);
    popup->setWindowFlags(Qt::ToolTip);
    popup->setFocusPolicy(Qt::NoFocus);
    popup->setAttribute(Qt::WA_ShowWithoutActivating);
    popup->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    popup->hide();

    ui->editor->installEventFilter(this);

    connect(ui->editor, &QTextEdit::textChanged,
            this, &MainWindow::updateSuggestions);
    connect(ui->editor, &QTextEdit::cursorPositionChanged,
            this, &MainWindow::updateSuggestions);
    connect(popup, &QListWidget::itemClicked,
            this, &MainWindow::onSuggestionClicked);
}

// The letters directly before the cursor, if the cursor is at the end of a word.
QString MainWindow::currentPrefix() const
{
    QTextCursor tc = ui->editor->textCursor();
    if (tc.hasSelection())
        return QString();

    const QString text = tc.block().text();
    const int pos = tc.positionInBlock();

    // Cursor in the middle of a word: no suggestions.
    if (pos < text.size() && isAsciiLetter(text[pos]))
        return QString();

    int start = pos;
    while (start > 0 && isAsciiLetter(text[start - 1]))
        --start;

    return text.mid(start, pos - start);
}

void MainWindow::updateSuggestions()
{
    if (!autocompleteReady)
        return;

    const QString prefix = currentPrefix();
    if (prefix.isEmpty()) {
        hidePopup();
        return;
    }

    // Walk the trie from the root for this prefix.
    TrieNode *node = autocomplete.getTrie().getRoot();
    const std::string lowered = prefix.toLower().toStdString();
    for (char c : lowered) {
        if (!node) break;
        node = autocomplete.getTrie().getChild(node, c);
    }

    std::vector<std::string> suggestions;
    if (node)
        suggestions = autocomplete.getTopKSuggestionsAtNode(node);

    popup->clear();
    for (const std::string &w : suggestions) {
        if (w == lowered)
            continue; // don't suggest what is already typed
        auto *item = new QListWidgetItem(
            QString::fromStdString(w) + "   (" +
            QString::number(autocomplete.getFrequency(w)) + ")", popup);
        item->setData(Qt::UserRole, QString::fromStdString(w));
    }

    if (popup->count() == 0) {
        hidePopup();
        return;
    }

    popup->setCurrentRow(0);

    const int rows = qMin(popup->count(), 6);
    const int rowH = popup->sizeHintForRow(0);
    popup->resize(240, rows * rowH + 2 * popup->frameWidth() + 2);

    const QRect r = ui->editor->cursorRect();
    popup->move(ui->editor->viewport()->mapToGlobal(r.bottomLeft()));
    popup->show();
}

void MainWindow::hidePopup()
{
    if (popup)
        popup->hide();
}

void MainWindow::acceptSuggestion(const QString &word)
{
    const QString prefix = currentPrefix();
    if (prefix.isEmpty() || word.isEmpty())
        return;

    QString out = word;
    if (prefix[0].isUpper())
        out[0] = out[0].toUpper(); // keep a capitalised start

    QTextCursor tc = ui->editor->textCursor();
    tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, prefix.length());
    tc.insertText(out);
    ui->editor->setTextCursor(tc);

    autocomplete.recordWord(word.toStdString());
    hidePopup();
}

void MainWindow::onSuggestionClicked(QListWidgetItem *item)
{
    acceptSuggestion(item->data(Qt::UserRole).toString());
    ui->editor->setFocus();
}

// Called when a word is finished by space / punctuation / Enter.
void MainWindow::recordFinishedWord()
{
    const QString prefix = currentPrefix();
    if (!prefix.isEmpty())
        autocomplete.recordWord(prefix.toLower().toStdString());
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->editor && autocompleteReady) {
        if (event->type() == QEvent::FocusOut) {
            hidePopup();
            return false;
        }

        if (event->type() == QEvent::KeyPress) {
            auto *ke = static_cast<QKeyEvent *>(event);

            if (popup->isVisible()) {
                switch (ke->key()) {
                case Qt::Key_Tab: {
                    QListWidgetItem *it = popup->currentItem();
                    if (it)
                        acceptSuggestion(it->data(Qt::UserRole).toString());
                    return true;
                }
                case Qt::Key_Down:
                    popup->setCurrentRow(
                        qMin(popup->currentRow() + 1, popup->count() - 1));
                    return true;
                case Qt::Key_Up:
                    popup->setCurrentRow(qMax(popup->currentRow() - 1, 0));
                    return true;
                case Qt::Key_Escape:
                    hidePopup();
                    return true;
                default:
                    break;
                }
            }

            // Record the word just finished, then let the key through.
            if (!(ke->modifiers() & (Qt::ControlModifier | Qt::MetaModifier))) {
                const QString t = ke->text();
                const bool boundary =
                    ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter ||
                    (!t.isEmpty() && (t[0].isSpace() || t[0].isPunct()));
                if (boundary)
                    recordFinishedWord();
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}
