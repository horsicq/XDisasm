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
#ifndef FORMDISASM_H
#define FORMDISASM_H

#include <QWidget>
#include <QScrollBar>
#include <QThread>
#include <QMenu>
#include "xdisasmmodel.h"
#include "dialogdisasmlabels.h"
#include "xshortcuts.h"
#include "dialoggotoaddress.h"
#include "dialogdisasmprocess.h"
#include "dialogdumpprocess.h"
#include "xlineedithex.h"
#include "dialogsignature.h"

namespace Ui {
class XDisasmWidget;
}

class XDisasmWidget : public QWidget
{
    Q_OBJECT

    struct SELECTION_STAT
    {
        qint64 nAddress;
        qint64 nSize;
        qint32 nCount;
    };

public:
    explicit XDisasmWidget(QWidget *parent=nullptr);
    void setData(QIODevice *pDevice,XDisasmModel::SHOWOPTIONS *pShowOptions=0,XDisasm::OPTIONS *pDisasmOptions=0,bool bAuto=true);
    void analyze();
    void goToAddress(qint64 nAddress);
    void goToOffset(qint64 nOffset);
    void goToRelAddress(qint64 nRelAddress);
    void goToDisasmAddress(qint64 nAddress);
    void goToEntryPoint();
    void disasm(qint64 nAddress);
    void toData(qint64 nAddress,qint64 nSize);
    void signature(qint64 nAddress);
    void clear();
    ~XDisasmWidget();
    void process(QIODevice *pDevice, XDisasm::OPTIONS *pOptions, qint64 nStartAddress, XDisasm::DM dm);
    XDisasm::STATS *getDisasmStats();

private slots:
    void on_pushButtonLabels_clicked();
    void on_tableViewDisasm_customContextMenuRequested(const QPoint &pos);
    void _goToAddress();
    void _goToRelAddress();
    void _goToOffset();
    void _dumpToFile();
    void _disasm();
    void _toData();
    void _signature();
    SELECTION_STAT getSelectionStat();
    void on_pushButtonAnalyze_clicked();
    void _goToPosition(qint32 nPosition);
    void on_pushButtonEntryPoint_clicked();

private:
    Ui::XDisasmWidget *ui;
    QIODevice *pDevice;
    XDisasmModel::SHOWOPTIONS *pShowOptions;
    XDisasm::OPTIONS *pDisasmOptions;
    XDisasmModel *pModel;
    XDisasmModel::SHOWOPTIONS __showOptions;
    XDisasm::OPTIONS __disasmOptions;
};

#endif // FORMDISASM_H
