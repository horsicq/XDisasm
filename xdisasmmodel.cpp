// copyright (c) 2019 hors<horsicq@gmail.com>
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
#include "xdisasmmodel.h"

XDisasmModel::XDisasmModel(QIODevice *pDevice, XDisasm::STATS *pStats,SHOWOPTIONS *pShowOptions, QObject *parent)
    : QAbstractTableModel(parent)
{
    this->pDevice=pDevice;
    this->pStats=pStats;
    this->pShowOptions=pShowOptions;
}

QVariant XDisasmModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QVariant result;

    if(orientation==Qt::Horizontal)
    {
        if(role==Qt::DisplayRole)
        {
            switch(section)
            {
                case DMCOLUMN_ADDRESS:  result=tr("Address");       break;
                case DMCOLUMN_OFFSET:   result=tr("Offset");        break;
                case DMCOLUMN_LABEL:    result=tr("Label");         break;
                case DMCOLUMN_BYTES:    result=tr("Bytes");         break;
                case DMCOLUMN_OPCODE:   result=tr("Opcode");        break;
            }
        }
    }

    return result;
}

int XDisasmModel::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid())
    {
        return 0;
    }

//    return XBinary::getTotalVirtualSize(&(pStats->listMM));
//    return pStats->mapVB.count();
    return getPositionCount();
}

int XDisasmModel::columnCount(const QModelIndex &parent) const
{
    int nResult=5; // TODO Def

    if(parent.isValid())
    {
        nResult=0;
    }

    return nResult;
}

QVariant XDisasmModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) // TODO optimize
    {
        return QVariant();
    }

    QVariant result;

    if(role==Qt::DisplayRole)
    {
        XDisasmModel* _this=const_cast<XDisasmModel *>(this);

        VEIW_RECORD vrRecord={};

        int nRow=index.row();

        if(_this->quRecords.contains(nRow))
        {
            vrRecord=_this->mapRecords.value(nRow);
        }
        else
        {
            vrRecord=_this->getViewRecord(nRow);

            _this->quRecords.enqueue(nRow);
            _this->mapRecords.insert(nRow,vrRecord);

            if(_this->quRecords.count()>1000) // TODO const
            {
                qint64 _nPos=_this->quRecords.dequeue();
                _this->mapRecords.remove(_nPos);
            }
        }

        int nColumn=index.column();

        switch(nColumn)
        {
            case DMCOLUMN_ADDRESS:      result=vrRecord.sAddress;       break;
            case DMCOLUMN_OFFSET:       result=vrRecord.sOffset;        break;
            case DMCOLUMN_LABEL:        result=vrRecord.sLabel;         break;
            case DMCOLUMN_BYTES:        result=vrRecord.sBytes;         break;
            case DMCOLUMN_OPCODE:       result=vrRecord.sOpcode;        break;
        }
    }

    return result;
}

XDisasmModel::VEIW_RECORD XDisasmModel::getViewRecord(int nRow)
{
    VEIW_RECORD result;

    qint64 nAddress=positionToAddress(nRow);

    qint64 nOffset=XBinary::addressToOffset(&(pStats->listMM),nAddress);

    qint64 nSize=1;

    if(nAddress>0xFFFFFFFF)
    {
        result.sAddress=XBinary::valueToHex((quint64)nAddress);
    }
    else
    {
        result.sAddress=XBinary::valueToHex((quint32)nAddress);
    }

    if(nOffset!=-1)
    {
        result.sOffset=XBinary::valueToHex((quint32)nOffset);
    }

    if(pStats->mapVB.contains(nAddress))
    {
        nSize=pStats->mapVB.value(nAddress).nSize;
    }

    if(pStats->mapVB.value(nAddress).type==XDisasm::VBT_OPCODE)
    {
        result.sOpcode=pStats->mapOpcodes.value(nAddress).sString;

        if(pShowOptions->bShowLabels)
        {
            if(pStats->mmapRefTo.contains(nAddress))
            {
                QList<qint64> listRefs=pStats->mmapRefTo.values(nAddress);

                for(int i=0;i<listRefs.count();i++)
                {
                    QString sAddress=QString("0x%1").arg(listRefs.at(i),0,16);
                    QString sRString=pStats->mapLabelStrings.value(listRefs.at(i));
                    result.sOpcode=result.sOpcode.replace(sAddress,sRString);
                }
            }
        }
    }

    if(nOffset!=-1)
    {
        if(pDevice->seek(nOffset))
        {
            QByteArray baData=pDevice->read(nSize);
            result.sBytes=baData.toHex();
        }
    }

    result.sLabel=pStats->mapLabelStrings.value(nAddress);

    return result;
}

qint64 XDisasmModel::getPositionCount() const
{
    return pStats->nPositions;
}

qint64 XDisasmModel::positionToAddress(qint64 nPosition)
{
    qint64 nResult=0;

    if(pStats->mapPositions.count())
    {
        nResult=pStats->mapPositions.value(nPosition,-1);

        if(nResult==-1)
        {
            QMap<qint64,qint64>::const_iterator iter=pStats->mapPositions.lowerBound(nPosition);

            if(iter!=pStats->mapPositions.end())
            {
                qint64 nDelta=iter.key()-nPosition;

                nResult=iter.value()-nDelta;
            }
            else
            {
                qint64 nLastPosition=pStats->mapPositions.lastKey();
                qint64 nDelta=nPosition-nLastPosition;

                nResult=pStats->mapPositions.value(nLastPosition);
                nResult+=pStats->mapVB.value(nResult).nSize;

                if(nDelta>1)
                {
                    nResult+=(nDelta-1);
                }
            }
        }
    }

    return nResult;
}

qint64 XDisasmModel::addressToPosition(qint64 nAddress)
{
    qint64 nResult=0;

    if(pStats)
    {
        if(pStats->mapPositions.count())
        {
            nResult=pStats->mapAddresses.value(nAddress,-1);

            if(nResult==-1)
            {
                QMap<qint64,qint64>::const_iterator iter=pStats->mapAddresses.lowerBound(nAddress);

                if(iter!=pStats->mapAddresses.end())
                {
                    qint64 nDelta=iter.key()-nAddress;

                    nResult=iter.value()-nDelta;
                }
                else
                {
                    qint64 nLastAddress=pStats->mapAddresses.lastKey();
                    nResult=pStats->mapAddresses.value(nLastAddress);
                    nResult++;

                    qint64 nDelta=nAddress-(nLastAddress+pStats->mapVB.value(nLastAddress).nSize);

                    if(nDelta>0)
                    {
                        nResult+=(nDelta);
                    }
                }
            }
        }

        if(nResult<0) // TODO Check
        {
            nResult=0;
        }
    }

    return nResult;
}

XDisasm::STATS *XDisasmModel::getStats()
{
    return pStats;
}
