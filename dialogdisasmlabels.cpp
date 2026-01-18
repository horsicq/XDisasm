// copyright (c) 2019-2026 hors<horsicq@gmail.com>
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
#include "dialogdisasmlabels.h"

#include "ui_dialogdisasmlabels.h"

DialogDisasmLabels::DialogDisasmLabels(QWidget *pParent, XDisasm::STATS *pDisasmStats) : QDialog(pParent), ui(new Ui::DialogDisasmLabels) {
    ui->setupUi(this);

    this->g_pDisasmStats = pDisasmStats;
    g_nAddress = 0;

    int nNumberOfLabels = pDisasmStats->mapLabelStrings.count();

    QStandardItemModel *pModel = new QStandardItemModel(nNumberOfLabels, 2, this);

    pModel->setHeaderData(0, Qt::Horizontal, tr("Name"));
    pModel->setHeaderData(1, Qt::Horizontal, tr("Address"));

    int i = 0;
    QMapIterator<qint64, QString> iMap(pDisasmStats->mapLabelStrings);
    while (iMap.hasNext()) {
        iMap.next();

        QString sName = iMap.value();
        qint64 nAddress = iMap.key();

        QStandardItem *pItemName = new QStandardItem;
        pItemName->setText(sName);
        pItemName->setData(nAddress);
        pModel->setItem(i, 0, pItemName);

        QStandardItem *pItemAddress = new QStandardItem;
        pItemAddress->setText(QString("0x%1").arg(nAddress, 8, 16, QChar('0')));  // TODO function in Binary
        pModel->setItem(i, 1, pItemAddress);

        i++;
    }

    ui->tableViewLabels->setModel(pModel);

    ui->tableViewLabels->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableViewLabels->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);

    ui->pushButtonGoTo->setEnabled(nNumberOfLabels);

    if (nNumberOfLabels) {
        ui->tableViewLabels->setCurrentIndex(pModel->index(0, 0));
    }
}

DialogDisasmLabels::~DialogDisasmLabels() {
    delete ui;
}

qint64 DialogDisasmLabels::getAddress() {
    return g_nAddress;
}

void DialogDisasmLabels::on_pushButtonClose_clicked() {
    done(QDialog::Rejected);
}

void DialogDisasmLabels::on_pushButtonGoTo_clicked() {
    goTo();
}

void DialogDisasmLabels::on_tableViewLabels_doubleClicked(const QModelIndex &index) {
    Q_UNUSED(index)

    goTo();
}

void DialogDisasmLabels::goTo() {
    QItemSelectionModel *pSelectionModel = ui->tableViewLabels->selectionModel();

    if (pSelectionModel) {
        QModelIndexList listIndexes = pSelectionModel->selectedRows(0);

        if (listIndexes.count()) {
            g_nAddress = listIndexes.at(0).data(Qt::UserRole + 1).toLongLong();

            done(QDialog::Accepted);
        }
    }
}
