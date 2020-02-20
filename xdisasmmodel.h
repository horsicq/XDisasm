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
#ifndef XDISASMMODEL_H
#define XDISASMMODEL_H

#include <QAbstractTableModel>
#include <QQueue>
#include "xdisasm.h"

class XDisasmModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum UD
    {
        UD_ADDRESS=0,
        UD_SIZE
    };

    enum DMCOLUMN
    {
        DMCOLUMN_ADDRESS=0,
        DMCOLUMN_OFFSET,
        DMCOLUMN_LABEL,
        DMCOLUMN_BYTES,
        DMCOLUMN_OPCODE
    };

    struct VEIW_RECORD
    {
        QString sAddress;
        QString sOffset;
        QString sLabel;
        QString sBytes;
        QString sOpcode;
    };

    struct SHOWOPTIONS
    {
        bool bShowLabels;
    };

    explicit XDisasmModel(QIODevice *pDevice, XDisasm::STATS *pStats,SHOWOPTIONS *pShowOptions,QObject *parent);
    ~XDisasmModel();
    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role=Qt::DisplayRole) const override;
    // Basic functionality:
    int rowCount(const QModelIndex &parent=QModelIndex()) const override;
    int columnCount(const QModelIndex &parent=QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role=Qt::DisplayRole) const override;
    VEIW_RECORD getViewRecord(int nRow);
    qint64 getPositionCount() const;
    qint64 positionToAddress(qint64 nPosition);
    qint64 addressToPosition(qint64 nAddress);
    XDisasm::STATS *getStats();
    void _beginResetModel();
    void _endResetModel();

private:
    QIODevice *pDevice;
    XDisasm::STATS *pStats;
    SHOWOPTIONS *pShowOptions;

    QQueue<qint64> quRecords;
    QMap<qint64,VEIW_RECORD> mapRecords;
    csh disasm_handle;
};

#endif // XDISASMMODEL_H
