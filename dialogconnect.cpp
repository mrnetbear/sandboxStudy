#include "dialogconnect.h"
#include "ui_dialogconnect.h"

DialogConnect::DialogConnect(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogConnect)
{
    this->setFixedSize(561, 414);
    ui->setupUi(this);
}

DialogConnect::~DialogConnect()
{
    delete ui;
}

void DialogConnect::on_closeButton_clicked()
{
    this->close();
}

