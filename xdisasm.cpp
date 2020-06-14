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
    pOptions=0;
    nStartAddress=0;
    disasm_handle=0;
    bStop=false;
}

XDisasm::~XDisasm()
{
    if(disasm_handle)
    {
        cs_close(&disasm_handle);
        disasm_handle=0;
    }
}

void XDisasm::setData(QIODevice *pDevice, XDisasm::OPTIONS *pOptions, qint64 nStartAddress, XDisasm::DM dm)
{
    this->pDevice=pDevice;
    this->pOptions=pOptions;
    this->nStartAddress=nStartAddress;
    this->dm=dm;
}

void XDisasm::_disasm(qint64 nInitAddress, qint64 nAddress)
{
    pOptions->stats.mmapRefFrom.insert(nAddress,nInitAddress);
    pOptions->stats.mmapRefTo.insert(nInitAddress,nAddress);

    while(!bStop)
    {
        if(pOptions->stats.mapRecords.contains(nAddress))
        {
            break;
        }

        bool bStopBranch=false;
        int nDelta=0;

        qint64 nOffset=XBinary::addressToOffset(&(pOptions->stats.memoryMap),nAddress); // TODO optimize if image
        if(nOffset!=-1)
        {
            char opcode[32];

            XBinary::_zeroMemory(opcode,N_X64_OPCODE_SIZE);

            XBinary::read_array(pDevice,nOffset,opcode,N_X64_OPCODE_SIZE); // TODO defs

            uint8_t *pData=(uint8_t *)opcode;
            size_t nDataSize=sizeof(opcode);

            cs_insn *insn;
            size_t count=cs_disasm(disasm_handle,pData,nDataSize,nAddress,1,&insn);

            if(count>0)
            {
                if(insn->size>1)
                {
                    bStopBranch=!XBinary::isAddressPhysical(&(pOptions->stats.memoryMap),nAddress+insn->size-1);
                }

                if(!bStopBranch)
                {
                    for(int i=0; i<insn->detail->x86.op_count; i++)
                    {
                        if(insn->detail->x86.operands[i].type==X86_OP_IMM)
                        {
                            qint64 nImm=insn->detail->x86.operands[i].imm;

                            if(isJmpOpcode(insn->id))
                            {
                                if(isCallOpcode(insn->id))
                                {
                                    pOptions->stats.stCalls.insert(nImm);
                                }
                                else
                                {
                                    pOptions->stats.stJumps.insert(nImm);
                                }

                                _disasm(nAddress,nImm);
                            }
                        }
                    }

                    RECORD opcode={};
                    opcode.nOffset=nOffset;
                    opcode.nSize=insn->size;
                    opcode.type=RECORD_TYPE_OPCODE;

                    pOptions->stats.mapRecords.insert(nAddress,opcode);

                    nDelta=insn->size;

                    if(isEndBranchOpcode(insn->id))
                    {
                        bStopBranch=true;
                    }
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

void XDisasm::processDisasm()
{
    bStop=false;

    if(!pOptions->stats.bInit)
    {
        pOptions->stats.csarch=CS_ARCH_X86;
        pOptions->stats.csmode=CS_MODE_16;

        pOptions->stats.ft=pOptions->ft;

        if(pOptions->stats.ft==XBinary::FT_UNKNOWN)
        {
            pOptions->stats.ft=XBinary::getPrefFileType(pDevice);
        }

        if((pOptions->stats.ft==XBinary::FT_PE32)||(pOptions->stats.ft==XBinary::FT_PE64))
        {
            XPE pe(pDevice,pOptions->bIsImage,pOptions->nImageBase);

            pOptions->stats.memoryMap=pe.getMemoryMap();
            pOptions->stats.nEntryPointAddress=pe.getEntryPointAddress(&pOptions->stats.memoryMap);
            pOptions->stats.bIsOverlayPresent=pe.isOverlayPresent();
            pOptions->stats.nOverlaySize=pe.getOverlaySize();
            pOptions->stats.nOverlayOffset=pe.getOverlayOffset();

            XBinary::MODE modeBinary=pe.getMode();
//            QString sArch=pe.getArch();
//            if(sArch=="I386") pOptions->stats.csarch=CS_ARCH_X86; // TODO more defs

            pOptions->stats.csarch=CS_ARCH_X86;
            if(modeBinary==XBinary::MODE_32)
            {
                pOptions->stats.csmode=CS_MODE_32;
            }
            else if(modeBinary==XBinary::MODE_64)
            {
                pOptions->stats.csmode=CS_MODE_64;
            }
        }
        else if((pOptions->stats.ft==XBinary::FT_ELF32)||(pOptions->stats.ft==XBinary::FT_ELF64))
        {
            XELF elf(pDevice,pOptions->bIsImage,pOptions->nImageBase);

            pOptions->stats.memoryMap=elf.getMemoryMap();
            pOptions->stats.nEntryPointAddress=elf.getEntryPointAddress(&pOptions->stats.memoryMap);
            pOptions->stats.bIsOverlayPresent=elf.isOverlayPresent();
            pOptions->stats.nOverlaySize=elf.getOverlaySize();
            pOptions->stats.nOverlayOffset=elf.getOverlayOffset();

            XBinary::MODE modeBinary=elf.getMode();
//            QString sArch=elf.getArch();
//            if(sArch=="386") pOptions->stats.csarch=CS_ARCH_X86; // TODO more defs

            pOptions->stats.csarch=CS_ARCH_X86;
            if(modeBinary==XBinary::MODE_32)
            {
                pOptions->stats.csmode=CS_MODE_32;
            }
            else if(modeBinary==XBinary::MODE_64)
            {
                pOptions->stats.csmode=CS_MODE_64;
            }
        }
        else if((pOptions->stats.ft==XBinary::FT_MACH32)||(pOptions->stats.ft==XBinary::FT_MACH64))
        {
            XMACH mach(pDevice,pOptions->bIsImage,pOptions->nImageBase);

            pOptions->stats.memoryMap=mach.getMemoryMap();
            pOptions->stats.nEntryPointAddress=mach.getEntryPointAddress(&pOptions->stats.memoryMap);
            pOptions->stats.bIsOverlayPresent=mach.isOverlayPresent();
            pOptions->stats.nOverlaySize=mach.getOverlaySize();
            pOptions->stats.nOverlayOffset=mach.getOverlayOffset();

            XBinary::MODE modeBinary=mach.getMode();
//            QString sArch=elf.getArch();
//            if(sArch=="386") pOptions->stats.csarch=CS_ARCH_X86; // TODO more defs

            pOptions->stats.csarch=CS_ARCH_X86;
            if(modeBinary==XBinary::MODE_32)
            {
                pOptions->stats.csmode=CS_MODE_32;
            }
            else if(modeBinary==XBinary::MODE_64)
            {
                pOptions->stats.csmode=CS_MODE_64;
            }
        }
        else if(pOptions->stats.ft==XBinary::FT_MSDOS)
        {
            XMSDOS msdos(pDevice,pOptions->bIsImage,pOptions->nImageBase);

            pOptions->stats.memoryMap=msdos.getMemoryMap();
            pOptions->stats.nEntryPointAddress=msdos.getEntryPointAddress(&pOptions->stats.memoryMap);
            pOptions->stats.bIsOverlayPresent=msdos.isOverlayPresent();
            pOptions->stats.nOverlaySize=msdos.getOverlaySize();
            pOptions->stats.nOverlayOffset=msdos.getOverlayOffset();

            pOptions->stats.csarch=CS_ARCH_X86;
            pOptions->stats.csmode=CS_MODE_16;
        }
        else if(pOptions->stats.ft==XBinary::FT_COM)
        {
            XCOM xcom(pDevice,pOptions->bIsImage,pOptions->nImageBase);

            pOptions->stats.memoryMap=xcom.getMemoryMap();
            pOptions->stats.nEntryPointAddress=xcom.getEntryPointAddress(&pOptions->stats.memoryMap);

            pOptions->stats.csarch=CS_ARCH_X86;
            pOptions->stats.csmode=CS_MODE_16;
        }
        else if((pOptions->stats.ft==XBinary::FT_BINARY16)||(pOptions->stats.ft==XBinary::FT_BINARY))
        {
            XBinary binary(pDevice,pOptions->bIsImage,pOptions->nImageBase);

            pOptions->stats.memoryMap=binary.getMemoryMap();
            pOptions->stats.nEntryPointAddress=binary.getEntryPointAddress(&pOptions->stats.memoryMap);

            pOptions->stats.csarch=CS_ARCH_X86;
            pOptions->stats.csmode=CS_MODE_16;
        }
        else if(pOptions->stats.ft==XBinary::FT_BINARY32)
        {
            XBinary binary(pDevice,pOptions->bIsImage,pOptions->nImageBase);

            pOptions->stats.memoryMap=binary.getMemoryMap();
            pOptions->stats.nEntryPointAddress=binary.getEntryPointAddress(&pOptions->stats.memoryMap);

            pOptions->stats.csarch=CS_ARCH_X86;
            pOptions->stats.csmode=CS_MODE_32;
        }
        else if(pOptions->stats.ft==XBinary::FT_BINARY64)
        {
            XBinary binary(pDevice,pOptions->bIsImage,pOptions->nImageBase);

            pOptions->stats.memoryMap=binary.getMemoryMap();
            pOptions->stats.nEntryPointAddress=binary.getEntryPointAddress(&pOptions->stats.memoryMap);

            pOptions->stats.csarch=CS_ARCH_X86;
            pOptions->stats.csmode=CS_MODE_64;
        }

        pOptions->stats.nImageBase=pOptions->stats.memoryMap.nBaseAddress;
//        pOptions->stats.nImageSize=XBinary::getTotalVirtualSize(&(pOptions->stats.memoryMap));
        pOptions->stats.nImageSize=pOptions->stats.memoryMap.nImageSize;

        cs_err err=cs_open(pOptions->stats.csarch,pOptions->stats.csmode,&disasm_handle);
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

        _disasm(0,pOptions->stats.nEntryPointAddress);

        if(nStartAddress!=-1)
        {
            if(nStartAddress!=pOptions->stats.nEntryPointAddress)
            {
                _disasm(0,nStartAddress);
            }
        }

        _adjust();
        _updatePositions();

        pOptions->stats.bInit=true;

        if(disasm_handle)
        {
            cs_close(&disasm_handle);
            disasm_handle=0;
        }
    }
    else
    {
        // TODO move to function
        if(disasm_handle==0)
        {
            cs_err err=cs_open(pOptions->stats.csarch,pOptions->stats.csmode,&disasm_handle);
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

void XDisasm::processToData()
{
    pOptions->stats.mapRecords.remove(this->nStartAddress);

    _adjust();
    _updatePositions();

    emit processFinished();
}

void XDisasm::process()
{
    if(dm==DM_DISASM)
    {
        processDisasm();
    }
    else if(dm==DM_TODATA)
    {
        processToData();
    }
}

void XDisasm::stop()
{
    bStop=true;
}

XDisasm::STATS *XDisasm::getStats()
{
    return &(pOptions->stats);
}

void XDisasm::_adjust()
{
    pOptions->stats.mapLabelStrings.clear();
    pOptions->stats.mapVB.clear();

    if(!bStop)
    {
        pOptions->stats.mapLabelStrings.insert(pOptions->stats.nEntryPointAddress,"entry_point");

        QSetIterator<qint64> iFL(pOptions->stats.stCalls);
        while(iFL.hasNext()&&(!bStop))
        {
            qint64 nAddress=iFL.next();

            if(!pOptions->stats.mapLabelStrings.contains(nAddress))
            {
                pOptions->stats.mapLabelStrings.insert(nAddress,QString("func_%1").arg(nAddress,0,16));
            }
        }

        QSetIterator<qint64> iJL(pOptions->stats.stJumps);
        while(iJL.hasNext()&&(!bStop))
        {
            qint64 nAddress=iJL.next();

            if(!pOptions->stats.mapLabelStrings.contains(nAddress))
            {
                pOptions->stats.mapLabelStrings.insert(nAddress,QString("lab_%1").arg(nAddress,0,16));
            }
        }

    //    QSet<qint64> stFunctionLabels;
    //    QSet<qint64> stJmpLabels;
    //    QMap<qint64,qint64> mapDataSizeLabels; // Set Max
    //    QSet<qint64> stDataLabels;

        // TODO Strings
        QMapIterator<qint64,XDisasm::RECORD> iRecords(pOptions->stats.mapRecords);
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

            if(!pOptions->stats.mapVB.contains(nAddress))
            {
                pOptions->stats.mapVB.insert(nAddress,record);
            }
        }

        int nMMCount=pOptions->stats.memoryMap.listRecords.count(); // TODO

        for(int i=0;(i<nMMCount)&&(!bStop);i++)
        {
            qint64 nRegionAddress=pOptions->stats.memoryMap.listRecords.at(i).nAddress;
            qint64 nRegionOffset=pOptions->stats.memoryMap.listRecords.at(i).nOffset;
            qint64 nRegionSize=pOptions->stats.memoryMap.listRecords.at(i).nSize;

            if(nRegionAddress!=-1)
            {
                for(qint64 nCurrentAddress=nRegionAddress,nCurrentOffset=nRegionOffset;nCurrentAddress<(nRegionAddress+nRegionSize);)
                {
                    VIEW_BLOCK vb=pOptions->stats.mapVB.value(nCurrentAddress);
                    // 418c00 418c04
                    if(!vb.nSize)
                    {
                        QMap<qint64,VIEW_BLOCK>::const_iterator iter=pOptions->stats.mapVB.lowerBound(nCurrentAddress);

                        qint64 nBlockAddress=0;
                        qint64 nBlockOffset=0;
                        qint64 nBlockSize=0;

                        qint64 nIterKey=iter.key();

                        if(pOptions->stats.mapVB.count())
                        {
                            if(nIterKey==pOptions->stats.mapVB.firstKey()) // TODO move outside 'for'
                            {
                                nBlockAddress=pOptions->stats.nImageBase;
                                nBlockOffset=0;
                                if(nIterKey<(nRegionAddress+nRegionSize))
                                {
                                    nBlockSize=iter.key()-pOptions->stats.nImageBase;
                                }
                                else
                                {
                                    nBlockSize=(nRegionAddress+nRegionSize)-nBlockAddress;
                                }
                            }
                            else if(iter==pOptions->stats.mapVB.end())
                            {
                                nBlockAddress=nCurrentAddress;
                                nBlockOffset=nCurrentOffset;

                                nBlockSize=(nRegionAddress+nRegionSize)-nBlockAddress;
                            }
                            else
                            {
                                nBlockAddress=nCurrentAddress;
                                nBlockOffset=nCurrentOffset;
                                if(nIterKey<(nRegionAddress+nRegionSize))
                                {
                                    nBlockSize=iter.key()-nBlockAddress;
                                }
                                else
                                {
                                    nBlockSize=(nRegionAddress+nRegionSize)-nBlockAddress;
                                }
                            }
                        }
                        else
                        {
                            nBlockAddress=nCurrentAddress;
                            nBlockOffset=nCurrentOffset;

                            nBlockSize=(nRegionAddress+nRegionSize)-nBlockAddress;
                        }

                        qint64 _nAddress=nBlockAddress;
                        qint64 _nOffset=nBlockOffset;
                        qint64 _nSize=nBlockSize;

                        if(_nOffset!=-1)
                        {
                            while(_nSize>=16)
                            {
                                VIEW_BLOCK record;
                                record.nAddress=_nAddress;
                                record.nOffset=_nOffset;
                                record.nSize=16;
                                record.type=VBT_DATABLOCK;

                                if(!pOptions->stats.mapVB.contains(_nAddress))
                                {
                                    pOptions->stats.mapVB.insert(_nAddress,record);
                                }

                                _nSize-=16;
                                _nAddress+=16;
                                _nOffset+=16;
                            }
                        }
                        else
                        {
                            VIEW_BLOCK record;
                            record.nAddress=_nAddress;
                            record.nOffset=-1;
                            record.nSize=_nSize;
                            record.type=VBT_DATABLOCK;

                            if(!pOptions->stats.mapVB.contains(_nAddress))
                            {
                                pOptions->stats.mapVB.insert(_nAddress,record);
                            }
                        }

                        nCurrentAddress=nBlockAddress+nBlockSize;
                        if(nBlockOffset!=-1)
                        {
                            nCurrentOffset=nBlockOffset+nBlockSize;
                        }
                    }
                    else
                    {
                        nCurrentAddress=vb.nAddress+vb.nSize;
                        if(nCurrentOffset!=-1)
                        {
                            nCurrentOffset=vb.nOffset+vb.nSize;
                        }
                    }
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
    qint64 nImageSize=pOptions->stats.nImageSize;
    qint64 nVBCount=pOptions->stats.mapVB.count();
    qint64 nVBSize=getVBSize(&(pOptions->stats.mapVB));
    pOptions->stats.nPositions=nImageSize+nVBCount-nVBSize; // TODO

    pOptions->stats.mapPositions.clear();
    // TODO cache
    qint64 nCurrentAddress=pOptions->stats.nImageBase; // TODO

    for(qint64 i=0;i<pOptions->stats.nPositions;i++)
    {
        bool bIsVBPresent=pOptions->stats.mapVB.contains(nCurrentAddress);

        if(bIsVBPresent)
        {
            pOptions->stats.mapPositions.insert(i,nCurrentAddress);
            pOptions->stats.mapAddresses.insert(nCurrentAddress,i);

            nCurrentAddress+=pOptions->stats.mapVB.value(nCurrentAddress).nSize-1;
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
        (nOpcodeID==X86_INS_RET)||
        (nOpcodeID==X86_INS_INT3))
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
