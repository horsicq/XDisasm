#include "xdisasmwidget.h"
#include "ui_xdisasmwidget.h"

XDisasmWidget::XDisasmWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::XDisasmWidget)
{
    ui->setupUi(this);

    QFont font=ui->tableViewDisasm->font();
    font.setFamily("Courier");
    ui->tableViewDisasm->setFont(font);

    pModel=0;
}

void XDisasmWidget::setData(QIODevice *pDevice, XDisasm::STATS *pStats, XDisasmModel::SHOWOPTIONS *pOptions)
{
    pModel=new XDisasmModel(pDevice,pStats,pOptions,this);
    ui->tableViewDisasm->setModel(pModel);
    //    ui->tableViewDisasm->setColumnHidden(1, true);
}

void XDisasmWidget::goToAddress(qint64 nAddress)
{
    qint64 nPosition=pModel->addressToPosition(nAddress);
    qDebug("goToAddress: %x",nAddress);
    qDebug("position: %x",nPosition);
//    ui->tableViewDisasm->scrollTo(pModel->index(nPosition,0));

    ui->tableViewDisasm->verticalScrollBar()->setValue(nPosition);
}

void XDisasmWidget::clear()
{
    ui->tableViewDisasm->setModel(0);
}

XDisasmWidget::~XDisasmWidget()
{
    delete ui;
}

void XDisasmWidget::on_pushButtonLabels_clicked()
{
    if(pModel)
    {
        DialogDisasmLabels dialogDisasmLabels(this,pModel->getStats());

        dialogDisasmLabels.exec();
    }
}
