#include "dialogsignature.h"
#include "ui_dialogsignature.h"

DialogSignature::DialogSignature(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSignature)
{
    ui->setupUi(this);
}

DialogSignature::~DialogSignature()
{
    delete ui;
}
