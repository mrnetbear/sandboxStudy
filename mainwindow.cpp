#include "mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setFixedSize(800, 600);
    //setCentralWidget(rootWidget);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::loadConfig(){
    if(!QFile::exists(pathToConfig)) {
        QMessageBox::warning(this, "Ошибка",
            "Не удалось найти файл конфигурации. Проверьте путь и повторите попытку.");
        return 1;
    }
    QFile confFile;
    confFile.setFileName(pathToConfig);

    if (!confFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        QMessageBox::warning(this, "Ошибка",
            "Не удалось открыть файл конфигурации.");
        return 1;
    }
    while(!confFile.atEnd()){
        qDebug() << confFile.readLine();
    }
    qDebug() << "Done!";
    confFile.close();
    return 0;
}

/*void MainWindow::on_ButtonConnect_clicked()
{
    DialogConnect *dlgConnect = new DialogConnect(this);

    for(size_t b=0; b<MAXNB; b++){
        digitizer.setErrorCode(CAEN_DGTZ_OpenDigitizer2(CAEN_DGTZ_USB,0,0,0,&digitizer.handle[b]));
    }
    dlgConnect -> setModal(true);

    dlgConnect->exec();

    delete(dlgConnect);
}*/

void MainWindow::on_ButtonConnect_clicked()
{
    // Создаем диалог подключения
    DialogConnect *dlgConnect = new DialogConnect(this);
    dlgConnect->setModal(true);

    // Подключаем сигналы от digitizer
    connect(&digitizer, &DigitizerOperation::progressUpdated,
            [this](QString msg) {
                qDebug() << msg;
                // Можно добавить вывод в статус-бар или лог
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
                ui->labelEventsCount->setText(QString("Events: %1").arg(digitizer.getTotalEventsCount()));
            });

    // Открываем и настраиваем оцифровщик
    if (!digitizer.openDigitizer(0, 0, 0)) {
        delete dlgConnect;
        return;
    }

    if (!digitizer.configureDigitizer()) {
        digitizer.closeDigitizer();
        delete dlgConnect;
        return;
    }

    // Показываем диалог подключения
    dlgConnect->exec();
    delete dlgConnect;

    // Запускаем acquisition
    if (!digitizer.startAcquisition()) {
        return;
    }

    // Обновляем UI - показываем статус
    ui->ButtonConnect->setEnabled(false);
    ui->ButtonDisconnect->setEnabled(true);
    ui->ButtonStartStop->setText("Stop");
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
        QMessageBox::warning(this, "Ошибка",
                             "Не удалось запустить ROOT приложение. Проверьте путь и повторите попытку.");
    } else {
        qDebug() << "ROOT приложение запущено:" << pathToVis;
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


void MainWindow::on_ButtonDisconnect_clicked()
{
    digitizer.stopAcquisition();
    digitizer.closeDigitizer();

    ui->ButtonConnect->setEnabled(true);
    ui->ButtonDisconnect->setEnabled(false);
    ui->ButtonStartStop->setText("Start");
}


void MainWindow::on_ButtonStartStop_clicked()
{
    if (digitizer.getAcquiringStatus()) {
        digitizer.stopAcquisition();
        ui->ButtonStartStop->setText("Start");
    } else {
        digitizer.startAcquisition();
        ui->ButtonStartStop->setText("Stop");
    }
}

