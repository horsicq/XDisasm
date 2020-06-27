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
#ifndef XDISASM_H
#define XDISASM_H

#include <QObject>
#include "xformats.h"
#include "capstone/capstone.h"


class XDisasm : public QObject
{
    Q_OBJECT
    static const int N_X64_OPCODE_SIZE=15;
    static const int N_OPCODE_COUNT=100000;
public:
    enum DM
    {
        DM_UNKNOWN=0,
        DM_DISASM,
        DM_TODATA
    };

    enum VBT
    {
        VBT_UNKNOWN=0,
        VBT_OPCODE,
        VBT_DATA,
        VBT_DATABLOCK
    };

    enum RECORD_TYPE
    {
        RECORD_TYPE_UNKNOWN=0,
        RECORD_TYPE_OPCODE,
        RECORD_TYPE_DATA,
    };

    struct RECORD
    {
        qint64 nOffset;
        qint64 nSize;
        RECORD_TYPE type;
    };

//    enum LABEL_TYPE
//    {
//        LABEL_TYPE_UNKNOWN=0;
//        LABEL_TYPE_
//    };

    struct LABEL
    {
        qint64 nName;
    };

    struct VIEW_BLOCK
    {
        qint64 nAddress;
        qint64 nOffset;
        qint64 nSize;
        VBT type;
    };

    struct STATS
    {
        bool bInit;
        XBinary::_MEMORY_MAP memoryMap;
        cs_arch csarch;
        cs_mode csmode;
        qint64 nImageBase;
        qint64 nImageSize;
        qint64 nEntryPointAddress;
        QMap<qint64,RECORD> mapRecords;
        QMultiMap<qint64,qint64> mmapRefTo;
        QMultiMap<qint64,qint64> mmapRefFrom;
        QSet<qint64> stCalls;
        QSet<qint64> stJumps;
        QMultiMap<qint64,qint64> mmapDataLabels; // TODO Check
        QMap<qint64,VIEW_BLOCK> mapVB;
        QMap<qint64,QString> mapLabelStrings;
        qint64 nPositions;
        QMap<qint64,qint64> mapPositions;
        QMap<qint64,qint64> mapAddresses;
        bool bIsOverlayPresent;
        qint64 nOverlayOffset;
        qint64 nOverlaySize;
    };

    struct OPTIONS
    {
        bool bIsImage;
        qint64 nImageBase;
        XBinary::FT ft;
        XDisasm::STATS stats;
    };

    explicit XDisasm(QObject *parent=nullptr);
    ~XDisasm();
    void setData(QIODevice *pDevice,OPTIONS *pOptions, qint64 nStartAddress,DM dm);
    void stop();
    STATS *getStats();
    static qint64 getVBSize(QMap<qint64,VIEW_BLOCK> *pMapVB);
    static QString getDisasmString(csh disasm_handle, qint64 nAddress, char *pData, qint32 nDataSize);

    enum SM
    {
        SM_NORMAL=0,
        SM_RELATIVEADDRESS
    };

    struct SIGNATURE_OPTIONS
    {
        QIODevice *pDevice;
        XBinary::_MEMORY_MAP memoryMap;
        cs_arch csarch;
        cs_mode csmode;
        int nCount;
        SM sm;
    };

    struct SIGNATURE_RECORD
    {
        qint64 nAddress;
        QString sOpcode;
        QByteArray baOpcode;
        qint32 nDispOffset;
        qint32 nDispSize;
        qint32 nImmOffset;
        qint32 nImmSize;
        bool bIsConst;
    };

    static QList<SIGNATURE_RECORD> getSignature(SIGNATURE_OPTIONS *pSignatureOptions,qint64 nAddress);

public slots:
    void processDisasm();
    void processToData();
    void process();

private:
    bool isEndBranchOpcode(uint nOpcodeID);
    static bool isJmpOpcode(uint nOpcodeID);
    static bool isCallOpcode(uint nOpcodeID);
    void _disasm(qint64 nInitAddress, qint64 nAddress);
    void _adjust();
    void _updatePositions();
    bool _insertOpcode(qint64 nAddress,RECORD *pOpcode);

signals:
    void errorMessage(QString sText);
    void processFinished();

private:
    DM dm;
    csh disasm_handle;
    bool bStop;
    QIODevice *pDevice;
    OPTIONS *pOptions;
    qint64 nStartAddress;
};

#endif // XDISASM_H
