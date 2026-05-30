#include "dialogprotocol.h"
#include "ui_dialogprotocol.h"

DialogProtocol::DialogProtocol(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogProtocol)
{
    this->setFixedSize(438,342);
    ui->setupUi(this);
}

DialogProtocol::~DialogProtocol()
{
    delete ui;
}

void DialogProtocol::on_okButton_clicked()
{
    this->close();
}

