#include "xdisasmwidget.h"
#include "ui_xdisasmwidget.h"

XDisasmWidget::XDisasmWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::XDisasmWidget)
{
    ui->setupUi(this);
}

void XDisasmWidget::setData(QIODevice *pDevice, XDisasm::STATS *pStats, XDisasmModel::SHOWOPTIONS *pOptions)
{
    XDisasmModel *pModel=new XDisasmModel(pDevice,pStats,pOptions,this);
    ui->tableViewDisasm->setModel(pModel);
    //    ui->tableViewDisasm->setColumnHidden(1, true);
}

void XDisasmWidget::clear()
{
    ui->tableViewDisasm->setModel(0);
}

XDisasmWidget::~XDisasmWidget()
{
    delete ui;
}
