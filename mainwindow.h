#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QProcess>
#include <QFile>
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QIntValidator>
#include <QByteArray>
#include <QTcpSocket>
#include <QTimer>

#include <unistd.h>

#include "./ui_mainwindow.h"
#include "logger.h"
#include "dialogconnect.h"
#include "dialogsettingvispath.h"
#include "dialogprotocol.h"
#include "digitizeroperation.h"
#include "digitizerprotocol.h"

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
    bool sendParams(quint16 channel, uint16_t gateWidth, uint16_t gatePosition);
    bool sendChannelMask(int16_t newChannelMask);
    bool sendTriggerMask(int16_t newTriggerMask);

    int16_t getSelectedChannels() const;
    int16_t getSelectedTriggers() const;

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

    void on_lineEditThreshold_returnPressed();

    void on_horizontalSlider_sliderMoved(int position);

    void on_horizontalSliderGateWidth_sliderMoved(int position);

    void on_horizontalSliderGatePosition_sliderMoved(int position);

    void on_lineEditOffset_returnPressed();

    void on_radioButtonECH0_clicked();

    void on_radioButtonECH1_clicked();

    void on_radioButtonECH2_clicked();

    void on_radioButtonECH3_clicked();

    void on_checkBoxW0_stateChanged(int arg1);

    void on_checkBoxW1_stateChanged(int arg1);

    void on_checkBoxW2_stateChanged(int arg1);

    void on_checkBoxW3_stateChanged(int arg1);

    void on_checkBoxT0_stateChanged(int arg1);

    void on_checkBoxT1_stateChanged(int arg1);

    void on_checkBoxT2_stateChanged(int arg1);

    void on_checkBoxT3_stateChanged(int arg1);

private:
    Ui::MainWindow *ui;
    QString pathToVis = "/home/mrnetlex/Documents/Progs/C++/qt-projects/sandboxStudyAlternative/altvis/build/rootVis";
    QString pathToConfig = "/home/mrnetlex/Documents/Progs/C++/qt-projects/sandboxStudyAlternative/config";
    DigitizerOperation digitizer;

    void initializeSettingsConnection();
    void flushSettingsPacket();

    QTcpSocket *settingsSocket = nullptr;
    QTimer *settingsReconnectTimer = nullptr;
    QTimer *settingsSendTimer = nullptr;

    // Пока один выбранный канал: храним его последнее сообщение.
    QByteArray latestSettingsPacket;
    bool settingsDirty = false;
};
#endif // MAINWINDOW_H
