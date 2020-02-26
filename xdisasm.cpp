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
#include "xdisasm.h"

XDisasm::XDisasm(QObject *parent) : QObject(parent)
{
    pDevice=nullptr;
    mode=MODE_UNKNOWN;
    nImageBase=0;
    nStartAddress=0;
    disasm_handle=0;
    bStop=false;
    pMap=0;
}

XDisasm::~XDisasm()
{
    if(disasm_handle)
    {
        cs_close(&disasm_handle);
        disasm_handle=0;
    }
}

void XDisasm::setData(QIODevice *pDevice, bool bIsImage, XDisasm::MODE mode, qint64 nStartAddress, STATS *pDisasmStats,qint64 nImageBase)
{
    this->pDevice=pDevice;
    this->bIsImage=bIsImage;
    this->mode=mode;
    this->nImageBase=nImageBase;
    this->nStartAddress=nStartAddress;
    this->pDisasmStats=pDisasmStats;
}

void XDisasm::processDisasmInit(QIODevice *pDevice, bool bIsImage, XDisasm::MODE mode, qint64 nStartAddress, XDisasm::STATS *pDisasmStats, qint64 nImageBase)
{
    XDisasm disasm;
    disasm.setData(pDevice,bIsImage,mode,nStartAddress,pDisasmStats,nImageBase);
    disasm.processDisasmInit();
}

void XDisasm::_disasm(qint64 nInitAddress, qint64 nAddress)
{
    pDisasmStats->mmapRefFrom.insert(nAddress,nInitAddress);
    pDisasmStats->mmapRefTo.insert(nInitAddress,nAddress);

    while(!bStop)
    {
        if(pDisasmStats->mapRecords.contains(nAddress))
        {
            break;
        }

        bool bStopBranch=false;
        int nDelta=0;
        qint64 nOffset=XBinary::addressToOffset(&(pDisasmStats->memoryMap),nAddress); // TODO optimize if image
        if(nOffset!=-1)
        {
            QByteArray baData=XBinary::read_array(pDevice,nOffset,N_X64_OPCODE_SIZE);

            uint8_t *pData=(uint8_t *)baData.data();
            size_t nDataSize=(size_t)baData.size();

            cs_insn *insn;
            size_t count=cs_disasm(disasm_handle,pData,nDataSize,nAddress,1,&insn);

            if(count>0)
            {
//                QString sMnemonic=insn->mnemonic;
//                QString sArgs=insn->op_str;

//                QString sOpcode=sMnemonic;
//                if(sArgs!="")
//                {
//                    sOpcode+=" "+sArgs;
//                }

                for(int i=0; i<insn->detail->x86.op_count; i++)
                {
                    if(insn->detail->x86.operands[i].type==X86_OP_IMM)
                    {
                        qint64 nImm=insn->detail->x86.operands[i].imm;

                        if(isJmpOpcode(insn->id))
                        {
                            if(isCallOpcode(insn->id))
                            {
                                pDisasmStats->stCalls.insert(nImm);
                            }
                            else
                            {
                                pDisasmStats->stJumps.insert(nImm);
                            }

                            _disasm(nAddress,nImm);
                        }
                    }
                }

                RECORD opcode={};
                opcode.nOffset=nOffset;
                opcode.nSize=insn->size;
                opcode.type=RECORD_TYPE_OPCODE;

                pDisasmStats->mapRecords.insert(nAddress,opcode);

                nDelta=insn->size;

                if(isEndBranchOpcode(insn->id))
                {
                    bStopBranch=true;
                }

                cs_free(insn,count);
            }
            else
            {
                bStopBranch=true;
            }
        }

        if(nDelta)
        {
            nAddress+=nDelta;
        }
        else
        {
            bStopBranch=true;
        }

        if(bStopBranch)
        {
            break;
        }
    }
}

void XDisasm::processDisasmInit() // TODO rename
{
    bStop=false;

    pDisasmStats->csarch=CS_ARCH_X86;
    pDisasmStats->csmode=CS_MODE_16;

    pDisasmStats->mode=mode;

    if(pDisasmStats->mode==MODE_UNKNOWN)
    {
        QSet<XBinary::FT> stFt=XBinary::getFileTypes(pDevice);

        if(stFt.contains(XBinary::FT_PE))
        {
            XPE pe(pDevice,bIsImage,nImageBase);

            pDisasmStats->memoryMap=pe.getMemoryMap();
            pDisasmStats->nEntryPointAddress=pe.getEntryPointAddress(&pDisasmStats->memoryMap);

            XBinary::MODE modeBinary=pe.getMode();
            QString sArch=pe.getArch();

            if(sArch=="I386") pDisasmStats->csarch=CS_ARCH_X86; // TODO more defs

            if(modeBinary==XBinary::MODE_32)
            {
                pDisasmStats->csmode=CS_MODE_32;
                pDisasmStats->mode=MODE_X86_32;
            }
            else if(modeBinary==XBinary::MODE_64)
            {
                pDisasmStats->csmode=CS_MODE_64;
                pDisasmStats->mode=MODE_X86_64;
            }
        }
    }
    else
    {
        if(pDisasmStats->mode==MODE_X86_16)
        {
            pDisasmStats->csarch=CS_ARCH_X86;
            pDisasmStats->csmode=CS_MODE_16;
        }
        else if(pDisasmStats->mode==MODE_X86_32)
        {
            pDisasmStats->csarch=CS_ARCH_X86;
            pDisasmStats->csmode=CS_MODE_32;
        }
        else if(pDisasmStats->mode==MODE_X86_64)
        {
            pDisasmStats->csarch=CS_ARCH_X86;
            pDisasmStats->csmode=CS_MODE_64;
        }

        XBinary binary(pDevice,bIsImage,nImageBase);

        pDisasmStats->memoryMap=binary.getMemoryMap();
        pDisasmStats->nEntryPointAddress=nStartAddress;
    }

    pDisasmStats->nImageBase=pDisasmStats->memoryMap.nBaseAddress;
    pDisasmStats->nImageSize=XBinary::getTotalVirtualSize(&(pDisasmStats->memoryMap));

    cs_err err=cs_open(pDisasmStats->csarch,pDisasmStats->csmode,&disasm_handle);
    if(!err)
    {
        cs_option(disasm_handle,CS_OPT_DETAIL,CS_OPT_ON); // TODO Check
    }

//    if(pBinary->metaObject()->className()==QString("XPE"))
//    {
//        XPE *pPE=(XPE *)pBinary;
//        if(pPE->isValid())
//        {
//            // TODO
//        }
//    }

    _disasm(0,pDisasmStats->nEntryPointAddress);

    if(nStartAddress!=-1)
    {
        if(nStartAddress!=pDisasmStats->nEntryPointAddress)
        {
            _disasm(0,nStartAddress);
        }
    }

    _adjust();
    _updatePositions();

    pDisasmStats->bInit=true;

    if(disasm_handle)
    {
        cs_close(&disasm_handle);
        disasm_handle=0;
    }

    emit processFinished();
}

void XDisasm::processDisasm()
{
    if(!pDisasmStats->bInit)
    {
        processDisasmInit();
    }
    else
    {
        if(disasm_handle==0)
        {
            cs_err err=cs_open(pDisasmStats->csarch,pDisasmStats->csmode,&disasm_handle);
            if(!err)
            {
                cs_option(disasm_handle,CS_OPT_DETAIL,CS_OPT_ON); // TODO Check
            }
        }

        _disasm(0,nStartAddress);

        _adjust();
        _updatePositions();

        if(disasm_handle)
        {
            cs_close(&disasm_handle);
            disasm_handle=0;
        }
    }

    emit processFinished();
}

void XDisasm::stop()
{
    bStop=true;
}

XDisasm::STATS *XDisasm::getStats()
{
    return pDisasmStats;
}

void XDisasm::_adjust()
{
    pDisasmStats->mapLabelStrings.clear();
    pDisasmStats->mapVB.clear();

    if(!bStop)
    {
        pDisasmStats->mapLabelStrings.insert(pDisasmStats->nEntryPointAddress,"entry_point");

        QSetIterator<qint64> iFL(pDisasmStats->stCalls);
        while(iFL.hasNext()&&(!bStop))
        {
            qint64 nAddress=iFL.next();

            if(!pDisasmStats->mapLabelStrings.contains(nAddress))
            {
                pDisasmStats->mapLabelStrings.insert(nAddress,QString("func_%1").arg(nAddress,0,16));
            }
        }

        QSetIterator<qint64> iJL(pDisasmStats->stJumps);
        while(iJL.hasNext()&&(!bStop))
        {
            qint64 nAddress=iJL.next();

            if(!pDisasmStats->mapLabelStrings.contains(nAddress))
            {
                pDisasmStats->mapLabelStrings.insert(nAddress,QString("lab_%1").arg(nAddress,0,16));
            }
        }

    //    QSet<qint64> stFunctionLabels;
    //    QSet<qint64> stJmpLabels;
    //    QMap<qint64,qint64> mapDataSizeLabels; // Set Max
    //    QSet<qint64> stDataLabels;

        // TODO Strings
        QMapIterator<qint64,XDisasm::RECORD> iRecords(pDisasmStats->mapRecords);
        while(iRecords.hasNext()&&(!bStop))
        {
            iRecords.next();

            qint64 nAddress=iRecords.key();

            VIEW_BLOCK record;
            record.nAddress=nAddress;
            record.nOffset=iRecords.value().nOffset;
            record.nSize=iRecords.value().nSize;

            if(iRecords.value().type==RECORD_TYPE_OPCODE)
            {
                record.type=VBT_OPCODE;
            }
            else if(iRecords.value().type==RECORD_TYPE_DATA)
            {
                record.type=VBT_DATA;
            }

            if(!pDisasmStats->mapVB.contains(nAddress))
            {
                pDisasmStats->mapVB.insert(nAddress,record);
            }
        }

        int nMMCount=pDisasmStats->memoryMap.listRecords.count(); // TODO

        for(int i=0;(i<nMMCount)&&(!bStop);i++)
        {
            qint64 nRegionAddress=pDisasmStats->memoryMap.listRecords.at(i).nAddress;
            qint64 nRegionOffset=pDisasmStats->memoryMap.listRecords.at(i).nOffset;
            qint64 nRegionSize=pDisasmStats->memoryMap.listRecords.at(i).nSize;

            for(qint64 nCurrentAddress=nRegionAddress,nCurrentOffset=nRegionOffset;nCurrentAddress<(nRegionAddress+nRegionSize);)
            {
                QMap<qint64,VIEW_BLOCK>::iterator iter=pDisasmStats->mapVB.lowerBound(nCurrentAddress);

                qint64 nBlockAddress=0;
                qint64 nBlockOffset=0;
                qint64 nBlockSize=0;

                if(iter!=pDisasmStats->mapVB.end())
                {
                    VIEW_BLOCK block=*iter;
                    nBlockAddress=block.nAddress;
                    nBlockOffset=block.nOffset;
                    nBlockSize=block.nSize;
                }
                else
                {
                    nBlockAddress=nRegionAddress+nRegionSize;

                    if(nRegionOffset!=-1)
                    {
                        nBlockOffset=nRegionOffset+nRegionSize;
                    }
                }

                qint64 _nAddress=qMin(nBlockAddress,nRegionAddress+nRegionSize);
                qint64 _nSize=_nAddress-nCurrentAddress;

                if(nCurrentOffset!=-1)
                {
                    while(_nSize>=16)
                    {
                        VIEW_BLOCK record;
                        record.nAddress=nCurrentAddress;
                        record.nOffset=nCurrentOffset;
                        record.nSize=16;
                        record.type=VBT_DATABLOCK;

                        if(!pDisasmStats->mapVB.contains(nCurrentAddress))
                        {
                            pDisasmStats->mapVB.insert(nCurrentAddress,record);
                        }

                        _nSize-=16;
                        nCurrentAddress+=16;
                        nCurrentOffset+=16;
                    }
                }
                else
                {
                    VIEW_BLOCK record;
                    record.nAddress=nCurrentAddress;
                    record.nOffset=-1;
                    record.nSize=_nSize;
                    record.type=VBT_DATABLOCK;

                    if(!pDisasmStats->mapVB.contains(nCurrentAddress))
                    {
                        pDisasmStats->mapVB.insert(nCurrentAddress,record);
                    }
                }

                nCurrentAddress=nBlockAddress+nBlockSize;
                if(nBlockOffset!=-1)
                {
                    nCurrentOffset=nBlockOffset+nBlockSize;
                }
            }
        }

//        QMapIterator<qint64,qint64> iDS(stats.mmapDataLabels);
//        while(iDS.hasNext())
//        {
//            iDS.next();

//            qint64 nAddress=iDS.key();

//            VIEW_BLOCK record;
//            record.nOffset=pBinary->addressToOffset(pListMM,nAddress);
//            record.nSize=iDS.value();
//            record.type=VBT_DATA;

//            if(!stats.mapVB.contains(nAddress))
//            {
//                stats.mapVB.insert(nAddress,record);
//            }
//        }

//        QSetIterator<qint64> iD(stats.stDataLabels);
//        while(iD.hasNext()&&(!bStop))
//        {
//            qint64 nAddress=iD.next();

//            VIEW_BLOCK record;
//            record.nOffset=pBinary->addressToOffset(pListMM,nAddress);
//            record.nSize=0;
//            record.type=VBT_DATA;

//            if(!stats.mapVB.contains(nAddress))
//            {
//                stats.mapVB.insert(nAddress,record);
//            }
//        }

        // TODO Check errors
    }
}

void XDisasm::_updatePositions()
{
    pDisasmStats->nPositions=pDisasmStats->nImageSize+pDisasmStats->mapVB.count()-getVBSize(&(pDisasmStats->mapVB)); // TODO

    pDisasmStats->mapPositions.clear();
    // TODO cache
    qint64 nCurrentAddress=pDisasmStats->nImageBase; // TODO

    for(qint64 i=0;i<pDisasmStats->nPositions;i++)
    {
        bool bIsVBPresent=pDisasmStats->mapVB.contains(nCurrentAddress);

        if(bIsVBPresent)
        {
            pDisasmStats->mapPositions.insert(i,nCurrentAddress);
            pDisasmStats->mapAddresses.insert(nCurrentAddress,i);

            nCurrentAddress+=pDisasmStats->mapVB.value(nCurrentAddress).nSize-1;
        }

        nCurrentAddress++;
    }
}

qint64 XDisasm::getVBSize(QMap<qint64, XDisasm::VIEW_BLOCK> *pMapVB)
{
    qint64 nResult=0;

    QMapIterator<qint64, XDisasm::VIEW_BLOCK> i(*pMapVB);
    while(i.hasNext())
    {
        i.next();

        nResult+=i.value().nSize;
    }

    return nResult;
}

QString XDisasm::getDisasmString(csh disasm_handle, qint64 nAddress, char *pData, qint32 nDataSize)
{
    QString sResult;

    cs_insn *insn;
    size_t count=cs_disasm(disasm_handle,(uint8_t *)pData,nDataSize,nAddress,1,&insn);

    if(count>0)
    {
        QString sMnemonic=insn->mnemonic;
        QString sArgs=insn->op_str;

        sResult=sMnemonic;
        if(sArgs!="")
        {
            sResult+=" "+sArgs;
        }

        cs_free(insn,count);
    }

    return sResult;
}

bool XDisasm::isEndBranchOpcode(uint nOpcodeID)
{
    bool bResult=false;

    if( (nOpcodeID==X86_INS_JMP)||
        (nOpcodeID==X86_INS_RET))
    {
        bResult=true;
    }

    return bResult;
}

bool XDisasm::isJmpOpcode(uint nOpcodeID)
{
    bool bResult=false;

    if( (nOpcodeID==X86_INS_JMP)||
        (nOpcodeID==X86_INS_JA)||
        (nOpcodeID==X86_INS_JAE)||
        (nOpcodeID==X86_INS_JB)||
        (nOpcodeID==X86_INS_JBE)||
        (nOpcodeID==X86_INS_JCXZ)||
        (nOpcodeID==X86_INS_JE)||
        (nOpcodeID==X86_INS_JECXZ)||
        (nOpcodeID==X86_INS_JG)||
        (nOpcodeID==X86_INS_JGE)||
        (nOpcodeID==X86_INS_JL)||
        (nOpcodeID==X86_INS_JLE)||
        (nOpcodeID==X86_INS_JNE)||
        (nOpcodeID==X86_INS_JNO)||
        (nOpcodeID==X86_INS_JNP)||
        (nOpcodeID==X86_INS_JNS)||
        (nOpcodeID==X86_INS_JO)||
        (nOpcodeID==X86_INS_JP)||
        (nOpcodeID==X86_INS_JRCXZ)||
        (nOpcodeID==X86_INS_JS)||
        (nOpcodeID==X86_INS_LOOP)||
        (nOpcodeID==X86_INS_LOOPE)||
        (nOpcodeID==X86_INS_LOOPNE)||
        (nOpcodeID==X86_INS_CALL))
    {
        bResult=true;
    }

    return bResult;
}

bool XDisasm::isCallOpcode(uint nOpcodeID)
{
    bool bResult=false;

    if(nOpcodeID==X86_INS_CALL)
    {
        bResult=true;
    }

    return bResult;
}
