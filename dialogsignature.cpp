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
#include "dialogsignature.h"
#include "ui_dialogsignature.h"

DialogSignature::DialogSignature(QWidget *parent, QIODevice *pDevice, XDisasmModel *pModel, qint64 nAddress) :
    QDialog(parent),
    ui(new Ui::DialogSignature)
{
    ui->setupUi(this);

    this->pDevice=pDevice;
    this->pModel=pModel;
    this->nAddress=nAddress;

//    XOptions::setMonoFont(ui->tableWidgetSignature);
    XOptions::setMonoFont(ui->textEditSignature);

    QSignalBlocker signalBlocker1(ui->spinBoxCount);
    QSignalBlocker signalBlocke2r(ui->comboBoxMethod);

    ui->comboBoxMethod->addItem("",XDisasm::SM_NORMAL);
    ui->comboBoxMethod->addItem(tr("Relative virtual address"),XDisasm::SM_RELATIVEADDRESS);

    reload();
}

DialogSignature::~DialogSignature()
{
    delete ui;
}

void DialogSignature::reload()
{
    XDisasm::SIGNATURE_OPTIONS options={};

    options.csarch=pModel->getStats()->csarch;
    options.csmode=pModel->getStats()->csmode;
    options.memoryMap=pModel->getStats()->memoryMap;
    options.pDevice=pDevice;
    options.nCount=ui->spinBoxCount->value();
    options.sm=(XDisasm::SM)(ui->comboBoxMethod->currentData().toInt());

    listRecords=XDisasm::getSignature(&options,nAddress);

    int nSymbolWidth=XLineEditHEX::getSymbolWidth(ui->tableWidgetSignature);

    int nCount=listRecords.count();

    ui->tableWidgetSignature->clear();

    ui->tableWidgetSignature->setColumnCount(5);
    ui->tableWidgetSignature->setRowCount(nCount);

    QStringList listHeaders;
    listHeaders.append(tr("Address"));
    listHeaders.append(tr("Bytes"));
    listHeaders.append(tr("Opcode"));
    listHeaders.append(tr(""));
    listHeaders.append(tr(""));

    ui->tableWidgetSignature->setHorizontalHeaderLabels(listHeaders);

    for(int i=0;i<nCount;i++)
    {
        ui->tableWidgetSignature->setItem(i,0,new QTableWidgetItem(XBinary::valueToHex(pModel->getStats()->memoryMap.mode,listRecords.at(i).nAddress)));
        ui->tableWidgetSignature->setItem(i,1,new QTableWidgetItem(listRecords.at(i).baOpcode.toHex().data()));

        if(!listRecords.at(i).bIsConst)
        {
            QPushButton *pUseSignatureButton=new QPushButton(this);
            pUseSignatureButton->setText(listRecords.at(i).sOpcode);
            pUseSignatureButton->setCheckable(true);
            connect(pUseSignatureButton,SIGNAL(clicked()),this,SLOT(reloadSignature()));

            ui->tableWidgetSignature->setCellWidget(i,2,pUseSignatureButton);

            if(listRecords.at(i).nDispSize)
            {
                QPushButton *pDispButton=new QPushButton(this);
                pDispButton->setText(QString("d"));
                pDispButton->setCheckable(true);
                pDispButton->setMaximumWidth(nSymbolWidth*6);
                connect(pDispButton,SIGNAL(clicked()),this,SLOT(reloadSignature()));

                ui->tableWidgetSignature->setCellWidget(i,3,pDispButton);
            }

            if(listRecords.at(i).nImmSize)
            {
                QPushButton *pImmButton=new QPushButton(this);
                pImmButton->setText(QString("i"));
                pImmButton->setCheckable(true);
                pImmButton->setMaximumWidth(nSymbolWidth*6);
                connect(pImmButton,SIGNAL(clicked()),this,SLOT(reloadSignature()));

                ui->tableWidgetSignature->setCellWidget(i,4,pImmButton);
            }
        }
        else
        {
            ui->tableWidgetSignature->setItem(i,2,new QTableWidgetItem(listRecords.at(i).sOpcode));
        }
    }

    ui->tableWidgetSignature->setColumnWidth(0,nSymbolWidth*12);
    ui->tableWidgetSignature->setColumnWidth(1,nSymbolWidth*8);
    ui->tableWidgetSignature->setColumnWidth(2,nSymbolWidth*20);
    ui->tableWidgetSignature->setColumnWidth(3,nSymbolWidth*6);
    ui->tableWidgetSignature->setColumnWidth(4,nSymbolWidth*6);

    ui->tableWidgetSignature->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Interactive);
    ui->tableWidgetSignature->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    ui->tableWidgetSignature->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Interactive);
    ui->tableWidgetSignature->horizontalHeader()->setSectionResizeMode(3,QHeaderView::Interactive);
    ui->tableWidgetSignature->horizontalHeader()->setSectionResizeMode(4,QHeaderView::Interactive);

    reloadSignature();
}

void DialogSignature::reloadSignature()
{
    QString sText;

    QChar cWild=QChar('.');
    QString _sWild=ui->lineEditWildcard->text();

    if(_sWild.size())
    {
        cWild=_sWild.at(0);
    }

    int nCount=listRecords.count();

    for(int i=0;i<nCount;i++)
    {
        bool bUse=true;
        bool bDisp=true;
        bool bImm=true;

        QPushButton *pUseSignatureButton=dynamic_cast<QPushButton *>(ui->tableWidgetSignature->cellWidget(i,2));
        QPushButton *pDispButton=dynamic_cast<QPushButton *>(ui->tableWidgetSignature->cellWidget(i,3));
        QPushButton *pImmButton=dynamic_cast<QPushButton *>(ui->tableWidgetSignature->cellWidget(i,4));

        if(pUseSignatureButton)
        {
            bUse=!(pUseSignatureButton->isChecked());
        }

        if(pDispButton)
        {
            pDispButton->setEnabled(bUse);
            bDisp=!(pDispButton->isChecked());
        }

        if(pImmButton)
        {
            pImmButton->setEnabled(bUse);
            bImm=!(pImmButton->isChecked());
        }

        int nSize=listRecords.at(i).baOpcode.size();

        QString sRecord;

        if(bUse)
        {
            sRecord=listRecords.at(i).baOpcode.toHex().data();

            if(!bDisp)
            {
                sRecord=replaceWild(sRecord,listRecords.at(i).nDispOffset,listRecords.at(i).nDispSize,cWild);
            }

            if(!bImm)
            {
                sRecord=replaceWild(sRecord,listRecords.at(i).nImmOffset,listRecords.at(i).nImmSize,cWild);
            }

            if(listRecords.at(i).bIsConst)
            {
                sRecord=replaceWild(sRecord,listRecords.at(i).nImmOffset,listRecords.at(i).nImmSize,QChar('$'));
            }
        }
        else
        {
            for(int j=0;j<nSize;j++)
            {
                sRecord+=cWild;
                sRecord+=cWild;
            }
        }

        sText+=sRecord;
    }

    if(ui->checkBoxUpper->isChecked())
    {
        sText=sText.toUpper();
    }
    else
    {
        sText=sText.toLower();
    }

    if(ui->checkBoxSpaces->isChecked())
    {
        QString _sText;

        int nSize=sText.size();

        for(int i=0;i<nSize;i++)
        {
            _sText+=sText.at(i);

            if((i%2)&&(i!=(nSize-1)))
            {
                _sText+=QChar(' ');
            }
        }

        sText=_sText;
    }

    ui->textEditSignature->setText(sText);
}

void DialogSignature::on_pushButtonOK_clicked()
{
    this->close();
}

void DialogSignature::on_checkBoxSpaces_toggled(bool bChecked)
{
    reloadSignature();
}

void DialogSignature::on_checkBoxUpper_toggled(bool bChecked)
{
    reloadSignature();
}

void DialogSignature::on_lineEditWildcard_textChanged(const QString &sText)
{
    reloadSignature();
}

void DialogSignature::on_pushButtonCopy_clicked()
{
    QClipboard *clipboard=QApplication::clipboard();
    clipboard->setText(ui->textEditSignature->toPlainText());
}

QString DialogSignature::replaceWild(QString sString, qint32 nOffset, qint32 nSize, QChar cWild)
{
    QString sResult=sString;
    QString sWild;

    sWild=sWild.fill(cWild,nSize*2);

    sResult=sResult.replace(nOffset*2,nSize*2,sWild);

    return sResult;
}

void DialogSignature::on_spinBoxCount_valueChanged(int nValue)
{
    Q_UNUSED(nValue)

    reload();
}

void DialogSignature::on_comboBoxMethod_currentIndexChanged(int nIndex)
{
    Q_UNUSED(nIndex)

    reload();
}
