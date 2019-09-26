#ifndef FORMDISASM_H
#define FORMDISASM_H

#include <QWidget>
#include "xdisasmmodel.h"

namespace Ui {
class XDisasmWidget;
}

class XDisasmWidget : public QWidget
{
    Q_OBJECT

public:
    explicit XDisasmWidget(QWidget *parent = nullptr);
    void setData(QIODevice *pDevice,XDisasm::STATS *pStats,XDisasmModel::SHOWOPTIONS *pOptions);
    void clear();
    ~XDisasmWidget();

private:
    Ui::XDisasmWidget *ui;
};

#endif // FORMDISASM_H
