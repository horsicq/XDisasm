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
#include "xdisasmmodel.h"

XDisasmModel::XDisasmModel(QIODevice *pDevice, XDisasm::STATS *pStats,
                           SHOWOPTIONS *pShowOptions, QObject *pParent)
    : QAbstractTableModel(pParent) {
    this->g_pDevice = pDevice;
    this->g_pStats = pStats;
    this->g_pShowOptions = pShowOptions;

    g_bDisasmInit = false;
}

XDisasmModel::~XDisasmModel() {
    if (g_bDisasmInit) {
        cs_close(&g_disasm_handle);
    }
}

QVariant XDisasmModel::headerData(int section, Qt::Orientation orientation,
                                  int nRole) const {
    QVariant result;

    if (orientation == Qt::Horizontal) {
        if (nRole == Qt::DisplayRole) {
            switch (section) {
                case DMCOLUMN_ADDRESS:
                    result = tr("Address");
                    break;
                case DMCOLUMN_OFFSET:
                    result = tr("Offset");
                    break;
                case DMCOLUMN_LABEL:
                    result = tr("Label");
                    break;
                case DMCOLUMN_BYTES:
                    result = tr("Bytes");
                    break;
                case DMCOLUMN_OPCODE:
                    result = tr("Opcode");
                    break;
            }
        }
    }

    return result;
}

int XDisasmModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }

    //    return XBinary::getTotalVirtualSize(&(pStats->listMM));
    //    return pStats->mapVB.count();
    return getPositionCount();
}

int XDisasmModel::columnCount(const QModelIndex &parent) const {
    int nResult = 5;  // TODO Def

    if (parent.isValid()) {
        nResult = 0;
    }

    return nResult;
}

QVariant XDisasmModel::data(const QModelIndex &index, int nRole) const {
    if (!index.isValid())  // TODO optimize
    {
        return QVariant();
    }

    QVariant result;

    if (nRole == Qt::DisplayRole) {
        XDisasmModel *_this = const_cast<XDisasmModel *>(this);

        VEIW_RECORD vrRecord = {};

        int nRow = index.row();

        if (_this->g_quRecords.contains(nRow)) {
            vrRecord = _this->g_mapRecords.value(nRow);
        } else {
            vrRecord = _this->getViewRecord(nRow);

            _this->g_quRecords.enqueue(nRow);
            _this->g_mapRecords.insert(nRow, vrRecord);

            if (_this->g_quRecords.count() > 1000)  // TODO const
            {
                qint64 _nPos = _this->g_quRecords.dequeue();
                _this->g_mapRecords.remove(_nPos);
            }
        }

        int nColumn = index.column();

        switch (nColumn) {
            case DMCOLUMN_ADDRESS:
                result = vrRecord.sAddress;
                break;
            case DMCOLUMN_OFFSET:
                result = vrRecord.sOffset;
                break;
            case DMCOLUMN_LABEL:
                result = vrRecord.sLabel;
                break;
            case DMCOLUMN_BYTES:
                result = vrRecord.sBytes;
                break;
            case DMCOLUMN_OPCODE:
                result = vrRecord.sOpcode;
                break;
        }
    } else if (nRole == Qt::UserRole + UD_ADDRESS) {
        XDisasmModel *_this = const_cast<XDisasmModel *>(this);

        int nRow = index.row();

        result = _this->positionToAddress(nRow);
    } else if (nRole == Qt::UserRole + UD_OFFSET) {
        XDisasmModel *_this = const_cast<XDisasmModel *>(this);

        int nRow = index.row();

        qint64 nAddress = _this->positionToAddress(nRow);

        result = XBinary::addressToOffset(&(g_pStats->memoryMap), nAddress);
    } else if (nRole == Qt::UserRole + UD_RELADDRESS) {
        XDisasmModel *_this = const_cast<XDisasmModel *>(this);

        int nRow = index.row();

        qint64 nAddress = _this->positionToAddress(nRow);

        result = XBinary::addressToRelAddress(&(g_pStats->memoryMap), nAddress);
    } else if (nRole == Qt::UserRole + UD_SIZE) {
        result = 1;

        XDisasmModel *_this = const_cast<XDisasmModel *>(this);

        int nRow = index.row();

        qint64 nAddress = _this->positionToAddress(nRow);

        if (g_pStats->mapVB.contains(nAddress)) {
            result = g_pStats->mapVB.value(nAddress).nSize;
        }
    }

    return result;
}

XDisasmModel::VEIW_RECORD XDisasmModel::getViewRecord(int nRow) {
    VEIW_RECORD result = {0};

    qint64 nAddress = positionToAddress(nRow);

    qint64 nOffset = XBinary::addressToOffset(&(g_pStats->memoryMap), nAddress);

    qint64 nSize = 1;

    // TODO check
    if (nAddress > 0xFFFFFFFF) {
        result.sAddress = XBinary::valueToHex((quint64)nAddress);
    } else {
        result.sAddress = XBinary::valueToHex((quint32)nAddress);
    }

    if (nOffset != -1) {
        result.sOffset = XBinary::valueToHex((quint32)nOffset);
    }

    if (g_pStats->mapVB.contains(nAddress)) {
        nSize = g_pStats->mapVB.value(nAddress).nSize;
    }

    QByteArray baData;

    if (nOffset != -1) {
        if (g_pDevice->seek(nOffset)) {
            baData = g_pDevice->read(nSize);
            result.sBytes = baData.toHex();
        }
    } else {
        result.sBytes = QString("byte 0x%1 dup(?)").arg(nSize, 0, 16);
    }

    if (g_pStats->mapVB.value(nAddress).type == XDisasm::VBT_OPCODE) {
        //        result.sOpcode=pStats->mapOpcodes.value(nAddress).sString;
        if (!g_bDisasmInit) {
            g_bDisasmInit = initDisasm();
        }

        result.sOpcode = XDisasm::getDisasmString(g_disasm_handle, nAddress,
                                                  baData.data(), baData.size());

        if (g_pShowOptions->bShowLabels) {
            if (g_pStats->mmapRefTo.contains(nAddress)) {
                QList<qint64> listRefs = g_pStats->mmapRefTo.values(nAddress);

                int nNumberOfRefs = listRefs.count();

                for (int i = 0; i < nNumberOfRefs; i++) {
                    QString sAddress =
                        QString("0x%1").arg(listRefs.at(i), 0, 16);
                    QString sRString =
                        g_pStats->mapLabelStrings.value(listRefs.at(i));
                    result.sOpcode = result.sOpcode.replace(sAddress, sRString);
                }
            }
        }
    }

    result.sLabel = g_pStats->mapLabelStrings.value(nAddress);

    return result;
}

qint64 XDisasmModel::getPositionCount() const { return g_pStats->nPositions; }

qint64 XDisasmModel::positionToAddress(qint64 nPosition) {
    qint64 nResult = 0;

    if (g_pStats->mapPositions.count()) {
        nResult = g_pStats->mapPositions.value(nPosition, -1);

        if (nResult == -1) {
            QMap<qint64, qint64>::const_iterator iter =
                g_pStats->mapPositions.lowerBound(nPosition);

            if (iter != g_pStats->mapPositions.end()) {
                qint64 nDelta = iter.key() - nPosition;

                nResult = iter.value() - nDelta;
            } else {
                qint64 nLastPosition = g_pStats->mapPositions.lastKey();
                qint64 nDelta = nPosition - nLastPosition;

                nResult = g_pStats->mapPositions.value(nLastPosition);
                nResult += g_pStats->mapVB.value(nResult).nSize;

                if (nDelta > 1) {
                    nResult += (nDelta - 1);
                }
            }
        }
    }

    return nResult;
}

qint64 XDisasmModel::addressToPosition(qint64 nAddress) {
    qint64 nResult = 0;

    if (g_pStats) {
        if (g_pStats->mapPositions.count()) {
            nResult = g_pStats->mapAddresses.value(nAddress, -1);

            if (nResult == -1) {
                QMap<qint64, qint64>::iterator iter =
                    g_pStats->mapAddresses.lowerBound(nAddress);

                if ((iter != g_pStats->mapAddresses.end()) &&
                    (iter != g_pStats->mapAddresses.begin())) {
                    iter--;
                    qint64 nPosition = iter.value();

                    qint64 nKeyAddress = g_pStats->mapAddresses.key(nPosition);

                    XDisasm::VIEW_BLOCK vb = g_pStats->mapVB.value(nKeyAddress);

                    if (vb.nSize) {
                        if ((vb.nAddress <= nAddress) &&
                            (nAddress < (vb.nAddress + vb.nSize))) {
                            nResult = nPosition;
                        }
                    }
                }
            }

            if (nResult == -1) {
                QMap<qint64, qint64>::const_iterator iterEnd =
                    g_pStats->mapAddresses.lowerBound(nAddress);

                if (iterEnd != g_pStats->mapAddresses.end()) {
                    qint64 nKeyAddress = iterEnd.key();
                    qint64 nPosition = iterEnd.value();

                    qint64 nDelta = nKeyAddress - nAddress;

                    nResult = nPosition - nDelta;
                } else {
                    qint64 nLastAddress = g_pStats->mapAddresses.lastKey();
                    nResult = g_pStats->mapAddresses.value(nLastAddress);
                    nResult++;

                    qint64 nDelta =
                        nAddress - (nLastAddress +
                                    g_pStats->mapVB.value(nLastAddress).nSize);

                    if (nDelta > 0) {
                        nResult += (nDelta);
                    }
                }
            }
        }

        if (nResult < 0)  // TODO Check
        {
            nResult = 0;
        }
    }

    return nResult;
}

qint64 XDisasmModel::offsetToPosition(qint64 nOffset) {
    qint64 nResult = 0;

    qint64 nAddress = XBinary::offsetToAddress(&(g_pStats->memoryMap), nOffset);

    if (nAddress != -1) {
        nResult = addressToPosition(nAddress);
    }

    return nResult;
}

qint64 XDisasmModel::relAddressToPosition(qint64 nRelAddress) {
    qint64 nResult = 0;

    qint64 nAddress =
        XBinary::relAddressToAddress(&(g_pStats->memoryMap), nRelAddress);

    if (nAddress != -1) {
        nResult = addressToPosition(nAddress);
    }

    return nResult;
}

XDisasm::STATS *XDisasmModel::getStats() { return g_pStats; }

void XDisasmModel::_beginResetModel() { beginResetModel(); }

void XDisasmModel::_endResetModel() {
    resetCache();
    endResetModel();
}

void XDisasmModel::resetCache() {
    g_mapRecords.clear();
    g_quRecords.clear();
}

bool XDisasmModel::initDisasm() {
    bool bResult = false;

    cs_err err = cs_open(g_pStats->csarch, g_pStats->csmode, &g_disasm_handle);
    if (!err) {
        cs_option(g_disasm_handle, CS_OPT_DETAIL, CS_OPT_ON);  // TODO Check
    }

    return bResult;
}
