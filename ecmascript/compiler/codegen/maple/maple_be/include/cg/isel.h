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

#ifndef MAPLEBE_INCLUDE_CG_ISEL_H
#define MAPLEBE_INCLUDE_CG_ISEL_H

#include "cgfunc.h"

namespace maplebe {
struct MirTypeInfo {
    PrimType primType;
    int32 offset = 0;
    uint32 size = 0; /* for aggType */
};
/* macro expansion instruction selection */
class MPISel {
public:
    MPISel(MemPool &mp, MapleAllocator &allocator, CGFunc &f) : isMp(&mp), cgFunc(&f) {}

    virtual ~MPISel()
    {
        isMp = nullptr;
        cgFunc = nullptr;
    }

    void doMPIS();

    CGFunc *GetCurFunc() const
    {
        return cgFunc;
    }

    Operand *HandleExpr(const BaseNode &parent, BaseNode &expr);

    void SelectDassign(const DassignNode &stmt, Operand &opndRhs);
    void SelectIassign(const IassignNode &stmt, Operand &opndAddr, Operand &opndRhs);
    RegOperand *SelectRegread(RegreadNode &expr);
    void SelectRegassign(RegassignNode &stmt, Operand &opnd0);
    Operand *SelectDread(const BaseNode &parent, const AddrofNode &expr);
    Operand *SelectBand(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
    Operand *SelectAdd(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
    Operand *SelectSub(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
    Operand *SelectNeg(const UnaryNode &node, Operand &opnd0, const BaseNode &parent);
    Operand *SelectCvt(const BaseNode &parent, const TypeCvtNode &node, Operand &opnd0);
    Operand *SelectExtractbits(const BaseNode &parent, const ExtractbitsNode &node, Operand &opnd0);
    virtual Operand *SelectAbs(UnaryNode &node, Operand &opnd0);
    ImmOperand *SelectIntConst(const MIRIntConst &intConst, PrimType primType);
    void SelectCallCommon(StmtNode &stmt, const MPISel &iSel);
    void SelectAdd(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
    void SelectSub(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
    Operand *SelectShift(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
    void SelectShift(Operand &resOpnd, Operand &opnd0, Operand &opnd1, Opcode shiftDirect, PrimType opnd0Type,
                     PrimType opnd1Type);
    void SelectBand(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
    virtual void SelectReturn(NaryStmtNode &retNode, Operand &opnd) = 0;
    virtual void SelectReturn() = 0;
    virtual void SelectGoto(GotoNode &stmt) = 0;
    virtual void SelectRangeGoto(RangeGotoNode &rangeGotoNode, Operand &srcOpnd) = 0;
    virtual void SelectCall(CallNode &callNode) = 0;
    virtual void SelectIcall(IcallNode &icallNode) = 0;
    virtual void SelectIntrinsicCall(IntrinsiccallNode &intrinsiccallNode) = 0;
    virtual Operand *SelectFloatingConst(MIRConst &floatingConst, PrimType primType) const = 0;
    virtual Operand &ProcessReturnReg(PrimType primType, int32 sReg) = 0;
    virtual void SelectCondGoto(CondGotoNode &stmt, BaseNode &condNode, Operand &opnd0) = 0;
    Operand *SelectBior(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
    Operand *SelectBxor(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
    Operand *SelectIread(const BaseNode &parent, const IreadNode &expr, int extraOffset = 0);
    virtual Operand *SelectMpy(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
    virtual Operand *SelectDiv(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
    virtual Operand *SelectRem(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
    virtual Operand *SelectCmpOp(CompareNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
    virtual Operand *SelectCclz(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
    virtual Operand *SelectCctz(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) = 0;
    Operand *SelectBnot(const UnaryNode &node, Operand &opnd0, const BaseNode &parent);
    virtual Operand *SelectLnot(const UnaryNode &node, Operand &opnd0, const BaseNode &parent) = 0;
    Operand *SelectMin(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
    Operand *SelectMax(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
    Operand *SelectRetype(TypeCvtNode &node, Operand &opnd0);
    virtual RegOperand &SelectSpecialRegread(PregIdx pregIdx, PrimType primType) = 0;
    virtual Operand *SelectSqrt(UnaryNode &node, Operand &opnd0, const BaseNode &parent) = 0;

    template <typename T>
    Operand *SelectLiteral(T &c, MIRFunction &func, uint32 labelIdx) const
    {
        MIRSymbol *st = func.GetSymTab()->CreateSymbol(kScopeLocal);
        std::string lblStr(".LB_");
        MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(func.GetStIdx().Idx());
        CHECK_FATAL(funcSt != nullptr, "must not be null pointer");
        std::string funcName = funcSt->GetName();
        (void)lblStr.append(funcName).append(std::to_string(labelIdx));
        st->SetNameStrIdx(lblStr);
        st->SetStorageClass(kScPstatic);
        st->SetSKind(kStConst);
        st->SetKonst(&c);
        PrimType primType = c.GetType().GetPrimType();
        st->SetTyIdx(TyIdx(primType));
        uint32 typeBitSize = GetPrimTypeBitSize(primType);

        // maybe need judge cgFunc->GetMirModule().IsXModule
        if ((T::GetPrimType() == PTY_f32 || T::GetPrimType() == PTY_f64)) {
            return &GetOrCreateMemOpndFromSymbol(*st, typeBitSize, 0);
        }
        CHECK_FATAL(false, "NIY");
        return nullptr;
    }

protected:
    MemPool *isMp;
    CGFunc *cgFunc;

    void SelectCopy(Operand &dest, Operand &src, PrimType toType, PrimType fromType);
    void SelectCopy(Operand &dest, Operand &src, PrimType toType);
    RegOperand &SelectCopy2Reg(Operand &src, PrimType toType, PrimType fromType);
    RegOperand &SelectCopy2Reg(Operand &src, PrimType toType);
    void SelectIntCvt(RegOperand &resOpnd, Operand &opnd0, PrimType toType, PrimType fromType);
    void SelectCvtInt2Float(RegOperand &resOpnd, Operand &origOpnd0, PrimType toType, PrimType fromType);
    void SelectFloatCvt(RegOperand &resOpnd, Operand &opnd0, PrimType toType, PrimType fromType);
    void SelectCvtFloat2Int(RegOperand &resOpnd, Operand &origOpnd0, PrimType toType, PrimType fromType);
    PrimType GetIntegerPrimTypeFromSize(bool isSigned, uint32 bitSize);
    std::pair<FieldID, MIRType *> GetFieldIdAndMirTypeFromMirNode(const BaseNode &node);
    MirTypeInfo GetMirTypeInfoFormFieldIdAndMirType(FieldID fieldId, MIRType *mirType);
    MirTypeInfo GetMirTypeInfoFromMirNode(const BaseNode &node);
    MemOperand *GetOrCreateMemOpndFromIreadNode(const IreadNode &expr, PrimType primType, int offset);

private:
    StmtNode *HandleFuncEntry();
    void HandleFuncExit();
    void SelectDassign(StIdx stIdx, FieldID fieldId, PrimType rhsPType, Operand &opndRhs);
    virtual MemOperand &GetOrCreateMemOpndFromSymbol(const MIRSymbol &symbol, FieldID fieldId = 0) const = 0;
    virtual MemOperand &GetOrCreateMemOpndFromSymbol(const MIRSymbol &symbol, uint32 opndSize, int64 offset) const = 0;
    virtual Operand &GetTargetRetOperand(PrimType primType, int32 sReg) = 0;
    void SelectBasicOp(Operand &resOpnd, Operand &opnd0, Operand &opnd1, MOperator mOp, PrimType primType);
    /*
     * Support conversion between all types and registers
     * only Support conversion between registers and memory
     * alltypes -> reg -> mem
     */
    void SelectCopyInsn(Operand &dest, Operand &src, PrimType type);
    void SelectNeg(Operand &resOpnd, Operand &opnd0, PrimType primType);
    void SelectBnot(Operand &resOpnd, Operand &opnd0, PrimType primType);
    void SelectBior(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
    void SelectExtractbits(RegOperand &resOpnd, RegOperand &opnd0, uint8 bitOffset, uint8 bitSize, PrimType primType);
    void SelectBxor(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
    virtual RegOperand &GetTargetBasicPointer(PrimType primType) = 0;
    void SelectMin(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
    void SelectMax(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
    virtual void SelectMinOrMax(bool isMin, Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;
    /* retype float/double reg to int64 reg, or i64 reg to f64 reg: no changing bit mov */
    virtual void SelectRetypeFloat(RegOperand &resOpnd, Operand &opnd0, PrimType toType, PrimType fromType) = 0;
};
MAPLE_FUNC_PHASE_DECLARE_BEGIN(InstructionSelector, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}  // namespace maplebe
#endif /* MAPLEBE_INCLUDE_CG_ISEL_H */
