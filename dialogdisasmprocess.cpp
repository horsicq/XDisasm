// copyright (c) 2019-2023 hors<horsicq@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

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

DialogDisasmProcess::DialogDisasmProcess(QWidget *pParent) : QDialog(pParent), ui(new Ui::DialogDisasmProcess) {
    ui->setupUi(this);

    g_pDisasm = new XDisasm;
    g_pThread = new QThread;

    g_pDisasm->moveToThread(g_pThread);

    connect(g_pDisasm, SIGNAL(processFinished()), this, SLOT(close()));
    connect(g_pThread, SIGNAL(started()), g_pDisasm, SLOT(process()));
    connect(g_pDisasm, SIGNAL(errorMessage(QString)), this, SIGNAL(errorMessage(QString)));

    g_pTimer = new QTimer(this);
    connect(g_pTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));
}

DialogDisasmProcess::~DialogDisasmProcess() {
    g_pTimer->stop();
    delete g_pTimer;

    g_pDisasm->stop();

    g_pThread->quit();
    g_pThread->wait();

    delete ui;

    delete g_pThread;
    delete g_pDisasm;
}

void DialogDisasmProcess::setData(QIODevice *pDevice, XDisasm::OPTIONS *pOptions, qint64 nStartAddress, XDisasm::DM dm) {
    g_pDisasm->setData(pDevice, pOptions, nStartAddress, dm);

    g_pThread->start();
    g_pTimer->start(1000);
}

void DialogDisasmProcess::on_pushButtonCancel_clicked() {
    g_pDisasm->stop();
}

void DialogDisasmProcess::timerSlot() {
    // TODO more info
    ui->lineEditOpcodes->setText(QString("%1").arg(g_pDisasm->getStats()->mapRecords.count()));
    ui->lineEditCalls->setText(QString("%1").arg(g_pDisasm->getStats()->stCalls.count()));
    ui->lineEditJumps->setText(QString("%1").arg(g_pDisasm->getStats()->stJumps.count()));
    ui->lineEditRefFrom->setText(QString("%1").arg(g_pDisasm->getStats()->mmapRefFrom.count()));
    ui->lineEditRefTo->setText(QString("%1").arg(g_pDisasm->getStats()->mmapRefTo.count()));

    ui->lineEditDataLabels->setText(QString("%1").arg(g_pDisasm->getStats()->mmapDataLabels.count()));
    ui->lineEditVB->setText(QString("%1").arg(g_pDisasm->getStats()->mapVB.count()));
    ui->lineEditStrings->setText(QString("%1").arg(g_pDisasm->getStats()->mapLabelStrings.count()));
    ui->lineEditPositions->setText(QString("%1").arg(g_pDisasm->getStats()->mapPositions.count()));
    ui->lineEditAddresses->setText(QString("%1").arg(g_pDisasm->getStats()->mapAddresses.count()));
}
