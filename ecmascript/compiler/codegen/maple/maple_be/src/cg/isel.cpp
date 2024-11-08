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

#include "isel.h"
#include "factory.h"
#include "cg.h"
#include "standardize.h"
#include <map>
#include <utility>

namespace maplebe {
/* register,                       imm ,                         memory,                        cond */
#define DEF_FAST_ISEL_MAPPING_INT(SIZE)                                                                               \
    MOperator fastIselMapI##SIZE[Operand::OperandType::kOpdPhi][Operand::OperandType::kOpdPhi] = {                    \
        {abstract::MOP_copy_rr_##SIZE, abstract::MOP_copy_ri_##SIZE, abstract::MOP_load_##SIZE, abstract::MOP_undef}, \
        {abstract::MOP_undef, abstract::MOP_undef, abstract::MOP_undef, abstract::MOP_undef},                         \
        {abstract::MOP_str_##SIZE, abstract::MOP_undef, abstract::MOP_undef, abstract::MOP_undef},                    \
        {abstract::MOP_undef, abstract::MOP_undef, abstract::MOP_undef, abstract::MOP_undef},                         \
    }
#define DEF_FAST_ISEL_MAPPING_FLOAT(SIZE)                                                            \
    MOperator fastIselMapF##SIZE[Operand::OperandType::kOpdPhi][Operand::OperandType::kOpdPhi] = {   \
        {abstract::MOP_copy_ff_##SIZE, abstract::MOP_copy_fi_##SIZE, abstract::MOP_load_f_##SIZE,    \
         abstract::MOP_undef},                                                                       \
        {abstract::MOP_undef, abstract::MOP_undef, abstract::MOP_undef, abstract::MOP_undef},        \
        {abstract::MOP_str_f_##SIZE, abstract::MOP_undef, abstract::MOP_undef, abstract::MOP_undef}, \
        {abstract::MOP_undef, abstract::MOP_undef, abstract::MOP_undef, abstract::MOP_undef},        \
    }

DEF_FAST_ISEL_MAPPING_INT(8);
DEF_FAST_ISEL_MAPPING_INT(16);
DEF_FAST_ISEL_MAPPING_INT(32);
DEF_FAST_ISEL_MAPPING_INT(64);
DEF_FAST_ISEL_MAPPING_FLOAT(8);
DEF_FAST_ISEL_MAPPING_FLOAT(16);
DEF_FAST_ISEL_MAPPING_FLOAT(32);
DEF_FAST_ISEL_MAPPING_FLOAT(64);

#define DEF_SEL_MAPPING_TBL(SIZE)                                           \
    MOperator SelMapping##SIZE(bool isInt, uint32 x, uint32 y)              \
    {                                                                       \
        return isInt ? fastIselMapI##SIZE[x][y] : fastIselMapF##SIZE[x][y]; \
    }
#define USE_SELMAPPING_TBL(SIZE) \
    {                            \
        SIZE, SelMapping##SIZE   \
    }

DEF_SEL_MAPPING_TBL(8);
DEF_SEL_MAPPING_TBL(16);
DEF_SEL_MAPPING_TBL(32);
DEF_SEL_MAPPING_TBL(64);

std::map<uint32, std::function<MOperator(bool, uint32, uint32)>> fastIselMappingTable = {
    USE_SELMAPPING_TBL(8), USE_SELMAPPING_TBL(16), USE_SELMAPPING_TBL(32), USE_SELMAPPING_TBL(64)};

MOperator GetFastIselMop(Operand::OperandType dTy, Operand::OperandType sTy, PrimType type)
{
    uint32 bitSize = GetPrimTypeBitSize(type);
    bool isInteger = IsPrimitiveInteger(type);
    auto tableDriven = fastIselMappingTable.find(bitSize);
    if (tableDriven != fastIselMappingTable.end()) {
        auto funcIt = tableDriven->second;
        return funcIt(isInteger, dTy, sTy);
    } else {
        CHECK_FATAL(false, "unsupport type");
    }
    return abstract::MOP_undef;
}

#define DEF_EXTEND_MAPPING_TBL(TYPE) \
    [](bool isSigned) -> MOperator { return isSigned ? abstract::MOP_sext_rr_##TYPE : abstract::MOP_zext_rr_##TYPE; }
using fromToTy = std::pair<uint32, uint32>; /* std::pair<from, to> */
#define DEF_USE_EXTEND_MAPPING_TBL(FROMSIZE, TOSIZE)                                            \
    {                                                                                           \
        {k##FROMSIZE##BitSize, k##TOSIZE##BitSize}, DEF_EXTEND_MAPPING_TBL(TOSIZE##_##FROMSIZE) \
    }

std::map<fromToTy, std::function<MOperator(bool)>> fastCvtMappingTableI = {
    DEF_USE_EXTEND_MAPPING_TBL(8, 16), /* Extend Mapping */
    DEF_USE_EXTEND_MAPPING_TBL(8, 32),  DEF_USE_EXTEND_MAPPING_TBL(8, 64),  DEF_USE_EXTEND_MAPPING_TBL(16, 32),
    DEF_USE_EXTEND_MAPPING_TBL(16, 64), DEF_USE_EXTEND_MAPPING_TBL(32, 64),
};
#undef DEF_USE_EXTEND_MAPPING_TBL
#undef DEF_EXTEND_MAPPING_TBL

static MOperator GetFastCvtMopI(uint32 fromSize, uint32 toSize, bool isSigned)
{
    if (toSize < k8BitSize || toSize > k64BitSize) {
        CHECK_FATAL(false, "unsupport type");
    }
    if (fromSize < k8BitSize || fromSize > k64BitSize) {
        CHECK_FATAL(false, "unsupport type");
    }
    /* Extend: fromSize < toSize */
    auto tableDriven = fastCvtMappingTableI.find({fromSize, toSize});
    if (tableDriven == fastCvtMappingTableI.end()) {
        CHECK_FATAL(false, "unsupport cvt");
    }
    MOperator mOp = tableDriven->second(isSigned);
    if (mOp == abstract::MOP_undef) {
        CHECK_FATAL(false, "unsupport cvt");
    }
    return mOp;
}

/*
 * fast get MOperator
 * such as : and, or, shl ...
 */
#define DEF_MOPERATOR_MAPPING_FUNC(TYPE)                                                           \
    [](uint32 bitSize)->MOperator {                                                              \
        /* 8-bits,                16-bits,                   32-bits,                   64-bits */ \
        constexpr static std::array<MOperator, kBitIndexEnd> fastMapping_##TYPE = {                \
            abstract::MOP_##TYPE##_8, abstract::MOP_##TYPE##_16, abstract::MOP_##TYPE##_32,        \
            abstract::MOP_##TYPE##_64};                                                            \
        return fastMapping_##TYPE[GetBitIndex(bitSize)];                                           \
    }

#define DEF_FLOAT_MOPERATOR_MAPPING_FUNC(TYPE)                                                     \
    [](uint32 bitSize)->MOperator {                                                              \
        /* 8-bits,                16-bits,                   32-bits,                   64-bits */ \
        constexpr static std::array<MOperator, kBitIndexEnd> fastMapping_f_##TYPE = {              \
            abstract::MOP_##TYPE##_f_8, abstract::MOP_##TYPE##_f_16, abstract::MOP_##TYPE##_f_32,  \
            abstract::MOP_##TYPE##_f_64};                                                          \
        return fastMapping_f_##TYPE[GetBitIndex(bitSize)];                                         \
    }

static void HandleDassign(StmtNode &stmt, MPISel &iSel)
{
    DEBUG_ASSERT(stmt.GetOpCode() == OP_dassign, "expect dassign");
    auto &dassignNode = static_cast<DassignNode &>(stmt);
    BaseNode *rhs = dassignNode.GetRHS();
    DEBUG_ASSERT(rhs != nullptr, "get rhs of dassignNode failed");
    Operand *opndRhs = iSel.HandleExpr(dassignNode, *rhs);
    if (opndRhs == nullptr) {
        return;
    }
    iSel.SelectDassign(dassignNode, *opndRhs);
}

static void HandleIassign(StmtNode &stmt, MPISel &iSel)
{
    DEBUG_ASSERT(stmt.GetOpCode() == OP_iassign, "expect iassign");
    auto &iassignNode = static_cast<IassignNode &>(stmt);
    BaseNode *rhs = iassignNode.GetRHS();
    DEBUG_ASSERT(rhs != nullptr, "null ptr check");
    Operand *opndRhs = iSel.HandleExpr(stmt, *rhs);
    BaseNode *addr = iassignNode.Opnd(0);
    DEBUG_ASSERT(addr != nullptr, "null ptr check");
    Operand *opndAddr = iSel.HandleExpr(stmt, *addr);
    if (opndRhs == nullptr || opndAddr == nullptr) {
        return;
    }
    iSel.SelectIassign(iassignNode, *opndAddr, *opndRhs);
}

static void HandleRegassign(StmtNode &stmt, MPISel &iSel)
{
    DEBUG_ASSERT(stmt.GetOpCode() == OP_regassign, "expect regAssign");
    auto &regAssignNode = static_cast<RegassignNode &>(stmt);
    BaseNode *operand = regAssignNode.Opnd(0);
    DEBUG_ASSERT(operand != nullptr, "get operand of regassignNode failed");
    Operand *opnd0 = iSel.HandleExpr(regAssignNode, *operand);
    iSel.SelectRegassign(regAssignNode, *opnd0);
}

static void HandleLabel(StmtNode &stmt, const MPISel &iSel)
{
    CGFunc *cgFunc = iSel.GetCurFunc();
    DEBUG_ASSERT(stmt.GetOpCode() == OP_label, "error");
    auto &label = static_cast<LabelNode &>(stmt);
    BB *newBB = cgFunc->StartNewBBImpl(false, label);
    newBB->AddLabel(label.GetLabelIdx());
    cgFunc->SetLab2BBMap(static_cast<int32>(newBB->GetLabIdx()), *newBB);
    cgFunc->SetCurBB(*newBB);
}

static void HandleGoto(StmtNode &stmt, MPISel &iSel)
{
    CGFunc *cgFunc = iSel.GetCurFunc();
    auto &gotoNode = static_cast<GotoNode &>(stmt);
    DEBUG_ASSERT(gotoNode.GetOpCode() == OP_goto, "expect goto");
    cgFunc->SetCurBBKind(BB::kBBGoto);
    iSel.SelectGoto(gotoNode);
    cgFunc->SetCurBB(*cgFunc->StartNewBB(gotoNode));
    DEBUG_ASSERT(&stmt == &gotoNode, "stmt must be same as gotoNoe");
    if ((gotoNode.GetNext() != nullptr) && (gotoNode.GetNext()->GetOpCode() != OP_label)) {
        DEBUG_ASSERT(cgFunc->GetCurBB()->GetPrev()->GetLastStmt() == &stmt, "check the relation between BB and stmt");
    }
}

static void HandleIntrinCall(StmtNode &stmt, MPISel &iSel)
{
    auto &call = static_cast<IntrinsiccallNode &>(stmt);
    iSel.SelectIntrinsicCall(call);
}

static void HandleRangeGoto(StmtNode &stmt, MPISel &iSel)
{
    CGFunc *cgFunc = iSel.GetCurFunc();
    auto &rangeGotoNode = static_cast<RangeGotoNode &>(stmt);
    DEBUG_ASSERT(rangeGotoNode.GetOpCode() == OP_rangegoto, "expect rangegoto");
    BaseNode *srcNode = rangeGotoNode.Opnd(0);
    Operand *srcOpnd = iSel.HandleExpr(rangeGotoNode, *srcNode);
    cgFunc->SetCurBBKind(BB::kBBRangeGoto);
    iSel.SelectRangeGoto(rangeGotoNode, *srcOpnd);
}

void HandleReturn(StmtNode &stmt, MPISel &iSel)
{
    CGFunc *cgFunc = iSel.GetCurFunc();
    auto &retNode = static_cast<NaryStmtNode &>(stmt);
    DEBUG_ASSERT(retNode.NumOpnds() <= 1, "NYI return nodes number > 1");
    if (retNode.NumOpnds() != 0) {
        Operand *opnd = iSel.HandleExpr(retNode, *retNode.Opnd(0));
        iSel.SelectReturn(retNode, *opnd);
    }
    iSel.SelectReturn();
    /* return stmt will jump to the ret BB, so curBB is gotoBB */
    cgFunc->SetCurBBKind(BB::kBBGoto);
    cgFunc->SetCurBB(*cgFunc->StartNewBB(retNode));
}

static void HandleComment(StmtNode &stmt, MPISel &iSel)
{
    return;
}

static void HandleIcall(StmtNode &stmt, MPISel &iSel)
{
    DEBUG_ASSERT(stmt.GetOpCode() == OP_icall || stmt.GetOpCode() == OP_icallproto, "error");
    auto &iCallNode = static_cast<IcallNode &>(stmt);
    iSel.SelectIcall(iCallNode);
    iSel.SelectCallCommon(stmt, iSel);
}

static void HandleCall(StmtNode &stmt, MPISel &iSel)
{
    DEBUG_ASSERT(stmt.GetOpCode() == OP_call, "error");
    auto &callNode = static_cast<CallNode &>(stmt);
    iSel.SelectCall(callNode);
    iSel.SelectCallCommon(stmt, iSel);
}

static void HandleCondbr(StmtNode &stmt, MPISel &iSel)
{
    CGFunc *cgFunc = iSel.GetCurFunc();
    auto &condGotoNode = static_cast<CondGotoNode &>(stmt);
    BaseNode *condNode = condGotoNode.Opnd(0);
    DEBUG_ASSERT(condNode != nullptr, "expect first operand of cond br");
    /* select cmpOp Insn and get the result "opnd0". However, the opnd0 is not used
     * in most backend architectures */
    Operand *opnd0 = iSel.HandleExpr(stmt, *condNode);
    iSel.SelectCondGoto(condGotoNode, *condNode, *opnd0);
    cgFunc->SetCurBB(*cgFunc->StartNewBB(condGotoNode));
}

static Operand *HandleShift(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    return iSel.SelectShift(static_cast<BinaryNode &>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                            *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleCvt(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    return iSel.SelectCvt(parent, static_cast<TypeCvtNode &>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)));
}

static Operand *HandleExtractBits(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    return iSel.SelectExtractbits(parent, static_cast<ExtractbitsNode &>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)));
}

static Operand *HandleDread(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    auto &dreadNode = static_cast<AddrofNode &>(expr);
    return iSel.SelectDread(parent, dreadNode);
}

static Operand *HandleAdd(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    return iSel.SelectAdd(static_cast<BinaryNode &>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                          *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleBior(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    return iSel.SelectBior(static_cast<BinaryNode &>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                           *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleBxor(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    DEBUG_ASSERT(expr.Opnd(1) != nullptr, "nullptr check");
    return iSel.SelectBxor(static_cast<BinaryNode &>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                           *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleSub(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    return iSel.SelectSub(static_cast<BinaryNode &>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                          *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleDiv(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    return iSel.SelectDiv(static_cast<BinaryNode &>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                          *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleRem(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    return iSel.SelectRem(static_cast<BinaryNode &>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                          *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleBand(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    return iSel.SelectBand(static_cast<BinaryNode &>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                           *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleMpy(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    return iSel.SelectMpy(static_cast<BinaryNode &>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                          *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleTrunc(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    DEBUG_ASSERT(expr.Opnd(0) != nullptr, "null ptr check");
    return iSel.SelectCvt(parent, static_cast<TypeCvtNode &>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)));
}

static Operand *HandleConstVal(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    auto &constValNode = static_cast<ConstvalNode &>(expr);
    MIRConst *mirConst = constValNode.GetConstVal();
    DEBUG_ASSERT(mirConst != nullptr, "get constval of constvalnode failed");
    if (mirConst->GetKind() == kConstInt) {
        auto *mirIntConst = safe_cast<MIRIntConst>(mirConst);
        return iSel.SelectIntConst(*mirIntConst, constValNode.GetPrimType());
    } else if (mirConst->GetKind() == kConstDoubleConst) {
        auto *mirDoubleConst = safe_cast<MIRDoubleConst>(mirConst);
        return iSel.SelectFloatingConst(*mirDoubleConst, constValNode.GetPrimType());
    } else if (mirConst->GetKind() == kConstFloatConst) {
        auto *mirFloatConst = safe_cast<MIRFloatConst>(mirConst);
        DEBUG_ASSERT(mirFloatConst != nullptr, "nullptr check");
        return iSel.SelectFloatingConst(*mirFloatConst, constValNode.GetPrimType());
    } else {
        CHECK_FATAL(false, "NIY");
    }
    return nullptr;
}

static Operand *HandleRegread(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    (void)parent;
    auto &regReadNode = static_cast<RegreadNode &>(expr);
    /* handle return Val */
    if (regReadNode.GetRegIdx() == -kSregRetval0 || regReadNode.GetRegIdx() == -kSregRetval1) {
        return &iSel.ProcessReturnReg(regReadNode.GetPrimType(), -(regReadNode.GetRegIdx()));
    }
    return iSel.SelectRegread(regReadNode);
}

static Operand *HandleIread(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    auto &ireadNode = static_cast<IreadNode &>(expr);
    return iSel.SelectIread(parent, ireadNode);
}

static Operand *HandleBnot(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    return iSel.SelectBnot(static_cast<UnaryNode &>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
}

static Operand *HandleLnot(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    return iSel.SelectLnot(static_cast<UnaryNode &>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
}

static Operand *HandleCmp(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    // fix opnd type before select insn
    PrimType targetPtyp = parent.GetPrimType();
    if (kOpcodeInfo.IsCompare(parent.GetOpCode())) {
        targetPtyp = static_cast<const CompareNode &>(parent).GetOpndType();
    } else if (kOpcodeInfo.IsTypeCvt(parent.GetOpCode())) {
        targetPtyp = static_cast<const TypeCvtNode &>(parent).FromType();
    }
    if ((IsPrimitiveInteger(targetPtyp) || IsPrimitiveFloat(targetPtyp)) && targetPtyp != expr.GetPrimType()) {
        expr.SetPrimType(targetPtyp);
    }
    return iSel.SelectCmpOp(static_cast<CompareNode &>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                            *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleAbs(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    return iSel.SelectAbs(static_cast<UnaryNode &>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)));
}

static Operand *HandleMin(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    return iSel.SelectMin(static_cast<BinaryNode &>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                          *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}

static Operand *HandleMax(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    return iSel.SelectMax(static_cast<BinaryNode &>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)),
                          *iSel.HandleExpr(expr, *expr.Opnd(1)), parent);
}
static Operand *HandleRetype(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    DEBUG_ASSERT(expr.Opnd(0) != nullptr, "expr.Opnd(0) should not be nullptr");
    return iSel.SelectRetype(static_cast<TypeCvtNode &>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)));
}

static Operand *HandleIntrinOp(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    auto &intrinsicopNode = static_cast<IntrinsicopNode &>(expr);
    switch (intrinsicopNode.GetIntrinsic()) {
        case INTRN_C_clz32:
        case INTRN_C_clz64:
            return iSel.SelectCclz(intrinsicopNode, *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
        default:
            DEBUG_ASSERT(false, "NIY, unsupported intrinsicop.");
            return nullptr;
    }
}

static Operand *HandleSqrt(const BaseNode &parent, BaseNode &expr, MPISel &iSel)
{
    DEBUG_ASSERT(expr.Opnd(0) != nullptr, "expr.Opnd(0) should not be nullptr");
    return iSel.SelectSqrt(static_cast<UnaryNode &>(expr), *iSel.HandleExpr(expr, *expr.Opnd(0)), parent);
}

using HandleStmtFactory = FunctionFactory<Opcode, void, StmtNode &, MPISel &>;
using HandleExprFactory = FunctionFactory<Opcode, maplebe::Operand *, const BaseNode &, BaseNode &, MPISel &>;
namespace isel {
static void InitHandleStmtFactory()
{
    RegisterFactoryFunction<HandleStmtFactory>(OP_label, HandleLabel);
    RegisterFactoryFunction<HandleStmtFactory>(OP_dassign, HandleDassign);
    RegisterFactoryFunction<HandleStmtFactory>(OP_iassign, HandleIassign);
    RegisterFactoryFunction<HandleStmtFactory>(OP_regassign, HandleRegassign);
    RegisterFactoryFunction<HandleStmtFactory>(OP_return, HandleReturn);
    RegisterFactoryFunction<HandleStmtFactory>(OP_comment, HandleComment);
    RegisterFactoryFunction<HandleStmtFactory>(OP_call, HandleCall);
    RegisterFactoryFunction<HandleStmtFactory>(OP_icall, HandleIcall);
    RegisterFactoryFunction<HandleStmtFactory>(OP_icallproto, HandleIcall);
    RegisterFactoryFunction<HandleStmtFactory>(OP_goto, HandleGoto);
    RegisterFactoryFunction<HandleStmtFactory>(OP_intrinsiccall, HandleIntrinCall);
    RegisterFactoryFunction<HandleStmtFactory>(OP_intrinsiccallassigned, HandleIntrinCall);
    RegisterFactoryFunction<HandleStmtFactory>(OP_intrinsiccallwithtype, HandleIntrinCall);
    RegisterFactoryFunction<HandleStmtFactory>(OP_rangegoto, HandleRangeGoto);
    RegisterFactoryFunction<HandleStmtFactory>(OP_brfalse, HandleCondbr);
    RegisterFactoryFunction<HandleStmtFactory>(OP_brtrue, HandleCondbr);
}
static void InitHandleExprFactory()
{
    RegisterFactoryFunction<HandleExprFactory>(OP_dread, HandleDread);
    RegisterFactoryFunction<HandleExprFactory>(OP_add, HandleAdd);
    RegisterFactoryFunction<HandleExprFactory>(OP_sub, HandleSub);
    RegisterFactoryFunction<HandleExprFactory>(OP_mul, HandleMpy);
    RegisterFactoryFunction<HandleExprFactory>(OP_constval, HandleConstVal);
    RegisterFactoryFunction<HandleExprFactory>(OP_regread, HandleRegread);
    RegisterFactoryFunction<HandleExprFactory>(OP_shl, HandleShift);
    RegisterFactoryFunction<HandleExprFactory>(OP_lshr, HandleShift);
    RegisterFactoryFunction<HandleExprFactory>(OP_ashr, HandleShift);
    RegisterFactoryFunction<HandleExprFactory>(OP_cvt, HandleCvt);
    RegisterFactoryFunction<HandleExprFactory>(OP_zext, HandleExtractBits);
    RegisterFactoryFunction<HandleExprFactory>(OP_sext, HandleExtractBits);
    RegisterFactoryFunction<HandleExprFactory>(OP_band, HandleBand);
    RegisterFactoryFunction<HandleExprFactory>(OP_bior, HandleBior);
    RegisterFactoryFunction<HandleExprFactory>(OP_bxor, HandleBxor);
    RegisterFactoryFunction<HandleExprFactory>(OP_iread, HandleIread);
    RegisterFactoryFunction<HandleExprFactory>(OP_bnot, HandleBnot);
    RegisterFactoryFunction<HandleExprFactory>(OP_lnot, HandleLnot);
    RegisterFactoryFunction<HandleExprFactory>(OP_div, HandleDiv);
    RegisterFactoryFunction<HandleExprFactory>(OP_rem, HandleRem);
    RegisterFactoryFunction<HandleExprFactory>(OP_le, HandleCmp);
    RegisterFactoryFunction<HandleExprFactory>(OP_ge, HandleCmp);
    RegisterFactoryFunction<HandleExprFactory>(OP_gt, HandleCmp);
    RegisterFactoryFunction<HandleExprFactory>(OP_lt, HandleCmp);
    RegisterFactoryFunction<HandleExprFactory>(OP_ne, HandleCmp);
    RegisterFactoryFunction<HandleExprFactory>(OP_eq, HandleCmp);
    RegisterFactoryFunction<HandleExprFactory>(OP_abs, HandleAbs);
    RegisterFactoryFunction<HandleExprFactory>(OP_min, HandleMin);
    RegisterFactoryFunction<HandleExprFactory>(OP_max, HandleMax);
    RegisterFactoryFunction<HandleExprFactory>(OP_retype, HandleRetype);
    RegisterFactoryFunction<HandleExprFactory>(OP_trunc, HandleTrunc);
    RegisterFactoryFunction<HandleExprFactory>(OP_intrinsicop, HandleIntrinOp);
    RegisterFactoryFunction<HandleExprFactory>(OP_sqrt, HandleSqrt);
}
}  // namespace isel

Operand *MPISel::HandleExpr(const BaseNode &parent, BaseNode &expr)
{
    auto function = CreateProductFunction<HandleExprFactory>(expr.GetOpCode());
    CHECK_FATAL(function != nullptr, "unsupported %d opCode in HandleExpr()", expr.GetOpCode());
    Operand *opnd = function(parent, expr, *this);
    return opnd;
}

void MPISel::doMPIS()
{
    isel::InitHandleStmtFactory();
    isel::InitHandleExprFactory();
    StmtNode *secondStmt = HandleFuncEntry();
    for (StmtNode *stmt = secondStmt; stmt != nullptr; stmt = stmt->GetNext()) {
        auto function = CreateProductFunction<HandleStmtFactory>(stmt->GetOpCode());
        CHECK_FATAL(function != nullptr, "unsupported opCode or has been lowered before");
        cgFunc->GetInsnBuilder()->SetDebugComment(stmt->GetDebugComment());
        function(*stmt, *this);
        cgFunc->GetInsnBuilder()->ClearDebugComment();
    }
    HandleFuncExit();
}

PrimType MPISel::GetIntegerPrimTypeFromSize(bool isSigned, uint32 bitSize)
{
    static constexpr std::array<PrimType, kBitIndexEnd> signedPrimType = {PTY_i8, PTY_i16, PTY_i32, PTY_i64};
    static constexpr std::array<PrimType, kBitIndexEnd> unsignedPrimType = {PTY_u8, PTY_u16, PTY_u32, PTY_u64};
    BitIndex index = GetBitIndex(bitSize);
    return isSigned ? signedPrimType[index] : unsignedPrimType[index];
}

void MPISel::SelectCallCommon(StmtNode &stmt, const MPISel &iSel)
{
    CGFunc *cgFunc = iSel.GetCurFunc();
    if (cgFunc->GetCurBB()->GetKind() != BB::kBBFallthru) {
        cgFunc->SetCurBB(*cgFunc->StartNewBB(stmt));
    }
}

void MPISel::SelectBasicOp(Operand &resOpnd, Operand &opnd0, Operand &opnd1, MOperator mOp, PrimType primType)
{
    RegOperand &firstOpnd = SelectCopy2Reg(opnd0, primType);
    RegOperand &secondOpnd = SelectCopy2Reg(opnd1, primType);
    Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDesc::GetAbstractId(mOp));
    (void)insn.AddOpndChain(resOpnd).AddOpndChain(firstOpnd).AddOpndChain(secondOpnd);
    cgFunc->GetCurBB()->AppendInsn(insn);
}

std::pair<FieldID, MIRType *> MPISel::GetFieldIdAndMirTypeFromMirNode(const BaseNode &node)
{
    FieldID fieldId = 0;
    MIRType *mirType = nullptr;
    if (node.GetOpCode() == maple::OP_iread) {
        /* mirType stored in an addr. */
        auto &iread = static_cast<const IreadNode &>(node);
        fieldId = iread.GetFieldID();
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread.GetTyIdx());
        MIRPtrType *pointerType = static_cast<MIRPtrType *>(type);
        DEBUG_ASSERT(pointerType != nullptr, "expect a pointer type at iread node");
        mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerType->GetPointedTyIdx());
        if (mirType->GetKind() == kTypeArray) {
            MIRArrayType *arrayType = static_cast<MIRArrayType *>(mirType);
            mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(arrayType->GetElemTyIdx());
        }
    } else if (node.GetOpCode() == maple::OP_dassign) {
        /* mirSymbol */
        auto &dassign = static_cast<const DassignNode &>(node);
        fieldId = dassign.GetFieldID();
        MIRSymbol *symbol = cgFunc->GetFunction().GetLocalOrGlobalSymbol(dassign.GetStIdx());
        mirType = symbol->GetType();
    } else if (node.GetOpCode() == maple::OP_dread) {
        /* mirSymbol */
        auto &dread = static_cast<const AddrofNode &>(node);
        fieldId = dread.GetFieldID();
        MIRSymbol *symbol = cgFunc->GetFunction().GetLocalOrGlobalSymbol(dread.GetStIdx());
        mirType = symbol->GetType();
    } else if (node.GetOpCode() == maple::OP_iassign) {
        auto &iassign = static_cast<const IassignNode &>(node);
        fieldId = iassign.GetFieldID();
        MIRType *iassignMirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iassign.GetTyIdx());
        MIRPtrType *pointerType = nullptr;
        DEBUG_ASSERT(iassignMirType->GetKind() == kTypePointer, "non-pointer");
        pointerType = static_cast<MIRPtrType *>(iassignMirType);
        mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pointerType->GetPointedTyIdx());
    } else {
        CHECK_FATAL(false, "unsupported OpCode");
    }
    return {fieldId, mirType};
}

MirTypeInfo MPISel::GetMirTypeInfoFormFieldIdAndMirType(FieldID fieldId, MIRType *mirType)
{
    MirTypeInfo mirTypeInfo;
    /* fixup primType and offset */
    mirTypeInfo.primType = mirType->GetPrimType();
    return mirTypeInfo;
}

MirTypeInfo MPISel::GetMirTypeInfoFromMirNode(const BaseNode &node)
{
    auto [fieldId, mirType] = GetFieldIdAndMirTypeFromMirNode(node);
    return GetMirTypeInfoFormFieldIdAndMirType(fieldId, mirType);
}

void MPISel::SelectDassign(const DassignNode &stmt, Operand &opndRhs)
{
    /* mirSymbol info */
    MIRSymbol *symbol = cgFunc->GetFunction().GetLocalOrGlobalSymbol(stmt.GetStIdx());
    MirTypeInfo symbolInfo = GetMirTypeInfoFromMirNode(stmt);
    /* Get symbol location */
    DEBUG_ASSERT(symbol != nullptr, "nullptr check");
    MemOperand &symbolMem = GetOrCreateMemOpndFromSymbol(*symbol, stmt.GetFieldID());
    /* rhs mirType info */
    PrimType rhsType = stmt.GetRHS()->GetPrimType();
    /* Generate Insn */
    PrimType memType = symbolInfo.primType;
    SelectCopy(symbolMem, opndRhs, memType, rhsType);
    if (rhsType == PTY_ref) {
        cgFunc->AddReferenceStackSlot(symbolMem.GetOffsetImmediate()->GetOffsetValue());
    }

    return;
}

void MPISel::SelectIassign(const IassignNode &stmt, Operand &opndAddr, Operand &opndRhs)
{
    /* mirSymbol info */
    MirTypeInfo symbolInfo = GetMirTypeInfoFromMirNode(stmt);
    /* handle Lhs, generate (%Rxx) via Rxx*/
    PrimType memType = symbolInfo.primType;
    RegOperand &lhsBaseOpnd = SelectCopy2Reg(opndAddr, stmt.Opnd(0)->GetPrimType());
    MemOperand &lhsMemOpnd =
        cgFunc->GetOpndBuilder()->CreateMem(lhsBaseOpnd, symbolInfo.offset, GetPrimTypeBitSize(memType));
    /* handle Rhs, get R## from Rhs */
    PrimType rhsType = stmt.GetRHS()->GetPrimType();
    /* mov %R##, (%Rxx) */
    SelectCopy(lhsMemOpnd, opndRhs, memType, rhsType);
}

ImmOperand *MPISel::SelectIntConst(const MIRIntConst &intConst, PrimType primType)
{
    return &cgFunc->GetOpndBuilder()->CreateImm(GetPrimTypeBitSize(primType), intConst.GetExtValue());
}

Operand *MPISel::SelectShift(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    PrimType primType = node.GetPrimType();
    RegOperand *resOpnd = nullptr;
    Opcode opcode = node.GetOpCode();

    if (IsPrimitiveInteger(primType)) {
        resOpnd =
            &(cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType), cgFunc->GetRegTyFromPrimTy(primType)));
        RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, primType, node.Opnd(0)->GetPrimType());
        SelectShift(*resOpnd, regOpnd0, opnd1, opcode, primType, node.Opnd(1)->GetPrimType());
    } else {
        CHECK_FATAL(false, "NIY vector cvt");
    }
    return resOpnd;
}

void MPISel::SelectShift(Operand &resOpnd, Operand &opnd0, Operand &opnd1, Opcode shiftDirect, PrimType opnd0Type,
                         PrimType opnd1Type)
{
    if (opnd1.IsIntImmediate() && static_cast<ImmOperand &>(opnd1).GetValue() == 0) {
        SelectCopy(resOpnd, opnd0, opnd0Type);
        return;
    }

    uint32 dsize = GetPrimTypeBitSize(opnd0Type);
    MOperator mOp = abstract::MOP_undef;
    if (shiftDirect == OP_shl) {
        const static auto fastShlMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(shl);
        mOp = fastShlMappingFunc(dsize);
    } else if (shiftDirect == OP_ashr) {
        const static auto fastAshrMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(ashr);
        mOp = fastAshrMappingFunc(dsize);
    } else if (shiftDirect == OP_lshr) {
        const static auto fastLshrMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(lshr);
        mOp = fastLshrMappingFunc(dsize);
    } else {
        CHECK_FATAL(false, "NIY, Not support shiftdirect case");
    }
    RegOperand &firstOpnd = SelectCopy2Reg(opnd0, opnd0Type);
    RegOperand &secondOpnd = SelectCopy2Reg(opnd1, opnd1Type);
    Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDesc::GetAbstractId(mOp));
    insn.AddOpndChain(resOpnd).AddOpndChain(firstOpnd).AddOpndChain(secondOpnd);
    cgFunc->GetCurBB()->AppendInsn(insn);
}

void MPISel::SelectRegassign(RegassignNode &stmt, Operand &opnd0)
{
    PrimType rhsType = stmt.Opnd(0)->GetPrimType();
    PregIdx pregIdx = stmt.GetRegIdx();
    PrimType regType = stmt.GetPrimType();
    RegOperand &regOpnd =
        cgFunc->GetOpndBuilder()->CreateVReg(cgFunc->GetVirtualRegNOFromPseudoRegIdx(pregIdx),
                                             GetPrimTypeBitSize(regType), cgFunc->GetRegTyFromPrimTy(regType));
    SelectCopy(regOpnd, opnd0, regType, rhsType);
    if (stmt.GetPrimType() == PTY_ref) {
        regOpnd.SetIsReference(true);
        cgFunc->AddReferenceReg(regOpnd.GetRegisterNumber());
    }
    if (pregIdx > 0) {
        // special MIRPreg is not supported
        cgFunc->SetPregIdx2Opnd(pregIdx, regOpnd);
    }
    const auto &derived2BaseRef = cgFunc->GetFunction().GetDerived2BaseRef();
    auto itr = derived2BaseRef.find(pregIdx);
    if (itr != derived2BaseRef.end()) {
        auto *opnd = cgFunc->GetOpndFromPregIdx(itr->first);
        CHECK_FATAL(opnd != nullptr, "pregIdx has not been assigned Operand");
        auto &derivedRegOpnd = static_cast<RegOperand &>(*opnd);
        opnd = cgFunc->GetOpndFromPregIdx(itr->second);
        CHECK_FATAL(opnd != nullptr, "pregIdx has not been assigned Operand");
        auto &baseRegOpnd = static_cast<RegOperand &>(*opnd);
        derivedRegOpnd.SetBaseRefOpnd(baseRegOpnd);
    }
    if ((Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0) && (pregIdx >= 0)) {
        const SymbolAlloc *symLoc = cgFunc->GetMemlayout()->GetSpillLocOfPseduoRegister(pregIdx);
        int64 offset = static_cast<int64>(cgFunc->GetBaseOffset(*symLoc));
        MIRPreg *preg = cgFunc->GetFunction().GetPregTab()->PregFromPregIdx(pregIdx);
        uint32 bitLen = GetPrimTypeSize(preg->GetPrimType()) * kBitsPerByte;
        RegOperand &base = GetTargetBasicPointer(PTY_u64);
        MemOperand *dest = &cgFunc->GetOpndBuilder()->CreateMem(base, offset, bitLen);
        SelectCopy(*dest, regOpnd, preg->GetPrimType(), regType);
    }
}

RegOperand *MPISel::SelectRegread(RegreadNode &expr)
{
    PregIdx pregIdx = expr.GetRegIdx();
    PrimType rhsType = expr.GetPrimType();
    if (pregIdx < 0) {
        return &SelectSpecialRegread(pregIdx, rhsType);
    }

    RegOperand &reg = cgFunc->GetOpndBuilder()->CreateVReg(cgFunc->GetVirtualRegNOFromPseudoRegIdx(pregIdx),
                                                           GetPrimTypeSize(rhsType) * kBitsPerByte,
                                                           cgFunc->GetRegTyFromPrimTy(rhsType));
    if (cgFunc->GetOpndFromPregIdx(pregIdx) == nullptr) {
        cgFunc->SetPregIdx2Opnd(pregIdx, reg);
    }
    if (expr.GetPrimType() == maple::PTY_ref) {
        reg.SetIsReference(true);
        cgFunc->AddReferenceReg(reg.GetRegisterNumber());
    }
    if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0) {
        const SymbolAlloc *symLoc = cgFunc->GetMemlayout()->GetSpillLocOfPseduoRegister(pregIdx);
        int64 offset = static_cast<int64>(cgFunc->GetBaseOffset(*symLoc));
        MIRPreg *preg = cgFunc->GetFunction().GetPregTab()->PregFromPregIdx(pregIdx);
        uint32 bitLen = GetPrimTypeSize(preg->GetPrimType()) * kBitsPerByte;
        RegOperand &base = GetTargetBasicPointer(PTY_u64);
        MemOperand *src = &cgFunc->GetOpndBuilder()->CreateMem(base, offset, bitLen);
        SelectCopy(reg, *src, rhsType, preg->GetPrimType());
    }
    return &reg;
}

Operand *MPISel::SelectDread(const BaseNode &parent, const AddrofNode &expr)
{
    /* get mirSymbol info*/
    MIRSymbol *symbol = cgFunc->GetFunction().GetLocalOrGlobalSymbol(expr.GetStIdx());
    MirTypeInfo symbolInfo = GetMirTypeInfoFromMirNode(expr);
    PrimType symbolType = symbolInfo.primType;
    /* Get symbol location */
    MemOperand &symbolMem = GetOrCreateMemOpndFromSymbol(*symbol, expr.GetFieldID());
    PrimType primType = expr.GetPrimType();
    if (primType == PTY_ref) {
        cgFunc->AddReferenceStackSlot(symbolMem.GetOffsetImmediate()->GetOffsetValue());
    }

    /* for BasicType, load symbolVal to register. */
    RegOperand &regOpnd =
        cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType), cgFunc->GetRegTyFromPrimTy(primType));
    if (primType == PTY_ref) {
        regOpnd.SetIsReference(true);
        cgFunc->AddReferenceReg(regOpnd.GetRegisterNumber());
    }
    /* Generate Insn */
    SelectCopy(regOpnd, symbolMem, primType, symbolType);
    return &regOpnd;
}

Operand *MPISel::SelectAdd(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    PrimType primType = node.GetPrimType();
    RegOperand &resReg =
        cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType), cgFunc->GetRegTyFromPrimTy(primType));
    RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, primType, node.Opnd(0)->GetPrimType());
    RegOperand &regOpnd1 = SelectCopy2Reg(opnd1, primType, node.Opnd(1)->GetPrimType());
    SelectAdd(resReg, regOpnd0, regOpnd1, primType);
    return &resReg;
}

Operand *MPISel::SelectBand(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    PrimType primType = node.GetPrimType();
    RegOperand &resReg =
        cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType), cgFunc->GetRegTyFromPrimTy(primType));
    RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, primType, node.Opnd(0)->GetPrimType());
    RegOperand &regOpnd1 = SelectCopy2Reg(opnd1, primType, node.Opnd(1)->GetPrimType());
    SelectBand(resReg, regOpnd0, regOpnd1, primType);
    return &resReg;
}

Operand *MPISel::SelectSub(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    PrimType primType = node.GetPrimType();
    RegOperand &resOpnd =
        cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType), cgFunc->GetRegTyFromPrimTy(primType));
    RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, primType, node.Opnd(0)->GetPrimType());
    RegOperand &regOpnd1 = SelectCopy2Reg(opnd1, primType, node.Opnd(1)->GetPrimType());
    SelectSub(resOpnd, regOpnd0, regOpnd1, primType);
    return &resOpnd;
}

void MPISel::SelectExtractbits(RegOperand &resOpnd, RegOperand &opnd0, uint8 bitOffset, uint8 bitSize,
                               PrimType primType)
{
    uint32 primBitSize = GetPrimTypeBitSize(primType);
    bool isSigned = IsSignedInteger(primType);
    if (bitOffset == 0 && !isSigned) {
        /*
         * resOpnd = opnd0 & ((1 << bitSize) - 1)
         */
        ImmOperand &imm = cgFunc->GetOpndBuilder()->CreateImm(primBitSize, (static_cast<int64>(1) << bitSize) - 1);
        SelectBand(resOpnd, opnd0, imm, primType);
    } else {
        /*
         * tmpOpnd = opnd0 << (primBitSize - bitSize - bitOffset)
         * resOpnd = tmpOpnd >> (primBitSize - bitSize)
         * if signed : use sar; else use shr
         */
        RegOperand &tmpOpnd =
            cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType), cgFunc->GetRegTyFromPrimTy(primType));
        ImmOperand &imm1Opnd = cgFunc->GetOpndBuilder()->CreateImm(primBitSize, primBitSize - bitSize - bitOffset);
        SelectShift(tmpOpnd, opnd0, imm1Opnd, OP_shl, primType, primType);
        Opcode opcode = isSigned ? OP_ashr : OP_lshr;
        ImmOperand &imm2Opnd = cgFunc->GetOpndBuilder()->CreateImm(primBitSize, primBitSize - bitSize);
        SelectShift(resOpnd, tmpOpnd, imm2Opnd, opcode, primType, primType);
    }
}

Operand *MPISel::SelectExtractbits(const BaseNode &parent, const ExtractbitsNode &node, Operand &opnd0)
{
    PrimType fromType = node.Opnd(0)->GetPrimType();
    PrimType toType = node.GetPrimType();
    uint8 bitSize = node.GetBitsSize();
    RegOperand &resOpnd =
        cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(toType), cgFunc->GetRegTyFromPrimTy(toType));
    if (IsPrimitiveInteger(toType)) {
        // OP_extractbits or bitSize < 8-bit or bitSize is not pow of 2
        if (node.GetOpCode() == OP_extractbits || bitSize < k8BitSize || (bitSize & (bitSize - 1)) != 0) {
            SelectCopy(resOpnd, opnd0, toType, fromType);
            SelectExtractbits(resOpnd, resOpnd, node.GetBitsOffset(), bitSize, toType);
        } else {
            PrimType opndType = GetIntegerPrimTypeFromSize(node.GetOpCode() == OP_sext, bitSize);
            RegOperand &tmpRegOpnd = SelectCopy2Reg(opnd0, opndType, fromType);
            SelectIntCvt(resOpnd, tmpRegOpnd, toType, opndType);
        }
    } else {
        CHECK_FATAL(false, "NIY vector cvt");
    }
    return &resOpnd;
}

Operand *MPISel::SelectCvt(const BaseNode &parent, const TypeCvtNode &node, Operand &opnd0)
{
    PrimType fromType = node.Opnd(0)->GetPrimType();
    PrimType toType = node.GetPrimType();
    if (fromType == toType) {
        return &opnd0;
    }
    RegOperand *resOpnd =
        &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(toType), cgFunc->GetRegTyFromPrimTy(toType));
    if (IsPrimitiveInteger(toType) && IsPrimitiveInteger(fromType)) {
        SelectIntCvt(*resOpnd, opnd0, toType, fromType);
    } else if (IsPrimitiveFloat(toType) && IsPrimitiveInteger(fromType)) {
        SelectCvtInt2Float(*resOpnd, opnd0, toType, fromType);
    } else if (IsPrimitiveFloat(toType) && IsPrimitiveFloat(fromType)) {
        SelectFloatCvt(*resOpnd, opnd0, toType, fromType);
    } else if (IsPrimitiveInteger(toType) && IsPrimitiveFloat(fromType)) {
        SelectCvtFloat2Int(*resOpnd, opnd0, toType, fromType);
    } else {
        CHECK_FATAL(false, "NIY cvt");
    }
    return resOpnd;
}

void MPISel::SelectCvtFloat2Int(RegOperand &resOpnd, Operand &opnd0, PrimType toType, PrimType fromType)
{
    uint32 toSize = GetPrimTypeBitSize(toType);
    bool isSigned = !IsPrimitiveUnsigned(toType);

    // Due to fp precision, should use one insn to perform cvt.
    RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, fromType);
    MOperator mOp = abstract::MOP_undef;
    switch (fromType) {
        case PTY_f64:
            mOp = (toSize <= k32BitSize) ? ((isSigned) ? abstract::MOP_cvt_i32_f64 : abstract::MOP_cvt_u32_f64) :
                                           ((isSigned) ? abstract::MOP_cvt_i64_f64 : abstract::MOP_cvt_u64_f64);
            break;
        case PTY_f32:
            mOp = (toSize <= k32BitSize) ? ((isSigned) ? abstract::MOP_cvt_i32_f32 : abstract::MOP_cvt_u32_f32) :
                                           ((isSigned) ? abstract::MOP_cvt_i64_f32 : abstract::MOP_cvt_u64_f32);
            break;
        default:
            CHECK_FATAL(false, "NYI");
    }
    Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDesc::GetAbstractId(mOp));
    (void)insn.AddOpndChain(resOpnd).AddOpndChain(regOpnd0);
    cgFunc->GetCurBB()->AppendInsn(insn);
}

void MPISel::SelectCvtInt2Float(RegOperand &resOpnd, Operand &opnd0, PrimType toType, PrimType fromType)
{
    uint32 fromSize = GetPrimTypeBitSize(fromType);
    bool isSigned = !IsPrimitiveUnsigned(fromType);
    MOperator mOp = abstract::MOP_undef;
    switch (toType) {
        case PTY_f64:
            mOp = (fromSize <= k32BitSize) ? ((isSigned) ? abstract::MOP_cvt_f64_i32 : abstract::MOP_cvt_f64_u32) :
                                             ((isSigned) ? abstract::MOP_cvt_f64_i64 : abstract::MOP_cvt_f64_u64);
            break;
        case PTY_f32:
            mOp = (fromSize <= k32BitSize) ? ((isSigned) ? abstract::MOP_cvt_f32_i32 : abstract::MOP_cvt_f32_u32) :
                                             ((isSigned) ? abstract::MOP_cvt_f32_i64 : abstract::MOP_cvt_f32_u64);
            break;
        default:
            CHECK_FATAL(false, "NYI");
    }
    RegOperand *regOpnd0 = nullptr;
    if (!isSigned && fromSize <= k32BitSize && toType == PTY_f64) {
        regOpnd0 = &cgFunc->GetOpndBuilder()->CreateVReg(k64BitSize, kRegTyInt);
        SelectIntCvt(*regOpnd0, opnd0, maple::PTY_i64, fromType);
    } else {
        regOpnd0 = &SelectCopy2Reg(opnd0, fromType);
    }
    Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDesc::GetAbstractId(mOp));
    (void)insn.AddOpndChain(resOpnd).AddOpndChain(*regOpnd0);
    cgFunc->GetCurBB()->AppendInsn(insn);
}

void MPISel::SelectIntCvt(RegOperand &resOpnd, Operand &opnd0, PrimType toType, PrimType fromType)
{
    uint32 fromSize = GetPrimTypeBitSize(fromType);
    uint32 toSize = GetPrimTypeBitSize(toType);
    /*
     * It is redundancy to insert "nop" casts (unsigned 32 -> singed 32) in abstract CG IR
     * The signedness of operands would be shown in the expression.
     */
    RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, fromType);
    if (toSize <= fromSize) {
        resOpnd = cgFunc->GetOpndBuilder()->CreateVReg(regOpnd0.GetRegisterNumber(), GetPrimTypeBitSize(toType),
                                                       cgFunc->GetRegTyFromPrimTy(toType));
        return;
    }
    bool isSigned = !IsPrimitiveUnsigned(fromType);
    MOperator mOp = GetFastCvtMopI(fromSize, toSize, isSigned);
    Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDesc::GetAbstractId(mOp));
    (void)insn.AddOpndChain(resOpnd).AddOpndChain(regOpnd0);
    cgFunc->GetCurBB()->AppendInsn(insn);
    return;
}

void MPISel::SelectFloatCvt(RegOperand &resOpnd, Operand &opnd0, PrimType toType, PrimType fromType)
{
    uint32 fromSize = GetPrimTypeBitSize(fromType);
    uint32 toSize = GetPrimTypeBitSize(toType);
    RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, fromType);
    if (fromSize == toSize) {
        resOpnd = regOpnd0;
        return;
    }
    MOperator mOp = abstract::MOP_undef;
    if (fromSize == k32BitSize && toSize == k64BitSize) {
        mOp = abstract::MOP_cvt_ff_64_32;
    } else if (fromSize == k64BitSize && toSize == k32BitSize) {
        mOp = abstract::MOP_cvt_ff_32_64;
    } else {
        CHECK_FATAL(false, "niy");
    }
    Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDesc::GetAbstractId(mOp));
    (void)insn.AddOpndChain(resOpnd).AddOpndChain(regOpnd0);
    cgFunc->GetCurBB()->AppendInsn(insn);
    return;
}

void MPISel::SelectSub(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    MOperator mOp = abstract::MOP_undef;
    if (IsPrimitiveInteger(primType)) {
        const static auto fastSubMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(sub);
        mOp = fastSubMappingFunc(GetPrimTypeBitSize(primType));
    } else {
        const static auto fastSubFloatMappingFunc = DEF_FLOAT_MOPERATOR_MAPPING_FUNC(sub);
        mOp = fastSubFloatMappingFunc(GetPrimTypeBitSize(primType));
    }
    SelectBasicOp(resOpnd, opnd0, opnd1, mOp, primType);
}

void MPISel::SelectBand(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    MOperator mOp = abstract::MOP_undef;
    if (IsPrimitiveInteger(primType)) {
        const static auto fastAndMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(and);
        mOp = fastAndMappingFunc(GetPrimTypeBitSize(primType));
    } else {
        const static auto fastAndFloatMappingFunc = DEF_FLOAT_MOPERATOR_MAPPING_FUNC(and);
        mOp = fastAndFloatMappingFunc(GetPrimTypeBitSize(primType));
    }
    SelectBasicOp(resOpnd, opnd0, opnd1, mOp, primType);
}

void MPISel::SelectAdd(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    MOperator mOp = abstract::MOP_undef;
    if (IsPrimitiveInteger(primType)) {
        const static auto fastAddMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(add);
        mOp = fastAddMappingFunc(GetPrimTypeBitSize(primType));
    } else {
        const static auto fastAddFloatMappingFunc = DEF_FLOAT_MOPERATOR_MAPPING_FUNC(add);
        mOp = fastAddFloatMappingFunc(GetPrimTypeBitSize(primType));
    }
    SelectBasicOp(resOpnd, opnd0, opnd1, mOp, primType);
}

Operand *MPISel::SelectBior(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    PrimType primType = node.GetPrimType();
    RegOperand *resOpnd =
        &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType), cgFunc->GetRegTyFromPrimTy(primType));
    RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, primType, node.Opnd(0)->GetPrimType());
    RegOperand &regOpnd1 = SelectCopy2Reg(opnd1, primType, node.Opnd(1)->GetPrimType());
    SelectBior(*resOpnd, regOpnd0, regOpnd1, primType);
    return resOpnd;
}

void MPISel::SelectBior(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    const static auto fastBiorMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(or);
    MOperator mOp = fastBiorMappingFunc(GetPrimTypeBitSize(primType));
    SelectBasicOp(resOpnd, opnd0, opnd1, mOp, primType);
}

Operand *MPISel::SelectBxor(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    PrimType primType = node.GetPrimType();
    RegOperand *resOpnd =
        &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType), cgFunc->GetRegTyFromPrimTy(primType));
    RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, primType, node.Opnd(0)->GetPrimType());
    RegOperand &regOpnd1 = SelectCopy2Reg(opnd1, primType, node.Opnd(1)->GetPrimType());
    SelectBxor(*resOpnd, regOpnd0, regOpnd1, primType);
    return resOpnd;
}

void MPISel::SelectBxor(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    const static auto fastBxorMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(xor);
    MOperator mOp = fastBxorMappingFunc(GetPrimTypeBitSize(primType));
    SelectBasicOp(resOpnd, opnd0, opnd1, mOp, primType);
}

MemOperand *MPISel::GetOrCreateMemOpndFromIreadNode(const IreadNode &expr, PrimType primType, int offset)
{
    /* get rhs*/
    Operand *addrOpnd = HandleExpr(expr, *expr.Opnd(0));
    RegOperand &addrOnReg = SelectCopy2Reg(*addrOpnd, PTY_a64);
    /* Generate memOpnd */
    MemOperand &memOpnd = cgFunc->GetOpndBuilder()->CreateMem(addrOnReg, offset, GetPrimTypeBitSize(primType));
    return &memOpnd;
}

Operand *MPISel::SelectIread(const BaseNode &parent, const IreadNode &expr, int extraOffset)
{
    /* get lhs mirType info */
    MirTypeInfo lhsInfo = GetMirTypeInfoFromMirNode(expr);
    /* get memOpnd */
    MemOperand &memOpnd = *GetOrCreateMemOpndFromIreadNode(expr, lhsInfo.primType, lhsInfo.offset + extraOffset);
    /* for BasicType, load val in addr to register. */
    PrimType primType = expr.GetPrimType();
    RegOperand &result =
        cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType), cgFunc->GetRegTyFromPrimTy(primType));
    SelectCopy(result, memOpnd, primType, lhsInfo.primType);
    return &result;
}

Operand *MPISel::SelectAbs(UnaryNode &node, Operand &opnd0)
{
    PrimType primType = node.GetPrimType();
    if (IsPrimitiveFloat(primType)) {
        /*
         * fabs(x) = x AND 0x7fffffff ffffffff [set sign bit to 0]
         */
        const static uint64 kNaN = 0x7fffffffffffffffUL;
        const static double kNaNDouble = *(double *)(&kNaN);
        const static uint64 kNaNf = 0x7fffffffUL;
        const static double kNaNFloat = *(double *)(&kNaNf);
        CHECK_FATAL(primType == PTY_f64 || primType == PTY_f32, "niy");

        double mask = primType == PTY_f64 ? kNaNDouble : kNaNFloat;
        MIRDoubleConst *c = cgFunc->GetMemoryPool()->New<MIRDoubleConst>(
            mask, *GlobalTables::GetTypeTable().GetTypeTable().at(PTY_f64));
        Operand *opnd1 = SelectFloatingConst(*c, PTY_f64);

        RegOperand &resOpnd =
            cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType), cgFunc->GetRegTyFromPrimTy(primType));
        SelectBand(resOpnd, opnd0, *opnd1, primType);
        return &resOpnd;
    } else if (IsUnsignedInteger(primType)) {
        return &opnd0;
    } else {
        /*
         * abs(x) = (x XOR y) - y
         * y = x >>> (bitSize - 1)
         */
        uint32 bitSize = GetPrimTypeBitSize(primType);
        CHECK_FATAL(bitSize == k64BitSize || bitSize == k32BitSize, "only support 32-bits or 64-bits");
        RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, primType);
        ImmOperand &immOpnd = cgFunc->GetOpndBuilder()->CreateImm(bitSize, bitSize - 1);
        RegOperand &regOpndy = cgFunc->GetOpndBuilder()->CreateVReg(bitSize, cgFunc->GetRegTyFromPrimTy(primType));
        SelectShift(regOpndy, regOpnd0, immOpnd, OP_ashr, primType, primType);
        RegOperand &tmpOpnd = cgFunc->GetOpndBuilder()->CreateVReg(bitSize, cgFunc->GetRegTyFromPrimTy(primType));
        SelectBxor(tmpOpnd, regOpnd0, regOpndy, primType);
        RegOperand &resOpnd = cgFunc->GetOpndBuilder()->CreateVReg(bitSize, cgFunc->GetRegTyFromPrimTy(primType));
        SelectSub(resOpnd, tmpOpnd, regOpndy, primType);
        return &resOpnd;
    }
}

StmtNode *MPISel::HandleFuncEntry()
{
    MIRFunction &mirFunc = cgFunc->GetFunction();
    BlockNode *block = mirFunc.GetBody();

    DEBUG_ASSERT(block != nullptr, "get func body block failed in CGFunc::GenerateInstruction");

    StmtNode *stmt = block->GetFirst();
    if (stmt == nullptr) {
        return nullptr;
    }
    DEBUG_ASSERT(stmt->GetOpCode() == OP_label, "The first statement should be a label");
    HandleLabel(*stmt, *this);
    cgFunc->SetFirstBB(*cgFunc->GetCurBB());
    stmt = stmt->GetNext();
    if (stmt == nullptr) {
        return nullptr;
    }
    cgFunc->SetCurBB(*cgFunc->StartNewBBImpl(false, *stmt));
    bool withFreqInfo = mirFunc.HasFreqMap() && !mirFunc.GetLastFreqMap().empty();
    if (withFreqInfo) {
        cgFunc->GetCurBB()->SetFrequency(kFreqBase);
    }

    return stmt;
}

/* This function loads src to a register, the src can be an imm, mem or a label.
 * Once the source and result(destination) types are different,
 * implicit conversion is executed here.*/
RegOperand &MPISel::SelectCopy2Reg(Operand &src, PrimType toType, PrimType fromType)
{
    uint32 fromSize = GetPrimTypeBitSize(fromType);
    uint32 toSize = GetPrimTypeBitSize(toType);
    if (src.IsRegister() && fromSize == toSize) {
        return static_cast<RegOperand &>(src);
    }
    RegOperand &dest =
        cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(toType), cgFunc->GetRegTyFromPrimTy(toType));
    if (fromSize != toSize) {
        SelectCopy(dest, src, toType, fromType);
    } else {
        SelectCopy(dest, src, toType);
    }
    return dest;
}
/* Pretty sure that implicit type conversions will not occur. */
RegOperand &MPISel::SelectCopy2Reg(Operand &src, PrimType dtype)
{
    DEBUG_ASSERT(src.GetSize() == GetPrimTypeBitSize(dtype), "NIY");
    if (src.IsRegister()) {
        return static_cast<RegOperand &>(src);
    }
    RegOperand &dest =
        cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(dtype), cgFunc->GetRegTyFromPrimTy(dtype));
    SelectCopy(dest, src, dtype);
    return dest;
}
/* This function copy/load/store src to a dest, Once the src and dest types
 * are different, implicit conversion is executed here. */
void MPISel::SelectCopy(Operand &dest, Operand &src, PrimType toType, PrimType fromType)
{
    if (GetPrimTypeBitSize(fromType) != GetPrimTypeBitSize(toType)) {
        RegOperand &srcRegOpnd = SelectCopy2Reg(src, fromType);
        RegOperand &dstRegOpnd =
            cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(toType), cgFunc->GetRegTyFromPrimTy(toType));
        SelectIntCvt(dstRegOpnd, srcRegOpnd, toType, fromType);
        SelectCopy(dest, dstRegOpnd, toType);
    } else {
        SelectCopy(dest, src, toType);
    }
}

/* Pretty sure that implicit type conversions will not occur. */
void MPISel::SelectCopy(Operand &dest, Operand &src, PrimType type)
{
    DEBUG_ASSERT(dest.GetSize() == src.GetSize(), "NIY");
    if (dest.GetKind() == Operand::kOpdRegister) {
        SelectCopyInsn(dest, src, type);
    } else if (dest.GetKind() == Operand::kOpdMem) {
        if (src.GetKind() != Operand::kOpdRegister) {
            RegOperand &tempReg =
                cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(type), cgFunc->GetRegTyFromPrimTy(type));
            SelectCopyInsn(tempReg, src, type);
            SelectCopyInsn(dest, tempReg, type);
        } else {
            SelectCopyInsn(dest, src, type);
        }
    } else {
        CHECK_FATAL(false, "NIY, CPU supports more than memory and registers");
    }
    return;
}

void MPISel::SelectCopyInsn(Operand &dest, Operand &src, PrimType type)
{
    MOperator mop = GetFastIselMop(dest.GetKind(), src.GetKind(), type);
    CHECK_FATAL(mop != abstract::MOP_undef, "get mop failed");
    Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mop, InsnDesc::GetAbstractId(mop));
    (void)insn.AddOpndChain(dest).AddOpndChain(src);
    cgFunc->GetCurBB()->AppendInsn(insn);
}

Operand *MPISel::SelectBnot(const UnaryNode &node, Operand &opnd0, const BaseNode &parent)
{
    PrimType dtype = node.GetPrimType();

    RegOperand *resOpnd = nullptr;
    resOpnd = &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(dtype), cgFunc->GetRegTyFromPrimTy(dtype));
    RegOperand &regOpnd0 = SelectCopy2Reg(opnd0, dtype, node.Opnd(0)->GetPrimType());
    SelectBnot(*resOpnd, regOpnd0, dtype);
    return resOpnd;
}

void MPISel::SelectBnot(Operand &resOpnd, Operand &opnd0, PrimType primType)
{
    const static auto fastBnotMappingFunc = DEF_MOPERATOR_MAPPING_FUNC(not);
    MOperator mOp = fastBnotMappingFunc(GetPrimTypeBitSize(primType));
    Insn &insn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDesc::GetAbstractId(mOp));
    (void)insn.AddOpndChain(resOpnd).AddOpndChain(opnd0);
    cgFunc->GetCurBB()->AppendInsn(insn);
}

Operand *MPISel::SelectMin(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    PrimType primType = node.GetPrimType();
    RegOperand &resOpnd =
        cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType), cgFunc->GetRegTyFromPrimTy(primType));
    SelectMin(resOpnd, opnd0, opnd1, primType);
    return &resOpnd;
}

void MPISel::SelectMin(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    SelectMinOrMax(true, resOpnd, opnd0, opnd1, primType);
}

Operand *MPISel::SelectMax(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent)
{
    PrimType primType = node.GetPrimType();
    RegOperand &resOpnd =
        cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(primType), cgFunc->GetRegTyFromPrimTy(primType));
    SelectMax(resOpnd, opnd0, opnd1, primType);
    return &resOpnd;
}

void MPISel::SelectMax(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType)
{
    SelectMinOrMax(false, resOpnd, opnd0, opnd1, primType);
}

Operand *MPISel::SelectRetype(TypeCvtNode &node, Operand &opnd0)
{
    PrimType fromType = node.Opnd(0)->GetPrimType();
    PrimType toType = node.GetPrimType();
    DEBUG_ASSERT(GetPrimTypeSize(fromType) == GetPrimTypeSize(toType), "retype bit widith doesn' match");
    if (IsPrimitivePoint(fromType) && IsPrimitivePoint(toType)) {
        return &SelectCopy2Reg(opnd0, toType);
    }
    if (IsPrimitiveInteger(fromType) && IsPrimitiveInteger(toType)) {
        return &SelectCopy2Reg(opnd0, toType, fromType);
    }
    if (IsPrimitiveInteger(fromType) && IsPrimitiveFloat(toType)) {
        RegOperand *resOpnd =
            &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(toType), cgFunc->GetRegTyFromPrimTy(toType));
        SelectRetypeFloat(*resOpnd, opnd0, toType, fromType);
        return &(*resOpnd);
    }
    if (IsPrimitiveFloat(fromType) && IsPrimitiveInteger(toType)) {
        RegOperand *resOpnd =
            &cgFunc->GetOpndBuilder()->CreateVReg(GetPrimTypeBitSize(toType), cgFunc->GetRegTyFromPrimTy(toType));
        SelectRetypeFloat(*resOpnd, opnd0, toType, fromType);
        return &(*resOpnd);
    }
    CHECK_FATAL(false, "NIY, retype");
    return nullptr;
}

void MPISel::HandleFuncExit()
{
    BlockNode *block = cgFunc->GetFunction().GetBody();
    DEBUG_ASSERT(block != nullptr, "get func body block failed in CGFunc::GenerateInstruction");
    cgFunc->GetCurBB()->SetLastStmt(*block->GetLast());
    /* Set lastbb's frequency */
    cgFunc->SetLastBB(*cgFunc->GetCurBB());
    /* the last BB is return BB */
    cgFunc->GetLastBB()->SetKind(BB::kBBReturn);
}

bool InstructionSelector::PhaseRun(maplebe::CGFunc &f)
{
    MPISel *mpIS = f.GetCG()->CreateMPIsel(*GetPhaseMemPool(), *GetPhaseAllocator(), f);
    DEBUG_ASSERT(mpIS != nullptr, "null ptr check");
    mpIS->doMPIS();
    Standardize *stdz = f.GetCG()->CreateStandardize(*GetPhaseMemPool(), f);
    DEBUG_ASSERT(stdz != nullptr, "null ptr check");
    stdz->DoStandardize();
    return true;
}
}  // namespace maplebe
