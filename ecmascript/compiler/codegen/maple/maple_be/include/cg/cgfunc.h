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

#ifndef MAPLEBE_INCLUDE_CG_CGFUNC_H
#define MAPLEBE_INCLUDE_CG_CGFUNC_H

#include "becommon.h"
#include "operand.h"
#include "memlayout.h"
#include "reg_info.h"
#include "cgbb.h"
#include "call_conv.h"
#include "cfi.h"
#include "cg_cfg.h"
#include "cg_irbuilder.h"
/* MapleIR headers. */
#include "mir_function.h"

/* Maple MP header */
#include "mempool_allocator.h"
#include "triple.h"

namespace maplebe {
constexpr int32 kBBLimit = 100000;
constexpr int32 kFreqBase = 100000;
struct MemOpndCmp {
    bool operator()(const MemOperand *lhs, const MemOperand *rhs) const
    {
        CHECK_FATAL(lhs != nullptr, "null ptr check");
        CHECK_FATAL(rhs != nullptr, "null ptr check");
        if (lhs == rhs) {
            return false;
        }
        return (lhs->Less(*rhs));
    }
};

class SpillMemOperandSet {
public:
    explicit SpillMemOperandSet(MapleAllocator &mallocator) : reuseSpillLocMem(mallocator.Adapter()) {}

    virtual ~SpillMemOperandSet() = default;

    void Add(MemOperand &op)
    {
        (void)reuseSpillLocMem.insert(&op);
    }

    void Remove(MemOperand &op)
    {
        reuseSpillLocMem.erase(&op);
    }

    MemOperand *GetOne()
    {
        if (!reuseSpillLocMem.empty()) {
            MemOperand *res = *reuseSpillLocMem.begin();
            reuseSpillLocMem.erase(res);
            return res;
        }
        return nullptr;
    }

private:
    MapleSet<MemOperand *, MemOpndCmp> reuseSpillLocMem;
};

#if TARGARM32
class LiveRange;
#endif /* TARGARM32 */
constexpr uint32 kVRegisterNumber = 80;
class CGFunc {
public:
    enum ShiftDirection : uint8 { kShiftLeft, kShiftAright, kShiftLright };

    CGFunc(MIRModule &mod, CG &cg, MIRFunction &mirFunc, BECommon &beCommon, MemPool &memPool, StackMemPool &stackMp,
           MapleAllocator &allocator, uint32 funcId);
    virtual ~CGFunc();

    const std::string &GetName() const
    {
        return func.GetName();
    }

    const MapleMap<LabelIdx, uint64> &GetLabelAndValueMap() const
    {
        return labelMap;
    }

    void InsertLabelMap(LabelIdx idx, uint64 value)
    {
        DEBUG_ASSERT(labelMap.find(idx) == labelMap.end(), "idx already exist");
        labelMap[idx] = value;
    }

    void LayoutStackFrame()
    {
        CHECK_FATAL(memLayout != nullptr, "memLayout should has been initialized in constructor");
        memLayout->LayoutStackFrame(structCopySize, maxParamStackSize);
    }

    bool HasCall() const
    {
        return func.HasCall();
    }

    bool HasVLAOrAlloca() const
    {
        return hasVLAOrAlloca;
    }

    void SetHasVLAOrAlloca(bool val)
    {
        hasVLAOrAlloca = val;
    }

    bool HasAlloca() const
    {
        return hasAlloca;
    }

    void SetHasAlloca(bool val)
    {
        hasAlloca = val;
    }

    InsnBuilder *GetInsnBuilder()
    {
        return insnBuilder;
    }
    OperandBuilder *GetOpndBuilder()
    {
        return opndBuilder;
    }

    void RemoveUnreachableBB();
    void GenerateInstruction();
    void HandleFunction();
    void ProcessExitBBVec();
    void AddCommonExitBB();
    virtual void MergeReturn() = 0;
    uint32 GetMaxRegNum() const
    {
        return vReg.GetMaxRegCount();
    };
    void SetMaxRegNum(uint32 num)
    {
        vReg.SetMaxRegCount(num);
    }
    void IncMaxRegNum(uint32 num) const
    {
        vReg.IncMaxRegCount(num);
    }
    // Attention! Do not invoke this interface in other processes except unit-test
    void SetMaxVReg(uint32 num) const
    {
        vReg.SetCount(num);
    }
    void DumpCFG() const;
    void DumpBBInfo(const BB *bb) const;
    void DumpCGIR() const;
    Operand *HandleExpr(const BaseNode &parent, BaseNode &expr);
    /* select stmt */
    virtual void SelectDassign(DassignNode &stmt, Operand &opnd0) = 0;
    virtual void SelectRegassign(RegassignNode &stmt, Operand &opnd0) = 0;
    virtual void SelectIassign(IassignNode &stmt) = 0;
    virtual void SelectReturn(Operand *opnd) = 0;
    virtual void SelectCondGoto(CondGotoNode &stmt, Operand &opnd0, Operand &opnd1) = 0;
    virtual void SelectCondSpecialCase1(CondGotoNode &stmt, BaseNode &opnd0) = 0;
    virtual void SelectCondSpecialCase2(const CondGotoNode &stmt, BaseNode &opnd0) = 0;
    virtual void SelectGoto(GotoNode &stmt) = 0;
    virtual void SelectCall(CallNode &callNode) = 0;
    virtual void SelectIcall(IcallNode &icallNode) = 0;
    virtual void SelectIntrinsicCall(IntrinsiccallNode &intrinsiccallNode) = 0;
    virtual void SelectDeoptCall(CallNode &callNode) = 0;
    virtual void SelectTailICall(IcallNode &icallNode) = 0;
    virtual Operand *SelectCclz(IntrinsicopNode &intrinsicopNode) = 0;
    virtual RegOperand *SelectHeapConstant(IntrinsicopNode &node, Operand &opnd0, Operand &opnd1) = 0;
    virtual RegOperand *SelectTaggedIsHeapObject(IntrinsicopNode &node, Operand &opnd0, Operand &opnd1) = 0;
    virtual RegOperand *SelectIsStableElements(IntrinsicopNode &node, Operand &opnd0,
                                               Operand &opnd1, Operand &opnd2) = 0;
    virtual RegOperand *SelectHasPendingException(IntrinsicopNode &node, Operand &opnd0,
                                                  Operand &opnd1, Operand &opnd2) = 0;
    virtual RegOperand *SelectGetHeapConstantTable(IntrinsicopNode &node, Operand &opnd0,
                                                   Operand &opnd1, Operand &opnd2) = 0;
    virtual RegOperand *SelectTaggedObjectIsString(IntrinsicopNode &node, Operand &opnd0, Operand &opnd1,
                                                   Operand &opnd2, Operand &opnd3, Operand &opnd4) = 0;
    virtual RegOperand *SelectIsCOWArray(IntrinsicopNode &node, Operand &opnd0, Operand &opnd1,
                                         Operand &opnd2, Operand &opnd3, Operand &opnd4, Operand &opnd5) = 0;
    virtual void SelectComment(CommentNode &comment) = 0;

    /* select expr */
    virtual Operand *SelectDread(const BaseNode &parent, AddrofNode &expr) = 0;
    virtual RegOperand *SelectRegread(RegreadNode &expr) = 0;
    virtual Operand *SelectIread(const BaseNode &parent, IreadNode &expr, int extraOffset = 0,
                                 PrimType finalBitFieldDestType = kPtyInvalid) = 0;
    virtual Operand *SelectIntConst(const MIRIntConst &intConst, const BaseNode &parent) = 0;
    virtual Operand *SelectFloatConst(MIRFloatConst &floatConst, const BaseNode &parent) = 0;
    virtual Operand *SelectDoubleConst(MIRDoubleConst &doubleConst, const BaseNode &parent) = 0;
    virtual void SelectAdd(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;
    virtual Operand *SelectAdd(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
    virtual Operand *SelectShift(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
    virtual void SelectMpy(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;
    virtual Operand *SelectMpy(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
    virtual Operand *SelectRem(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
    virtual void SelectDiv(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;
    virtual Operand *SelectDiv(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
    virtual Operand *SelectSub(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
    virtual void SelectSub(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;
    virtual Operand *SelectBand(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
    virtual void SelectBand(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;
    virtual void SelectMin(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;
    virtual Operand *SelectMin(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
    virtual void SelectMax(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;
    virtual Operand *SelectMax(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
    virtual Operand *SelectCmpOp(CompareNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
    virtual Operand *SelectBior(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
    virtual void SelectBior(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;
    virtual Operand *SelectBxor(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
    virtual void SelectBxor(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) = 0;
    virtual Operand *SelectAbs(UnaryNode &node, Operand &opnd0) = 0;
    virtual Operand *SelectBnot(UnaryNode &node, Operand &opnd0, const BaseNode &parent) = 0;
    virtual Operand *SelectExtractbits(ExtractbitsNode &node, Operand &opnd0, const BaseNode &parent) = 0;
    virtual Operand *SelectRegularBitFieldLoad(ExtractbitsNode &node, const BaseNode &parent) = 0;
    virtual Operand *SelectLnot(UnaryNode &node, Operand &opnd0, const BaseNode &parent) = 0;
    virtual Operand *SelectNeg(UnaryNode &node, Operand &opnd0, const BaseNode &parent) = 0;
    virtual Operand *SelectSqrt(UnaryNode &node, Operand &opnd0, const BaseNode &parent) = 0;
    virtual Operand *SelectCeil(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) = 0;
    virtual Operand *SelectFloor(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) = 0;
    virtual Operand *SelectRetype(TypeCvtNode &node, Operand &opnd0) = 0;
    virtual Operand *SelectCvt(const BaseNode &parent, TypeCvtNode &node, Operand &opnd0) = 0;
    virtual Operand *SelectTrunc(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) = 0;
    virtual RegOperand &SelectCopy(Operand &src, PrimType srcType, PrimType dstType) = 0;
    virtual void SelectRangeGoto(RangeGotoNode &rangeGotoNode, Operand &opnd0) = 0;

    virtual Operand &GetOrCreateRflag() = 0;
    virtual const Operand *GetRflag() const = 0;
    virtual const LabelOperand *GetLabelOperand(LabelIdx labIdx) const = 0;
    virtual LabelOperand &GetOrCreateLabelOperand(LabelIdx labIdx) = 0;
    virtual LabelOperand &GetOrCreateLabelOperand(BB &bb) = 0;
    virtual RegOperand &CreateVirtualRegisterOperand(regno_t vRegNO) = 0;
    virtual RegOperand &GetOrCreateVirtualRegisterOperand(regno_t vRegNO) = 0;
    virtual RegOperand &GetOrCreateFramePointerRegOperand() = 0;
    virtual RegOperand &GetOrCreateStackBaseRegOperand() = 0;
    virtual RegOperand *GetBaseReg(const SymbolAlloc &symAlloc) = 0;
    virtual int32 GetBaseOffset(const SymbolAlloc &symbolAlloc) = 0;
    virtual RegOperand &GetZeroOpnd(uint32 size) = 0;
    virtual Operand &CreateCfiRegOperand(uint32 reg, uint32 size) = 0;
    virtual Operand &CreateImmOperand(PrimType primType, int64 val) = 0;
    virtual MemOperand *GetOrCreatSpillMem(regno_t vrNum, uint32 bitSize) = 0;

    /* ra */
    virtual void AddtoCalleeSaved(regno_t reg) = 0;

    virtual void HandleFuncCfg(CGCFG *cfg)
    {
        AddCommonExitBB();
    }

    LabelIdx CreateLabel();

    void AddReferenceReg(regno_t regNO)
    {
        referenceVirtualRegs.insert(regNO);
    }

    bool IsRegReference(regno_t regNO) const
    {
        return referenceVirtualRegs.count(regNO) != 0;
    }

    const MapleUnorderedSet<regno_t> &GetReferenceRegs() const
    {
        return referenceVirtualRegs;
    }

    void ReplaceRegReference(regno_t oldReg, regno_t newReg)
    {
        auto iter = referenceVirtualRegs.find(oldReg);
        if (iter != referenceVirtualRegs.end()) {
            referenceVirtualRegs.erase(iter);
            referenceVirtualRegs.insert(newReg);
        }
    }

    virtual RegOperand *GetOrCreateRegOpndFromPregIdx(PregIdx preg, PrimType type)
    {
        DEBUG_ASSERT(preg > 0, "NIY, preg must be greater than 0.");
        auto idx = static_cast<size_t>(preg);
        if (pregIdx2Opnd[idx] == nullptr) {
            pregIdx2Opnd[idx] = &GetOpndBuilder()->CreateVReg(GetVirtualRegNOFromPseudoRegIdx(preg),
                GetPrimTypeBitSize(type), GetRegTyFromPrimTy(type));
        }
        auto *regOpnd = pregIdx2Opnd[idx];
        if (type == maple::PTY_ref) {
            regOpnd->SetIsReference(true);
            AddReferenceReg(regOpnd->GetRegisterNumber());
        }
        return regOpnd;
    }

    Operand &CreateCfiImmOperand(int64 val, uint32 size) const
    {
        return *memPool->New<cfi::ImmOperand>(val, size);
    }

    bool IsSpecialPseudoRegister(PregIdx spr) const
    {
        return spr < 0;
    }

    regno_t NewVReg(RegType regType, uint32 size)
    {
        if (CGOptions::UseGeneralRegOnly()) {
            CHECK_FATAL(regType != kRegTyFloat, "cannot use float | SIMD register with --general-reg-only");
        }
        return vReg.GetNextVregNO(regType, size);
    }

    virtual regno_t NewVRflag()
    {
        return 0;
    }

    RegType GetRegTyFromPrimTy(PrimType primType) const
    {
        switch (primType) {
            case PTY_u1:
            case PTY_i8:
            case PTY_u8:
            case PTY_i16:
            case PTY_u16:
            case PTY_i32:
            case PTY_u32:
            case PTY_i64:
            case PTY_u64:
            case PTY_a64:
            case PTY_ptr:
            case PTY_ref:
                return kRegTyInt;
            case PTY_f32:
            case PTY_f64:
                return kRegTyFloat;
            default:
                DEBUG_ASSERT(false, "Unexpected pty");
                return kRegTyUndef;
        }
    }

    /* return Register Type */
    virtual RegType GetRegisterType(regno_t rNum) const
    {
        CHECK(rNum < vReg.VRegTableSize(), "index out of range in GetVRegSize");
        return vReg.VRegTableGetType(rNum);
    }

#if defined(TARGX86_64) && TARGX86_64
    uint32 GetMaxVReg() const
    {
        return vReg.GetCount() + opndBuilder->GetCurrentVRegNum();
    }
#else
    uint32 GetMaxVReg() const
    {
        return vReg.GetCount();
    }
#endif

    void VerifyAllInsn();

    virtual uint32 MaxCondBranchDistance()
    {
        return INT_MAX;
    }

    virtual void InsertJumpPad(Insn *)
    {
        return;
    }

    virtual LabelIdx GetLabelInInsn(Insn &insn)
    {
        return 0;
    }

    uint32 NumBBs() const
    {
        return bbCnt;
    }

    int32 GetStructCopySize() const
    {
        return structCopySize;
    }

    int32 GetMaxParamStackSize() const
    {
        return maxParamStackSize;
    }

    /* Get And Set private members */
    CG *GetCG()
    {
        return cg;
    }

    const CG *GetCG() const
    {
        return cg;
    }

    MIRModule &GetMirModule()
    {
        return mirModule;
    }

    const MIRModule &GetMirModule() const
    {
        return mirModule;
    }

    template <typename T>
    MIRConst *NewMirConst(T &mirConst)
    {
        MIRConst *newConst = mirModule.GetMemPool()->New<T>(mirConst.GetValue(), mirConst.GetType());
        return newConst;
    }

    MIRFunction &GetFunction()
    {
        return func;
    }

    const MIRFunction &GetFunction() const
    {
        return func;
    }

    uint32 GetLabelIdx() const
    {
        return labelIdx;
    }

    void SetLabelIdx(uint32 idx)
    {
        labelIdx = idx;
    }

    LabelNode *GetEndLabel()
    {
        return endLabel;
    }

    const LabelNode *GetEndLabel() const
    {
        return endLabel;
    }

    void SetEndLabel(LabelNode &label)
    {
        endLabel = &label;
    }

    BB *GetFirstBB()
    {
        return firstBB;
    }

    const BB *GetFirstBB() const
    {
        return firstBB;
    }

    void SetFirstBB(BB &bb)
    {
        firstBB = &bb;
    }

    BB *GetLastBB()
    {
        return lastBB;
    }

    const BB *GetLastBB() const
    {
        return lastBB;
    }

    void SetLastBB(BB &bb)
    {
        lastBB = &bb;
    }

    BB *GetCurBB()
    {
        return curBB;
    }

    const BB *GetCurBB() const
    {
        return curBB;
    }

    void SetCurBB(BB &bb)
    {
        curBB = &bb;
    }

    BB *GetDummyBB()
    {
        return dummyBB;
    }

    const BB *GetDummyBB() const
    {
        return dummyBB;
    }

    BB *GetCommonExitBB()
    {
        return commonExitBB;
    }

    BB *GetCommonEntryBB()
    {
        DEBUG_ASSERT(bbVec[0]->GetId() == 0 && bbVec[0] != firstBB, "there is no commonEntryBB");
        return bbVec[0];
    }

    MapleVector<BB *> &GetExitBBsVec()
    {
        return exitBBVec;
    }

    const MapleVector<BB *> GetExitBBsVec() const
    {
        return exitBBVec;
    }

    size_t ExitBBsVecSize() const
    {
        return exitBBVec.size();
    }

    bool IsExitBBsVecEmpty() const
    {
        return exitBBVec.empty();
    }

    void EraseExitBBsVec(MapleVector<BB *>::iterator it)
    {
        exitBBVec.erase(it);
    }

    void PushBackExitBBsVec(BB &bb)
    {
        exitBBVec.emplace_back(&bb);
    }

    void ClearExitBBsVec()
    {
        exitBBVec.clear();
    }

    bool IsExitBB(const BB &currentBB)
    {
        for (BB *exitBB : exitBBVec) {
            if (exitBB == &currentBB) {
                return true;
            }
        }
        return false;
    }

    BB *GetExitBB(int32 index)
    {
        return exitBBVec.at(index);
    }

    const BB *GetExitBB(int32 index) const
    {
        return exitBBVec.at(index);
    }

    MapleVector<BB *>::iterator EraseNoReturnCallBB(MapleVector<BB *>::iterator it)
    {
        return noReturnCallBBVec.erase(it);
    }

    void PushBackNoReturnCallBBsVec(BB &bb)
    {
        noReturnCallBBVec.emplace_back(&bb);
    }

    MapleVector<BB *> &GetNoRetCallBBVec()
    {
        return noReturnCallBBVec;
    }

    void SetLab2BBMap(int32 index, BB &bb)
    {
        lab2BBMap[index] = &bb;
    }

    BB *GetBBFromLab2BBMap(uint32 index)
    {
        return lab2BBMap[index];
    }

    MapleUnorderedMap<LabelIdx, BB *> &GetLab2BBMap()
    {
        return lab2BBMap;
    }

    BECommon &GetBecommon()
    {
        return beCommon;
    }

    const BECommon GetBecommon() const
    {
        return beCommon;
    }

    MemLayout *GetMemlayout()
    {
        return memLayout;
    }

    const MemLayout *GetMemlayout() const
    {
        return memLayout;
    }

    void SetMemlayout(MemLayout &layout)
    {
        memLayout = &layout;
    }

    RegisterInfo *GetTargetRegInfo()
    {
        return targetRegInfo;
    }

    void SetTargetRegInfo(RegisterInfo &regInfo)
    {
        targetRegInfo = &regInfo;
    }

    MemPool *GetMemoryPool()
    {
        return memPool;
    }

    const MemPool *GetMemoryPool() const
    {
        return memPool;
    }

    StackMemPool &GetStackMemPool()
    {
        return stackMp;
    }

    MapleAllocator *GetFuncScopeAllocator()
    {
        return funcScopeAllocator;
    }

    const MapleAllocator *GetFuncScopeAllocator() const
    {
        return funcScopeAllocator;
    }

    const MapleMap<uint32, MIRSymbol *> GetEmitStVec() const
    {
        return emitStVec;
    }

    MIRSymbol *GetEmitSt(uint32 id)
    {
        return emitStVec[id];
    }

    void AddEmitSt(uint32 id, MIRSymbol &symbol)
    {
        CHECK_FATAL(symbol.GetKonst()->GetKind() == kConstAggConst, "not a kConstAggConst");
        MIRAggConst *arrayConst = safe_cast<MIRAggConst>(symbol.GetKonst());
        for (size_t i = 0; i < arrayConst->GetConstVec().size(); ++i) {
            CHECK_FATAL(arrayConst->GetConstVecItem(i)->GetKind() == kConstLblConst, "not a kConstLblConst");
            MIRLblConst *lblConst = safe_cast<MIRLblConst>(arrayConst->GetConstVecItem(i));
            ++switchLabelCnt[lblConst->GetValue()];
        }
        emitStVec[id] = &symbol;
    }

    void DeleteEmitSt(uint32 id)
    {
        MIRSymbol &symbol = *emitStVec[id];
        CHECK_FATAL(symbol.GetKonst()->GetKind() == kConstAggConst, "not a kConstAggConst");
        MIRAggConst *arrayConst = safe_cast<MIRAggConst>(symbol.GetKonst());
        for (size_t i = 0; i < arrayConst->GetConstVec().size(); ++i) {
            CHECK_FATAL(arrayConst->GetConstVecItem(i)->GetKind() == kConstLblConst, "not a kConstLblConst");
            MIRLblConst *lblConst = safe_cast<MIRLblConst>(arrayConst->GetConstVecItem(i));

            LabelIdx labelIdx = lblConst->GetValue();
            CHECK_FATAL(switchLabelCnt[labelIdx] > 0, "error labelIdx");
            --switchLabelCnt[labelIdx];
            if (switchLabelCnt[labelIdx] == 0) {
                switchLabelCnt.erase(labelIdx);
            }
        }
        (void)emitStVec.erase(id);
    }

    bool InSwitchTable(LabelIdx label) const
    {
        if (switchLabelCnt.empty()) {
            return false;
        }
        return (switchLabelCnt.find(label) != switchLabelCnt.end());
    }

    LabelIdx GetLocalSymLabelIndex(const MIRSymbol &symbol) const
    {
        auto itr = funcLocalSym2Label.find(&symbol);
        CHECK_FATAL(itr != funcLocalSym2Label.end(), "not assign labelIndex to sym");
        return itr->second;
    }

    void SetLocalSymLabelIndex(const MIRSymbol &mirSymbol, LabelIdx labelIndex)
    {
        funcLocalSym2Label[&mirSymbol] = labelIndex;
    }

    MapleVector<BB *> &GetAllBBs()
    {
        return bbVec;
    }

    std::size_t GetAllBBSize() const
    {
        return bbVec.size();
    }

    BB *GetBBFromID(uint32 id)
    {
        return bbVec[id];
    }
    void ClearBBInVec(uint32 id)
    {
        bbVec[id] = nullptr;
    }

    CGCFG *GetTheCFG()
    {
        return theCFG;
    }

    void SetTheCFG(CGCFG *cfg)
    {
        theCFG = cfg;
    }

    const CGCFG *GetTheCFG() const
    {
        return theCFG;
    }

    regno_t GetVirtualRegNOFromPseudoRegIdx(PregIdx idx) const
    {
        return regno_t(idx + firstMapleIrVRegNO);
    }

    bool GetHasProEpilogue() const
    {
        return hasProEpilogue;
    }

    void SetHasProEpilogue(bool state)
    {
        hasProEpilogue = state;
    }

    BB *CreateNewBB()
    {
        BB *bb = memPool->New<BB>(bbCnt++, *funcScopeAllocator);
        bbVec.emplace_back(bb);
        return bb;
    }

    BB *CreateNewBB(bool unreachable, BB::BBKind kind, uint32 frequency)
    {
        BB *newBB = CreateNewBB();
        newBB->SetKind(kind);
        newBB->SetUnreachable(unreachable);
        newBB->SetFrequency(frequency);
        return newBB;
    }

    BB *CreateNewBB(LabelIdx label, bool unreachable, BB::BBKind kind, uint32 frequency)
    {
        BB *newBB = CreateNewBB(unreachable, kind, frequency);
        newBB->AddLabel(label);
        SetLab2BBMap(label, *newBB);
        return newBB;
    }

    BB *StartNewBBImpl(bool stmtIsCurBBLastStmt, StmtNode &stmt)
    {
        BB *newBB = CreateNewBB();
        DEBUG_ASSERT(newBB != nullptr, "newBB should not be nullptr");
        if (stmtIsCurBBLastStmt) {
            DEBUG_ASSERT(curBB != nullptr, "curBB should not be nullptr");
            curBB->SetLastStmt(stmt);
            curBB->AppendBB(*newBB);
            newBB->SetFirstStmt(*stmt.GetNext());
        } else {
            newBB->SetFirstStmt(stmt);
            if (curBB != nullptr) {
                if (stmt.GetPrev() != nullptr) {
                    DEBUG_ASSERT(stmt.GetPrev()->GetNext() == &stmt, " the next of stmt's prev should be stmt self");
                }
                curBB->SetLastStmt(*stmt.GetPrev());
                curBB->AppendBB(*newBB);
            }
        }
        return newBB;
    }

    BB *StartNewBB(StmtNode &stmt)
    {
        BB *bb = curBB;
        if (stmt.GetNext() != nullptr && stmt.GetNext()->GetOpCode() != OP_label) {
            bb = StartNewBBImpl(true, stmt);
        }
        return bb;
    }

    void SetCurBBKind(BB::BBKind bbKind) const
    {
        curBB->SetKind(bbKind);
    }

    bool IsAfterRegAlloc() const
    {
        return isAfterRegAlloc;
    }

    void SetIsAfterRegAlloc()
    {
        isAfterRegAlloc = true;
    }

    const MapleString &GetShortFuncName() const
    {
        return shortFuncName;
    }

    size_t GetLSymSize() const
    {
        return lSymSize;
    }

    virtual InsnVisitor *NewInsnModifier() = 0;

    bool GenCfi() const
    {
        return (mirModule.GetSrcLang() != kSrcLangC) || mirModule.IsWithDbgInfo();
    }

    uint32 GetUniqueID() const
    {
        return func.GetPuidx();
    }
    void SetUseFP(bool canUseFP)
    {
        useFP = canUseFP;
    }

    bool UseFP() const
    {
        return useFP;
    }

    void UnsetSeenFP()
    {
        seenFP = false;
    }

    bool SeenFP() const
    {
        return seenFP;
    }

    void RegisterVregMapping(regno_t vRegNum, PregIdx pidx)
    {
        vregsToPregsMap[vRegNum] = pidx;
    }

    const MapleList<Insn *> &GetStackMapInsns() const
    {
        return stackMapInsns;
    }

    void AppendStackMapInsn(Insn &insn)
    {
        insn.InitStackMapInfo();
        stackMapInsns.emplace_back(&insn);
    }

    bool IsStackMapComputed()
    {
        return isStackMapComputed;
    }

    void SetStackMapComputed()
    {
        isStackMapComputed = true;
    }

    void EraseUnreachableStackMapInsns()
    {
        for (auto it = stackMapInsns.begin(); it != stackMapInsns.end();) {
            if ((*it)->GetBB()->IsUnreachable()) {
                it = stackMapInsns.erase(it);
            } else {
                ++it;
            }
        }
    }

    uint32 GetFirstMapleIrVRegNO() const
    {
        return firstMapleIrVRegNO;
    }

    void SetStackProtectInfo(StackProtectKind kind)
    {
        stackProtectInfo |= kind;
    }

    uint8 GetStackProtectInfo() const
    {
        return stackProtectInfo;
    }

    void SetNeedStackProtect(bool val)
    {
        needStackProtect = val;
    }

    bool GetNeedStackProtect() const
    {
        return needStackProtect;
    }

    CallConvKind GetCurCallConvKind() const
    {
        return callingConventionKind;
    }

    void SetFuncEmitInfo(FuncEmitInfo *fnInfo)
    {
        funcEmitInfo = fnInfo;
    }

    FuncEmitInfo *GetFuncEmitInfo()
    {
        return funcEmitInfo;
    }

    void SetExitBBLost(bool val)
    {
        exitBBLost = val;
    }
    bool GetExitBBLost()
    {
        return exitBBLost;
    }

    void SetHasBuiltCfg(bool hasBuilt)
    {
        hasBuiltCfg = hasBuilt;
    }

    bool HasBuiltCfg() const
    {
        return hasBuiltCfg;
    }

    bool GetWithSrc() const
    {
        return withSrc;
    }
protected:
    uint32 firstMapleIrVRegNO = 200; /* positioned after physical regs */
    uint32 firstNonPregVRegNO;
    VregInfo vReg;           /* for assigning a number for each CG virtual register */
    size_t lSymSize;         /* size of local symbol table imported */
    MapleVector<BB *> bbVec;
    MapleUnorderedSet<regno_t> referenceVirtualRegs;
    MapleVector<RegOperand *> pregIdx2Opnd;
    MapleUnorderedMap<PregIdx, MemOperand *> pRegSpillMemOperands;
    MapleUnorderedMap<regno_t, MemOperand *> spillRegMemOperands;
    MapleUnorderedMap<uint32, SpillMemOperandSet *> reuseSpillLocMem;
    MapleMap<LabelIdx, uint64> labelMap;
    MapleMap<regno_t, PregIdx> vregsToPregsMap;
    MapleList<Insn *> stackMapInsns;
    int32 structCopySize;
    int32 maxParamStackSize;
    static constexpr int kRegIncrStepLen = 80; /* reg number increate step length */

    bool hasVLAOrAlloca = false;
    bool hasAlloca = false;
    bool hasProEpilogue = false;
    bool isVolLoad = false;
    bool isVolStore = false;
    bool isAfterRegAlloc = false;
    bool isAggParamInReg = false;
    bool hasTakenLabel = false;
    bool hasLaidOutByPgoUse = false;
    bool withSrc = true;
    RegOperand *aggParamReg = nullptr;
    CG *cg;
    MIRModule &mirModule;
    MemPool *memPool;
    StackMemPool &stackMp;

    PregIdx GetPseudoRegIdxFromVirtualRegNO(const regno_t vRegNO) const
    {
        if (IsVRegNOForPseudoRegister(vRegNO)) {
            return PregIdx(vRegNO - firstMapleIrVRegNO);
        }
        return VRegNOToPRegIdx(vRegNO);
    }

    bool IsVRegNOForPseudoRegister(regno_t vRegNum) const
    {
        /* 0 is not allowed for preg index */
        uint32 n = static_cast<uint32>(vRegNum);
        return (firstMapleIrVRegNO < n && n < firstNonPregVRegNO);
    }

    PregIdx VRegNOToPRegIdx(regno_t vRegNum) const
    {
        auto it = vregsToPregsMap.find(vRegNum);
        if (it == vregsToPregsMap.end()) {
            return PregIdx(-1);
        }
        return it->second;
    }

    VirtualRegNode &GetVirtualRegNodeFromPseudoRegIdx(PregIdx idx)
    {
        return vReg.VRegTableElementGet(GetVirtualRegNOFromPseudoRegIdx(idx));
    }

    PrimType GetTypeFromPseudoRegIdx(PregIdx idx)
    {
        VirtualRegNode &vRegNode = GetVirtualRegNodeFromPseudoRegIdx(idx);
        RegType regType = vRegNode.GetType();
        DEBUG_ASSERT(regType == kRegTyInt || regType == kRegTyFloat, "");
        uint32 size = vRegNode.GetSize(); /* in bytes */
        return (regType == kRegTyInt ? (size <= sizeof(int32) ? PTY_i32 : PTY_i64)
                                     : (size <= sizeof(float) ? PTY_f32 : PTY_f64));
    }

    int64 GetPseudoRegisterSpillLocation(PregIdx idx)
    {
        const SymbolAlloc *symLoc = memLayout->GetSpillLocOfPseduoRegister(idx);
        return static_cast<int64>(GetBaseOffset(*symLoc));
    }

    int64 GetOrCreatSpillRegLocation(regno_t vrNum, uint32 memByteSize)
    {
        auto *symLoc = GetMemlayout()->GetLocOfSpillRegister(vrNum, memByteSize);
        return static_cast<int64>(GetBaseOffset(*symLoc));
    }

    virtual MemOperand *GetPseudoRegisterSpillMemoryOperand(PregIdx idx) = 0;

    uint32 GetSpillLocation(uint32 size)
    {
        uint32 offset = RoundUp(nextSpillLocation, static_cast<uint64>(size));
        nextSpillLocation = offset + size;
        return offset;
    }

    PrimType GetPrimTypeFromSize(uint32 byteSize, PrimType defaultType) const
    {
        constexpr uint32 oneByte = 1;
        constexpr uint32 twoByte = 2;
        constexpr uint32 fourByte = 4;
        switch (byteSize) {
            case oneByte:
                return PTY_i8;
            case twoByte:
                return PTY_i16;
            case fourByte:
                return PTY_i32;
            default:
                return defaultType;
        }
    }

    // clone old mem and add offset
    // oldMem: [base, imm:12] -> newMem: [base, imm:(12 + offset)]
    MemOperand &GetMemOperandAddOffset(const MemOperand &oldMem, uint32 offset, uint32 newSize)
    {
        auto &newMem = static_cast<MemOperand &>(*oldMem.Clone(*GetMemoryPool()));
        auto &oldOffset = *oldMem.GetOffsetOperand();
        auto &newOffst = static_cast<ImmOperand &>(*oldOffset.Clone(*GetMemoryPool()));
        newOffst.SetValue(oldOffset.GetValue() + offset);
        newMem.SetOffsetOperand(newOffst);
        newMem.SetSize(newSize);
        return newMem;
    }

private:
    CGFunc &operator=(const CGFunc &cgFunc);
    CGFunc(const CGFunc &);
    StmtNode *HandleFirstStmt();
    MIRFunction &func;

    InsnBuilder *insnBuilder = nullptr;
    OperandBuilder *opndBuilder = nullptr;

    uint32 bbCnt = 0;
    uint32 labelIdx = 0;               /* local label index number */
    LabelNode *startLabel = nullptr;   /* start label of the function */
    LabelNode *returnLabel = nullptr;  /* return label of the function */
    LabelNode *endLabel = nullptr;     /* end label of the function */

    BB *firstBB = nullptr;
    BB *lastBB = nullptr;
    BB *curBB = nullptr;
    BB *dummyBB;                    /* use this bb for add some instructions to bb that is no curBB. */
    BB *commonExitBB = nullptr;     /* this post-dominate all BBs */
    MapleVector<BB *> exitBBVec;
    MapleVector<BB *> noReturnCallBBVec;
    MapleSet<regno_t> extendSet; /* use to mark regs which spilled 32 bits but loaded 64 bits. */
    MapleUnorderedMap<LabelIdx, BB *> lab2BBMap;
    BECommon &beCommon;
    CCImpl *callConv = nullptr;
    MemLayout *memLayout = nullptr;
    RegisterInfo *targetRegInfo = nullptr;
    MapleAllocator *funcScopeAllocator;
    MapleMap<uint32, MIRSymbol *> emitStVec; /* symbol that needs to be emit as a local symbol. i.e, switch table */
    MapleUnorderedMap<LabelIdx, int32> switchLabelCnt; /* label in switch table */
    std::map<const MIRSymbol *, LabelIdx> funcLocalSym2Label;
    CallConvKind callingConventionKind;
    CGCFG *theCFG = nullptr;
    FuncEmitInfo *funcEmitInfo = nullptr;
    uint32 nextSpillLocation = 0;
    regno_t spSaveReg = 0;

    const MapleString shortFuncName;
    bool hasAsm = false;
    bool useFP = true;
    bool seenFP = true;
    bool hasBuiltCfg = false;
    bool isStackMapComputed = false;

    /* save stack protect kinds which can trigger stack protect */
    uint8 stackProtectInfo = 0;
    bool needStackProtect = false;

    // mark exitBB is unreachable
    bool exitBBLost = false;
}; /* class CGFunc */

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgLayoutFrame, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgHandleFunction, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgVerify, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgGenCfi, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgEmission, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgGenProEpiLog, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_CGFUNC_H */
