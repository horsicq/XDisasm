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
#include "xdisasmwidget.h"

#include "ui_xdisasmwidget.h"

XDisasmWidget::XDisasmWidget(QWidget *pParent) : QWidget(pParent), ui(new Ui::XDisasmWidget) {
    ui->setupUi(this);

    XOptions::setMonoFont(ui->tableViewDisasm);

//    new QShortcut(QKeySequence(XShortcuts::GOTOENTRYPOINT), this, SLOT(_goToEntryPoint()));
//    new QShortcut(QKeySequence(XShortcuts::GOTOADDRESS), this, SLOT(_goToAddress()));
//    new QShortcut(QKeySequence(XShortcuts::GOTOOFFSET), this, SLOT(_goToOffset()));
//    new QShortcut(QKeySequence(XShortcuts::GOTORELADDRESS), this, SLOT(_goToRelAddress()));
//    new QShortcut(QKeySequence(XShortcuts::DUMPTOFILE), this, SLOT(_dumpToFile()));
//    new QShortcut(QKeySequence(XShortcuts::DISASM), this, SLOT(_disasm()));
//    new QShortcut(QKeySequence(XShortcuts::TODATA), this, SLOT(_toData()));
//    new QShortcut(QKeySequence(XShortcuts::HEXSIGNATURE), this, SLOT(_signature()));
//    new QShortcut(QKeySequence(XShortcuts::COPYADDRESS), this, SLOT(_copyAddress()));
//    new QShortcut(QKeySequence(XShortcuts::COPYOFFSET), this, SLOT(_copyOffset()));
//    new QShortcut(QKeySequence(XShortcuts::COPYRELADDRESS), this, SLOT(_copyRelAddress()));
//    new QShortcut(QKeySequence(XShortcuts::HEX), this, SLOT(_hex()));

    g_pShowOptions = 0;
    g_pDisasmOptions = 0;
    g_pModel = 0;

    g_showOptions = {};
    g_disasmOptions = {};
}

void XDisasmWidget::setData(QIODevice *pDevice, XDisasmModel::SHOWOPTIONS *pShowOptions, XDisasm::OPTIONS *pDisasmOptions, bool bAuto) {
    this->g_pDevice = pDevice;

    if (pShowOptions) {
        this->g_pShowOptions = pShowOptions;
    } else {
        this->g_pShowOptions = &g_showOptions;
    }

    if (pDisasmOptions) {
        this->g_pDisasmOptions = pDisasmOptions;
    } else {
        this->g_pDisasmOptions = &g_disasmOptions;
    }

    QSet<XBinary::FT> stFileType = XBinary::getFileTypes(pDevice);

    stFileType.remove(XBinary::FT_BINARY);
    stFileType.insert(XBinary::FT_BINARY16);
    stFileType.insert(XBinary::FT_BINARY32);
    stFileType.insert(XBinary::FT_BINARY64);
    stFileType.insert(XBinary::FT_COM);

    QList<XBinary::FT> listFileTypes = XBinary::_getFileTypeListFromSet(stFileType);

    XFormats::setFileTypeComboBox(this->g_pDisasmOptions->fileType, pDevice, ui->comboBoxType);

    if (bAuto) {
        analyze();
    }
}

void XDisasmWidget::analyze() {
    if (g_pDisasmOptions && g_pShowOptions) {
        XBinary::FT fileType = (XBinary::FT)ui->comboBoxType->currentData().toInt();
        g_pDisasmOptions->fileType = fileType;

        g_pDisasmOptions->stats = {};

        QItemSelectionModel *modelOld = ui->tableViewDisasm->selectionModel();
        ui->tableViewDisasm->setModel(0);

        process(g_pDevice, g_pDisasmOptions, -1, XDisasm::DM_DISASM);

        g_pModel = new XDisasmModel(g_pDevice, &(g_pDisasmOptions->stats), g_pShowOptions, this);

        ui->tableViewDisasm->setModel(g_pModel);
        delete modelOld;

        int nSymbolWidth = XLineEditHEX::getSymbolWidth(this);

        // TODO 16/32/64 width
        ui->tableViewDisasm->setColumnWidth(0, nSymbolWidth * 14);
        ui->tableViewDisasm->setColumnWidth(1, nSymbolWidth * 8);
        ui->tableViewDisasm->setColumnWidth(2, nSymbolWidth * 12);
        ui->tableViewDisasm->setColumnWidth(3, nSymbolWidth * 20);
        ui->tableViewDisasm->setColumnWidth(4, nSymbolWidth * 8);

        ui->tableViewDisasm->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
        ui->tableViewDisasm->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
        ui->tableViewDisasm->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
        ui->tableViewDisasm->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
        ui->tableViewDisasm->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

        ui->pushButtonOverlay->setEnabled(g_pDisasmOptions->stats.bIsOverlayPresent);

        goToAddress(g_pDisasmOptions->stats.nEntryPointAddress);
    }
}

void XDisasmWidget::goToAddress(qint64 nAddress) {
    if (g_pModel) {
        qint64 nPosition = g_pModel->addressToPosition(nAddress);

        _goToPosition(nPosition);
    }
}

void XDisasmWidget::goToOffset(qint64 nOffset) {
    if (g_pModel) {
        qint64 nPosition = g_pModel->offsetToPosition(nOffset);

        _goToPosition(nPosition);
    }
}

void XDisasmWidget::goToRelAddress(qint64 nRelAddress) {
    if (g_pModel) {
        qint64 nPosition = g_pModel->relAddressToPosition(nRelAddress);

        _goToPosition(nPosition);
    }
}

void XDisasmWidget::goToDisasmAddress(qint64 nAddress) {
    if (!g_pDisasmOptions->stats.bInit) {
        process(g_pDevice, g_pDisasmOptions, nAddress, XDisasm::DM_DISASM);
    }

    goToAddress(nAddress);
}

void XDisasmWidget::goToEntryPoint() {
    if (!g_pDisasmOptions->stats.bInit) {
        process(g_pDevice, g_pDisasmOptions, -1, XDisasm::DM_DISASM);
    }

    goToAddress(g_pDisasmOptions->stats.nEntryPointAddress);
}

void XDisasmWidget::disasm(qint64 nAddress) {
    process(g_pDevice, g_pDisasmOptions, nAddress, XDisasm::DM_DISASM);

    goToAddress(nAddress);
}

void XDisasmWidget::toData(qint64 nAddress, qint64 nSize) {
    process(g_pDevice, g_pDisasmOptions, nAddress, XDisasm::DM_TODATA);

    goToAddress(nAddress);
}

void XDisasmWidget::signature(qint64 nAddress, qint64 nSize) {
    if (g_pDisasmOptions->stats.mapRecords.value(nAddress).type == XDisasm::RECORD_TYPE_OPCODE) {
        DialogAsmSignature ds(this, g_pDevice, g_pModel, nAddress);

        ds.exec();
    } else {
        qint64 nOffset = XBinary::addressToOffset(&(g_pDisasmOptions->stats.memoryMap), nAddress);

        if (nOffset != -1) {
            DialogHexSignature dhs(this, g_pDevice, nOffset, nSize);

            dhs.exec();
        }
    }
}

void XDisasmWidget::hex(qint64 nOffset) {
    QHexView::OPTIONS hexOptions = {};

    XBinary binary(g_pDevice);

    hexOptions.memoryMap = binary.getMemoryMap();
    hexOptions.sBackupFileName = g_sBackupFileName;
    hexOptions.nStartAddress = nOffset;
    hexOptions.nStartSelectionAddress = nOffset;
    hexOptions.nSizeOfSelection = 1;

    DialogHex dialogHex(this, g_pDevice, &hexOptions);

    connect(&dialogHex, SIGNAL(editState(bool)), this, SLOT(setEdited(bool)));

    dialogHex.exec();
}

void XDisasmWidget::clear() {
    ui->tableViewDisasm->setModel(0);
}

XDisasmWidget::~XDisasmWidget() {
    delete ui;
}

void XDisasmWidget::process(QIODevice *pDevice, XDisasm::OPTIONS *pOptions, qint64 nStartAddress, XDisasm::DM dm) {
    DialogDisasmProcess ddp(this);

    connect(&ddp, SIGNAL(errorMessage(QString)), this, SLOT(errorMessage(QString)));

    ddp.setData(pDevice, pOptions, nStartAddress, dm);
    ddp.exec();

    if (g_pModel) {
        g_pModel->resetCache();
    }

    //    if(pModel)
    //    {
    //        pModel->_beginResetModel();

    //        DialogDisasmProcess ddp(this);

    //        connect(&ddp,SIGNAL(errorMessage(QString)),this,SLOT(errorMessage(QString)));

    //        ddp.setData(pDevice,pOptions,nStartAddress,dm);
    //        ddp.exec();

    //        pModel->_endResetModel();
    //    }
}

XDisasm::STATS *XDisasmWidget::getDisasmStats() {
    return &(g_pDisasmOptions->stats);
}

void XDisasmWidget::setBackupFileName(QString sBackupFileName) {
    this->g_sBackupFileName = sBackupFileName;
}

void XDisasmWidget::on_pushButtonLabels_clicked() {
    if (g_pModel) {
        DialogDisasmLabels dialogDisasmLabels(this, g_pModel->getStats());

        if (dialogDisasmLabels.exec() == QDialog::Accepted) {
            goToAddress(dialogDisasmLabels.getAddress());
        }
    }
}

void XDisasmWidget::on_tableViewDisasm_customContextMenuRequested(const QPoint &pos) {
    if (g_pModel) {
        SELECTION_STAT selectionStat = getSelectionStat();

        QMenu contextMenu(this);

        QMenu goToMenu(tr("Go to"), this);

        QAction actionGoToEntryPoint(tr("Entry point"), this);
//        actionGoToEntryPoint.setShortcut(QKeySequence(XShortcuts::GOTOENTRYPOINT));
        connect(&actionGoToEntryPoint, SIGNAL(triggered()), this, SLOT(_goToEntryPoint()));

        QAction actionGoToAddress(tr("Virtual address"), this);
 //       actionGoToAddress.setShortcut(QKeySequence(XShortcuts::GOTOADDRESS));
        connect(&actionGoToAddress, SIGNAL(triggered()), this, SLOT(_goToAddress()));

        QAction actionGoToRelAddress(tr("Relative virtual address"), this);
 //       actionGoToRelAddress.setShortcut(QKeySequence(XShortcuts::GOTORELADDRESS));
        connect(&actionGoToRelAddress, SIGNAL(triggered()), this, SLOT(_goToRelAddress()));

        QAction actionGoToOffset(tr("File offset"), this);
//        actionGoToOffset.setShortcut(QKeySequence(XShortcuts::GOTOOFFSET));
        connect(&actionGoToOffset, SIGNAL(triggered()), this, SLOT(_goToOffset()));

        goToMenu.addAction(&actionGoToEntryPoint);
        goToMenu.addAction(&actionGoToAddress);
        goToMenu.addAction(&actionGoToRelAddress);
        goToMenu.addAction(&actionGoToOffset);

        contextMenu.addMenu(&goToMenu);

        QMenu copyMenu(tr("Copy"), this);

        QAction actionCopyAddress(tr("Virtual address"), this);
//        actionCopyAddress.setShortcut(QKeySequence(XShortcuts::COPYADDRESS));
        connect(&actionCopyAddress, SIGNAL(triggered()), this, SLOT(_copyAddress()));

        QAction actionCopyRelAddress(tr("Relative virtual address"), this);
//        actionCopyRelAddress.setShortcut(QKeySequence(XShortcuts::COPYRELADDRESS));
        connect(&actionCopyRelAddress, SIGNAL(triggered()), this, SLOT(_copyRelAddress()));

        QAction actionCopyOffset(tr("File offset"), this);
//        actionCopyOffset.setShortcut(QKeySequence(XShortcuts::COPYOFFSET));
        connect(&actionCopyOffset, SIGNAL(triggered()), this, SLOT(_copyOffset()));

        copyMenu.addAction(&actionCopyAddress);
        copyMenu.addAction(&actionCopyRelAddress);
        copyMenu.addAction(&actionCopyOffset);

        contextMenu.addMenu(&copyMenu);

        QAction actionHex(QString("Hex"), this);
//        actionHex.setShortcut(QKeySequence(XShortcuts::HEX));
        connect(&actionHex, SIGNAL(triggered()), this, SLOT(_hex()));

        QAction actionSignature(tr("Signature"), this);
//        actionSignature.setShortcut(QKeySequence(XShortcuts::HEXSIGNATURE));
        connect(&actionSignature, SIGNAL(triggered()), this, SLOT(_signature()));

        QAction actionDump(tr("Dump to file"), this);
//        actionDump.setShortcut(QKeySequence(XShortcuts::DUMPTOFILE));
        connect(&actionDump, SIGNAL(triggered()), this, SLOT(_dumpToFile()));

        QAction actionDisasm(tr("Disasm"), this);
//        actionDisasm.setShortcut(QKeySequence(XShortcuts::DISASM));
        connect(&actionDisasm, SIGNAL(triggered()), this, SLOT(_disasm()));

        QAction actionToData(tr("To data"), this);
//        actionToData.setShortcut(QKeySequence(XShortcuts::TODATA));
        connect(&actionToData, SIGNAL(triggered()), this, SLOT(_toData()));

        contextMenu.addAction(&actionHex);
        contextMenu.addAction(&actionSignature);

        if ((selectionStat.nSize) && XBinary::isSolidAddressRange(&(g_pModel->getStats()->memoryMap), selectionStat.nAddress, selectionStat.nSize)) {
            contextMenu.addAction(&actionDump);
        }

        if (selectionStat.nCount == 1) {
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

void XDisasmWidget::_goToAddress() {
    if (g_pModel) {
        DialogGoToAddress da(this, &(g_pModel->getStats()->memoryMap), DialogGoToAddress::TYPE_ADDRESS);
        if (da.exec() == QDialog::Accepted) {
            goToAddress(da.getValue());
        }
    }
}

void XDisasmWidget::_goToRelAddress() {
    if (g_pModel) {
        DialogGoToAddress da(this, &(g_pModel->getStats()->memoryMap), DialogGoToAddress::TYPE_RELVIRTUALADDRESS);
        if (da.exec() == QDialog::Accepted) {
            goToRelAddress(da.getValue());
        }
    }
}

void XDisasmWidget::_goToOffset() {
    if (g_pModel) {
        DialogGoToAddress da(this, &(g_pModel->getStats()->memoryMap), DialogGoToAddress::TYPE_OFFSET);
        if (da.exec() == QDialog::Accepted) {
            goToOffset(da.getValue());
        }
    }
}

void XDisasmWidget::_goToEntryPoint() {
    goToEntryPoint();
}

void XDisasmWidget::_copyAddress() {
    if (g_pModel) {
        SELECTION_STAT selectionStat = getSelectionStat();

        if (selectionStat.nSize) {
            QApplication::clipboard()->setText(QString("%1").arg(selectionStat.nAddress, 0, 16));
        }
    }
}

void XDisasmWidget::_copyOffset() {
    if (g_pModel) {
        SELECTION_STAT selectionStat = getSelectionStat();

        if (selectionStat.nSize) {
            QApplication::clipboard()->setText(QString("%1").arg(selectionStat.nOffset, 0, 16));
        }
    }
}

void XDisasmWidget::_copyRelAddress() {
    if (g_pModel) {
        SELECTION_STAT selectionStat = getSelectionStat();

        if (selectionStat.nSize) {
            QApplication::clipboard()->setText(QString("%1").arg(selectionStat.nRelAddress, 0, 16));
        }
    }
}

void XDisasmWidget::_dumpToFile() {
    if (g_pModel) {
        SELECTION_STAT selectionStat = getSelectionStat();

        if (selectionStat.nSize) {
            QString sFilter;
            sFilter += QString("%1 (*.bin)").arg(tr("Raw data"));
            QString sSaveFileName = "Result";  // TODO default directory / TODO getDumpName
            QString sFileName = QFileDialog::getSaveFileName(this, tr("Save dump"), sSaveFileName, sFilter);

            qint64 nOffset = XBinary::addressToOffset(&(g_pModel->getStats()->memoryMap), selectionStat.nAddress);

            if (!sFileName.isEmpty()) {
                DialogDumpProcess dd(this, g_pDevice, nOffset, selectionStat.nSize, sFileName, DumpProcess::DT_OFFSET);

                dd.exec();
            }
        }
    }
}

void XDisasmWidget::_disasm() {
    if (g_pModel) {
        SELECTION_STAT selectionStat = getSelectionStat();

        if (selectionStat.nSize) {
            disasm(selectionStat.nAddress);
        }
    }
}

void XDisasmWidget::_toData() {
    if (g_pModel) {
        SELECTION_STAT selectionStat = getSelectionStat();

        if (selectionStat.nSize) {
            toData(selectionStat.nAddress, selectionStat.nSize);
        }
    }
}

void XDisasmWidget::_signature() {
    if (g_pModel) {
        SELECTION_STAT selectionStat = getSelectionStat();

        if (selectionStat.nSize) {
            signature(selectionStat.nAddress, selectionStat.nSize);
        }
    }
}

void XDisasmWidget::_hex() {
    if (g_pModel) {
        SELECTION_STAT selectionStat = getSelectionStat();

        hex(selectionStat.nOffset);
    }
}

XDisasmWidget::SELECTION_STAT XDisasmWidget::getSelectionStat() {
    SELECTION_STAT result = {};
    result.nAddress = -1;

    QModelIndexList il = ui->tableViewDisasm->selectionModel()->selectedRows();

    result.nCount = il.count();

    if (result.nCount) {
        result.nAddress = il.at(0).data(Qt::UserRole + XDisasmModel::UD_ADDRESS).toLongLong();
        result.nOffset = il.at(0).data(Qt::UserRole + XDisasmModel::UD_OFFSET).toLongLong();
        result.nRelAddress = il.at(0).data(Qt::UserRole + XDisasmModel::UD_RELADDRESS).toLongLong();

        qint64 nLastElementAddress = il.at(result.nCount - 1).data(Qt::UserRole + XDisasmModel::UD_ADDRESS).toLongLong();
        qint64 nLastElementSize = il.at(result.nCount - 1).data(Qt::UserRole + XDisasmModel::UD_SIZE).toLongLong();

        result.nSize = (nLastElementAddress + nLastElementSize) - result.nAddress;
    }

    return result;
}

void XDisasmWidget::on_pushButtonAnalyze_clicked() {
    analyze();
}

void XDisasmWidget::_goToPosition(qint32 nPosition) {
    if (ui->tableViewDisasm->verticalScrollBar()->maximum() == 0) {
        ui->tableViewDisasm->verticalScrollBar()->setMaximum(nPosition);  // Hack
    }

    ui->tableViewDisasm->verticalScrollBar()->setValue(nPosition);

    ui->tableViewDisasm->setCurrentIndex(ui->tableViewDisasm->model()->index(nPosition, 0));
}

void XDisasmWidget::on_pushButtonOverlay_clicked() {
    hex(g_pDisasmOptions->stats.nOverlayOffset);
}

void XDisasmWidget::setEdited(bool bState) {
    if (bState) {
        analyze();
    }
}

void XDisasmWidget::on_pushButtonHex_clicked() {
    hex(0);
}

void XDisasmWidget::errorMessage(QString sText) {
    QMessageBox::critical(this, tr("Error"), sText);
}
