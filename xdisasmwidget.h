// copyright (c) 2019-2025 hors<horsicq@gmail.com>
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
#ifndef FORMDISASM_H
#define FORMDISASM_H

#include <QClipboard>
#include <QMenu>
#include <QScrollBar>
#include <QThread>
#include <QWidget>

#include "dialogasmsignature.h"
#include "dialogdisasmlabels.h"
#include "dialogdisasmprocess.h"
#include "dialogdumpprocess.h"
#include "dialoggotoaddress.h"
#include "dialoghex.h"
#include "dialoghexsignature.h"
#include "xdisasmmodel.h"
#include "xlineedithex.h"
#include "xoptions.h"
#include "xshortcuts.h"

namespace Ui {
class XDisasmWidget;
}

class XDisasmWidget : public QWidget {
    Q_OBJECT

    struct SELECTION_STAT {
        qint64 nAddress;
        qint64 nOffset;
        qint64 nRelAddress;
        qint64 nSize;
        qint32 nCount;
    };

public:
    explicit XDisasmWidget(QWidget *pParent = nullptr);
    void setData(QIODevice *pDevice, XDisasmModel::SHOWOPTIONS *pShowOptions = 0, XDisasm::OPTIONS *pDisasmOptions = 0, bool bAuto = true);
    void analyze();
    void goToAddress(qint64 nAddress);
    void goToOffset(qint64 nOffset);
    void goToRelAddress(qint64 nRelAddress);
    void goToDisasmAddress(qint64 nAddress);
    void goToEntryPoint();
    void disasm(qint64 nAddress);
    void toData(qint64 nAddress, qint64 nSize);
    void signature(qint64 nAddress, qint64 nSize);
    void hex(qint64 nOffset);
    void clear();
    ~XDisasmWidget();
    void process(QIODevice *pDevice, XDisasm::OPTIONS *pOptions, qint64 nStartAddress, XDisasm::DM dm);
    XDisasm::STATS *getDisasmStats();
    void setBackupFileName(QString sBackupFileName);

private slots:
    void on_pushButtonLabels_clicked();
    void on_tableViewDisasm_customContextMenuRequested(const QPoint &pos);
    void _goToAddress();
    void _goToRelAddress();
    void _goToOffset();
    void _goToEntryPoint();
    void _copyAddress();
    void _copyOffset();
    void _copyRelAddress();
    void _dumpToFile();
    void _disasm();
    void _toData();
    void _signature();
    void _hex();
    SELECTION_STAT getSelectionStat();
    void on_pushButtonAnalyze_clicked();
    void _goToPosition(qint32 nPosition);
    void on_pushButtonOverlay_clicked();
    void setEdited(bool bState);
    void on_pushButtonHex_clicked();
    void errorMessage(QString sText);

private:
    Ui::XDisasmWidget *ui;
    QIODevice *g_pDevice;
    XDisasmModel::SHOWOPTIONS *g_pShowOptions;
    XDisasm::OPTIONS *g_pDisasmOptions;
    XDisasmModel *g_pModel;
    XDisasmModel::SHOWOPTIONS g_showOptions;
    XDisasm::OPTIONS g_disasmOptions;
    QString g_sBackupFileName;  // TODO save backup
};

#endif  // FORMDISASM_H
