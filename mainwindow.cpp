#include "mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setFixedSize(800, 600);
    //setCentralWidget(rootWidget);
    Logger::instance()->setTextEdit(ui->textEdit);
    Logger::instance()->info("Application Started");
    Logger::instance()->info(QString("Qt Version: %1").arg(qVersion()));

    //ui->lineEditRecordLength->setValidator(new QIntValidator(1, 1048576, this));
    //ui->lineEditThreshold->setValidator(new QIntValidator(-1250, 1250, this));

    connect(ui->lineEditRecordLength, &QLineEdit::returnPressed,
            this, &MainWindow::on_lineEditRecordLength_returnPressed);
    connect(ui->lineEditThreshold, &QLineEdit::returnPressed,
            this, &MainWindow::on_lineEditThreshold_returnPressed);

    ui->lineEditRecordLength->setText(QString::number(digitizer.getRecLength()));
    QString thresText = (digitizer.getTrigThreshold() - ZERO_TRESHOLD_STATE) / 26 <= 0 ?
                            QString::number((ZERO_TRESHOLD_STATE - digitizer.getTrigThreshold()) / 26):
                            ("-" + QString::number((digitizer.getTrigThreshold() - ZERO_TRESHOLD_STATE) / 26));
    ui->lineEditThreshold->setText(thresText);
    //ui->lineEditRecordLength->setValidator(new QIntValidator(this));
    //ui->lineEditThreshold->setValidator(new QIntValidator(this));
    ui->lineEditRecordLength->setValidator(new QIntValidator(1, 1048576, this));
    ui->lineEditThreshold->setValidator(new QIntValidator(-1250, 1250, this));

    ui->ButtonDisconnect->setEnabled(false);

    // Enable the digitizer signals
    connect(&digitizer, &DigitizerOperation::progressUpdated,
            [this](QString msg) {
                qDebug() << msg;
            });

    connect(&digitizer, &DigitizerOperation::errorOccurred,
            [this](QString errorMsg) {
                QMessageBox::critical(this, "Digitizer Error", errorMsg);
            });

    connect(&digitizer, &DigitizerOperation::digitizerConnected,
            [this]() {
                QMessageBox::information(this, "Success", "Digitizer connected successfully!");
                // Обновляем UI - например, делаем кнопки активными
            });

    connect(&digitizer, &DigitizerOperation::digitizerDisconnected,
            [this]() {
                qDebug() << "Digitizer disconnected";
            });

    connect(&digitizer, &DigitizerOperation::dataAcquired,
            [this](int eventsCount) {
                qDebug() << "Acquired" << eventsCount << "events";
                // Обновляем счетчик на UI
                ui->labelEventsCount->setText(QString(" %1").arg(digitizer.getTotalEventsCount()));
            });
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::loadConfig(){
    if(!QFile::exists(pathToConfig)) {
        QMessageBox::warning(this, "Error",
            "Config file is missed. Check the path and try again.");
        return 1;
    }
    QFile confFile;
    confFile.setFileName(pathToConfig);

    if (!confFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        QMessageBox::warning(this, "Error",
            "Cannot open a config file.");
        return 1;
    }
    while(!confFile.atEnd()){
        qDebug() << confFile.readLine();
    }
    qDebug() << "Done!";
    confFile.close();
    return 0;
}

bool MainWindow::sendParams(quint16 channel,
                            uint16_t gateWidth, uint16_t gatePosition){
    if  (!digitizer.openVisualizationConnection())
        return false;

    DigitizerProtocol::Params packetParams;
    packetParams.channel = 0;
    packetParams.gatePosition = digitizer.getGatePosition();
    packetParams.gateWidth = digitizer.getGateWidth();

    const QByteArray packet = DigitizerProtocol::makeParametersPacket(packetParams);
    if(digitizer.visualizationSocket->write(packet) != packet.size() ||
        !digitizer.visualizationSocket->waitForBytesWritten(100)) {
        digitizer.closeVisualizationConnection();
        return false;
    }

    return true;
}

void MainWindow::on_ButtonConnect_clicked()
{
    //ui->ButtonConnect->setEnabled(false);
    //ui->ButtonDisconnect->setEnabled(true);
    // Открываем и настраиваем оцифровщик
    if (!digitizer.openDigitizer()) {
        Logger::instance()->error("Failed to open digitizer");
        //delete dlgConnect;
        return;
    }else{
        Logger::instance()->info("Digitizer opened successfully");
    }

    if (!digitizer.configureDigitizer()) {
        digitizer.closeDigitizer();
        //delete dlgConnect;
        return;
    }

    // Обновляем UI - показываем статус
    ui->ButtonConnect->setEnabled(false);
    ui->ButtonDisconnect->setEnabled(true);
    //ui->ButtonStartStop->setText("Stop");
    ui->labelStatus->setText("CONNECTED");
}

void MainWindow::on_ButtonDisconnect_clicked()
{
    digitizer.stopAcquisition();
    digitizer.closeDigitizer();

    ui->ButtonConnect->setEnabled(true);
    ui->ButtonDisconnect->setEnabled(false);
    ui->ButtonStartStop->setText("Start");
    ui->labelStatus->setText("DISCONNECTED");
}


void MainWindow::on_ButtonStartStop_clicked()
{
    if (digitizer.getAcquiringStatus()) {
        digitizer.stopAcquisition();
        ui->ButtonStartStop->setText("Start");
        Logger::instance()->info("Data aquisition stopped.");
    } else if (digitizer.startAcquisition()) {
        digitizer.startAcquisition();
        ui->ButtonStartStop->setText("Stop");
        Logger::instance()->info("Data aquisition started.");
    }
}

void MainWindow::on_action_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Открыть файл", QDir::homePath(), "Все файлы (*.*)");
    pathToConfig = fileName;
    this->loadConfig();

}


void MainWindow::on_action_ROOT_triggered()
{

    QProcess *process = new QProcess(this);
    //qDebug() << QCoreApplication::applicationDirPath();
    //process->startDetached(pathToVis);
    if (!process->startDetached(pathToVis)) {
        QMessageBox::warning(this, "Error!",
                             "Cannot open a ROOT application. Check the path and try again.");
    } else {
        qDebug() << "ROOT application started at:" << pathToVis;
    }
    process->waitForFinished();

}

void MainWindow::on_action_settings_vis_path_triggered()
{
    DialogSettingVisPath *dlgSettingVisPath = new DialogSettingVisPath(this);

    dlgSettingVisPath -> setModal(true);
    dlgSettingVisPath -> setPathToFile(pathToVis);
    dlgSettingVisPath -> setText(pathToVis);
    dlgSettingVisPath -> exec();

    pathToVis = dlgSettingVisPath->getPathToFile();
    delete(dlgSettingVisPath);
}


void MainWindow::on_actionProtocol_triggered()
{
    DialogProtocol *dlgProtocol = new DialogProtocol();

    dlgProtocol -> setModal(true);

    dlgProtocol->exec();

    delete(dlgProtocol);
}





void MainWindow::on_lineEditRecordLength_editingFinished()
{

}


void MainWindow::on_lineEditRecordLength_returnPressed()
{
    bool ok = false;
    const uint32_t value = ui->lineEditRecordLength->text().toInt(&ok);
    if (!ok || !digitizer.setRecLength(value))
        ui->lineEditRecordLength->setText(QString::number(digitizer.getRecLength()));
}

void MainWindow::on_lineEditThreshold_returnPressed()
{
    bool ok = false;
    const int32_t value = ui->lineEditThreshold->text().toInt(&ok);
    if (!ok || !digitizer.setTrigThreshold(- value * 26 + ZERO_TRESHOLD_STATE)){
        if (value > 1250 || value < -1250) return;
        QString thresText = (digitizer.getTrigThreshold() - ZERO_TRESHOLD_STATE) / 26 <= 0 ?
                                QString::number((ZERO_TRESHOLD_STATE - digitizer.getTrigThreshold()) / 26):
                                ("-" + QString::number((digitizer.getTrigThreshold() - ZERO_TRESHOLD_STATE) / 26));
        ui->lineEditThreshold->setText(thresText);
    }
    qDebug() << "Current threshold: " << (digitizer.getTrigThreshold());
}

void MainWindow::on_horizontalSlider_sliderMoved(int position)
{
    digitizer.setPostTrigSize(position);
    sendParams(0, digitizer.getGateWidth(),
               digitizer.getGatePosition());
    qDebug() << "Current Post trigger size: " << digitizer.getPostTrigSize();
}


void MainWindow::on_horizontalSliderGateWidth_sliderMoved(int position)
{
    digitizer.setGateWidth(position);
    sendParams(0, digitizer.getGateWidth(),
               digitizer.getGatePosition());
    qDebug() << "Current Gate Width: " << digitizer.getGateWidth();
}


void MainWindow::on_horizontalSliderGatePosition_sliderMoved(int position)
{
    digitizer.setGatePosition(position);
    qDebug() << "==================";
    qDebug() << "Current Gate Position: " << digitizer.getGatePosition();
    qDebug() << "Current Gate Width: " << digitizer.getGateWidth();
    qDebug() << "==================";
}

