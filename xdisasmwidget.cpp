#include "xdisasmwidget.h"
#include "ui_xdisasmwidget.h"

XDisasmWidget::XDisasmWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::XDisasmWidget)
{
    ui->setupUi(this);

    QFont font=ui->tableViewDisasm->font();
    font.setFamily("Courier"); // TODO
    ui->tableViewDisasm->setFont(font);

    new QShortcut(QKeySequence(XShortcuts::GOTOADDRESS),    this,SLOT(_goToAddress()));
    new QShortcut(QKeySequence(XShortcuts::DUMPTOFILE),     this,SLOT(_dumpToFile()));
    new QShortcut(QKeySequence(XShortcuts::DISASM),         this,SLOT(_disasm()));
    new QShortcut(QKeySequence(XShortcuts::TODATA),         this,SLOT(_toData()));

    pDisasmStats=0;
    pModel=0;
}

void XDisasmWidget::setData(QIODevice *pDevice, XDisasmModel::SHOWOPTIONS *pOptions, XDisasm::STATS *pDisasmStats)
{
    this->pDevice=pDevice;
    this->pOptions=pOptions;
    this->pDisasmStats=pDisasmStats;

    pModel=new XDisasmModel(pDevice,pDisasmStats,pOptions,this);

    QItemSelectionModel *modelOld=ui->tableViewDisasm->selectionModel();
    ui->tableViewDisasm->setModel(pModel);
    delete modelOld;
}

void XDisasmWidget::goToAddress(qint64 nAddress)
{
    if(pModel)
    {
        qint64 nPosition=pModel->addressToPosition(nAddress);

        if(ui->tableViewDisasm->verticalScrollBar()->maximum()==0)
        {
            ui->tableViewDisasm->verticalScrollBar()->setMaximum(nPosition); // Hack
        }

        ui->tableViewDisasm->verticalScrollBar()->setValue(nPosition);
    }
}

void XDisasmWidget::goToDisasmAddress(qint64 nAddress)
{
    if(!pDisasmStats->bInit)
    {
        process(nAddress,XDisasm::DM_DISASM);
    }

    goToAddress(nAddress);
}

void XDisasmWidget::goToEntryPoint()
{
    if(!pDisasmStats->bInit)
    {
        process(-1,XDisasm::DM_DISASM);
    }

    goToAddress(pDisasmStats->nEntryPointAddress);
}

void XDisasmWidget::disasm(qint64 nAddress)
{
    process(nAddress,XDisasm::DM_DISASM);
}

void XDisasmWidget::toData(qint64 nAddress, qint64 nSize)
{
    process(nAddress,XDisasm::DM_TODATA);
}

void XDisasmWidget::clear()
{
    ui->tableViewDisasm->setModel(0);
}

XDisasmWidget::~XDisasmWidget()
{    
    delete ui;
}

void XDisasmWidget::process(qint64 nAddress,XDisasm::DM dm)
{
    if(pModel)
    {
        pModel->_beginResetModel();

        DialogDisasmProcess ddp(this);

        ddp.setData(pDevice,false,XDisasm::MODE_UNKNOWN,nAddress,pDisasmStats,dm);
        ddp.exec();

        pModel->_endResetModel();
    }
}

XDisasm::STATS *XDisasmWidget::getDisasmStats()
{
    return pDisasmStats;
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
        SELECTION_STAT selectionStat=getSelectionStat();

        QMenu contextMenu(this);

        QAction actionGoToAddress(tr("Go to Address"),this);
        actionGoToAddress.setShortcut(QKeySequence(XShortcuts::GOTOADDRESS));
        connect(&actionGoToAddress,SIGNAL(triggered()),this,SLOT(_goToAddress()));

        QAction actionDump(tr("Dump to File"),this);
        actionDump.setShortcut(QKeySequence(XShortcuts::DUMPTOFILE));
        connect(&actionDump,SIGNAL(triggered()),this,SLOT(_dumpToFile()));

        QAction actionDisasm(tr("Disasm"),this);
        actionDisasm.setShortcut(QKeySequence(XShortcuts::DISASM));
        connect(&actionDisasm,SIGNAL(triggered()),this,SLOT(_disasm()));

        QAction actionToData(tr("To Data"),this);
        actionToData.setShortcut(QKeySequence(XShortcuts::TODATA));
        connect(&actionToData,SIGNAL(triggered()),this,SLOT(_toData()));

        contextMenu.addAction(&actionGoToAddress);

        if(selectionStat.nSize)
        {
            contextMenu.addAction(&actionDump);
        }

        if(selectionStat.nCount==1)
        {
            contextMenu.addAction(&actionDisasm);
            contextMenu.addAction(&actionToData);
        }

        contextMenu.exec(ui->tableViewDisasm->viewport()->mapToGlobal(pos));

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
        SELECTION_STAT selectionStat=getSelectionStat();

        if(selectionStat.nSize)
        {
            QString sFilter;
            sFilter+=QString("%1 (*.bin)").arg(tr("Raw data"));
            QString sSaveFileName="Result"; // TODO default directory
            QString sFileName=QFileDialog::getSaveFileName(this,tr("Save dump"),sSaveFileName,sFilter);

            if(!sFileName.isEmpty())
            {
                DialogDumpProcess dd(this,pDevice,selectionStat.nAddress,selectionStat.nSize,sFileName,DumpProcess::DT_ADDRESS);

                dd.exec();
            }
        }
    }
}

void XDisasmWidget::_disasm()
{
    if(pModel)
    {
        SELECTION_STAT selectionStat=getSelectionStat();

        if(selectionStat.nSize)
        {
            disasm(selectionStat.nAddress);
        }
    }
}

void XDisasmWidget::_toData()
{
    if(pModel)
    {
        SELECTION_STAT selectionStat=getSelectionStat();

        if(selectionStat.nSize)
        {
            toData(selectionStat.nAddress,selectionStat.nSize);
        }
    }
}

XDisasmWidget::SELECTION_STAT XDisasmWidget::getSelectionStat()
{
    SELECTION_STAT result={};
    result.nAddress=-1;

    QModelIndexList il=ui->tableViewDisasm->selectionModel()->selectedRows();

    result.nCount=il.count();

    if(result.nCount)
    {
        result.nAddress=il.at(0).data(Qt::UserRole+XDisasmModel::UD_ADDRESS).toLongLong();

        qint64 nLastElementAddress=il.at(result.nCount-1).data(Qt::UserRole+XDisasmModel::UD_ADDRESS).toLongLong();
        qint64 nLastElementSize=il.at(result.nCount-1).data(Qt::UserRole+XDisasmModel::UD_SIZE).toLongLong();

        result.nSize=(nLastElementAddress+nLastElementSize)-result.nAddress;
    }

    return result;
}
