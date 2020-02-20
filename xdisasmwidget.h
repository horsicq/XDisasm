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
    void setData(QIODevice *pDevice, XDisasmModel::SHOWOPTIONS *pOptions);
    void goToAddress(qint64 nAddress);
    void goToDisasmAddress(qint64 nAddress);
    void goToEntryPoint();
    void clear();
    void waitTillModelLoaded(qint64 nAddress);
    ~XDisasmWidget();
    void process(qint64 nAddress);

private slots:
    void on_pushButtonLabels_clicked();
    void on_tableViewDisasm_customContextMenuRequested(const QPoint &pos);
    void _goToAddress();
    void _dumpToFile();
    SELECTION_STAT getSelectionStat();

private:
    Ui::XDisasmWidget *ui;
    QIODevice *pDevice;
    XDisasmModel::SHOWOPTIONS *pOptions;
    XDisasmModel *pModel;
    XDisasm::STATS disasmStats;
};

#endif // FORMDISASM_H
