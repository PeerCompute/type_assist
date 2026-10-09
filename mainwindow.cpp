#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "filemanager.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QTextEdit>
#include <QTextDocument>
#include <QCloseEvent>
#include <QKeySequence>
#include <QAction>

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
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}