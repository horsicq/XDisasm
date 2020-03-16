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
#include "dialogdisasmprocess.h"
#include "ui_dialogdisasmprocess.h"

DialogDisasmProcess::DialogDisasmProcess(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogDisasmProcess)
{
    ui->setupUi(this);

    pDisasm=new XDisasm;
    pThread=new QThread;

    pDisasm->moveToThread(pThread);

    connect(pDisasm, SIGNAL(processFinished()), this, SLOT(close()));
    connect(pThread, SIGNAL(started()), pDisasm, SLOT(process()));

    pTimer=new QTimer(this);
    connect(pTimer,SIGNAL(timeout()),this,SLOT(timerSlot()));
}

DialogDisasmProcess::~DialogDisasmProcess()
{
    pTimer->stop();
    delete pTimer;

    pDisasm->stop();

    pThread->quit();
    pThread->wait();

    delete ui;

    delete pThread;
    delete pDisasm;
}

void DialogDisasmProcess::setData(QIODevice *pDevice, bool bIsImage, XDisasm::MODE mode, qint64 nStartAddress, XDisasm::STATS *pDisasmStats,qint64 nImageBase, XDisasm::DM dm)
{
    pDisasm->setData(pDevice,bIsImage,mode,nStartAddress,pDisasmStats,nImageBase,dm);

    pThread->start();
    pTimer->start(1000);
}

void DialogDisasmProcess::on_pushButtonCancel_clicked()
{
    pDisasm->stop();
}

void DialogDisasmProcess::timerSlot()
{
    // TODO more info
    ui->labelOpcodes->setText(QString("%1").arg(pDisasm->getStats()->mapRecords.count()));
    ui->labelCalls->setText(QString("%1").arg(pDisasm->getStats()->stCalls.count()));
    ui->labelJumps->setText(QString("%1").arg(pDisasm->getStats()->stJumps.count()));
    ui->labelRefFrom->setText(QString("%1").arg(pDisasm->getStats()->mmapRefFrom.count()));
    ui->labelRefTo->setText(QString("%1").arg(pDisasm->getStats()->mmapRefTo.count()));
}
