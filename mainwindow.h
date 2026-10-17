#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include "Autocomplete.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QCloseEvent;
class QListWidget;
class QListWidgetItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void newFile();
    void openFile();
    void saveFile();
    void saveFileAs();
    void updateSuggestions();
    void onSuggestionClicked(QListWidgetItem *item);

private:
    Ui::MainWindow *ui;
    QString currentFile;

    // Autocomplete
    Autocomplete autocomplete;
    bool autocompleteReady = false;
    QString freqPath;
    QListWidget *popup = nullptr;

    bool saveCurrentFile();
    bool saveAs();
    bool maybeSave();
    void updateWindowTitle();

    void setupAutocomplete();
    QString currentPrefix() const;
    void acceptSuggestion(const QString &word);
    void recordFinishedWord();
    void hidePopup();
};

#endif
