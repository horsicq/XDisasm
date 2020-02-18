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

    new QShortcut(QKeySequence(XShortcuts::GOTOADDRESS),this,SLOT(_goToAddress()));

    pModel=0;
}

void XDisasmWidget::setData(QIODevice *pDevice, XDisasm::STATS *pStats, XDisasmModel::SHOWOPTIONS *pOptions)
{
    if(pModel)
    {
        delete pModel;
    }

    pModel=new XDisasmModel(pDevice,pStats,pOptions,this);
    ui->tableViewDisasm->setModel(pModel);
    //    ui->tableViewDisasm->setColumnHidden(1, true);
}

void XDisasmWidget::goToAddress(qint64 nAddress)
{
    if(pModel)
    {
        qint64 nPosition=pModel->addressToPosition(nAddress);

        ui->tableViewDisasm->verticalScrollBar()->setValue(nPosition);
    }
}

void XDisasmWidget::clear()
{
    ui->tableViewDisasm->setModel(0);
}

void XDisasmWidget::waitTillModelLoaded(qint64 nAddress)
{
    if(pModel)
    {
        qint64 nPosition=pModel->addressToPosition(nAddress);
        int nMaximum=0;

        while(true)
        {
            nMaximum=ui->tableViewDisasm->verticalScrollBar()->maximum();

            if(nPosition<=nMaximum)
            {
                break;
            }

            QThread::msleep(100);
        }
    }
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

void XDisasmWidget::on_tableViewDisasm_customContextMenuRequested(const QPoint &pos)
{
    if(pModel)
    {
        QMenu contextMenu(this);

        QAction actionGoToAddress(tr("Go to Address"),this);
        actionGoToAddress.setShortcut(QKeySequence(XShortcuts::GOTOADDRESS));
        connect(&actionGoToAddress,SIGNAL(triggered()),this,SLOT(_goToAddress()));
        contextMenu.addAction(&actionGoToAddress);

        QAction actionDump(tr("Dump to File"),this);
        actionDump.setShortcut(QKeySequence(XShortcuts::DUMPTOFILE));
        connect(&actionDump,SIGNAL(triggered()),this,SLOT(_dumpToFile()));
        contextMenu.addAction(&actionDump);

        contextMenu.exec(ui->tableViewDisasm->viewport()->mapToGlobal(pos));

        // TODO Disasm
        // TODO code -> data
        // TODO data -> group
    }
}

void XDisasmWidget::_goToAddress()
{
    if(pModel)
    {
        DialogGoToAddress da(this,&(pModel->getStats()->memoryMap));
        if(da.exec()==QDialog::Accepted)
        {
            goToAddress(da.getAddress());
        }
    }
}

void XDisasmWidget::_dumpToFile()
{
    if(pModel)
    {
        XBinary::ADDRESSSIZE addressSize=getSelectedAddressSize();

        if(addressSize.nSize)
        {
            // TODO
            qDebug("_dumpToFile");
        }
    }
}

XBinary::ADDRESSSIZE XDisasmWidget::getSelectedAddressSize()
{
    XBinary::ADDRESSSIZE result={};

    QModelIndexList il=ui->tableViewDisasm->selectionModel()->selectedRows();

    int nCount=il.count();

    if(nCount)
    {
        result.nAddress=il.at(0).data(Qt::UserRole+XDisasmModel::UD_ADDRESS).toLongLong();
        result.nSize=(il.at(nCount-1).data(Qt::UserRole+XDisasmModel::UD_ADDRESS).toLongLong()+il.at(nCount-1).data(Qt::UserRole+XDisasmModel::UD_SIZE).toLongLong())-result.nAddress;
    }

    return result;
}
