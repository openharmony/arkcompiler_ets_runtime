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

#include "cgfunc.h"
#if DEBUG
#include <iomanip>
#endif
#include <fstream>
#include "cg.h"
#include "insn.h"
#include "loop.h"
#include "mir_builder.h"
#include "factory.h"
#include "cfgo.h"
#include "optimize_common.h"

namespace maplebe {
using namespace maple;

static Operand *HandleDread(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    auto &dreadNode = static_cast<AddrofNode &>(expr);
    return cgFunc.SelectDread(parent, dreadNode);
}

static Operand *HandleRegread(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    (void)parent;
    auto &regReadNode = static_cast<RegreadNode &>(expr);
    return cgFunc.SelectRegread(regReadNode);
}

static Operand *HandleConstVal(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    auto &constValNode = static_cast<ConstvalNode &>(expr);
    MIRConst *mirConst = constValNode.GetConstVal();
    DEBUG_ASSERT(mirConst != nullptr, "get constval of constvalnode failed");
    if (mirConst->GetKind() == kConstInt) {
        auto *mirIntConst = safe_cast<MIRIntConst>(mirConst);
        return cgFunc.SelectIntConst(*mirIntConst, parent);
    } else if (mirConst->GetKind() == kConstFloatConst) {
        auto *mirFloatConst = safe_cast<MIRFloatConst>(mirConst);
        return cgFunc.SelectFloatConst(*mirFloatConst, parent);
    } else {
        CHECK_FATAL(mirConst->GetKind() == kConstDoubleConst, "NYI");
        auto *mirDoubleConst = safe_cast<MIRDoubleConst>(mirConst);
        return cgFunc.SelectDoubleConst(*mirDoubleConst, parent);
    }
    return nullptr;
}

static Operand *HandleAdd(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    return cgFunc.SelectAdd(static_cast<BinaryNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                            *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleShift(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    return cgFunc.SelectShift(static_cast<BinaryNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                              *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleMpy(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    return cgFunc.SelectMpy(static_cast<BinaryNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                            *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleDiv(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    return cgFunc.SelectDiv(static_cast<BinaryNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                            *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleRem(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    return cgFunc.SelectRem(static_cast<BinaryNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                            *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleIread(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    auto &ireadNode = static_cast<IreadNode &>(expr);
    return cgFunc.SelectIread(parent, ireadNode);
}

static Operand *HandleSub(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    return cgFunc.SelectSub(static_cast<BinaryNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                            *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleBand(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    return cgFunc.SelectBand(static_cast<BinaryNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                             *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleBior(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    return cgFunc.SelectBior(static_cast<BinaryNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                             *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleBxor(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    return cgFunc.SelectBxor(static_cast<BinaryNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                             *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleAbs(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    (void)parent;
    DEBUG_ASSERT(expr.Opnd(0) != nullptr, "expr.Opnd(0) should not be nullptr");
    return cgFunc.SelectAbs(static_cast<UnaryNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

static Operand *HandleBnot(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    return cgFunc.SelectBnot(static_cast<UnaryNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)), parent);
}

static Operand *HandleExtractBits(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    ExtractbitsNode &node = static_cast<ExtractbitsNode &>(expr);
    uint8 bitOffset = node.GetBitsOffset();
    uint8 bitSize = node.GetBitsSize();
    if (!CGOptions::IsBigEndian() && (bitSize == k8BitSize || bitSize == k16BitSize) &&
        GetPrimTypeBitSize(node.GetPrimType()) != k64BitSize &&
        (bitOffset == 0 || bitOffset == k8BitSize || bitOffset == k16BitSize || bitOffset == k24BitSize) &&
        expr.Opnd(0)->GetOpCode() == OP_iread && node.GetOpCode() == OP_extractbits) {
        return cgFunc.SelectRegularBitFieldLoad(node, parent);
    }
    return cgFunc.SelectExtractbits(static_cast<ExtractbitsNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                                    parent);
}

static Operand *HandleLnot(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    DEBUG_ASSERT(expr.Opnd(0) != nullptr, "nullptr check");
    return cgFunc.SelectLnot(static_cast<UnaryNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)), parent);
}


static Operand *HandleMin(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    return cgFunc.SelectMin(static_cast<BinaryNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                            *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleMax(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    return cgFunc.SelectMax(static_cast<BinaryNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                            *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleSqrt(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    return cgFunc.SelectSqrt(static_cast<UnaryNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)), parent);
}

static Operand *HandleCeil(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    return cgFunc.SelectCeil(static_cast<TypeCvtNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)), parent);
}

static Operand *HandleFloor(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    DEBUG_ASSERT(expr.Opnd(0) != nullptr, "expr.Opnd(0) should not be nullptr");
    return cgFunc.SelectFloor(static_cast<TypeCvtNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)), parent);
}

static Operand *HandleRetype(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    (void)parent;
    DEBUG_ASSERT(expr.Opnd(0) != nullptr, "nullptr check");
    return cgFunc.SelectRetype(static_cast<TypeCvtNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

static Operand *HandleCvt(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    return cgFunc.SelectCvt(parent, static_cast<TypeCvtNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)));
}

static Operand *HandleTrunc(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    DEBUG_ASSERT(expr.Opnd(0) != nullptr, "expr should not be nullptr");
    return cgFunc.SelectTrunc(static_cast<TypeCvtNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)), parent);
}

static Operand *HandleCmp(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    // fix opnd type before select insn
    PrimType targetPtyp = parent.GetPrimType();
    if (kOpcodeInfo.IsCompare(parent.GetOpCode())) {
        targetPtyp = static_cast<const CompareNode &>(parent).GetOpndType();
    } else if (kOpcodeInfo.IsTypeCvt(parent.GetOpCode())) {
        targetPtyp = static_cast<const TypeCvtNode &>(parent).FromType();
    }
    if (IsPrimitiveInteger(targetPtyp) && targetPtyp != expr.GetPrimType()) {
        expr.SetPrimType(targetPtyp);
    }
    return cgFunc.SelectCmpOp(static_cast<CompareNode &>(expr), *cgFunc.HandleExpr(expr, *expr.Opnd(0)),
                              *cgFunc.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleIntrinOp(const BaseNode &parent, BaseNode &expr, CGFunc &cgFunc)
{
    auto &intrinsicopNode = static_cast<IntrinsicopNode &>(expr);
    switch (intrinsicopNode.GetIntrinsic()) {
        case INTRN_C_clz32:
        case INTRN_C_clz64:
            return cgFunc.SelectCclz(intrinsicopNode);
        default:
            DEBUG_ASSERT(false, "Should not reach here.");
            return nullptr;
    }
}

using HandleExprFactory = FunctionFactory<Opcode, maplebe::Operand *, const BaseNode &, BaseNode &, CGFunc &>;
void InitHandleExprFactory()
{
    RegisterFactoryFunction<HandleExprFactory>(OP_dread, HandleDread);
    RegisterFactoryFunction<HandleExprFactory>(OP_regread, HandleRegread);
    RegisterFactoryFunction<HandleExprFactory>(OP_constval, HandleConstVal);
    RegisterFactoryFunction<HandleExprFactory>(OP_add, HandleAdd);
    RegisterFactoryFunction<HandleExprFactory>(OP_ashr, HandleShift);
    RegisterFactoryFunction<HandleExprFactory>(OP_lshr, HandleShift);
    RegisterFactoryFunction<HandleExprFactory>(OP_shl, HandleShift);
    RegisterFactoryFunction<HandleExprFactory>(OP_mul, HandleMpy);
    RegisterFactoryFunction<HandleExprFactory>(OP_div, HandleDiv);
    RegisterFactoryFunction<HandleExprFactory>(OP_rem, HandleRem);
    RegisterFactoryFunction<HandleExprFactory>(OP_iread, HandleIread);
    RegisterFactoryFunction<HandleExprFactory>(OP_sub, HandleSub);
    RegisterFactoryFunction<HandleExprFactory>(OP_band, HandleBand);
    RegisterFactoryFunction<HandleExprFactory>(OP_bior, HandleBior);
    RegisterFactoryFunction<HandleExprFactory>(OP_bxor, HandleBxor);
    RegisterFactoryFunction<HandleExprFactory>(OP_abs, HandleAbs);
    RegisterFactoryFunction<HandleExprFactory>(OP_bnot, HandleBnot);
    RegisterFactoryFunction<HandleExprFactory>(OP_sext, HandleExtractBits);
    RegisterFactoryFunction<HandleExprFactory>(OP_zext, HandleExtractBits);
    RegisterFactoryFunction<HandleExprFactory>(OP_lnot, HandleLnot);
    RegisterFactoryFunction<HandleExprFactory>(OP_min, HandleMin);
    RegisterFactoryFunction<HandleExprFactory>(OP_max, HandleMax);
    RegisterFactoryFunction<HandleExprFactory>(OP_sqrt, HandleSqrt);
    RegisterFactoryFunction<HandleExprFactory>(OP_ceil, HandleCeil);
    RegisterFactoryFunction<HandleExprFactory>(OP_floor, HandleFloor);
    RegisterFactoryFunction<HandleExprFactory>(OP_retype, HandleRetype);
    RegisterFactoryFunction<HandleExprFactory>(OP_cvt, HandleCvt);
    RegisterFactoryFunction<HandleExprFactory>(OP_trunc, HandleTrunc);
    RegisterFactoryFunction<HandleExprFactory>(OP_le, HandleCmp);
    RegisterFactoryFunction<HandleExprFactory>(OP_ge, HandleCmp);
    RegisterFactoryFunction<HandleExprFactory>(OP_gt, HandleCmp);
    RegisterFactoryFunction<HandleExprFactory>(OP_lt, HandleCmp);
    RegisterFactoryFunction<HandleExprFactory>(OP_ne, HandleCmp);
    RegisterFactoryFunction<HandleExprFactory>(OP_eq, HandleCmp);
    RegisterFactoryFunction<HandleExprFactory>(OP_intrinsicop, HandleIntrinOp);
}

static void HandleLabel(StmtNode &stmt, CGFunc &cgFunc)
{
    DEBUG_ASSERT(stmt.GetOpCode() == OP_label, "error");
    auto &label = static_cast<LabelNode &>(stmt);
    BB *newBB = cgFunc.StartNewBBImpl(false, label);
    newBB->AddLabel(label.GetLabelIdx());
    if (newBB->GetId() == 1) {
        newBB->SetFrequency(kFreqBase);
    }
    cgFunc.SetLab2BBMap(newBB->GetLabIdx(), *newBB);
    cgFunc.SetCurBB(*newBB);
}

static void HandleGoto(StmtNode &stmt, CGFunc &cgFunc)
{
    auto &gotoNode = static_cast<GotoNode &>(stmt);
    cgFunc.SetCurBBKind(BB::kBBGoto);
    cgFunc.SelectGoto(gotoNode);
    cgFunc.SetCurBB(*cgFunc.StartNewBB(gotoNode));
    DEBUG_ASSERT(&stmt == &gotoNode, "stmt must be same as gotoNoe");

    if ((gotoNode.GetNext() != nullptr) && (gotoNode.GetNext()->GetOpCode() != OP_label)) {
        DEBUG_ASSERT(cgFunc.GetCurBB()->GetPrev()->GetLastStmt() == &stmt, "check the relation between BB and stmt");
    }
}

static void HandleCondbr(StmtNode &stmt, CGFunc &cgFunc)
{
    auto &condGotoNode = static_cast<CondGotoNode &>(stmt);
    BaseNode *condNode = condGotoNode.Opnd(0);
    DEBUG_ASSERT(condNode != nullptr, "expect first operand of cond br");
    Opcode condOp = condGotoNode.GetOpCode();
    if (condNode->GetOpCode() == OP_constval) {
        auto *constValNode = static_cast<ConstvalNode *>(condNode);
        if ((constValNode->GetConstVal()->IsZero() && (OP_brfalse == condOp)) ||
            (!constValNode->GetConstVal()->IsZero() && (OP_brtrue == condOp))) {
            auto *gotoStmt = cgFunc.GetMemoryPool()->New<GotoNode>(OP_goto);
            gotoStmt->SetOffset(condGotoNode.GetOffset());
            HandleGoto(*gotoStmt, cgFunc);
            auto *labelStmt = cgFunc.GetMemoryPool()->New<LabelNode>();
            labelStmt->SetLabelIdx(cgFunc.CreateLabel());
            HandleLabel(*labelStmt, cgFunc);
        }
        return;
    }
    cgFunc.SetCurBBKind(BB::kBBIf);
    /* if condNode is not a cmp node, cmp it with zero. */
    if (!kOpcodeInfo.IsCompare(condNode->GetOpCode())) {
        Operand *opnd0 = cgFunc.HandleExpr(condGotoNode, *condNode);
        PrimType primType = condNode->GetPrimType();
        Operand *zeroOpnd = nullptr;
        if (IsPrimitiveInteger(primType)) {
            zeroOpnd = &cgFunc.CreateImmOperand(primType, 0);
        } else {
            DEBUG_ASSERT(((PTY_f32 == primType) || (PTY_f64 == primType)),
                         "we don't support half-precision FP operands yet");
            zeroOpnd = &cgFunc.CreateImmOperand(primType, 0);
        }
        cgFunc.SelectCondGoto(condGotoNode, *opnd0, *zeroOpnd);
        cgFunc.SetCurBB(*cgFunc.StartNewBB(condGotoNode));
        return;
    }
    /*
     * Special case:
     * bgt (cmp (op0, op1), 0) ==>
     * bgt (op0, op1)
     * but skip the case cmp(op0, 0)
     */
    BaseNode *op0 = condNode->Opnd(0);
    DEBUG_ASSERT(op0 != nullptr, "get first opnd of a condNode failed");
    BaseNode *op1 = condNode->Opnd(1);
    DEBUG_ASSERT(op1 != nullptr, "get second opnd of a condNode failed");
    if ((op0->GetOpCode() == OP_cmp) && (op1->GetOpCode() == OP_constval)) {
        auto *constValNode = static_cast<ConstvalNode *>(op1);
        MIRConst *mirConst = constValNode->GetConstVal();
        auto *cmpNode = static_cast<CompareNode *>(op0);
        bool skip = false;
        if (cmpNode->Opnd(1)->GetOpCode() == OP_constval) {
            auto *constVal = static_cast<ConstvalNode *>(cmpNode->Opnd(1))->GetConstVal();
            if (constVal->IsZero()) {
                skip = true;
            }
        }
        if (!skip && mirConst->IsZero()) {
            cgFunc.SelectCondSpecialCase1(condGotoNode, *op0);
            cgFunc.SetCurBB(*cgFunc.StartNewBB(condGotoNode));
            return;
        }
    }

    Operand *opnd0 = cgFunc.HandleExpr(*condNode, *condNode->Opnd(0));
    Operand *opnd1 = cgFunc.HandleExpr(*condNode, *condNode->Opnd(1));
    cgFunc.SelectCondGoto(condGotoNode, *opnd0, *opnd1);
    cgFunc.SetCurBB(*cgFunc.StartNewBB(condGotoNode));
}

static void HandleReturn(StmtNode &stmt, CGFunc &cgFunc)
{
    auto &retNode = static_cast<NaryStmtNode &>(stmt);
    DEBUG_ASSERT(retNode.NumOpnds() <= 1, "NYI return nodes number > 1");
    Operand *opnd = nullptr;
    if (retNode.NumOpnds() != 0) {
        opnd = cgFunc.HandleExpr(retNode, *retNode.Opnd(0));
    }
    cgFunc.SelectReturn(opnd);
    cgFunc.SetCurBBKind(BB::kBBReturn);
    cgFunc.SetCurBB(*cgFunc.StartNewBB(retNode));
}

static void HandleCall(StmtNode &stmt, CGFunc &cgFunc)
{
    auto &callNode = static_cast<CallNode &>(stmt);
    cgFunc.SelectCall(callNode);
}

static void HandleICall(StmtNode &stmt, CGFunc &cgFunc)
{
    auto &icallNode = static_cast<IcallNode &>(stmt);
    cgFunc.GetCurBB()->SetHasCall();
    cgFunc.SelectIcall(icallNode);
}

static void HandleIntrinsicCall(StmtNode &stmt, CGFunc &cgFunc)
{
    auto &call = static_cast<IntrinsiccallNode &>(stmt);
    cgFunc.SelectIntrinsicCall(call);
}

static void HandleDassign(StmtNode &stmt, CGFunc &cgFunc)
{
    auto &dassignNode = static_cast<DassignNode &>(stmt);
    DEBUG_ASSERT(dassignNode.GetOpCode() == OP_dassign, "expect dassign");
    BaseNode *rhs = dassignNode.GetRHS();
    DEBUG_ASSERT(rhs != nullptr, "get rhs of dassignNode failed");
    bool isSaveRetvalToLocal = false;
    if (rhs->GetOpCode() == OP_regread) {
        isSaveRetvalToLocal = (static_cast<RegreadNode *>(rhs)->GetRegIdx() == -kSregRetval0);
    }
    Operand *opnd0 = cgFunc.HandleExpr(dassignNode, *rhs);
    cgFunc.SelectDassign(dassignNode, *opnd0);
    if (isSaveRetvalToLocal && cgFunc.GetCurBB() && cgFunc.GetCurBB()->GetLastMachineInsn()) {
        cgFunc.GetCurBB()->GetLastMachineInsn()->MarkAsSaveRetValToLocal();
    }
}

static void HandleRegassign(StmtNode &stmt, CGFunc &cgFunc)
{
    DEBUG_ASSERT(stmt.GetOpCode() == OP_regassign, "expect regAssign");
    auto &regAssignNode = static_cast<RegassignNode &>(stmt);
    bool isSaveRetvalToLocal = false;
    BaseNode *operand = regAssignNode.Opnd(0);
    DEBUG_ASSERT(operand != nullptr, "get operand of regassignNode failed");
    if (operand->GetOpCode() == OP_regread) {
        isSaveRetvalToLocal = (static_cast<RegreadNode *>(operand)->GetRegIdx() == -kSregRetval0);
    }
    Operand *opnd0 = cgFunc.HandleExpr(regAssignNode, *operand);
    cgFunc.SelectRegassign(regAssignNode, *opnd0);
    if (isSaveRetvalToLocal && cgFunc.GetCurBB() && cgFunc.GetCurBB()->GetLastMachineInsn()) {
        cgFunc.GetCurBB()->GetLastMachineInsn()->MarkAsSaveRetValToLocal();
    }
}

static void HandleIassign(StmtNode &stmt, CGFunc &cgFunc)
{
    DEBUG_ASSERT(stmt.GetOpCode() == OP_iassign, "expect stmt");
    auto &iassignNode = static_cast<IassignNode &>(stmt);
    if ((iassignNode.GetRHS() != nullptr)) {
        cgFunc.SelectIassign(iassignNode);
    } else {
        CHECK_FATAL(false, "NIY");
    }
}

void HandleRangeGoto(StmtNode &stmt, CGFunc &cgFunc)
{
    auto &rangeGotoNode = static_cast<RangeGotoNode &>(stmt);
    cgFunc.SetCurBBKind(BB::kBBRangeGoto);
    cgFunc.SelectRangeGoto(rangeGotoNode, *cgFunc.HandleExpr(rangeGotoNode, *rangeGotoNode.Opnd(0)));
    cgFunc.SetCurBB(*cgFunc.StartNewBB(rangeGotoNode));
}

void HandleComment(StmtNode &stmt, CGFunc &cgFunc)
{
    if (cgFunc.GetCG()->GenerateVerboseAsm() || cgFunc.GetCG()->GenerateVerboseCG()) {
        cgFunc.SelectComment(static_cast<CommentNode &>(stmt));
    }
}

using HandleStmtFactory = FunctionFactory<Opcode, void, StmtNode &, CGFunc &>;
static void InitHandleStmtFactory()
{
    RegisterFactoryFunction<HandleStmtFactory>(OP_label, HandleLabel);
    RegisterFactoryFunction<HandleStmtFactory>(OP_goto, HandleGoto);
    RegisterFactoryFunction<HandleStmtFactory>(OP_brfalse, HandleCondbr);
    RegisterFactoryFunction<HandleStmtFactory>(OP_brtrue, HandleCondbr);
    RegisterFactoryFunction<HandleStmtFactory>(OP_return, HandleReturn);
    RegisterFactoryFunction<HandleStmtFactory>(OP_call, HandleCall);
    RegisterFactoryFunction<HandleStmtFactory>(OP_icall, HandleICall);
    RegisterFactoryFunction<HandleStmtFactory>(OP_icallproto, HandleICall);
    RegisterFactoryFunction<HandleStmtFactory>(OP_intrinsiccall, HandleIntrinsicCall);
    RegisterFactoryFunction<HandleStmtFactory>(OP_intrinsiccallassigned, HandleIntrinsicCall);
    RegisterFactoryFunction<HandleStmtFactory>(OP_intrinsiccallwithtype, HandleIntrinsicCall);
    RegisterFactoryFunction<HandleStmtFactory>(OP_dassign, HandleDassign);
    RegisterFactoryFunction<HandleStmtFactory>(OP_regassign, HandleRegassign);
    RegisterFactoryFunction<HandleStmtFactory>(OP_iassign, HandleIassign);
    RegisterFactoryFunction<HandleStmtFactory>(OP_rangegoto, HandleRangeGoto);
    RegisterFactoryFunction<HandleStmtFactory>(OP_comment, HandleComment);
}

CGFunc::CGFunc(MIRModule &mod, CG &cg, MIRFunction &mirFunc, BECommon &beCommon, MemPool &memPool,
               StackMemPool &stackMp, MapleAllocator &allocator, uint32 funcId)
    : bbVec(allocator.Adapter()),
      referenceVirtualRegs(allocator.Adapter()),
      referenceStackSlots(allocator.Adapter()),
      pregIdx2Opnd(mirFunc.GetPregTab()->Size(), nullptr, allocator.Adapter()),
      pRegSpillMemOperands(allocator.Adapter()),
      spillRegMemOperands(allocator.Adapter()),
      reuseSpillLocMem(allocator.Adapter()),
      labelMap(std::less<LabelIdx>(), allocator.Adapter()),
      vregsToPregsMap(std::less<regno_t>(), allocator.Adapter()),
      stackMapInsns(allocator.Adapter()),
      hasVLAOrAlloca(mirFunc.HasVlaOrAlloca()),
      cg(&cg),
      mirModule(mod),
      memPool(&memPool),
      stackMp(stackMp),
      func(mirFunc),
      exitBBVec(allocator.Adapter()),
      noReturnCallBBVec(allocator.Adapter()),
      extendSet(allocator.Adapter()),
      lab2BBMap(allocator.Adapter()),
      beCommon(beCommon),
      funcScopeAllocator(&allocator),
      emitStVec(allocator.Adapter()),
      switchLabelCnt(allocator.Adapter()),
      shortFuncName(mirFunc.GetName() + "." + std::to_string(funcId), &memPool)
{
    mirModule.SetCurFunction(&func);
    dummyBB = CreateNewBB();
    vReg.SetCount(static_cast<uint32>(kBaseVirtualRegNO + func.GetPregTab()->Size()));
    firstNonPregVRegNO = vReg.GetCount();
    /* maximum register count initial be increased by 1024 */
    SetMaxRegNum(vReg.GetCount() + 1024);

    maplebe::VregInfo::vRegOperandTable.clear();

    insnBuilder = memPool.New<InsnBuilder>(memPool);
    opndBuilder = memPool.New<OperandBuilder>(memPool, func.GetPregTab()->Size());

    vReg.VRegTableResize(GetMaxRegNum());
    /* func.GetPregTab()->_preg_table[0] is nullptr, so skip it */
    DEBUG_ASSERT(func.GetPregTab()->PregFromPregIdx(0) == nullptr, "PregFromPregIdx(0) must be nullptr");
    for (size_t i = 1; i < func.GetPregTab()->Size(); ++i) {
        PrimType primType = func.GetPregTab()->PregFromPregIdx(i)->GetPrimType();
        uint32 byteLen = GetPrimTypeSize(primType);
        if (byteLen < k4ByteSize) {
            byteLen = k4ByteSize;
        }
        new (&GetVirtualRegNodeFromPseudoRegIdx(i)) VirtualRegNode(GetRegTyFromPrimTy(primType), byteLen);
    }
    lSymSize = 0;
    if (func.GetSymTab()) {
        lSymSize = func.GetSymTab()->GetSymbolTableSize();
    }
    callingConventionKind = CCImpl::GetCallConvKind(mirFunc);
}

CGFunc::~CGFunc()
{
    mirModule.SetCurFunction(nullptr);
}

Operand *CGFunc::HandleExpr(const BaseNode &parent, BaseNode &expr)
{
    auto function = CreateProductFunction<HandleExprFactory>(expr.GetOpCode());
    CHECK_FATAL(function != nullptr, "unsupported %d opCode in HandleExpr()", expr.GetOpCode());
    return function(parent, expr, *this);
}

StmtNode *CGFunc::HandleFirstStmt()
{
    BlockNode *block = func.GetBody();

    DEBUG_ASSERT(block != nullptr, "get func body block failed in CGFunc::GenerateInstruction");
    StmtNode *stmt = block->GetFirst();
    if (stmt == nullptr) {
        return nullptr;
    }
    DEBUG_ASSERT(stmt->GetOpCode() == OP_label, "The first statement should be a label");
    HandleLabel(*stmt, *this);
    firstBB = curBB;
    stmt = stmt->GetNext();
    if (stmt == nullptr) {
        return nullptr;
    }
    curBB = StartNewBBImpl(false, *stmt);
    return stmt;
}

void CGFunc::RemoveUnreachableBB()
{
    OptimizationPattern *pattern = memPool->New<UnreachBBPattern>(*this);
    for (BB *bb = firstBB; bb != nullptr; bb = bb->GetNext()) {
        (void)pattern->Optimize(*bb);
    }
}

void CGFunc::GenerateInstruction()
{
    InitHandleExprFactory();
    InitHandleStmtFactory();
    StmtNode *secondStmt = HandleFirstStmt();

    std::set<uint32> bbFreqSet;
    for (StmtNode *stmt = secondStmt; stmt != nullptr; stmt = stmt->GetNext()) {
        GetInsnBuilder()->SetDebugComment(stmt->GetDebugComment());
        auto function = CreateProductFunction<HandleStmtFactory>(stmt->GetOpCode());
        CHECK_FATAL(function != nullptr, "unsupported opCode or has been lowered before");
        function(*stmt, *this);
        GetInsnBuilder()->ClearDebugComment();
    }

    /* Set lastbb's frequency */
    BlockNode *block = func.GetBody();
    DEBUG_ASSERT(block != nullptr, "get func body block failed in CGFunc::GenerateInstruction");
    curBB->SetLastStmt(*block->GetLast());
    lastBB = curBB;
}

LabelIdx CGFunc::CreateLabel()
{
    MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(func.GetStIdx().Idx());
    DEBUG_ASSERT(funcSt != nullptr, "Get func failed at CGFunc::CreateLabel");
    std::string funcName = funcSt->GetName();
    std::string labelStr = funcName += std::to_string(labelIdx++);
    return func.GetOrCreateLableIdxFromName(labelStr);
}

void CGFunc::ProcessExitBBVec()
{
    if (exitBBVec.empty()) {
        BB *retBB = CreateNewBB();
        retBB->SetKind(BB::kBBReturn);
        SetLab2BBMap(retBB->GetLabIdx(), *retBB);
        GetLastBB()->PrependBB(*retBB);
        exitBBVec.emplace_back(retBB);
        return;
    }
    /* split an empty exitBB */
    BB *bb = exitBBVec[0];
    if (bb->NumInsn() > 0) {
        BB *retBBPart = CreateNewBB(false, BB::kBBFallthru, bb->GetFrequency());
        DEBUG_ASSERT(retBBPart != nullptr, "retBBPart should not be nullptr");
        LabelIdx retBBPartLabelIdx = bb->GetLabIdx();
        if (retBBPartLabelIdx != MIRLabelTable::GetDummyLabel()) {
            retBBPart->AddLabel(retBBPartLabelIdx);
            lab2BBMap[retBBPartLabelIdx] = retBBPart;
        }
        Insn *insn = bb->GetFirstInsn();
        while (insn != nullptr) {
            bb->RemoveInsn(*insn);
            retBBPart->AppendInsn(*insn);
            insn = bb->GetFirstInsn();
        }
        bb->PrependBB(*retBBPart);
        LabelIdx newLabelIdx = CreateLabel();
        bb->AddLabel(newLabelIdx);
        lab2BBMap[newLabelIdx] = bb;
    }
}

void CGFunc::AddCommonExitBB()
{
    if (commonExitBB != nullptr) {
        return;
    }
    // create fake commonExitBB
    commonExitBB = CreateNewBB(true, BB::kBBFallthru, 0);
    DEBUG_ASSERT(commonExitBB != nullptr, "cannot create fake commonExitBB");
    for (BB *cgbb : exitBBVec) {
        if (!cgbb->IsUnreachable()) {
            commonExitBB->PushBackPreds(*cgbb);
        }
    }
}

void CGFunc::HandleFunction()
{
    /* select instruction */
    GenerateInstruction();
    /* merge multi return */
    MergeReturn();
    ProcessExitBBVec();
    /* build control flow graph */
    theCFG = memPool->New<CGCFG>(*this);
    theCFG->BuildCFG();
    RemoveUnreachableBB();
    AddCommonExitBB();
    theCFG->UnreachCodeAnalysis();
    EraseUnreachableStackMapInsns();
}

void CGFunc::DumpCFG() const
{
#ifdef ARK_LITECG_DEBUG
    MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(func.GetStIdx().Idx());
    DEBUG_ASSERT(funcSt != nullptr, "null ptr check");
    LogInfo::MapleLogger() << "\n****** CFG built by CG for " << funcSt->GetName() << " *******\n";
    FOR_ALL_BB_CONST(bb, this)
    {
        LogInfo::MapleLogger() << "=== BB ( " << std::hex << bb << std::dec << " ) <" << bb->GetKindName() << "> ===\n";
        LogInfo::MapleLogger() << "BB id:" << bb->GetId() << "\n";
        if (!bb->GetPreds().empty()) {
            LogInfo::MapleLogger() << " pred [ ";
            for (auto *pred : bb->GetPreds()) {
                LogInfo::MapleLogger() << pred->GetId() << " ";
            }
            LogInfo::MapleLogger() << "]\n";
        }
        if (!bb->GetSuccs().empty()) {
            LogInfo::MapleLogger() << " succ [ ";
            for (auto *succ : bb->GetSuccs()) {
                LogInfo::MapleLogger() << succ->GetId() << " ";
            }
            LogInfo::MapleLogger() << "]\n";
        }
        const StmtNode *stmt = bb->GetFirstStmt();
        if (stmt != nullptr) {
            bool done = false;
            do {
                done = stmt == bb->GetLastStmt();
                stmt->Dump(1);
                LogInfo::MapleLogger() << "\n";
                stmt = stmt->GetNext();
            } while (!done);
        } else {
            LogInfo::MapleLogger() << "<empty BB>\n";
        }
    }
#endif
}

void CGFunc::DumpBBInfo(const BB *bb) const
{
#ifdef ARK_LITECG_DEBUG
    LogInfo::MapleLogger() << "=== BB " << " <" << bb->GetKindName();
    if (bb->GetLabIdx() != MIRLabelTable::GetDummyLabel()) {
        LogInfo::MapleLogger() << "[labeled with " << bb->GetLabIdx();
        LogInfo::MapleLogger() << " ==> @" << func.GetLabelName(bb->GetLabIdx()) << "]";
    }

    LogInfo::MapleLogger() << "> <" << bb->GetId() << "> ";
    if (bb->IsCleanup()) {
        LogInfo::MapleLogger() << "[is_cleanup] ";
    }
    if (bb->IsUnreachable()) {
        LogInfo::MapleLogger() << "[unreachable] ";
    }
    if (!bb->GetSuccs().empty()) {
        LogInfo::MapleLogger() << "succs: ";
        for (auto *succBB : bb->GetSuccs()) {
            LogInfo::MapleLogger() << succBB->GetId() << " ";
        }
    }
    if (!bb->GetPreds().empty()) {
        LogInfo::MapleLogger() << "preds: ";
        for (auto *predBB : bb->GetPreds()) {
            LogInfo::MapleLogger() << predBB->GetId() << " ";
        }
    }
    LogInfo::MapleLogger() << "===\n";
    LogInfo::MapleLogger() << "frequency:" << bb->GetFrequency() << "\n";
#endif
}

void CGFunc::DumpCGIR() const
{
#ifdef ARK_LITECG_DEBUG
    MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(func.GetStIdx().Idx());
    DEBUG_ASSERT(funcSt != nullptr, "null ptr check");
    LogInfo::MapleLogger() << "\n******  CGIR for " << funcSt->GetName() << " *******\n";
    FOR_ALL_BB_CONST(bb, this)
    {
        if (bb->IsUnreachable()) {
            continue;
        }
        DumpBBInfo(bb);
        FOR_BB_INSNS_CONST(insn, bb)
        {
            insn->Dump();
        }
    }
#endif
}

// Cgirverify phase function: all insns will be verified before cgemit.
void CGFunc::VerifyAllInsn()
{
#ifdef ARK_LITECG_DEBUG
    FOR_ALL_BB(bb, this)
    {
        FOR_BB_INSNS(insn, bb)
        {
            if (VERIFY_INSN(insn) && insn->CheckMD()) {
                continue;
            }
            LogInfo::MapleLogger() << "Illegal insn is:\n";
            insn->Dump();
            LogInfo::MapleLogger() << "Function name is:\n" << GetName() << "\n";
            CHECK_FATAL_FALSE("The problem is illegal insn, info is above.");
        }
    }
#endif
}

bool CgHandleFunction::PhaseRun(maplebe::CGFunc &f)
{
    f.HandleFunction();
    return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgHandleFunction, handlefunction)

bool CgVerify::PhaseRun(maplebe::CGFunc &f)
{
#ifdef ARK_LITECG_DEBUG
    f.VerifyAllInsn();
    if (!f.GetCG()->GetCGOptions().DoEmitCode() || f.GetCG()->GetCGOptions().DoDumpCFG()) {
        f.DumpCFG();
    }
#endif
    return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgVerify, cgirverify)
} /* namespace maplebe */
