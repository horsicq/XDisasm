// copyright (c) 2019-2020 hors<horsicq@gmail.com>
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
#include "dialogasmsignature.h"

#include "ui_dialogasmsignature.h"

DialogAsmSignature::DialogAsmSignature(QWidget *pParent, QIODevice *pDevice,
                                       XDisasmModel *pModel, qint64 nAddress)
    : QDialog(pParent), ui(new Ui::DialogAsmSignature) {
    ui->setupUi(this);

    this->g_pDevice = pDevice;
    this->g_pModel = pModel;
    this->g_nAddress = nAddress;

    //    XOptions::setMonoFont(ui->tableWidgetSignature);
    XOptions::setMonoFont(ui->textEditSignature);

    QSignalBlocker signalBlocker1(ui->spinBoxCount);
    QSignalBlocker signalBlocke2r(ui->comboBoxMethod);

    ui->comboBoxMethod->addItem("", XDisasm::SM_NORMAL);
    ui->comboBoxMethod->addItem(tr("Relative virtual address"),
                                XDisasm::SM_RELATIVEADDRESS);

    reload();
}

DialogAsmSignature::~DialogAsmSignature() { delete ui; }

void DialogAsmSignature::reload() {
    XDisasm::SIGNATURE_OPTIONS options = {};

    options.csarch = g_pModel->getStats()->csarch;
    options.csmode = g_pModel->getStats()->csmode;
    options.memoryMap = g_pModel->getStats()->memoryMap;
    options.pDevice = g_pDevice;
    options.nCount = ui->spinBoxCount->value();
    options.sm = (XDisasm::SM)(ui->comboBoxMethod->currentData().toInt());

    g_listRecords = XDisasm::getSignature(&options, g_nAddress);

    int nSymbolWidth = XLineEditHEX::getSymbolWidth(ui->tableWidgetSignature);

    int nNumberOfRecords = g_listRecords.count();

    ui->tableWidgetSignature->clear();

    ui->tableWidgetSignature->setColumnCount(5);
    ui->tableWidgetSignature->setRowCount(nNumberOfRecords);

    QStringList listHeaders;
    listHeaders.append(tr("Address"));
    listHeaders.append(tr("Bytes"));
    listHeaders.append(tr("Opcode"));
    listHeaders.append(tr(""));
    listHeaders.append(tr(""));

    ui->tableWidgetSignature->setHorizontalHeaderLabels(listHeaders);

    for (int i = 0; i < nNumberOfRecords; i++) {
        ui->tableWidgetSignature->setItem(
            i, 0,
            new QTableWidgetItem(
                XBinary::valueToHex(g_pModel->getStats()->memoryMap.mode,
                                    g_listRecords.at(i).nAddress)));
        ui->tableWidgetSignature->setItem(
            i, 1,
            new QTableWidgetItem(g_listRecords.at(i).baOpcode.toHex().data()));

        if (!g_listRecords.at(i).bIsConst) {
            QPushButton *pUseSignatureButton = new QPushButton(this);
            pUseSignatureButton->setText(g_listRecords.at(i).sOpcode);
            pUseSignatureButton->setCheckable(true);
            connect(pUseSignatureButton, SIGNAL(clicked()), this,
                    SLOT(reloadSignature()));

            ui->tableWidgetSignature->setCellWidget(i, 2, pUseSignatureButton);

            if (g_listRecords.at(i).nDispSize) {
                QPushButton *pDispButton = new QPushButton(this);
                pDispButton->setText(QString("d"));
                pDispButton->setCheckable(true);
                pDispButton->setMaximumWidth(nSymbolWidth * 6);
                connect(pDispButton, SIGNAL(clicked()), this,
                        SLOT(reloadSignature()));

                ui->tableWidgetSignature->setCellWidget(i, 3, pDispButton);
            }

            if (g_listRecords.at(i).nImmSize) {
                QPushButton *pImmButton = new QPushButton(this);
                pImmButton->setText(QString("i"));
                pImmButton->setCheckable(true);
                pImmButton->setMaximumWidth(nSymbolWidth * 6);
                connect(pImmButton, SIGNAL(clicked()), this,
                        SLOT(reloadSignature()));

                ui->tableWidgetSignature->setCellWidget(i, 4, pImmButton);
            }
        } else {
            ui->tableWidgetSignature->setItem(
                i, 2, new QTableWidgetItem(g_listRecords.at(i).sOpcode));
        }
    }

    ui->tableWidgetSignature->setColumnWidth(0, nSymbolWidth * 12);
    ui->tableWidgetSignature->setColumnWidth(1, nSymbolWidth * 8);
    ui->tableWidgetSignature->setColumnWidth(2, nSymbolWidth * 20);
    ui->tableWidgetSignature->setColumnWidth(3, nSymbolWidth * 6);
    ui->tableWidgetSignature->setColumnWidth(4, nSymbolWidth * 6);

    ui->tableWidgetSignature->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Interactive);
    ui->tableWidgetSignature->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);
    ui->tableWidgetSignature->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::Interactive);
    ui->tableWidgetSignature->horizontalHeader()->setSectionResizeMode(
        3, QHeaderView::Interactive);
    ui->tableWidgetSignature->horizontalHeader()->setSectionResizeMode(
        4, QHeaderView::Interactive);

    reloadSignature();
}

void DialogAsmSignature::reloadSignature() {
    QString sText;

    QChar cWild = QChar('.');
    QString _sWild = ui->lineEditWildcard->text();

    if (_sWild.size()) {
        cWild = _sWild.at(0);
    }

    int nNumberOfRecords = g_listRecords.count();

    for (int i = 0; i < nNumberOfRecords; i++) {
        bool bUse = true;
        bool bDisp = true;
        bool bImm = true;

        QPushButton *pUseSignatureButton = dynamic_cast<QPushButton *>(
            ui->tableWidgetSignature->cellWidget(i, 2));
        QPushButton *pDispButton = dynamic_cast<QPushButton *>(
            ui->tableWidgetSignature->cellWidget(i, 3));
        QPushButton *pImmButton = dynamic_cast<QPushButton *>(
            ui->tableWidgetSignature->cellWidget(i, 4));

        if (pUseSignatureButton) {
            bUse = !(pUseSignatureButton->isChecked());
        }

        if (pDispButton) {
            pDispButton->setEnabled(bUse);
            bDisp = !(pDispButton->isChecked());
        }

        if (pImmButton) {
            pImmButton->setEnabled(bUse);
            bImm = !(pImmButton->isChecked());
        }

        int nSize = g_listRecords.at(i).baOpcode.size();

        QString sRecord;

        if (bUse) {
            sRecord = g_listRecords.at(i).baOpcode.toHex().data();

            if (!bDisp) {
                sRecord = replaceWild(sRecord, g_listRecords.at(i).nDispOffset,
                                      g_listRecords.at(i).nDispSize, cWild);
            }

            if (!bImm) {
                sRecord = replaceWild(sRecord, g_listRecords.at(i).nImmOffset,
                                      g_listRecords.at(i).nImmSize, cWild);
            }

            if (g_listRecords.at(i).bIsConst) {
                sRecord = replaceWild(sRecord, g_listRecords.at(i).nImmOffset,
                                      g_listRecords.at(i).nImmSize, QChar('$'));
            }
        } else {
            for (int j = 0; j < nSize; j++) {
                sRecord += cWild;
                sRecord += cWild;
            }
        }

        sText += sRecord;
    }

    if (ui->checkBoxUpper->isChecked()) {
        sText = sText.toUpper();
    } else {
        sText = sText.toLower();
    }

    if (ui->checkBoxSpaces->isChecked()) {
        QString _sText;

        int nSize = sText.size();

        for (int i = 0; i < nSize; i++) {
            _sText += sText.at(i);

            if ((i % 2) && (i != (nSize - 1))) {
                _sText += QChar(' ');
            }
        }

        sText = _sText;
    }

    ui->textEditSignature->setText(sText);
}

void DialogAsmSignature::on_pushButtonOK_clicked() { this->close(); }

void DialogAsmSignature::on_checkBoxSpaces_toggled(bool bChecked) {
    reloadSignature();
}

void DialogAsmSignature::on_checkBoxUpper_toggled(bool bChecked) {
    reloadSignature();
}

void DialogAsmSignature::on_lineEditWildcard_textChanged(const QString &sText) {
    reloadSignature();
}

void DialogAsmSignature::on_pushButtonCopy_clicked() {
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(ui->textEditSignature->toPlainText());
}

QString DialogAsmSignature::replaceWild(QString sString, qint32 nOffset,
                                        qint32 nSize, QChar cWild) {
    QString sResult = sString;
    QString sWild;

    sWild = sWild.fill(cWild, nSize * 2);

    sResult = sResult.replace(nOffset * 2, nSize * 2, sWild);

    return sResult;
}

void DialogAsmSignature::on_spinBoxCount_valueChanged(int nValue) {
    Q_UNUSED(nValue)

    reload();
}

void DialogAsmSignature::on_comboBoxMethod_currentIndexChanged(int nIndex) {
    Q_UNUSED(nIndex)

    reload();
}
