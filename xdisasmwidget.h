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
    explicit XDisasmWidget(QWidget *parent = nullptr);
    void setData(QIODevice *pDevice, XDisasmModel::SHOWOPTIONS *pOptions,XDisasm::STATS *pDisasmStats);
    void goToAddress(qint64 nAddress);
    void goToDisasmAddress(qint64 nAddress);
    void goToEntryPoint();
    void disasm(qint64 nAddress);
    void toData(qint64 nAddress,qint64 nSize);
    void clear();
    ~XDisasmWidget();
    void process(qint64 nAddress, XDisasm::DM dm);
    XDisasm::STATS *getDisasmStats();

private slots:
    void on_pushButtonLabels_clicked();
    void on_tableViewDisasm_customContextMenuRequested(const QPoint &pos);
    void _goToAddress();
    void _dumpToFile();
    void _disasm();
    void _toData();
    SELECTION_STAT getSelectionStat();

private:
    Ui::XDisasmWidget *ui;
    QIODevice *pDevice;
    XDisasmModel::SHOWOPTIONS *pOptions;
    XDisasmModel *pModel;
    XDisasm::STATS *pDisasmStats;
};

#endif // FORMDISASM_H
