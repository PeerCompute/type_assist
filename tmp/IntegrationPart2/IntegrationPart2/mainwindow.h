#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QCloseEvent;
class QLabel;
class QDialog;
class QLineEdit;
class QCheckBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void newFile();
    void openFile();
    void saveFile();
    void saveFileAs();
    void showFindDialog();
    void findNext();
    void findPrevious();
    void toggleWordWrap(bool enable);
    void updateZoomLabel(int percent);
    void updateCursorInfo(int line, int col, int wordCount, int charCount);

private:
    Ui::MainWindow *ui;
    QString currentFile;

    QLabel *m_cursorPosLabel = nullptr;
    QLabel *m_docStatsLabel = nullptr;
    QLabel *m_zoomLabel = nullptr;
    QLabel *m_lineEndingLabel = nullptr;
    QLabel *m_encodingLabel = nullptr;

    // Find dialog widgets
    QDialog *m_findDialog = nullptr;
    QLineEdit *m_findLineEdit = nullptr;
    QCheckBox *m_matchCaseBox = nullptr;

    void setupMenusAndShortcuts();
    void setupStatusBar();
    bool saveCurrentFile();
    bool saveAs();
    bool maybeSave();
    void updateWindowTitle();

protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // MAINWINDOW_H