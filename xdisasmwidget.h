#ifndef FORMDISASM_H
#define FORMDISASM_H

#include <QWidget>
#include <QScrollBar>
#include "xdisasmmodel.h"
#include "dialogdisasmlabels.h"

namespace Ui {
class XDisasmWidget;
}

class XDisasmWidget : public QWidget
{
    Q_OBJECT

public:
    explicit XDisasmWidget(QWidget *parent = nullptr);
    void setData(QIODevice *pDevice,XDisasm::STATS *pStats,XDisasmModel::SHOWOPTIONS *pOptions);
    void goToAddress(qint64 nAddress);
    void clear();
    ~XDisasmWidget();

private slots:
    void on_pushButtonLabels_clicked();

private:
    Ui::XDisasmWidget *ui;
    XDisasmModel *pModel;
};

#endif // FORMDISASM_H
