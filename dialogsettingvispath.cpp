#include "dialogsettingvispath.h"
#include "qdebug.h"

DialogSettingVisPath::DialogSettingVisPath(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogSettingVisPath)
{
    this->setFixedSize(370, 91);
    //ui->lineEditPath->setText(pathToFile);
    //qDebug() << pathToFile;
    ui->setupUi(this);
}

DialogSettingVisPath::~DialogSettingVisPath()
{
    delete ui;
}
void DialogSettingVisPath::setPathToFile(const QString &a){
    pathToFile = a;
}

QString DialogSettingVisPath::getPathToFile(){
    return pathToFile;
}

void DialogSettingVisPath::on_okButton_clicked()
{
    QString filename = ui->lineEditPath->text();
    pathToFile = filename;
    qDebug() << pathToFile;
    ui->lineEditPath->setText(pathToFile);
    this->close();
}

void DialogSettingVisPath::setText(const QString& x)
{
    pathToFile = x;
    ui->lineEditPath->setText(pathToFile);
}

void DialogSettingVisPath::on_cancelButton_clicked()
{
    this->close();
}


void DialogSettingVisPath::on_toolButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Открыть файл", QDir::homePath(), "Все файлы (*.*)");
    this -> setText(fileName);
}

