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
#include "xdisasm.h"

XDisasm::XDisasm(QObject *parent) : QObject(parent)
{
    pBinary=nullptr;
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

bool XDisasm::setData(XBinary *pBinary, XDisasm::MODE mode, qint64 nStartAddress, STATS *pDisasmStats)
{
    this->pBinary=pBinary;
    this->mode=mode;
    this->nStartAddress=nStartAddress;
    this->pDisasmStats=pDisasmStats;

    return true;
}

void XDisasm::_process(qint64 nInitAddress, qint64 nAddress)
{
    pDisasmStats->mmapRefFrom.insert(nAddress,nInitAddress);
    pDisasmStats->mmapRefTo.insert(nInitAddress,nAddress);

    while(!bStop)
    {
        if(pDisasmStats->mapOpcodes.contains(nAddress))
        {
            break;
        }

        bool bStopBranch=false;
        int nDelta=0;
        qint64 nOffset=pBinary->addressToOffset(&(pDisasmStats->listMM),nAddress);
        if(nOffset!=-1)
        {
            QByteArray baData=pBinary->read_array(nOffset,15); // TODO

            uint8_t *pData=(uint8_t *)baData.data();
            size_t nDataSize=(size_t)baData.size();

            cs_insn *insn;
            size_t count=cs_disasm(disasm_handle,pData,nDataSize,nAddress,1,&insn);

            if(count>0)
            {
                QString sMnemonic=insn->mnemonic;
                QString sArgs=insn->op_str;

                QString sOpcode=sMnemonic;
                if(sArgs!="")
                {
                    sOpcode+=" "+sArgs;
                }

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

                            _process(nAddress,nImm);
                        }
                    }
                }

                OPCODE opcode={};
                opcode.nOffset=nOffset;
                opcode.nSize=insn->size;
                opcode.sString=sOpcode;

                pDisasmStats->mapOpcodes.insert(nAddress,opcode);

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

void XDisasm::process()
{
    bStop=false;

    clear();

    cs_arch arch=CS_ARCH_X86;
    cs_mode _mode=CS_MODE_16;

    pDisasmStats->listMM=pBinary->getMemoryMapList();

    pDisasmStats->nEntryPointAddress=nStartAddress;

    if(mode==MODE_UNKNOWN)
    {
        pDisasmStats->nImageBase=pBinary->getLowestAddress(&(pDisasmStats->listMM));
        pDisasmStats->nImageSize=pBinary->getTotalVirtualSize(&(pDisasmStats->listMM));
        pDisasmStats->nEntryPointAddress=pBinary->getEntryPointAddress();

        XBinary::MODE modeBinary=pBinary->getMode();
        XBinary::ARCH archBinary=pBinary->getArch();

        if(archBinary==XBinary::ARCH_X86) arch=CS_ARCH_X86;

        if      (modeBinary==XBinary::MODE_16) _mode=CS_MODE_16;
        else if (modeBinary==XBinary::MODE_32) _mode=CS_MODE_32;
        else if (modeBinary==XBinary::MODE_64) _mode=CS_MODE_64;
    }
    else if(mode==MODE_X86_16)
    {
        arch=CS_ARCH_X86;
        _mode=CS_MODE_16;
    }
    else if(mode==MODE_X86_32)
    {
        arch=CS_ARCH_X86;
        _mode=CS_MODE_32;
    }
    else if(mode==MODE_X86_64)
    {
        arch=CS_ARCH_X86;
        _mode=CS_MODE_64;
    }

    cs_err err=cs_open(arch,_mode,&disasm_handle);
    if(!err)
    {
        cs_option(disasm_handle,CS_OPT_DETAIL,CS_OPT_ON);
    }

    if(pBinary->metaObject()->className()==QString("XPE"))
    {
        XPE *pPE=(XPE *)pBinary;
        if(pPE->isValid())
        {

        }
    }

    _process(0,pDisasmStats->nEntryPointAddress);
    _adjust();
    _updatePositions();

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
                pDisasmStats->mapLabelStrings.insert(nAddress,QString("loc_%1").arg(nAddress,0,16));
            }
        }

    //    QSet<qint64> stFunctionLabels;
    //    QSet<qint64> stJmpLabels;
    //    QMap<qint64,qint64> mapDataSizeLabels; // Set Max
    //    QSet<qint64> stDataLabels;

        // TODO Strings
        QMapIterator<qint64,XDisasm::OPCODE> iVB(pDisasmStats->mapOpcodes);
        while(iVB.hasNext())
        {
            iVB.next();

            qint64 nAddress=iVB.key();

            VIEW_BLOCK record;
            record.nOffset=iVB.value().nOffset;
            record.nSize=iVB.value().nSize;
            record.type=VBT_OPCODE;

            if(!pDisasmStats->mapVB.contains(nAddress))
            {
                pDisasmStats->mapVB.insert(nAddress,record);
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

void XDisasm::clear()
{
    if(disasm_handle)
    {
        cs_close(&disasm_handle);
        disasm_handle=0;
    }
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
