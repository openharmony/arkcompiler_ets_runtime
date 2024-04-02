/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mir_function.h"
#include "opcode_info.h"
#include "mir_pragma.h"
#include "mir_builder.h"
#include "bin_mplt.h"
#include <sstream>
#include <vector>

using namespace std;
namespace maple {
void BinaryMplExport::OutputInfoVector(const MIRInfoVector &infoVector, const MapleVector<bool> &infoVectorIsString)
{
    if (!mod.IsWithDbgInfo()) {
        Write(0);
        return;
    }
    WriteNum(infoVector.size());
    for (uint32 i = 0; i < infoVector.size(); i++) {
        OutputStr(infoVector[i].first);
        WriteNum(infoVectorIsString[i] ? 1 : 0);
        if (!infoVectorIsString[i]) {
            WriteNum(infoVector[i].second);
        } else {
            OutputStr(GStrIdx(infoVector[i].second));
        }
    }
}

void BinaryMplExport::OutputFuncIdInfo(MIRFunction *func)
{
    WriteNum(kBinFuncIdInfoStart);
    WriteNum(func->GetPuidxOrigin());  // the funcid
    OutputInfoVector(func->GetInfoVector(), func->InfoIsString());
    if (mod.GetFlavor() == kFlavorLmbc) {
        WriteNum(func->GetFrameSize());
    }
}

void BinaryMplExport::OutputBaseNode(const BaseNode *b)
{
    Write(static_cast<uint8>(b->GetOpCode()));
    Write(static_cast<uint8>(b->GetPrimType()));
}

void BinaryMplExport::OutputLocalSymbol(MIRSymbol *sym)
{
    CHECK_FATAL(sym != nullptr, "null ptr");
    std::unordered_map<const MIRSymbol *, int64>::iterator it = localSymMark.find(sym);
    if (it != localSymMark.end()) {
        WriteNum(-(it->second));
        return;
    }

    WriteNum(kBinSymbol);
    OutputStr(sym->GetNameStrIdx());
    WriteNum(sym->GetSKind());
    WriteNum(sym->GetStorageClass());
    size_t mark = localSymMark.size();
    localSymMark[sym] = static_cast<int64>(mark);
    OutputTypeAttrs(sym->GetAttrs());
    WriteNum(static_cast<int64>(sym->GetIsTmp()));
    if (sym->GetSKind() == kStVar || sym->GetSKind() == kStFunc) {
        OutputSrcPos(sym->GetSrcPosition());
    }
    OutputType(sym->GetTyIdx());
    if (sym->GetSKind() == kStPreg) {
        OutputPreg(sym->GetPreg());
    } else if (sym->GetSKind() == kStConst || sym->GetSKind() == kStVar) {
        OutputConst(sym->GetKonst());
    } else if (sym->GetSKind() == kStFunc) {
        OutputFuncViaSym(sym->GetFunction()->GetPuidx());
    } else {
        CHECK_FATAL(false, "should not used");
    }
}

void BinaryMplExport::OutputPreg(MIRPreg *preg)
{
    if (preg->GetPregNo() < 0) {
        WriteNum(kBinSpecialReg);
        Write(static_cast<uint8>(-preg->GetPregNo()));
        return;
    }
    std::unordered_map<const MIRPreg *, int64>::iterator it = localPregMark.find(preg);
    if (it != localPregMark.end()) {
        WriteNum(-(it->second));
        return;
    }

    WriteNum(kBinPreg);
    Write(static_cast<uint8>(preg->GetPrimType()));
    size_t mark = localPregMark.size();
    localPregMark[preg] = static_cast<int>(mark);
}

void BinaryMplExport::OutputLabel(LabelIdx lidx)
{
    std::unordered_map<LabelIdx, int64>::iterator it = labelMark.find(lidx);
    if (it != labelMark.end()) {
        WriteNum(-(it->second));
        return;
    }

    WriteNum(kBinLabel);
    size_t mark = labelMark.size();
    labelMark[lidx] = static_cast<int64>(mark);
}

void BinaryMplExport::OutputLocalTypeNameTab(const MIRTypeNameTable *typeNameTab)
{
    WriteNum(kBinTypenameStart);
    WriteNum(static_cast<int64>(typeNameTab->Size()));
    for (std::pair<GStrIdx, TyIdx> it : typeNameTab->GetGStrIdxToTyIdxMap()) {
        OutputStr(it.first);
        OutputType(it.second);
    }
}

void BinaryMplExport::OutputFormalsStIdx(MIRFunction *func)
{
    WriteNum(kBinFormalStart);
    WriteNum(func->GetFormalDefVec().size());
    for (FormalDef formalDef : func->GetFormalDefVec()) {
        OutputLocalSymbol(formalDef.formalSym);
    }
}

void BinaryMplExport::OutputAliasMap(MapleMap<GStrIdx, MIRAliasVars> &aliasVarMap)
{
    WriteNum(kBinAliasMapStart);
    WriteInt(static_cast<int32>(aliasVarMap.size()));
    for (std::pair<GStrIdx, MIRAliasVars> it : aliasVarMap) {
        OutputStr(it.first);
        OutputStr(it.second.mplStrIdx);
        OutputType(it.second.tyIdx);
        OutputStr(it.second.sigStrIdx);
    }
}

void BinaryMplExport::OutputFuncViaSym(PUIdx puIdx)
{
    MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
    MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(func->GetStIdx().Idx());
    OutputSymbol(funcSt);
}

void BinaryMplExport::OutputExpression(BaseNode *e)
{
    OutputBaseNode(e);
    switch (e->GetOpCode()) {
        // leaf
        case OP_constval: {
            MIRConst *constVal = static_cast<ConstvalNode *>(e)->GetConstVal();
            OutputConst(constVal);
            return;
        }
        case OP_conststr: {
            UStrIdx strIdx = static_cast<ConststrNode *>(e)->GetStrIdx();
            OutputUsrStr(strIdx);
            return;
        }
        case OP_addroflabel: {
            AddroflabelNode *lNode = static_cast<AddroflabelNode *>(e);
            OutputLabel(lNode->GetOffset());
            return;
        }
        case OP_addroffunc: {
            AddroffuncNode *addrNode = static_cast<AddroffuncNode *>(e);
            OutputFuncViaSym(addrNode->GetPUIdx());
            return;
        }
        case OP_sizeoftype: {
            SizeoftypeNode *sot = static_cast<SizeoftypeNode *>(e);
            OutputType(sot->GetTyIdx());
            return;
        }
        case OP_addrof:
        case OP_addrofoff:
        case OP_dread:
        case OP_dreadoff: {
            StIdx stIdx;
            if (e->GetOpCode() == OP_addrof || e->GetOpCode() == OP_dread) {
                AddrofNode *drNode = static_cast<AddrofNode *>(e);
                WriteNum(drNode->GetFieldID());
                stIdx = drNode->GetStIdx();
            } else {
                DreadoffNode *droff = static_cast<DreadoffNode *>(e);
                WriteNum(droff->offset);
                stIdx = droff->stIdx;
            }
            WriteNum(stIdx.Scope());
            if (stIdx.Islocal()) {
                OutputLocalSymbol(curFunc->GetLocalOrGlobalSymbol(stIdx));
            } else {
                OutputSymbol(curFunc->GetLocalOrGlobalSymbol(stIdx));
            }
            return;
        }
        case OP_regread: {
            RegreadNode *regreadNode = static_cast<RegreadNode *>(e);
            MIRPreg *preg = curFunc->GetPregTab()->PregFromPregIdx(regreadNode->GetRegIdx());
            OutputPreg(preg);
            return;
        }
        case OP_gcmalloc:
        case OP_gcpermalloc:
        case OP_stackmalloc: {
            GCMallocNode *gcNode = static_cast<GCMallocNode *>(e);
            OutputType(gcNode->GetTyIdx());
            return;
        }
        // unary
        case OP_ceil:
        case OP_cvt:
        case OP_floor:
        case OP_trunc: {
            TypeCvtNode *typecvtNode = static_cast<TypeCvtNode *>(e);
            Write(static_cast<uint8>(typecvtNode->FromType()));
            break;
        }
        case OP_retype: {
            RetypeNode *retypeNode = static_cast<RetypeNode *>(e);
            OutputType(retypeNode->GetTyIdx());
            break;
        }
        case OP_iread:
        case OP_iaddrof: {
            IreadNode *irNode = static_cast<IreadNode *>(e);
            OutputType(irNode->GetTyIdx());
            WriteNum(irNode->GetFieldID());
            break;
        }
        case OP_ireadoff: {
            IreadoffNode *irNode = static_cast<IreadoffNode *>(e);
            WriteNum(irNode->GetOffset());
            break;
        }
        case OP_ireadfpoff: {
            IreadFPoffNode *irNode = static_cast<IreadFPoffNode *>(e);
            WriteNum(irNode->GetOffset());
            break;
        }
        case OP_sext:
        case OP_zext:
        case OP_extractbits: {
            ExtractbitsNode *extNode = static_cast<ExtractbitsNode *>(e);
            Write(extNode->GetBitsOffset());
            Write(extNode->GetBitsSize());
            break;
        }
        case OP_depositbits: {
            DepositbitsNode *dbNode = static_cast<DepositbitsNode *>(e);
            Write(dbNode->GetBitsOffset());
            Write(dbNode->GetBitsSize());
            break;
        }
        case OP_gcmallocjarray:
        case OP_gcpermallocjarray: {
            JarrayMallocNode *gcNode = static_cast<JarrayMallocNode *>(e);
            OutputType(gcNode->GetTyIdx());
            break;
        }
        // binary
        case OP_sub:
        case OP_mul:
        case OP_div:
        case OP_rem:
        case OP_ashr:
        case OP_lshr:
        case OP_shl:
        case OP_max:
        case OP_min:
        case OP_band:
        case OP_bior:
        case OP_bxor:
        case OP_cand:
        case OP_cior:
        case OP_land:
        case OP_lior:
        case OP_add: {
            break;
        }
        case OP_eq:
        case OP_ne:
        case OP_lt:
        case OP_gt:
        case OP_le:
        case OP_ge:
        case OP_cmpg:
        case OP_cmpl:
        case OP_cmp: {
            CompareNode *cmpNode = static_cast<CompareNode *>(e);
            Write(static_cast<uint8>(cmpNode->GetOpndType()));
            break;
        }
        case OP_resolveinterfacefunc:
        case OP_resolvevirtualfunc: {
            ResolveFuncNode *rsNode = static_cast<ResolveFuncNode *>(e);
            OutputFuncViaSym(rsNode->GetPuIdx());
            break;
        }
        // ternary
        case OP_select: {
            break;
        }
        // nary
        case OP_array: {
            ArrayNode *arrNode = static_cast<ArrayNode *>(e);
            OutputType(arrNode->GetTyIdx());
            Write(static_cast<int8>(arrNode->GetBoundsCheck()));
            WriteNum(static_cast<int64>(arrNode->NumOpnds()));
            break;
        }
        case OP_intrinsicop: {
            IntrinsicopNode *intrnNode = static_cast<IntrinsicopNode *>(e);
            WriteNum(intrnNode->GetIntrinsic());
            WriteNum(static_cast<int64>(intrnNode->NumOpnds()));
            break;
        }
        case OP_intrinsicopwithtype: {
            IntrinsicopNode *intrnNode = static_cast<IntrinsicopNode *>(e);
            WriteNum(intrnNode->GetIntrinsic());
            OutputType(intrnNode->GetTyIdx());
            WriteNum(static_cast<int64>(intrnNode->NumOpnds()));
            break;
        }
        default:
            break;
    }
    for (uint32 i = 0; i < e->NumOpnds(); ++i) {
        OutputExpression(e->Opnd(i));
    }
}

static SrcPosition lastOutputSrcPosition;

void BinaryMplExport::OutputSrcPos(const SrcPosition &pos)
{
    if (!mod.IsWithDbgInfo()) {
        return;
    }
    if (pos.FileNum() == 0 || pos.LineNum() == 0) {  // error case, so output 0
        WriteNum(lastOutputSrcPosition.RawData());
        WriteNum(lastOutputSrcPosition.LineNum());
        return;
    }
    WriteNum(pos.RawData());
    WriteNum(pos.LineNum());
    lastOutputSrcPosition = pos;
}

void BinaryMplExport::OutputReturnValues(const CallReturnVector *retv)
{
    WriteNum(kBinReturnvals);
    WriteNum(static_cast<int64>(retv->size()));
    for (uint32 i = 0; i < retv->size(); i++) {
        RegFieldPair rfp = (*retv)[i].second;
        if (rfp.IsReg()) {
            MIRPreg *preg = curFunc->GetPregTab()->PregFromPregIdx(rfp.GetPregIdx());
            OutputPreg(preg);
        } else {
            WriteNum(0);
            WriteNum((rfp.GetFieldID()));
            OutputLocalSymbol(curFunc->GetLocalOrGlobalSymbol((*retv)[i].first));
        }
    }
}

void BinaryMplExport::OutputBlockNode(BlockNode *block)
{
    WriteNum(kBinNodeBlock);
    if (!block->GetStmtNodes().empty()) {
        OutputSrcPos(block->GetSrcPos());
    } else {
        OutputSrcPos(SrcPosition());  // output 0
    }
    int32 num = 0;
    uint64 idx = buf.size();
    ExpandFourBuffSize();  // place holder, Fixup later
    for (StmtNode *s = block->GetFirst(); s; s = s->GetNext()) {
        bool doneWithOpnds = false;
        OutputSrcPos(s->GetSrcPos());
        WriteNum(s->GetOpCode());
        switch (s->GetOpCode()) {
            case OP_dassign:
            case OP_dassignoff: {
                StIdx stIdx;
                if (s->GetOpCode() == OP_dassign) {
                    DassignNode *dass = static_cast<DassignNode *>(s);
                    WriteNum(dass->GetFieldID());
                    stIdx = dass->GetStIdx();
                } else {
                    DassignoffNode *dassoff = static_cast<DassignoffNode *>(s);
                    WriteNum(dassoff->GetPrimType());
                    WriteNum(dassoff->offset);
                    stIdx = dassoff->stIdx;
                }
                WriteNum(stIdx.Scope());
                if (stIdx.Islocal()) {
                    OutputLocalSymbol(curFunc->GetLocalOrGlobalSymbol(stIdx));
                } else {
                    OutputSymbol(curFunc->GetLocalOrGlobalSymbol(stIdx));
                }
                break;
            }
            case OP_regassign: {
                RegassignNode *rass = static_cast<RegassignNode *>(s);
                Write(static_cast<uint8>(rass->GetPrimType()));
                MIRPreg *preg = curFunc->GetPregTab()->PregFromPregIdx(rass->GetRegIdx());
                OutputPreg(preg);
                break;
            }
            case OP_iassign: {
                IassignNode *iass = static_cast<IassignNode *>(s);
                OutputType(iass->GetTyIdx());
                WriteNum(iass->GetFieldID());
                break;
            }
            case OP_iassignoff: {
                IassignoffNode *iassoff = static_cast<IassignoffNode *>(s);
                Write(static_cast<uint8>(iassoff->GetPrimType()));
                WriteNum(iassoff->GetOffset());
                break;
            }
            case OP_iassignspoff:
            case OP_iassignfpoff: {
                IassignFPoffNode *iassfpoff = static_cast<IassignFPoffNode *>(s);
                Write(static_cast<uint8>(iassfpoff->GetPrimType()));
                WriteNum(iassfpoff->GetOffset());
                break;
            }
            case OP_blkassignoff: {
                BlkassignoffNode *bass = static_cast<BlkassignoffNode *>(s);
                int32 offsetAlign = (bass->offset << 4) | bass->alignLog2;
                WriteNum(offsetAlign);
                WriteNum(bass->blockSize);
                break;
            }
            case OP_call:
            case OP_virtualcall:
            case OP_virtualicall:
            case OP_superclasscall:
            case OP_interfacecall:
            case OP_interfaceicall:
            case OP_customcall:
            case OP_polymorphiccall: {
                CallNode *callnode = static_cast<CallNode *>(s);
                OutputFuncViaSym(callnode->GetPUIdx());
                if (s->GetOpCode() == OP_polymorphiccall) {
                    OutputType(static_cast<CallNode *>(callnode)->GetTyIdx());
                }
                WriteNum(static_cast<int64>(s->NumOpnds()));
                break;
            }
            case OP_callassigned:
            case OP_virtualcallassigned:
            case OP_virtualicallassigned:
            case OP_superclasscallassigned:
            case OP_interfacecallassigned:
            case OP_interfaceicallassigned:
            case OP_customcallassigned: {
                CallNode *callnode = static_cast<CallNode *>(s);
                OutputFuncViaSym(callnode->GetPUIdx());
                OutputReturnValues(&callnode->GetReturnVec());
                WriteNum(static_cast<int64>(s->NumOpnds()));
                break;
            }
            case OP_polymorphiccallassigned: {
                CallNode *callnode = static_cast<CallNode *>(s);
                OutputFuncViaSym(callnode->GetPUIdx());
                OutputType(callnode->GetTyIdx());
                OutputReturnValues(&callnode->GetReturnVec());
                WriteNum(static_cast<int64>(s->NumOpnds()));
                break;
            }
            case OP_icallproto:
            case OP_icall: {
                IcallNode *icallnode = static_cast<IcallNode *>(s);
                OutputType(icallnode->GetRetTyIdx());
                WriteNum(static_cast<int64>(s->NumOpnds()));
                break;
            }
            case OP_icallprotoassigned:
            case OP_icallassigned: {
                IcallNode *icallnode = static_cast<IcallNode *>(s);
                OutputType(icallnode->GetRetTyIdx());
                OutputReturnValues(&icallnode->GetReturnVec());
                WriteNum(static_cast<int64>(s->NumOpnds()));
                break;
            }
            case OP_intrinsiccall:
            case OP_xintrinsiccall: {
                IntrinsiccallNode *intrnNode = static_cast<IntrinsiccallNode *>(s);
                WriteNum(intrnNode->GetIntrinsic());
                WriteNum(static_cast<int64>(s->NumOpnds()));
                break;
            }
            case OP_intrinsiccallassigned:
            case OP_xintrinsiccallassigned: {
                IntrinsiccallNode *intrnNode = static_cast<IntrinsiccallNode *>(s);
                WriteNum(intrnNode->GetIntrinsic());
                OutputReturnValues(&intrnNode->GetReturnVec());
                WriteNum(static_cast<int64>(s->NumOpnds()));
                break;
            }
            case OP_intrinsiccallwithtype: {
                IntrinsiccallNode *intrnNode = static_cast<IntrinsiccallNode *>(s);
                WriteNum(intrnNode->GetIntrinsic());
                OutputType(intrnNode->GetTyIdx());
                WriteNum(static_cast<int64>(s->NumOpnds()));
                break;
            }
            case OP_intrinsiccallwithtypeassigned: {
                IntrinsiccallNode *intrnNode = static_cast<IntrinsiccallNode *>(s);
                WriteNum(intrnNode->GetIntrinsic());
                OutputType(intrnNode->GetTyIdx());
                OutputReturnValues(&intrnNode->GetReturnVec());
                WriteNum(static_cast<int64>(s->NumOpnds()));
                break;
            }
            case OP_syncenter:
            case OP_syncexit:
            case OP_return: {
                WriteNum(static_cast<int64>(s->NumOpnds()));
                break;
            }
            case OP_jscatch:
            case OP_cppcatch:
            case OP_finally:
            case OP_endtry:
            case OP_cleanuptry:
            case OP_retsub:
            case OP_membaracquire:
            case OP_membarrelease:
            case OP_membarstorestore:
            case OP_membarstoreload: {
                break;
            }
            case OP_eval:
            case OP_throw:
            case OP_free:
            case OP_decref:
            case OP_incref:
            case OP_decrefreset:
                CASE_OP_ASSERT_NONNULL
            case OP_igoto: {
                break;
            }
            case OP_label: {
                LabelNode *lNode = static_cast<LabelNode *>(s);
                OutputLabel(lNode->GetLabelIdx());
                break;
            }
            case OP_goto:
            case OP_gosub: {
                GotoNode *gtoNode = static_cast<GotoNode *>(s);
                OutputLabel(gtoNode->GetOffset());
                break;
            }
            case OP_brfalse:
            case OP_brtrue: {
                CondGotoNode *cgotoNode = static_cast<CondGotoNode *>(s);
                OutputLabel(cgotoNode->GetOffset());
                break;
            }
            case OP_switch: {
                SwitchNode *swNode = static_cast<SwitchNode *>(s);
                OutputLabel(swNode->GetDefaultLabel());
                WriteNum(static_cast<int64>(swNode->GetSwitchTable().size()));
                for (CasePair cpair : swNode->GetSwitchTable()) {
                    WriteNum(cpair.first);
                    OutputLabel(cpair.second);
                }
                break;
            }
            case OP_rangegoto: {
                RangeGotoNode *rgoto = static_cast<RangeGotoNode *>(s);
                WriteNum(rgoto->GetTagOffset());
                WriteNum(static_cast<int64>(rgoto->GetRangeGotoTable().size()));
                for (SmallCasePair cpair : rgoto->GetRangeGotoTable()) {
                    WriteNum(cpair.first);
                    OutputLabel(cpair.second);
                }
                break;
            }
            case OP_jstry: {
                JsTryNode *tryNode = static_cast<JsTryNode *>(s);
                OutputLabel(tryNode->GetCatchOffset());
                OutputLabel(tryNode->GetFinallyOffset());
                break;
            }
            case OP_cpptry:
            case OP_try: {
                TryNode *tryNode = static_cast<TryNode *>(s);
                WriteNum(static_cast<int64>(tryNode->GetOffsetsCount()));
                for (LabelIdx lidx : tryNode->GetOffsets()) {
                    OutputLabel(lidx);
                }
                break;
            }
            case OP_catch: {
                CatchNode *catchNode = static_cast<CatchNode *>(s);
                WriteNum(static_cast<int64>(catchNode->GetExceptionTyIdxVec().size()));
                for (TyIdx tidx : catchNode->GetExceptionTyIdxVec()) {
                    OutputType(tidx);
                }
                break;
            }
            case OP_comment: {
                string str(static_cast<CommentNode *>(s)->GetComment().c_str());
                WriteAsciiStr(str);
                break;
            }
            case OP_dowhile:
            case OP_while: {
                WhileStmtNode *whileNode = static_cast<WhileStmtNode *>(s);
                OutputBlockNode(whileNode->GetBody());
                OutputExpression(whileNode->Opnd());
                doneWithOpnds = true;
                break;
            }
            case OP_if: {
                IfStmtNode *ifNode = static_cast<IfStmtNode *>(s);
                bool hasElsePart = ifNode->GetElsePart() != nullptr;
                WriteNum(static_cast<int64>(hasElsePart));
                OutputBlockNode(ifNode->GetThenPart());
                if (hasElsePart) {
                    OutputBlockNode(ifNode->GetElsePart());
                }
                OutputExpression(ifNode->Opnd());
                doneWithOpnds = true;
                break;
            }
            case OP_block: {
                BlockNode *blockNode = static_cast<BlockNode *>(s);
                OutputBlockNode(blockNode);
                doneWithOpnds = true;
                break;
            }
            case OP_asm: {
                AsmNode *asmNode = static_cast<AsmNode *>(s);
                WriteNum(asmNode->qualifiers);
                string str(asmNode->asmString.c_str());
                WriteAsciiStr(str);
                // the outputs
                size_t count = asmNode->asmOutputs.size();
                WriteNum(static_cast<int64>(count));
                for (size_t i = 0; i < count; ++i) {
                    OutputUsrStr(asmNode->outputConstraints[i]);
                }
                OutputReturnValues(&asmNode->asmOutputs);
                // the clobber list
                count = asmNode->clobberList.size();
                WriteNum(static_cast<int64>(count));
                for (size_t i = 0; i < count; ++i) {
                    OutputUsrStr(asmNode->clobberList[i]);
                }
                // the labels
                count = asmNode->gotoLabels.size();
                WriteNum(static_cast<int64>(count));
                for (size_t i = 0; i < count; ++i) {
                    OutputLabel(asmNode->gotoLabels[i]);
                }
                // the inputs
                WriteNum(asmNode->NumOpnds());
                for (uint8 i = 0; i < asmNode->numOpnds; ++i) {
                    OutputUsrStr(asmNode->inputConstraints[i]);
                }
                break;
            }
            default:
                CHECK_FATAL(false, "Unhandled opcode %d", s->GetOpCode());
                break;
        }
        num++;
        if (!doneWithOpnds) {
            for (uint32 i = 0; i < s->NumOpnds(); ++i) {
                OutputExpression(s->Opnd(i));
            }
        }
    }
    Fixup(idx, num);
}

void BinaryMplExport::WriteFunctionBodyField(uint64 contentIdx, std::unordered_set<std::string> *dumpFuncSet)
{
    Fixup(contentIdx, static_cast<int32>(buf.size()));
    WriteNum(kBinFunctionBodyStart);
    uint64 totalSizeIdx = buf.size();
    ExpandFourBuffSize();  /// total size of this field to ~BIN_FUNCTIONBODY_START
    uint64 outFunctionBodySizeIdx = buf.size();
    ExpandFourBuffSize();  /// size of outFunctionBody
    int32 size = 0;

    if (not2mplt) {
        for (MIRFunction *func : GetMIRModule().GetFunctionList()) {
            curFunc = func;
            if (func->GetAttr(FUNCATTR_optimized)) {
                continue;
            }
            if (func->GetCodeMemPool() == nullptr || func->GetBody() == nullptr) {
                continue;
            }
            if (dumpFuncSet != nullptr && !dumpFuncSet->empty()) {
                // output only if this func matches any name in *dumpFuncSet
                const std::string &name = func->GetName();
                bool matched = false;
                for (std::string elem : *dumpFuncSet) {
                    if (name.find(elem.c_str()) != string::npos) {
                        matched = true;
                        break;
                    }
                }
                if (!matched) {
                    continue;
                }
            }
            localSymMark.clear();
            localSymMark[nullptr] = 0;
            localPregMark.clear();
            localPregMark[nullptr] = 0;
            labelMark.clear();
            labelMark[0] = 0;
            OutputFunction(func->GetPuidx());
            CHECK_FATAL(func->GetBody() != nullptr, "WriteFunctionBodyField: no function body");
            OutputFuncIdInfo(func);
            OutputLocalTypeNameTab(func->GetTypeNameTab());
            OutputFormalsStIdx(func);
            if (mod.GetFlavor() < kMmpl) {
                OutputAliasMap(func->GetAliasVarMap());
            }
            lastOutputSrcPosition = SrcPosition();
            OutputBlockNode(func->GetBody());
            size++;
        }
    }

    Fixup(totalSizeIdx, static_cast<int32>(buf.size() - totalSizeIdx));
    Fixup(outFunctionBodySizeIdx, size);
    return;
}
}  // namespace maple
