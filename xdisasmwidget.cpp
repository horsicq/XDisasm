// copyright (c) 2019-2020 hors<horsicq@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
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

    pShowOptions=0;
    pDisasmOptions=0;
    pModel=0;
}

void XDisasmWidget::setData(XDisasmModel::SHOWOPTIONS *pShowOptions, XDisasm::OPTIONS *pDisasmOptions, bool bAuto)
{
    this->pShowOptions=pShowOptions;
    this->pDisasmOptions=pDisasmOptions;

    if(bAuto)
    {
        analize();
    }
}

void XDisasmWidget::analize()
{
    if(pDisasmOptions&&pShowOptions)
    {
        pDisasmOptions->stats={};
        process(pDisasmOptions,-1,XDisasm::DM_DISASM);

        pModel=new XDisasmModel(pDisasmOptions->pDevice,&(pDisasmOptions->stats),pShowOptions,this);

        QItemSelectionModel *modelOld=ui->tableViewDisasm->selectionModel();
        ui->tableViewDisasm->setModel(pModel);
        delete modelOld;
    }
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
    if(!pDisasmOptions->stats.bInit)
    {
        process(pDisasmOptions,nAddress,XDisasm::DM_DISASM);
    }

    goToAddress(nAddress);
}

void XDisasmWidget::goToEntryPoint()
{
    if(!pDisasmOptions->stats.bInit)
    {
        process(pDisasmOptions,-1,XDisasm::DM_DISASM);
    }

    goToAddress(pDisasmOptions->stats.nEntryPointAddress);
}

void XDisasmWidget::disasm(qint64 nAddress)
{
    process(pDisasmOptions,nAddress,XDisasm::DM_DISASM);
}

void XDisasmWidget::toData(qint64 nAddress, qint64 nSize)
{
    process(pDisasmOptions,nAddress,XDisasm::DM_TODATA);
}

void XDisasmWidget::clear()
{
    ui->tableViewDisasm->setModel(0);
}

XDisasmWidget::~XDisasmWidget()
{    
    delete ui;
}

void XDisasmWidget::process(XDisasm::OPTIONS *pOptions, qint64 nStartAddress, XDisasm::DM dm)
{
    if(pModel)
    {
        pModel->_beginResetModel();

        DialogDisasmProcess ddp(this);

        ddp.setData(pOptions,nStartAddress,dm);
        ddp.exec();

        pModel->_endResetModel();
    }
}

XDisasm::STATS *XDisasmWidget::getDisasmStats()
{
    return &(pDisasmOptions->stats);
}

void XDisasmWidget::on_pushButtonLabels_clicked()
{
    if(pModel)
    {
        DialogDisasmLabels dialogDisasmLabels(this,pModel->getStats());

        if(dialogDisasmLabels.exec()==QDialog::Accepted)
        {
            goToAddress(dialogDisasmLabels.getAddress());
        }
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

        if((selectionStat.nSize)&&XBinary::isSolidAddressRange(&(pModel->getStats()->memoryMap),selectionStat.nAddress,selectionStat.nSize))
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
        // TODO add Label
        // TODO rename label
        // TODO remove label mb TODO custom label and Disasm label
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

            qint64 nOffset=XBinary::addressToOffset(&(pModel->getStats()->memoryMap),selectionStat.nAddress);

            if(!sFileName.isEmpty())
            {
                DialogDumpProcess dd(this,pDisasmOptions->pDevice,nOffset,selectionStat.nSize,sFileName,DumpProcess::DT_OFFSET);

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

void XDisasmWidget::on_pushButtonAnalize_clicked()
{
    analize();
}
