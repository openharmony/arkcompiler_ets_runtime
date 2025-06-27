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

#include "x64_MPISel.h"
#include "x64_isa_tbl.h"
#include "x64_cg.h"

namespace maplebe {
/* Field-ID 0 is assigned to the top level structure. (Field-ID also defaults to 0 if it is not a structure.) */
MemOperand &X64MPIsel::GetOrCreateMemOpndFromSymbol(const MIRSymbol &symbol, FieldID fieldId) const
{
    PrimType symType;
    int32 fieldOffset = 0;
    CHECK_FATAL(fieldId == 0, "fieldId must be 0");
    symType = symbol.GetType()->GetPrimType();
    uint32 opndSz = GetPrimTypeBitSize(symType);
    return GetOrCreateMemOpndFromSymbol(symbol, opndSz, fieldOffset);
}
MemOperand &X64MPIsel::GetOrCreateMemOpndFromSymbol(const MIRSymbol &symbol, uint32 opndSize, int64 offset) const
{
    MIRStorageClass storageClass = symbol.GetStorageClass();
    MemOperand *result = nullptr;
    RegOperand *stackBaseReg = nullptr;
    if ((storageClass == kScAuto) || (storageClass == kScFormal)) {
        auto *symloc = static_cast<X64SymbolAlloc *>(cgFunc->GetMemlayout()->GetSymAllocInfo(symbol.GetStIndex()));
        DEBUG_ASSERT(symloc != nullptr, "sym loc should have been defined");
        stackBaseReg = static_cast<X64CGFunc *>(cgFunc)->GetBaseReg(*symloc);
        int stOfst = cgFunc->GetBaseOffset(*symloc);
        /* Create field symbols in aggregate structure */
        result = &GetCurFunc()->GetOpndBuilder()->CreateMem(opndSize);
        result->SetBaseRegister(*stackBaseReg);
        result->SetOffsetOperand(GetCurFunc()->GetOpndBuilder()->CreateImm(k64BitSize, stOfst + offset));
        CHECK_FATAL(result != nullptr, "NIY");
        return *result;
    }
    if ((storageClass == kScGlobal) || (storageClass == kScExtern) || (storageClass == kScPstatic) ||
        (storageClass == kScFstatic)) {
        stackBaseReg = &GetCurFunc()->GetOpndBuilder()->CreatePReg(x64::RIP, k64BitSize, kRegTyInt);
        result = &GetCurFunc()->GetOpndBuilder()->CreateMem(opndSize);
        ImmOperand &stOfstOpnd = GetCurFunc()->GetOpndBuilder()->CreateImm(symbol, offset, 0);
        result->SetBaseRegister(*stackBaseReg);
        result->SetOffsetOperand(stOfstOpnd);
        CHECK_FATAL(result != nullptr, "NIY");
        return *result;
    }
    CHECK_FATAL(false, "NIY");
    return *result;
}

void X64MPIsel::SelectReturn(NaryStmtNode &retNode, Operand &opnd)
{
    MIRType *retType = cgFunc->GetFunction().GetReturnType();
    X64CallConvImpl retLocator(cgFunc->GetBecommon());
    CCLocInfo retMech;
    retLocator.LocateRetVal(*retType, retMech);
    if (retMech.GetRegCount() == 0) {
        return;
    }
    std::vector<RegOperand *> retRegs;
    PrimType oriPrimType = retMech.GetPrimTypeOfReg0();
    regno_t retReg = retMech.GetReg0();
    DEBUG_ASSERT(retReg != kRinvalid, "NIY");
    RegOperand &retOpnd = cgFunc->GetOpndBuilder()->CreatePReg(retReg, GetPrimTypeBitSize(oriPrimType),
                                                               cgFunc->GetRegTyFromPrimTy(oriPrimType));
    retRegs.push_back(&retOpnd);
    SelectCopy(retOpnd, opnd, oriPrimType, retNode.Opnd(0)->GetPrimType());
    /* for optimization ,insert pseudo ret ,in case rax,rdx is removed*/
    SelectPseduoForReturn(retRegs);
}

void X64MPIsel::SelectPseduoForReturn(std::vector<RegOperand *> &retRegs)
{
    for (auto retReg : retRegs) {
        MOperator mop = x64::MOP_pseudo_ret_int;
        Insn &pInsn = cgFunc->GetInsnBuilder()->BuildInsn(mop, X64CG::kMd[mop]);
        cgFunc->GetCurBB()->AppendInsn(pInsn);
        pInsn.AddOpndChain(*retReg);
    }
}

void X64MPIsel::SelectReturn()
{
    /* jump to epilogue */
    MOperator mOp = x64::MOP_jmpq_l;
    LabelNode *endLabel = cgFunc->GetEndLabel();
    auto endLabelName = ".L." + std::to_string(cgFunc->GetUniqueID()) + "__" + std::to_string(endLabel->GetLabelIdx());
    LabelOperand &targetOpnd = cgFunc->GetOpndBuilder()->CreateLabel(endLabelName.c_str(), endLabel->GetLabelIdx());
    Insn &jmpInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
    jmpInsn.AddOpndChain(targetOpnd);
    cgFunc->GetCurBB()->AppendInsn(jmpInsn);
    cgFunc->GetExitBBsVec().emplace_back(cgFunc->GetCurBB());
}

/*
 * SelectParmList generates an instrunction for each of the parameters
 * to load the parameter value into the corresponding register.
 * We return a list of registers to the call instruction because
 * they may be needed in the register allocation phase.
 * fp Num is a return value which is the number of vector
 * registers used;
 */
void X64MPIsel::SelectParmList(StmtNode &naryNode, ListOperand &srcOpnds, uint32 &fpNum)
{
    paramPassByReg.clear();
    fpNum = 0;
    /* for IcallNode, the 0th operand is the function pointer */
    size_t argBegin = 0;
    if (naryNode.GetOpCode() == OP_icall || naryNode.GetOpCode() == OP_icallproto ||
        naryNode.GetOpCode() == OP_tailicall) {
        ++argBegin;
    }

    MIRFunction *callee = nullptr;
    if (naryNode.GetOpCode() == OP_call) {
        PUIdx calleePuIdx = static_cast<CallNode &>(naryNode).GetPUIdx();
        callee = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(calleePuIdx);
    }
    X64CallConvImpl parmLocator(cgFunc->GetBecommon(), X64CallConvImpl::GetCallConvKind(naryNode));
    CCLocInfo ploc;
    for (size_t i = argBegin; i < naryNode.NumOpnds(); ++i) {
        BaseNode *argExpr = naryNode.Opnd(i);
        DEBUG_ASSERT(argExpr != nullptr, "not null check");
        PrimType primType = argExpr->GetPrimType();
        DEBUG_ASSERT(primType != PTY_void, "primType should not be void");
        bool isArgUnused = (callee != nullptr && callee->GetFuncDesc().IsArgUnused(i));

        Operand *argOpnd = HandleExpr(naryNode, *argExpr);
        DEBUG_ASSERT(argOpnd != nullptr, "not null check");
        MIRType *mirType = GlobalTables::GetTypeTable().GetTypeTable()[static_cast<uint32>(primType)];
        parmLocator.LocateNextParm(*mirType, ploc);

        /* skip unused args */
        if (isArgUnused) {
            continue;
        }

        if (ploc.reg0 != x64::kRinvalid) {
            /* load to the register. */
            RegOperand &parmRegOpnd = cgFunc->GetOpndBuilder()->CreatePReg(ploc.reg0, GetPrimTypeBitSize(primType),
                                                                           cgFunc->GetRegTyFromPrimTy(primType));
            paramPassByReg.push_back({&parmRegOpnd, argOpnd, primType});
            if (x64::IsFPSIMDRegister(static_cast<X64reg>(ploc.reg0))) {
                fpNum++;
            }
        } else {
            /* load to stack memory */
            RegOperand &baseOpnd =
                cgFunc->GetOpndBuilder()->CreatePReg(x64::RSP, k64BitSize, cgFunc->GetRegTyFromPrimTy(primType));
            MemOperand &actMemOpnd =
                cgFunc->GetOpndBuilder()->CreateMem(baseOpnd, ploc.memOffset, GetPrimTypeBitSize(primType));
            SelectCopy(actMemOpnd, *argOpnd, primType);
        }
        DEBUG_ASSERT(ploc.reg1 == 0, "SelectCall NIY");
    }

    /* param pass by reg */
    for (auto [regOpnd, argOpnd, primType] : paramPassByReg) {
        DEBUG_ASSERT(regOpnd != nullptr, "not null check");
        DEBUG_ASSERT(argOpnd != nullptr, "not null check");
        SelectCopy(*regOpnd, *argOpnd, primType);
        srcOpnds.PushOpnd(*regOpnd);
    }
}

RegOperand &X64MPIsel::SelectSpecialRegread(PregIdx pregIdx, PrimType primType)
{
    switch (-pregIdx) {
        case kSregFp: {
            return cgFunc->GetOpndBuilder()->CreatePReg(x64::RFP, k64BitSize, cgFunc->GetRegTyFromPrimTy(primType));
        }
        case kSregSp: {
            return cgFunc->GetOpndBuilder()->CreatePReg(x64::RSP, k64BitSize, cgFunc->GetRegTyFromPrimTy(primType));
        }
        default: {
            CHECK_FATAL(false, "ERROR: Not supported special register!");
        }
    }
}

Operand *X64MPIsel::SelectFloatingConst(MIRConst &floatingConst, PrimType primType) const
{
    CHECK_FATAL(primType == PTY_f64 || primType == PTY_f32, "wrong const");
    uint32 labelIdxTmp = cgFunc->GetLabelIdx();
    Operand *result = nullptr;
    if (primType == PTY_f64) {
        result = SelectLiteral(static_cast<MIRDoubleConst &>(floatingConst), cgFunc->GetFunction(), labelIdxTmp++);
    } else {
        result = SelectLiteral(static_cast<MIRFloatConst &>(floatingConst), cgFunc->GetFunction(), labelIdxTmp++);
    }
    cgFunc->SetLabelIdx(labelIdxTmp);
    return result;
}

Insn &X64MPIsel::AppendCall(x64::X64MOP_t mOp, Operand &targetOpnd, ListOperand &paramOpnds, ListOperand &retOpnds)
{
    Insn &callInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
    callInsn.AddOpndChain(targetOpnd).AddOpndChain(paramOpnds).AddOpndChain(retOpnds);
    cgFunc->GetCurBB()->AppendInsn(callInsn);
    cgFunc->GetCurBB()->SetHasCall();
    cgFunc->GetFunction().SetHasCall();
    return callInsn;
}

void X64MPIsel::SelectCalleeReturn(MIRType *retType, ListOperand &retOpnds)
{
    if (retType == nullptr) {
        return;
    }
    auto retSize = retType->GetSize() * kBitsPerByte;
    if (retSize <= k128BitSize) {
        if (retSize > k0BitSize) {
            retOpnds.PushOpnd(cgFunc->GetOpndBuilder()->CreatePReg(x64::RAX, k64BitSize, kRegTyInt));
        }
        if (retSize > k64BitSize) {
            retOpnds.PushOpnd(cgFunc->GetOpndBuilder()->CreatePReg(x64::RDX, k64BitSize, kRegTyInt));
        }
    }
}

void X64MPIsel::SelectCall(CallNode &callNode)
{
    MIRFunction *fn = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode.GetPUIdx());
    MIRSymbol *fsym = GlobalTables::GetGsymTable().GetSymbolFromStidx(fn->GetStIdx().Idx(), false);
    Operand &targetOpnd = cgFunc->GetOpndBuilder()->CreateFuncNameOpnd(*fsym);

    ListOperand &paramOpnds = cgFunc->GetOpndBuilder()->CreateList();
    uint32 fpNum = 0;
    SelectParmList(callNode, paramOpnds, fpNum);
    /* x64abi： rax = with variable arguments passes information about the number of vector registers used */
    if (fn->IsVarargs()) {
        ImmOperand &fpNumImm = cgFunc->GetOpndBuilder()->CreateImm(k64BitSize, fpNum);
        RegOperand &raxOpnd = cgFunc->GetOpndBuilder()->CreatePReg(x64::RAX, k64BitSize, kRegTyInt);
        SelectCopy(raxOpnd, fpNumImm, PTY_i64);
    }

    MIRType *retType = fn->GetReturnType();
    ListOperand &retOpnds = cgFunc->GetOpndBuilder()->CreateList();
    SelectCalleeReturn(retType, retOpnds);

    Insn &callInsn = AppendCall(x64::MOP_callq_l, targetOpnd, paramOpnds, retOpnds);
    if (retType != nullptr) {
        callInsn.SetRetSize(static_cast<uint32>(retType->GetSize()));
        callInsn.SetIsCallReturnUnsigned(IsUnsignedInteger(retType->GetPrimType()));
    }
    const auto &deoptBundleInfo = callNode.GetDeoptBundleInfo();
    for (const auto &elem : deoptBundleInfo) {
        auto valueKind = elem.second.GetMapleValueKind();
        if (valueKind == MapleValue::kPregKind) {
            auto *opnd = cgFunc->GetOrCreateRegOpndFromPregIdx(elem.second.GetPregIdx(), PTY_ref);
            callInsn.AddDeoptBundleInfo(elem.first, *opnd);
        } else if (valueKind == MapleValue::kConstKind) {
            auto *opnd = SelectIntConst(static_cast<const MIRIntConst &>(elem.second.GetConstValue()), PTY_i32);
            callInsn.AddDeoptBundleInfo(elem.first, *opnd);
        } else {
            CHECK_FATAL(false, "not supported currently");
        }
    }
    cgFunc->AppendStackMapInsn(callInsn);
}

void X64MPIsel::SelectIcall(IcallNode &iCallNode)
{
    Operand *opnd0 = HandleExpr(iCallNode, *iCallNode.GetNopndAt(0));
    RegOperand &targetOpnd = SelectCopy2Reg(*opnd0, iCallNode.Opnd(0)->GetPrimType());
    ListOperand &paramOpnds = cgFunc->GetOpndBuilder()->CreateList();
    uint32 fpNum = 0;
    SelectParmList(iCallNode, paramOpnds, fpNum);

    MIRType *retType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iCallNode.GetRetTyIdx());
    if (iCallNode.GetOpCode() == OP_icallproto) {
        CHECK_FATAL((retType->GetKind() == kTypeFunction), "NIY, must be func");
        auto calleeType = static_cast<MIRFuncType *>(retType);
        retType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(calleeType->GetRetTyIdx());
    }
    ListOperand &retOpnds = cgFunc->GetOpndBuilder()->CreateList();
    SelectCalleeReturn(retType, retOpnds);

    Insn &callInsn = AppendCall(x64::MOP_callq_r, targetOpnd, paramOpnds, retOpnds);
    if (retType != nullptr) {
        callInsn.SetRetSize(static_cast<uint32>(retType->GetSize()));
        callInsn.SetIsCallReturnUnsigned(IsUnsignedInteger(retType->GetPrimType()));
    }
    const auto &deoptBundleInfo = iCallNode.GetDeoptBundleInfo();
    for (const auto &elem : deoptBundleInfo) {
        auto valueKind = elem.second.GetMapleValueKind();
        if (valueKind == MapleValue::kPregKind) {
            auto *opnd = cgFunc->GetOrCreateRegOpndFromPregIdx(elem.second.GetPregIdx(), PTY_ref);
            callInsn.AddDeoptBundleInfo(elem.first, *opnd);
        } else if (valueKind == MapleValue::kConstKind) {
            auto *opnd = SelectIntConst(static_cast<const MIRIntConst &>(elem.second.GetConstValue()), PTY_i32);
            callInsn.AddDeoptBundleInfo(elem.first, *opnd);
        } else {
            CHECK_FATAL(false, "not supported currently");
        }
    }
    cgFunc->AppendStackMapInsn(callInsn);
}

Operand &X64MPIsel::ProcessReturnReg(PrimType primType, int32 sReg)
{
    return GetTargetRetOperand(primType, sReg);
}

void X64MPIsel::SelectGoto(GotoNode &stmt)
{
    MOperator mOp = x64::MOP_jmpq_l;
    auto funcName = ".L." + std::to_string(cgFunc->GetUniqueID()) + "__" + std::to_string(stmt.GetOffset());
    LabelOperand &targetOpnd = cgFunc->GetOpndBuilder()->CreateLabel(funcName.c_str(), stmt.GetOffset());
    Insn &jmpInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
    cgFunc->GetCurBB()->AppendInsn(jmpInsn);
    jmpInsn.AddOpndChain(targetOpnd);
    cgFunc->GetCurBB()->SetKind(BB::kBBGoto);
    return;
}

void X64MPIsel::SelectOverFlowCall(const IntrinsiccallNode &intrnNode)
{
    DEBUG_ASSERT(intrnNode.NumOpnds() == kOpndNum2, "must be 2 operands");
    MIRIntrinsicID intrinsic = intrnNode.GetIntrinsic();
    // add
    PrimType type = intrnNode.Opnd(0)->GetPrimType();
    PrimType type2 = intrnNode.Opnd(1)->GetPrimType();
    CHECK_FATAL(type == PTY_i32 || type == PTY_u32, "only support i32 or u32 here");
    CHECK_FATAL(type2 == PTY_i32 || type2 == PTY_u32, "only support i32 or u32 here");
    RegOperand &opnd0 = SelectCopy2Reg(*HandleExpr(intrnNode, *intrnNode.Opnd(0)),
                                       intrnNode.Opnd(0)->GetPrimType()); /* first argument of intrinsic */
    RegOperand &opnd1 = SelectCopy2Reg(*HandleExpr(intrnNode, *intrnNode.Opnd(1)),
                                       intrnNode.Opnd(1)->GetPrimType()); /* first argument of intrinsic */
    RegOperand &resReg =
        cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(type), cgFunc->GetRegTyFromPrimTy(type));
    if (intrinsic == INTRN_ADD_WITH_OVERFLOW) {
        SelectAdd(resReg, opnd0, opnd1, type);
    } else if (intrinsic == INTRN_SUB_WITH_OVERFLOW) {
        SelectSub(resReg, opnd0, opnd1, type);
    } else if (intrinsic == INTRN_MUL_WITH_OVERFLOW) {
        SelectMpy(resReg, opnd0, opnd1, type);
    } else {
        CHECK_FATAL(false, "niy");
    }

    // store
    auto *p2nrets = &intrnNode.GetReturnVec();
    if (p2nrets->size() == k2ByteSize) {
        CHECK_NULL_FATAL(cgFunc->GetBecommon().GetMIRModule().CurFunction());
        PregIdx pregIdx = (*p2nrets)[0].second.GetPregIdx();
        MIRPreg *mirPreg = cgFunc->GetFunction().GetPregTab()->PregFromPregIdx(pregIdx);
        PrimType regType = mirPreg->GetPrimType();
        RegOperand &retReg = cgFunc->GetOpndBuilder()->CreateVReg(cgFunc->GetVirtualRegNOFromPseudoRegIdx(pregIdx),
            GetPrimTypeBitSize(regType), cgFunc->GetRegTyFromPrimTy(regType));
        SelectCopy(retReg, resReg, type);
        PregIdx pregIdx2 = (*p2nrets)[1].second.GetPregIdx();
        MIRPreg *mirPreg2 = cgFunc->GetFunction().GetPregTab()->PregFromPregIdx(pregIdx2);
        PrimType regType2 = mirPreg2->GetPrimType();
        RegOperand &retReg2 = cgFunc->GetOpndBuilder()->CreateVReg(cgFunc->GetVirtualRegNOFromPseudoRegIdx(pregIdx2),
            GetPrimTypeBitSize(regType2), cgFunc->GetRegTyFromPrimTy(regType2));
        Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(MOP_seto_r, X64CG::kMd[MOP_seto_r]);
        insn.AddOpndChain(retReg2);
        cgFunc->GetCurBB()->AppendInsn(insn);
    } else {
        CHECK_FATAL(false, "should not happen");
    }
    return;
}

void X64MPIsel::SelectPureCall(const IntrinsiccallNode &intrnNode)
{
    DEBUG_ASSERT(intrnNode.NumOpnds() == 6, "must be 6 operands");  // must be 6 operands
    ListOperand &srcOpnds = cgFunc->GetOpndBuilder()->CreateList();
    auto &callee = *intrnNode.Opnd(0);
    auto ptyp = callee.GetPrimType();
    RegOperand &calleeReg = SelectCopy2Reg(*HandleExpr(intrnNode, callee), ptyp);
    uint32 i = 1;
    for (; i < kSeventhReg; i++) {
        srcOpnds.PushOpnd(LoadOpndIntoPhysicalRegister(intrnNode, i));
    }
    // R11 is used in asm call
    srcOpnds.PushOpnd(cgFunc->GetOpndBuilder()->CreatePReg(x64::R11, GetPrimTypeBitSize(PTY_i64), kRegTyInt));
    MOperator mOp = x64::MOP_pure_call;
    Insn &callInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
    callInsn.AddOpndChain(calleeReg);
    callInsn.AddOpndChain(srcOpnds);
    cgFunc->GetCurBB()->AppendInsn(callInsn);
    return;
}

RegOperand &X64MPIsel::LoadOpndIntoPhysicalRegister(const IntrinsiccallNode &intrnNode, uint32 index)
{
    auto &opnd = *intrnNode.Opnd(index);
    auto ptyp = opnd.GetPrimType();
    RegOperand &opndReg = SelectCopy2Reg(*HandleExpr(intrnNode, opnd), ptyp);
    PRegNo regId;
    switch (index - 1) {
        case kFirstReg:
            regId = x64::RDI;
            break;
        case kSecondReg:
            regId = x64::RSI;
            break;
        case kThirdReg:
            regId = x64::RDX;
            break;
        case kFourthReg:
            regId = x64::RCX;
            break;
        case kFifthReg:
            regId = x64::R8;
            break;
        case kSixthReg:
            regId = x64::R9;
            break;
        default:
            CHECK_FATAL_FALSE("Unreachable!");
    }
    RegOperand &realReg = cgFunc->GetOpndBuilder()->CreatePReg(regId, GetPrimTypeBitSize(PTY_i64), kRegTyInt);
    SelectCopy(realReg, opndReg, ptyp, ptyp);
    return realReg;
}

void X64MPIsel::SelectIntrinsicCall(IntrinsiccallNode &intrinsiccallNode)
{
    MIRIntrinsicID intrinsic = intrinsiccallNode.GetIntrinsic();
    if (intrinsic == INTRN_ADD_WITH_OVERFLOW || intrinsic == INTRN_SUB_WITH_OVERFLOW ||
        intrinsic == INTRN_MUL_WITH_OVERFLOW) {
        SelectOverFlowCall(intrinsiccallNode);
        return;
    }
    if (intrinsic == maple::INTRN_JS_PURE_CALL) {
        SelectPureCall(intrinsiccallNode);
        return;
    }

    CHECK_FATAL(false, "Intrinsic %d: %s not implemented by the X64 CG.", intrinsic, GetIntrinsicName(intrinsic));
}

void X64MPIsel::SelectDeoptCall(CallNode &callNode)
{
    MIRFunction *fn = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode.GetPUIdx());
    MIRSymbol *fsym = GlobalTables::GetGsymTable().GetSymbolFromStidx(fn->GetStIdx().Idx(), false);
    Operand &targetOpnd = cgFunc->GetOpndBuilder()->CreateFuncNameOpnd(*fsym);

    ListOperand &paramOpnds = cgFunc->GetOpndBuilder()->CreateList();
    uint32 fpNum = 0;
    SelectParmList(callNode, paramOpnds, fpNum);
    /* x64abi： rax = with variable arguments passes information about the number of vector registers used */
    if (fn->IsVarargs()) {
        ImmOperand &fpNumImm = cgFunc->GetOpndBuilder()->CreateImm(k64BitSize, fpNum);
        RegOperand &raxOpnd = cgFunc->GetOpndBuilder()->CreatePReg(x64::RAX, k64BitSize, kRegTyInt);
        SelectCopy(raxOpnd, fpNumImm, PTY_i64);
    }
    ListOperand &retOpnds = cgFunc->GetOpndBuilder()->CreateList();
    Insn &callInsn = AppendCall(x64::MOP_callq_l, targetOpnd, paramOpnds, retOpnds);
    const auto &deoptBundleInfo = callNode.GetDeoptBundleInfo();
    for (const auto &elem : deoptBundleInfo) {
        auto valueKind = elem.second.GetMapleValueKind();
        if (valueKind == MapleValue::kPregKind) {
            auto *opnd = cgFunc->GetOrCreateRegOpndFromPregIdx(elem.second.GetPregIdx(), PTY_ref);
            callInsn.AddDeoptBundleInfo(elem.first, *opnd);
        } else if (valueKind == MapleValue::kConstKind) {
            auto *opnd = SelectIntConst(static_cast<const MIRIntConst &>(elem.second.GetConstValue()), PTY_i32);
            callInsn.AddDeoptBundleInfo(elem.first, *opnd);
        } else {
            CHECK_FATAL(false, "not supported currently");
        }
    }
    cgFunc->AppendStackMapInsn(callInsn);
}

void X64MPIsel::SelectTailICall(IcallNode &icallNode)
{
    Operand *opnd0 = HandleExpr(icallNode, *icallNode.GetNopndAt(0));
    RegOperand &targetOpnd = SelectCopy2Reg(*opnd0, icallNode.Opnd(0)->GetPrimType());
    ListOperand &paramOpnds = cgFunc->GetOpndBuilder()->CreateList();
    uint32 fpNum = 0;
    SelectParmList(icallNode, paramOpnds, fpNum);
    Insn &callInsn = cgFunc->GetInsnBuilder()->BuildInsn(x64::MOP_tail_callq_r, targetOpnd, paramOpnds);
    cgFunc->GetCurBB()->AppendInsn(callInsn);
}

void X64MPIsel::SelectRangeGoto(RangeGotoNode &rangeGotoNode, Operand &srcOpnd)
{
    MIRType *etype = GlobalTables::GetTypeTable().GetTypeFromTyIdx((TyIdx)PTY_a64);
    std::vector<uint64> sizeArray;
    const SmallCaseVector &switchTable = rangeGotoNode.GetRangeGotoTable();
    sizeArray.emplace_back(switchTable.size());
    MemPool *memPool = cgFunc->GetMemoryPool();
    MIRArrayType *arrayType = memPool->New<MIRArrayType>(etype->GetTypeIndex(), sizeArray);
    MIRAggConst *arrayConst = memPool->New<MIRAggConst>(cgFunc->GetMirModule(), *arrayType);
    for (const auto &itPair : switchTable) {
        LabelIdx labelIdx = itPair.second;
        cgFunc->GetCurBB()->PushBackRangeGotoLabel(labelIdx);
        MIRConst *mirConst = memPool->New<MIRLblConst>(labelIdx, cgFunc->GetFunction().GetPuidx(), *etype);
        arrayConst->AddItem(mirConst, 0);
    }
    MIRSymbol *lblSt = cgFunc->GetFunction().GetSymTab()->CreateSymbol(kScopeLocal);
    lblSt->SetStorageClass(kScFstatic);
    lblSt->SetSKind(kStConst);
    lblSt->SetTyIdx(arrayType->GetTypeIndex());
    lblSt->SetKonst(arrayConst);
    std::string lblStr(".L_");
    uint32 labelIdxTmp = cgFunc->GetLabelIdx();
    lblStr.append(std::to_string(cgFunc->GetUniqueID())).append("_LOCAL_CONST.").append(std::to_string(labelIdxTmp++));
    cgFunc->SetLabelIdx(labelIdxTmp);
    lblSt->SetNameStrIdx(lblStr);
    cgFunc->AddEmitSt(cgFunc->GetCurBB()->GetId(), *lblSt);
    ImmOperand &stOpnd = cgFunc->GetOpndBuilder()->CreateImm(*lblSt, 0, 0);
    /* get index */
    PrimType srcType = rangeGotoNode.Opnd(0)->GetPrimType();
    RegOperand &opnd0 = SelectCopy2Reg(srcOpnd, srcType);
    int32 minIdx = switchTable[0].first;
    ImmOperand &opnd1 =
        cgFunc->GetOpndBuilder()->CreateImm(GetPrimTypeBitSize(srcType), -minIdx - rangeGotoNode.GetTagOffset());
    RegOperand &indexOpnd = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(srcType), kRegTyInt);
    SelectAdd(indexOpnd, opnd0, opnd1, srcType);

    /* load the displacement into a register by accessing memory at base + index * 8 */
    /* mov .L_xxx_LOCAL_CONST.x(%baseReg, %indexOpnd, 8), %dstRegOpnd */
    MemOperand &dstMemOpnd = cgFunc->GetOpndBuilder()->CreateMem(GetPrimTypeBitSize(PTY_a64));
    RegOperand &baseReg = cgFunc->GetOpndBuilder()->CreatePReg(x64::RBP, GetPrimTypeBitSize(PTY_i64), kRegTyInt);
    dstMemOpnd.SetBaseRegister(baseReg);
    dstMemOpnd.SetIndexRegister(indexOpnd);
    dstMemOpnd.SetOffsetOperand(stOpnd);
    dstMemOpnd.SetScaleOperand(cgFunc->GetOpndBuilder()->CreateImm(baseReg.GetSize(), k8ByteSize));

    /* jumping to the absolute address which is stored in dstRegOpnd */
    MOperator mOp = x64::MOP_jmpq_m;
    Insn &jmpInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
    jmpInsn.AddOpndChain(dstMemOpnd);
    cgFunc->GetCurBB()->AppendInsn(jmpInsn);
}

/*
 * unorded   ZF, PF, CF  ==> 1,1,1
 * above     ZF, PF, CF  ==> 0,0,0
 * below     ZF, PF, CF  ==> 0,0,1
 * equal     ZF, PF, CF  ==> 1,0,0
 *
 * To distinguish between less than(only check whether CF = 1 or not) and unorderd(CF=1),
 * So ** judging gt/ge by swaping operands is used to represent lt/le in float**
 */
static X64MOP_t PickJmpInsn(Opcode brOp, Opcode cmpOp, bool isFloat, bool isSigned)
{
    switch (cmpOp) {
        case OP_ne:
            return (brOp == OP_brtrue) ? MOP_jne_l : MOP_je_l;
        case OP_eq:
            return (brOp == OP_brtrue) ? MOP_je_l : MOP_jne_l;
        case OP_lt:
            return (brOp == OP_brtrue) ? (isFloat ? MOP_ja_l : (isSigned ? MOP_jl_l : MOP_jb_l))
                                       : (isSigned ? MOP_jge_l : MOP_jae_l);
        case OP_le:
            return (brOp == OP_brtrue) ? (isFloat ? MOP_jae_l : (isSigned ? MOP_jle_l : MOP_jbe_l))
                                       : (isSigned ? MOP_jg_l : MOP_ja_l);
        case OP_gt:
            return (brOp == OP_brtrue) ? (isFloat ? MOP_ja_l : (isSigned ? MOP_jg_l : MOP_ja_l))
                                       : (isSigned ? MOP_jle_l : MOP_jbe_l);
        case OP_ge:
            return (brOp == OP_brtrue) ? (isSigned ? MOP_jge_l : MOP_jae_l) : (isSigned ? MOP_jl_l : MOP_jb_l);
        default:
            CHECK_FATAL(false, "PickJmpInsn error");
    }
}

/*
 * handle brfalse/brtrue op, opnd0 can be a compare node or non-compare node
 * such as a dread for example
 */
void X64MPIsel::SelectCondGoto(CondGotoNode &stmt, BaseNode &condNode, Operand &opnd0)
{
    Opcode opcode = stmt.GetOpCode();
    X64MOP_t jmpOperator = x64::MOP_begin;
    if (opnd0.IsImmediate()) {
        DEBUG_ASSERT(opnd0.IsIntImmediate(), "only support int immediate");
        DEBUG_ASSERT(opcode == OP_brtrue || opcode == OP_brfalse, "unsupported opcode");
        ImmOperand &immOpnd0 = static_cast<ImmOperand &>(opnd0);
        if ((opcode == OP_brtrue && !(immOpnd0.GetValue() != 0)) ||
            (opcode == OP_brfalse && !(immOpnd0.GetValue() == 0))) {
            return;
        }
        jmpOperator = x64::MOP_jmpq_l;
        cgFunc->SetCurBBKind(BB::kBBGoto);
    } else {
        PrimType primType;
        Opcode condOpcode = condNode.GetOpCode();
        // op_ne
        if (!kOpcodeInfo.IsCompare(condOpcode)) {
            primType = condNode.GetPrimType();
            ImmOperand &imm0 = cgFunc->GetOpndBuilder()->CreateImm(GetPrimTypeBitSize(primType), 0);
            SelectCmp(opnd0, imm0, primType);
            condOpcode = OP_ne;
        } else {
            primType = static_cast<CompareNode &>(condNode).GetOpndType();
        }
        bool isFloat = IsPrimitiveFloat(primType);
        jmpOperator = PickJmpInsn(opcode, condOpcode, isFloat, IsSignedInteger(primType));
        cgFunc->SetCurBBKind(BB::kBBIf);
    }
    /* gen targetOpnd, .L.xxx__xx */
    auto funcName = ".L." + std::to_string(cgFunc->GetUniqueID()) + "__" + std::to_string(stmt.GetOffset());
    LabelOperand &targetOpnd = cgFunc->GetOpndBuilder()->CreateLabel(funcName.c_str(), stmt.GetOffset());
    /* select jump Insn */
    Insn &jmpInsn = (cgFunc->GetInsnBuilder()->BuildInsn(jmpOperator, X64CG::kMd[jmpOperator]));
    jmpInsn.AddOpndChain(targetOpnd);
    cgFunc->GetCurBB()->AppendInsn(jmpInsn);
}

Operand &X64MPIsel::GetTargetRetOperand(PrimType primType, int32 sReg)
{
    uint32 bitSize = GetPrimTypeBitSize(primType);
    regno_t retReg = 0;
    switch (sReg) {
        case kSregRetval0:
            retReg = IsPrimitiveFloat(primType) ? x64::V0 : x64::RAX;
            break;
        case kSregRetval1:
            retReg = x64::RDX;
            break;
        default:
            CHECK_FATAL(false, "GetTargetRetOperand: NIY");
            break;
    }
    RegOperand &parmRegOpnd =
        cgFunc->GetOpndBuilder()->CreatePReg(retReg, bitSize, cgFunc->GetRegTyFromPrimTy(primType));
    return parmRegOpnd;
}

Operand *X64MPIsel::SelectMpy(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    PrimType dtype = node.GetPrimType();
    RegOperand *resOpnd = nullptr;
    resOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(dtype), cgFunc->GetRegTyFromPrimTy(dtype));
    RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, dtype, node.Opnd(0)->GetPrimType());
    RegOperand &regOpnd1 = SelectCopy2Reg(opnd1, dtype, node.Opnd(1)->GetPrimType());
    SelectMpy(*resOpnd, regOpnd0, regOpnd1, dtype);
    return resOpnd;
}

void X64MPIsel::SelectMpy(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    uint32 bitSize = GetPrimTypeBitSize(primType);
    SelectCopy(resOpnd, opnd0, primType);
    RegOperand &regOpnd1 = SelectCopy2Reg(opnd1, primType);
    if (IsSignedInteger(primType) || IsUnsignedInteger(primType)) {
        X64MOP_t mOp = (bitSize == k64BitSize)   ? x64::MOP_imulq_r_r
                       : (bitSize == k32BitSize) ? x64::MOP_imull_r_r
                       : (bitSize == k16BitSize) ? x64::MOP_imulw_r_r
                                                 : x64::MOP_begin;
        CHECK_FATAL(mOp != x64::MOP_begin, "NIY mapping");
        Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
        insn.AddOpndChain(regOpnd1).AddOpndChain(resOpnd);
        cgFunc->GetCurBB()->AppendInsn(insn);
    } else if (IsPrimitiveFloat(primType)) {
        X64MOP_t mOp = (bitSize == k64BitSize)   ? x64::MOP_mulfd_r_r
                       : (bitSize == k32BitSize) ? x64::MOP_mulfs_r_r
                                                 : x64::MOP_begin;
        CHECK_FATAL(mOp != x64::MOP_begin, "NIY mapping");
        Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
        insn.AddOpndChain(regOpnd1).AddOpndChain(resOpnd);
        cgFunc->GetCurBB()->AppendInsn(insn);
    }
}

/*
 *  Dividend(EDX:EAX) / Divisor(reg/mem32) = Quotient(EAX)     Remainder(EDX)
 *  IDIV instruction perform signed division of EDX:EAX by the contents of 32-bit register or memory location and
 *  store the quotient in EAX and the remainder in EDX.
 *  The instruction truncates non-integral results towards 0. The sign of the remainder is always the same as the sign
 *  of the dividend, and the absolute value of the remainder is less than the absolute value of the divisor.
 *  An overflow generates a #DE (divide error) exception, rather than setting the OF flag.
 *  To avoid overflow problems, precede this instruction with a CDQ instruction to sign-extend the dividend Divisor.
 *  CDQ Sign-extend EAX into EDX:EAX. This action helps avoid overflow problems in signed number arithmetic.
 */
Operand *X64MPIsel::SelectDiv(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    PrimType primType = node.GetPrimType();
    Operand *resOpnd = nullptr;
    RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, primType, node.Opnd(0)->GetPrimType());
    RegOperand &regOpnd1 = SelectCopy2Reg(opnd1, primType, node.Opnd(1)->GetPrimType());
    resOpnd = SelectDivRem(regOpnd0, regOpnd1, primType, node.GetOpCode());
    return resOpnd;
}

Operand *X64MPIsel::SelectRem(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    PrimType primType = node.GetPrimType();
    Operand *resOpnd = nullptr;
    RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, primType, node.Opnd(0)->GetPrimType());
    RegOperand &regOpnd1 = SelectCopy2Reg(opnd1, primType, node.Opnd(1)->GetPrimType());
    resOpnd = SelectDivRem(regOpnd0, regOpnd1, primType, node.GetOpCode());
    return resOpnd;
}

Operand *X64MPIsel::SelectDivRem(RegOperand &opnd0, RegOperand &opnd1, PrimType primType, Opcode opcode)
{
    DEBUG_ASSERT(opcode == OP_div || opcode == OP_rem, "unsupported opcode");
    if (IsSignedInteger(primType) || IsUnsignedInteger(primType)) {
        uint32 bitSize = GetPrimTypeBitSize(primType);
        /* copy dividend to eax */
        RegOperand &raxOpnd =
            cgFunc->GetOpndBuilder()->CreatePReg(x64::RAX, bitSize, cgFunc->GetRegTyFromPrimTy(primType));
        SelectCopy(raxOpnd, opnd0, primType);

        RegOperand &rdxOpnd =
            cgFunc->GetOpndBuilder()->CreatePReg(x64::RDX, bitSize, cgFunc->GetRegTyFromPrimTy(primType));
        bool isSigned = IsSignedInteger(primType);
        if (isSigned) {
            /* cdq edx:eax = sign-extend of eax*/
            X64MOP_t cvtMOp = (bitSize == k64BitSize)   ? x64::MOP_cqo
                              : (bitSize == k32BitSize) ? x64::MOP_cdq
                              : (bitSize == k16BitSize) ? x64::MOP_cwd
                                                        : x64::MOP_begin;
            CHECK_FATAL(cvtMOp != x64::MOP_begin, "NIY mapping");
            Insn &cvtInsn = cgFunc->GetInsnBuilder()->BuildInsn(cvtMOp, raxOpnd, rdxOpnd);
            cgFunc->GetCurBB()->AppendInsn(cvtInsn);
        } else {
            /* set edx = 0 */
            SelectCopy(rdxOpnd, cgFunc->GetOpndBuilder()->CreateImm(bitSize, 0), primType);
        }
        /* div */
        X64MOP_t divMOp = (bitSize == k64BitSize)   ? (isSigned ? x64::MOP_idivq_r : x64::MOP_divq_r)
                          : (bitSize == k32BitSize) ? (isSigned ? x64::MOP_idivl_r : x64::MOP_divl_r)
                          : (bitSize == k16BitSize) ? (isSigned ? x64::MOP_idivw_r : x64::MOP_divw_r)
                                                    : x64::MOP_begin;
        CHECK_FATAL(divMOp != x64::MOP_begin, "NIY mapping");
        Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(divMOp, opnd1, raxOpnd, rdxOpnd);
        cgFunc->GetCurBB()->AppendInsn(insn);
        /* return */
        RegOperand &resOpnd = cgFunc->GetOpndBuilder()->CreateVReg(bitSize, cgFunc->GetRegTyFromPrimTy(primType));
        SelectCopy(resOpnd, ((opcode == OP_div) ? raxOpnd : rdxOpnd), primType);
        return &resOpnd;
    } else if (IsPrimitiveFloat(primType)) {
        uint32 bitSize = GetPrimTypeBitSize(primType);
        auto &resOpnd = cgFunc->GetOpndBuilder()->CreateVReg(bitSize, cgFunc->GetRegTyFromPrimTy(primType));
        SelectCopy(resOpnd, opnd0, primType);
        Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(x64::MOP_divsd_r, opnd1, resOpnd);
        cgFunc->GetCurBB()->AppendInsn(insn);
        return &resOpnd;
    } else {
        CHECK_FATAL(false, "NIY");
    }
}

Operand *X64MPIsel::SelectLnot(const UnaryNode &node, Operand &opnd0, const BaseNode &parent)
{
    PrimType dtype = node.GetPrimType();
    RegOperand *resOpnd = nullptr;
    resOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(dtype), cgFunc->GetRegTyFromPrimTy(dtype));
    RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, dtype, node.Opnd(0)->GetPrimType());
    ImmOperand &immOpnd = cgFunc->GetOpndBuilder()->CreateImm(GetPrimTypeBitSize(dtype), 0);
    if (IsPrimitiveFloat(dtype)) {
        SelectCmpFloatEq(*resOpnd, regOpnd0, immOpnd, dtype, dtype);
    } else {
        SelectCmp(regOpnd0, immOpnd, dtype);
        SelectCmpResult(*resOpnd, OP_eq, dtype, dtype);
    }
    return resOpnd;
}

/*
 * unorded   ZF, PF, CF  ==> 1,1,1
 * above     ZF, PF, CF  ==> 0,0,0
 * below     ZF, PF, CF  ==> 0,0,1
 * equal     ZF, PF, CF  ==> 1,0,0
 *
 * To distinguish between less than(only check whether CF = 1 or not) and unorderd(CF=1),
 * So ** lt/le in float is replaced by judging gt/ge and swaping operands **
 *
 * float eq using cmpeqsd, same with llvm
 */
Operand *X64MPIsel::SelectCmpOp(CompareNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    PrimType dtype = node.GetPrimType();
    PrimType primOpndType = node.GetOpndType();
    RegOperand *resOpnd = nullptr;
    RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, primOpndType, node.Opnd(0)->GetPrimType());
    RegOperand &regOpnd1 = SelectCopy2Reg(opnd1, primOpndType, node.Opnd(1)->GetPrimType());
    resOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(dtype), cgFunc->GetRegTyFromPrimTy(dtype));
    auto nodeOp = node.GetOpCode();
    Opcode parentOp = parent.GetOpCode();
    bool isFloat = IsPrimitiveFloat(primOpndType);
    bool isJump = (parentOp == OP_brfalse || parentOp == OP_brtrue);
    // float eq
    if (isFloat && (nodeOp == maple::OP_eq) && (!isJump)) {
        SelectCmpFloatEq(*resOpnd, regOpnd0, regOpnd1, dtype, primOpndType);
        return resOpnd;
    }

    bool isSwap = (isFloat && (nodeOp == maple::OP_le || nodeOp == maple::OP_lt) && (parentOp != OP_brfalse));
    SelectCmp(regOpnd0, regOpnd1, primOpndType, isSwap);
    if (isJump) {
        return resOpnd;
    }
    SelectCmpResult(*resOpnd, nodeOp, dtype, primOpndType);
    return resOpnd;
}

void X64MPIsel::SelectCmp(Operand &opnd0, Operand &opnd1, PrimType primType, bool isSwap)
{
    x64::X64MOP_t cmpMOp = x64::MOP_begin;
    if (IsPrimitiveInteger(primType)) {
        cmpMOp = GetCmpMop(opnd0.GetKind(), opnd1.GetKind(), primType);
    } else if (IsPrimitiveFloat(primType)) {
        cmpMOp = x64::MOP_ucomisd_r_r;
    } else {
        CHECK_FATAL(false, "NIY");
    }
    DEBUG_ASSERT(cmpMOp != x64::MOP_begin, "unsupported mOp");
    Insn &cmpInsn = (cgFunc->GetInsnBuilder()->BuildInsn(cmpMOp, X64CG::kMd[cmpMOp]));
    if (isSwap) {
        cmpInsn.AddOpndChain(opnd0).AddOpndChain(opnd1);
    } else {
        cmpInsn.AddOpndChain(opnd1).AddOpndChain(opnd0);
    }
    cgFunc->GetCurBB()->AppendInsn(cmpInsn);
}

void X64MPIsel::SelectCmpFloatEq(RegOperand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primResType,
                                 PrimType primOpndType)
{
    /* float eq using cmpeqsd is same with llvm */
    x64::X64MOP_t eqMOp = x64::MOP_cmpeqsd_r_r;
    Insn &setInsn = cgFunc->GetInsnBuilder()->BuildInsn(eqMOp, X64CG::kMd[eqMOp]);

    auto &regOpnd1 = cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primOpndType),
                                                          cgFunc->GetRegTyFromPrimTy(primOpndType));
    SelectCopy(regOpnd1, opnd1, primOpndType);
    /* CMPEQSD xmm1, xmm2  =>  CMPSD xmm1, xmm2, 0 */
    setInsn.AddOpndChain(opnd0).AddOpndChain(regOpnd1);
    cgFunc->GetCurBB()->AppendInsn(setInsn);

    /* set result -> u64/u32 */
    auto tmpResType = (primOpndType == maple::PTY_f64) ? PTY_u64 : PTY_u32;
    RegOperand &tmpResOpnd =
        cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(tmpResType), cgFunc->GetRegTyFromPrimTy(tmpResType));
    SelectRetypeFloat(tmpResOpnd, regOpnd1, tmpResType, primOpndType);
    /* cvt u64/u32 -> primType */
    SelectIntCvt(resOpnd, tmpResOpnd, primResType, tmpResType);
}

void X64MPIsel::SelectCmpResult(RegOperand &resOpnd, Opcode opCode, PrimType primType, PrimType primOpndType)
{
    bool isFloat = IsPrimitiveFloat(primOpndType);
    bool isSigned = (!IsPrimitiveUnsigned(primOpndType) && !IsPrimitiveFloat(primOpndType));
    /* set result -> u8 */
    RegOperand &tmpResOpnd = cgFunc->GetOpndBuilder()->CreateVReg(k8BitSize, cgFunc->GetRegTyFromPrimTy(PTY_u8));
    x64::X64MOP_t setMOp = GetSetCCMop(opCode, tmpResOpnd.GetKind(), isSigned, isFloat);
    DEBUG_ASSERT(setMOp != x64::MOP_begin, "unsupported mOp");
    Insn &setInsn = cgFunc->GetInsnBuilder()->BuildInsn(setMOp, X64CG::kMd[setMOp]);
    setInsn.AddOpndChain(tmpResOpnd);
    cgFunc->GetCurBB()->AppendInsn(setInsn);
    /* cvt u8 -> primType */
    SelectIntCvt(resOpnd, tmpResOpnd, primType, PTY_u8);
}

void X64MPIsel::SelectSelect(Operand &resOpnd, Operand &trueOpnd, Operand &falseOpnd, PrimType primType,
                             Opcode cmpOpcode, PrimType cmpPrimType)
{
    CHECK_FATAL(!IsPrimitiveFloat(primType), "NIY");
    bool isSigned = !IsPrimitiveUnsigned(primType);
    uint32 bitSize = GetPrimTypeBitSize(primType);
    if (bitSize == k8BitSize) {
        /* cmov unsupported 8bit, cvt to 32bit */
        PrimType cvtType = isSigned ? PTY_i32 : PTY_u32;
        RegOperand &tmpResOpnd = cgFunc->GetOpndBuilder()->CreateVReg(k32BitSize, kRegTyInt);
        Operand &tmpTrueOpnd = SelectCopy2Reg(trueOpnd, cvtType, primType);
        Operand &tmpFalseOpnd = SelectCopy2Reg(falseOpnd, cvtType, primType);
        SelectSelect(tmpResOpnd, tmpTrueOpnd, tmpFalseOpnd, cvtType, cmpOpcode, cmpPrimType);
        SelectCopy(resOpnd, tmpResOpnd, primType, cvtType);
        return;
    }
    RegOperand &tmpOpnd = SelectCopy2Reg(trueOpnd, primType);
    SelectCopy(resOpnd, falseOpnd, primType);
    x64::X64MOP_t cmovMop = GetCMovCCMop(cmpOpcode, bitSize, !IsPrimitiveUnsigned(cmpPrimType));
    DEBUG_ASSERT(cmovMop != x64::MOP_begin, "unsupported mOp");
    Insn &comvInsn = cgFunc->GetInsnBuilder()->BuildInsn(cmovMop, X64CG::kMd[cmovMop]);
    comvInsn.AddOpndChain(tmpOpnd).AddOpndChain(resOpnd);
    cgFunc->GetCurBB()->AppendInsn(comvInsn);
}

void X64MPIsel::SelectMinOrMax(bool isMin, Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    if (IsPrimitiveInteger(primType)) {
        SelectCmp(opnd0, opnd1, primType);
        Opcode cmpOpcode = isMin ? OP_lt : OP_gt;
        SelectSelect(resOpnd, opnd0, opnd1, primType, cmpOpcode, primType);
    } else {
        // float lt/le need to swap operands, and using seta
        CHECK_FATAL(false, "NIY type max or min");
    }
}

Operand *X64MPIsel::SelectCctz(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent)
{
    CHECK_FATAL(opnd0.IsImmediate() || opnd0.IsRegister(), "unhandled operand type here!");
    PrimType origPrimType = node.Opnd(0)->GetPrimType();
    RegOperand &opnd = SelectCopy2Reg(opnd0, origPrimType);

    MOperator mopBsf = x64::MOP_bsfl_r_r;
    Insn &bsfInsn = cgFunc->GetInsnBuilder()->BuildInsn(mopBsf, X64CG::kMd[mopBsf]);
    bsfInsn.AddOpndChain(opnd).AddOpndChain(opnd);
    cgFunc->GetCurBB()->AppendInsn(bsfInsn);

    PrimType retType = node.GetPrimType();
    RegOperand &destReg =
        cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(retType), cgFunc->GetRegTyFromPrimTy(retType));
    // ctz i32 (u32) => cvt u32 -> i32
    // ctz i32 (u64) => cvt u64 -> i32
    SelectIntCvt(destReg, opnd, retType, origPrimType);
    return &destReg;
}

Operand *X64MPIsel::SelectCclz(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent)
{
    CHECK_FATAL(opnd0.IsImmediate() || opnd0.IsRegister(), "unhandled operand type here!");
    CHECK_FATAL(node.GetIntrinsic() == INTRN_C_clz32, "only support clz32");
    PrimType origPrimType = node.Opnd(0)->GetPrimType();
    RegOperand &opnd = SelectCopy2Reg(opnd0, origPrimType);
    // bsr opnd tmp2
    ImmOperand &imm =
        cgFunc->GetOpndBuilder()->CreateImm(GetPrimTypeBitSize(origPrimType), -1);
    RegOperand &tmp1 = SelectCopy2Reg(imm, origPrimType);
    RegOperand &tmp2 =
        cgFunc->GetOpndBuilder()->CreateVReg(
            GetPrimTypeBitSize(origPrimType), cgFunc->GetRegTyFromPrimTy(origPrimType));
    MOperator mopBsr = x64::MOP_bsrl_r_r;
    Insn &bsrInsn = cgFunc->GetInsnBuilder()->BuildInsn(mopBsr, X64CG::kMd[mopBsr]);
    bsrInsn.AddOpndChain(opnd).AddOpndChain(tmp2);
    cgFunc->GetCurBB()->AppendInsn(bsrInsn);
    // cmove -1, tmp2
    MOperator mopComv = x64::MOP_cmovel_r_r;
    Insn &cmovInsn = cgFunc->GetInsnBuilder()->BuildInsn(mopComv, X64CG::kMd[mopComv]);
    cmovInsn.AddOpndChain(tmp1).AddOpndChain(tmp2);
    cgFunc->GetCurBB()->AppendInsn(cmovInsn);
    // neg tmp2
    MOperator mopNeg = x64::MOP_negl_r;
    Insn &negInsn = cgFunc->GetInsnBuilder()->BuildInsn(mopNeg, X64CG::kMd[mopNeg]);
    negInsn.AddOpndChain(tmp2);
    cgFunc->GetCurBB()->AppendInsn(negInsn);
    // add res 31 tmp2
    ImmOperand &imm2 =
        cgFunc->GetOpndBuilder()->CreateImm(GetPrimTypeBitSize(origPrimType), k32BitSize - 1);
    RegOperand &tmp3 =
        cgFunc->GetOpndBuilder()->CreateVReg(
            GetPrimTypeBitSize(origPrimType), cgFunc->GetRegTyFromPrimTy(origPrimType));
    SelectAdd(tmp3, imm2, tmp2, origPrimType);
    PrimType retType = node.GetPrimType();
    RegOperand &destReg =
        cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(retType), cgFunc->GetRegTyFromPrimTy(retType));
    SelectIntCvt(destReg, tmp3, retType, origPrimType);
    return &destReg;
}
RegOperand &X64MPIsel::SelectHeapConstant(IntrinsicopNode &node, Operand &opnd0,
                                          Operand &opnd1, const BaseNode &parent)
{
    PrimType retType = node.GetPrimType();
    RegOperand &destReg =
        cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(retType), cgFunc->GetRegTyFromPrimTy(retType));
    MOperator mOp = x64::MOP_heap_const;
    Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
    insn.AddOpndChain(destReg).AddOpndChain(opnd0).AddOpndChain(opnd1);
    cgFunc->GetCurBB()->AppendInsn(insn);
    return destReg;
}

RegOperand &X64MPIsel::SelectTaggedIsHeapObject(IntrinsicopNode &node, Operand &opnd0,
                                                Operand &opnd1, const BaseNode &parent)
{
    RegOperand &destReg = cgFunc->GetOpndBuilder()->CreateVReg(k8BitSize, kRegTyInt);
    if (opnd0.IsImmediate()) {
        uint64 value = static_cast<uint64>(static_cast<ImmOperand &>(opnd0).GetValue());
        uint64 heapObjectMask = static_cast<uint64_t>(static_cast<ImmOperand&>(opnd1).GetValue());
        MOperator mOp = x64::MOP_movb_i_r;
        Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
        int8 retValue = static_cast<int64>(value & heapObjectMask) == 0;
        ImmOperand &imm0 = cgFunc->GetOpndBuilder()->CreateImm(GetPrimTypeActualBitSize(PTY_i8), retValue);
        insn.AddOpndChain(imm0).AddOpndChain(destReg);
        cgFunc->GetCurBB()->AppendInsn(insn);
    } else {
        RegOperand &tmpReg = cgFunc->GetOpndBuilder()->CreateVReg(k64BitSize, kRegTyInt);
        MOperator mopMov = x64::MOP_movabs_i_r;
        Insn &movInsn = cgFunc->GetInsnBuilder()->BuildInsn(mopMov, X64CG::kMd[mopMov]);
        movInsn.AddOpndChain(opnd1).AddOpndChain(tmpReg);
        cgFunc->GetCurBB()->AppendInsn(movInsn);
        MOperator mOp = x64::MOP_tagged_is_heapobject;
        Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
        insn.AddOpndChain(destReg).AddOpndChain(tmpReg).AddOpndChain(opnd0);
        cgFunc->GetCurBB()->AppendInsn(insn);
    }
    return destReg;
}

RegOperand &X64MPIsel::SelectIsStableElements(IntrinsicopNode &node, Operand &opnd0, Operand &opnd1,
                                              Operand &opnd2, const BaseNode &parent)
{
    RegOperand &destReg = cgFunc->GetOpndBuilder()->CreateVReg(k32BitSize, kRegTyInt);
    MOperator mOp = x64::MOP_is_stable_elements;
    Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
    insn.AddOpndChain(destReg).AddOpndChain(opnd0).AddOpndChain(opnd1).AddOpndChain(opnd2);
    cgFunc->GetCurBB()->AppendInsn(insn);
    return destReg;
}

RegOperand &X64MPIsel::SelectHasPendingException(
    IntrinsicopNode &node, Operand &opnd0, Operand &opnd1, Operand &opnd2, const BaseNode &parent)
{
    RegOperand &destReg = cgFunc->GetOpndBuilder()->CreateVReg(k8BitSize, kRegTyInt);
    MOperator mOp = x64::MOP_has_pending_exception;
    Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
    insn.AddOpndChain(destReg).AddOpndChain(opnd0).AddOpndChain(opnd1).AddOpndChain(opnd2);
    cgFunc->GetCurBB()->AppendInsn(insn);
    return destReg;
}

RegOperand &X64MPIsel::SelectGetHeapConstantTable(IntrinsicopNode &node, Operand &opnd0,
    Operand &opnd1, Operand &opnd2, const BaseNode &parent)
{
    PrimType retType = node.GetPrimType();
    RegOperand &destReg =
        cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(retType), cgFunc->GetRegTyFromPrimTy(retType));
    MOperator mOp = x64::MOP_get_heap_const_table;
    Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
    insn.AddOpndChain(destReg).AddOpndChain(opnd0).AddOpndChain(opnd1).AddOpndChain(opnd2);
    cgFunc->GetCurBB()->AppendInsn(insn);
    return destReg;
}

RegOperand &X64MPIsel::SelectTaggedObjectIsString(IntrinsicopNode &node, Operand &opnd0,
    Operand &opnd1, Operand &opnd2, Operand &opnd3, Operand &opnd4, const BaseNode &parent)
{
    RegOperand &dstReg = cgFunc->GetOpndBuilder()->CreateVReg(k8BitSize, kRegTyInt);
    RegOperand &tmpReg1 = cgFunc->GetOpndBuilder()->CreateVReg(k32BitSize, kRegTyInt);
    RegOperand &tmpReg2 = cgFunc->GetOpndBuilder()->CreateVReg(k64BitSize, kRegTyInt);
    MOperator mOp = x64::MOP_tagged_object_is_string;
    Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
    insn.AddOpndChain(dstReg).AddOpndChain(opnd0).AddOpndChain(opnd1).AddOpndChain(opnd2).AddOpndChain(opnd3).
        AddOpndChain(opnd4).AddOpndChain(tmpReg1).AddOpndChain(tmpReg2);
    cgFunc->GetCurBB()->AppendInsn(insn);
    return dstReg;
}

RegOperand &X64MPIsel::SelectIsCOWArray(IntrinsicopNode &node, Operand &opnd0,
    Operand &opnd1, Operand &opnd2, Operand &opnd3, Operand &opnd4, Operand &opnd5, const BaseNode &parent)
{
    RegOperand &dstReg = cgFunc->GetOpndBuilder()->CreateVReg(k8BitSize, kRegTyInt);
    RegOperand &tmpReg1 = cgFunc->GetOpndBuilder()->CreateVReg(k32BitSize, kRegTyInt);
    RegOperand &tmpReg2 = cgFunc->GetOpndBuilder()->CreateVReg(k64BitSize, kRegTyInt);
    MOperator mOp = x64::MOP_is_cow_array;
    Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
    insn.AddOpndChain(dstReg).AddOpndChain(opnd0).AddOpndChain(opnd1).AddOpndChain(opnd2).AddOpndChain(opnd3).
        AddOpndChain(opnd4).AddOpndChain(opnd5).AddOpndChain(tmpReg1).AddOpndChain(tmpReg2);
    cgFunc->GetCurBB()->AppendInsn(insn);
    return dstReg;
}

RegOperand &X64MPIsel::GetTargetBasicPointer(PrimType primType)
{
    return cgFunc->GetOpndBuilder()->CreatePReg(x64::RBP, GetPrimTypeBitSize(primType),
                                                cgFunc->GetRegTyFromPrimTy(primType));
}

void X64MPIsel::SelectRetypeFloat(RegOperand &resOpnd, Operand &opnd0, PrimType toType, PrimType fromType)
{
    uint32 fromSize = GetPrimTypeBitSize(fromType);
    [[maybe_unused]] uint32 toSize = GetPrimTypeBitSize(toType);
    DEBUG_ASSERT(fromSize == toSize, "retype bit widith doesn' match");
    RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, fromType);
    MOperator mOp = x64::MOP_begin;
    if (fromSize == k32BitSize) {
        mOp = IsPrimitiveFloat(fromType) ? x64::MOP_movd_fr_r : x64::MOP_movd_r_fr;
    } else if (fromSize == k64BitSize) {
        mOp = IsPrimitiveFloat(fromType) ? x64::MOP_movq_fr_r : x64::MOP_movq_r_fr;
    } else {
        CHECK_FATAL(false, "niy");
    }
    CHECK_FATAL(mOp != x64::MOP_begin, "NIY");
    Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
    (void)insn.AddOpndChain(regOpnd0).AddOpndChain(resOpnd);
    cgFunc->GetCurBB()->AppendInsn(insn);
    return;
}

Operand *X64MPIsel::SelectSqrt(UnaryNode &node, Operand &src, const BaseNode &parent)
{
    PrimType dtype = node.GetPrimType();
    if (!IsPrimitiveFloat(dtype)) {
        DEBUG_ASSERT(false, "should be float type");
        return nullptr;
    }
    auto bitSize = GetPrimTypeBitSize(dtype);
    MOperator mOp = x64::MOP_begin;
    if (bitSize == k64BitSize) {
        mOp = MOP_sqrtd_r_r;
    } else if (bitSize == k32BitSize) {
        mOp = MOP_sqrts_r_r;
    } else {
        CHECK_FATAL(false, "niy");
    }
    RegOperand &regOpnd0 = SelectCopy2Reg(src, dtype);
    Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, X64CG::kMd[mOp]);
    Operand &retReg = cgFunc->GetOpndBuilder()->CreateVReg(bitSize, cgFunc->GetRegTyFromPrimTy(dtype));

    (void)insn.AddOpndChain(regOpnd0).AddOpndChain(retReg);
    cgFunc->GetCurBB()->AppendInsn(insn);
    return &retReg;
}
}  // namespace maplebe
