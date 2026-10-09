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

private:
    Ui::MainWindow *ui;
    QString currentFile;

    bool saveCurrentFile();
    bool saveAs();
    bool maybeSave();
    void updateWindowTitle();

protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif