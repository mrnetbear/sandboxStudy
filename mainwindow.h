#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "./ui_mainwindow.h"
#include "logger.h"
#include <QMainWindow>
#include <QMessageBox>
#include <QProcess>
#include <QFile>
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QIntValidator>
#include <unistd.h>
#include "dialogconnect.h"
#include "dialogsettingvispath.h"
#include "dialogprotocol.h"
#include "digitizeroperation.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool loadConfig();

protected:


private slots:
    void on_ButtonConnect_clicked();

    void on_action_triggered();

    void on_action_ROOT_triggered();

    void on_action_settings_vis_path_triggered();

    void on_actionProtocol_triggered();

    void on_ButtonDisconnect_clicked();

    void on_ButtonStartStop_clicked();

    void on_lineEditRecordLength_editingFinished();

    void on_lineEditRecordLength_returnPressed();

private:
    Ui::MainWindow *ui;
    QString pathToVis = "/home/mrnetlex/qt-projects/sandboxStudy/vis/build/rootVis";
    QString pathToConfig = "//home/mrnetlex/qt-projects/sandboxStudy/config";
    DigitizerOperation digitizer;
};
#endif // MAINWINDOW_H
