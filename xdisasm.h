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
#ifndef XDISASM_H
#define XDISASM_H

#include <QObject>
#include "xpe.h"
#include "capstone/capstone.h"


class XDisasm : public QObject
{
    Q_OBJECT

public:
    enum MODE
    {
        MODE_UNKNOWN=0,
        MODE_X86_16,
        MODE_X86_32,
        MODE_X86_64
    };

    enum RECORD_TYPE
    {
        RECORD_TYPE_UNKNOWN=0,
        RECORD_TYPE_OPCODE
    };

    struct OPCODE
    {
        qint64 nOffset;
        qint64 nSize;
        QString sString;
        qint64 nArg1;
        // TODO enum with types
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

    enum VBT
    {
        VBT_UNKNOWN=0,
        VBT_OPCODE,
        VBT_DATA,
        VBT_DATABLOCK
    };

    struct VIEW_BLOCK
    {
        qint64 nOffset;
        qint64 nSize;
        VBT type;
    };

    struct STATS
    {
        XBinary::_MEMORY_MAP memoryMap;
        qint64 nImageBase;
        qint64 nImageSize;
        qint64 nEntryPointAddress;
        QMap<qint64,OPCODE> mapOpcodes;
        QMultiMap<qint64,qint64> mmapRefTo;
        QMultiMap<qint64,qint64> mmapRefFrom;
        QSet<qint64> stCalls;
        QSet<qint64> stJumps;
        QMultiMap<qint64,qint64> mmapDataLabels;
        QMap<qint64,VIEW_BLOCK> mapVB;
        QMap<qint64,QString> mapLabelStrings;
        qint64 nPositions;
        QMap<qint64,qint64> mapPositions;
        QMap<qint64,qint64> mapAddresses;
    };

    explicit XDisasm(QObject *parent=nullptr);
    ~XDisasm();
    void setData(QIODevice *pDevice, bool bIsImage, XDisasm::MODE mode, qint64 nStartAddress, XDisasm::STATS *pDisasmStats, qint64 nImageBase=-1);
    static void process(QIODevice *pDevice, bool bIsImage, XDisasm::MODE mode, qint64 nStartAddress, XDisasm::STATS *pDisasmStats, qint64 nImageBase=-1);
    void stop();
    STATS *getStats();
    static qint64 getVBSize(QMap<qint64,VIEW_BLOCK> *pMapVB);

public slots:
    void process();

private:
    const int N_X64_OPCODE_SIZE=15;

    void clear();
    bool isEndBranchOpcode(uint nOpcodeID);
    bool isJmpOpcode(uint nOpcodeID);
    bool isCallOpcode(uint nOpcodeID);
    void _process(qint64 nInitAddress, qint64 nAddress);
    void _adjust();
    void _updatePositions();

signals:
    void processFinished();

private:
    QIODevice *pDevice;
    bool bIsImage;
    XDisasm::MODE mode;
    qint64 nImageBase;
    qint64 nStartAddress;
    csh disasm_handle;
    bool bStop;
    STATS *pDisasmStats;
    char *pMap;
};

#endif // XDISASM_H
