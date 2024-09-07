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

#ifndef MAPLEBE_INCLUDE_CG_INSN_H
#define MAPLEBE_INCLUDE_CG_INSN_H
/* C++ headers */
#include <cstddef> /* for nullptr */
#include <string>
#include <vector>
#include <list>
/* Maple CG headers */
#include "operand.h"
#include "isa.h"
#include "common_utils.h"
/* Maple IR header */
#include "types_def.h" /* for uint32 */
/* Maple Util headers */
#include "mpl_logging.h"

#include "stackmap.h"
#include "sparse_datainfo.h"

/* Maple Util headers */
#include "mem_reference_table.h" /* for alias */
namespace maplebe {
/* forward declaration */
class BB;
class CG;
class Emitter;
class DepNode;
struct InsnDesc;
class InsnBuilder;
class OperandBuilder;

class Insn {
public:
    /* MCC_DecRefResetPair clear 2 stack position, MCC_ClearLocalStackRef clear 1 stack position */
    static constexpr uint8 kMaxStackOffsetSize = 2;

    Insn(MemPool &memPool, MOperator opc)
        : mOp(opc),
          localAlloc(&memPool),
          opnds(localAlloc.Adapter()),
          registerBinding(localAlloc.Adapter()),
          comment(&memPool),
          stackMapLiveIn(localAlloc.Adapter())
    {
    }
    Insn(MemPool &memPool, MOperator opc, Operand &opnd0) : Insn(memPool, opc)
    {
        opnds.emplace_back(&opnd0);
    }
    Insn(MemPool &memPool, MOperator opc, Operand &opnd0, Operand &opnd1) : Insn(memPool, opc)
    {
        opnds.emplace_back(&opnd0);
        opnds.emplace_back(&opnd1);
    }
    Insn(MemPool &memPool, MOperator opc, Operand &opnd0, Operand &opnd1, Operand &opnd2) : Insn(memPool, opc)
    {
        opnds.emplace_back(&opnd0);
        opnds.emplace_back(&opnd1);
        opnds.emplace_back(&opnd2);
    }
    Insn(MemPool &memPool, MOperator opc, Operand &opnd0, Operand &opnd1, Operand &opnd2, Operand &opnd3)
        : Insn(memPool, opc)
    {
        opnds.emplace_back(&opnd0);
        opnds.emplace_back(&opnd1);
        opnds.emplace_back(&opnd2);
        opnds.emplace_back(&opnd3);
    }
    Insn(MemPool &memPool, MOperator opc, Operand &opnd0, Operand &opnd1, Operand &opnd2, Operand &opnd3,
         Operand &opnd4)
        : Insn(memPool, opc)
    {
        opnds.emplace_back(&opnd0);
        opnds.emplace_back(&opnd1);
        opnds.emplace_back(&opnd2);
        opnds.emplace_back(&opnd3);
        opnds.emplace_back(&opnd4);
    }
    virtual ~Insn() = default;

    void DeepClone(const Insn &insn, MapleAllocator &allocator)
    {
        opnds.clear();
        for (auto opnd : insn.opnds) {
            opnds.emplace_back(opnd->CloneTree(allocator));
        }
    }

    // Custom deep copy
    virtual Insn *CloneTree(MapleAllocator &allocator) const
    {
        auto *insn = allocator.GetMemPool()->New<Insn>(*this);
        insn->DeepClone(*this, allocator);
        return insn;
    }

    MOperator GetMachineOpcode() const
    {
        return mOp;
    }

    void SetMOP(const InsnDesc &idesc);

    void AddOperand(Operand &opnd)
    {
        opnds.emplace_back(&opnd);
    }

    Insn &AddOpndChain(Operand &opnd)
    {
        AddOperand(opnd);
        return *this;
    }
    /* use carefully which might cause insn to illegal */
    void CommuteOperands(uint32 dIndex, uint32 sIndex);
    void CleanAllOperand()
    {
        opnds.clear();
    }

    void PopBackOperand()
    {
        opnds.pop_back();
    }

    Operand &GetOperand(uint32 index) const
    {
        DEBUG_ASSERT(index < opnds.size(), "index out of range");
        return *opnds[index];
    }

    void ResizeOpnds(uint32 newSize)
    {
        opnds.resize(static_cast<std::size_t>(newSize));
    }

    uint32 GetOperandSize() const
    {
        return static_cast<uint32>(opnds.size());
    }

    void SetOperand(uint32 index, Operand &opnd)
    {
        DEBUG_ASSERT(index <= opnds.size(), "index out of range");
        opnds[index] = &opnd;
    }

    /* Get size info from machine description */
    uint32 GetOperandSize(uint32 index) const
    {
        CHECK_FATAL(index < opnds.size(), "index out of range!");
        const OpndDesc *opndMD = md->GetOpndDes(index);
        return opndMD->GetSize();
    }

    void SetRetSize(uint32 size)
    {
        DEBUG_ASSERT(IsCall(), "Insn should be a call.");
        retSize = size;
    }

    uint32 GetRetSize() const
    {
        DEBUG_ASSERT(IsCall(), "Insn should be a call.");
        return retSize;
    }

    // Insn Function: check legitimacy of opnds.
    bool VerifySelf() const
    {
        if (this->IsCfiInsn() || this->IsDbgInsn()) {
            return true;
        }
        return md->Verify(opnds);
    }

    void SplitSelf(bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
    {
        md->Split(this, isAfterRegAlloc, insnBuilder, opndBuilder);
    }

    virtual bool IsMachineInstruction() const;

    virtual bool IsIntrinsic() const
    {
        return false;
    }

    bool OpndIsDef(uint32 id) const;

    virtual bool IsPCLoad() const
    {
        return false;
    }

    Operand *GetMemOpnd() const;

    bool IsCall() const;
    bool IsSpecialCall() const;
    bool IsTailCall() const;
    bool IsAsmInsn() const;
    bool IsBranch() const;
    bool IsCondBranch() const;
    bool IsUnCondBranch() const;
    bool IsMove() const;
    bool IsBasicOp() const;
    bool IsUnaryOp() const;
    bool IsShift() const;
    bool IsPhi() const;
    bool IsLoad() const;
    bool IsStore() const;
    bool IsConversion() const;
    bool IsAtomic() const;
    bool IsLoadStorePair() const;
    bool IsLoadLabel() const;

    virtual bool NoAlias() const
    {
        return false;
    }

    bool IsMemAccess() const;

    virtual bool HasSideEffects() const
    {
        return false;
    }

    virtual bool IsSpecialIntrinsic() const;

    bool IsComment() const;
    bool IsImmaterialInsn() const;

    bool IsPseudo() const;

    virtual bool IsTargetInsn() const
    {
        return true;
    }

    virtual bool IsCfiInsn() const
    {
        return false;
    }

    virtual bool IsDbgInsn() const
    {
        return false;
    }

    virtual bool IsDbgLine() const
    {
        return false;
    }

    virtual Operand *GetCallTargetOperand() const;

    uint32 GetAtomicNum() const;
    bool IsAtomicStore() const
    {
        return IsStore() && IsAtomic();
    }

    void SetCondDef()
    {
        flags |= kOpCondDef;
    }

    bool IsCondDef() const
    {
        return flags & kOpCondDef;
    }

    bool AccessMem() const
    {
        return IsLoad() || IsStore();
    }

    bool IsFrameDef() const
    {
        return isFrameDef;
    }

    void SetFrameDef(bool b)
    {
        isFrameDef = b;
    }

    bool IsStackDef() const
    {
        return isStackDef;
    }

    void SetStackDef(bool flag)
    {
        isStackDef = flag;
    }

    bool IsStackRevert() const
    {
        return isStackRevert;
    }

    void SetStackRevert(bool flag)
    {
        isStackRevert = flag;
    }

    bool IsAsmDefCondCode() const
    {
        return asmDefCondCode;
    }

    void SetAsmDefCondCode()
    {
        asmDefCondCode = true;
    }

    bool IsAsmModMem() const
    {
        return asmModMem;
    }

    void SetAsmModMem()
    {
        asmModMem = true;
    }

    virtual uint32 GetUnitType()
    {
        return 0;
    }

    virtual void Dump() const;

    virtual bool CheckMD() const;

    void SetComment(const std::string &str)
    {
        comment = str;
    }

    void SetComment(const MapleString &str)
    {
        comment = str;
    }

    const MapleString &GetComment() const
    {
        return comment;
    }

    void AppendComment(const std::string &str)
    {
        comment += str;
    }

    void SetDebugComment(const MapleString *str)
    {
        debugComment = str;
    }

    const MapleString *GetDebugComment() const
    {
        return debugComment;
    }

    void MarkAsSaveRetValToLocal()
    {
        flags |= kOpDassignToSaveRetValToLocal;
    }

    bool IsSaveRetValToLocal() const
    {
        return ((flags & kOpDassignToSaveRetValToLocal) != 0);
    }

    void MarkAsAccessRefField(bool cond)
    {
        if (cond) {
            flags |= kOpAccessRefField;
        }
    }

    bool IsAccessRefField() const
    {
        return ((flags & kOpAccessRefField) != 0);
    }

    Insn *GetPreviousMachineInsn() const
    {
        for (Insn *returnInsn = prev; returnInsn != nullptr; returnInsn = returnInsn->prev) {
            DEBUG_ASSERT(returnInsn->bb == bb, "insn and it's prev insn must have same bb");
            if (returnInsn->IsMachineInstruction()) {
                return returnInsn;
            }
        }
        return nullptr;
    }

    Insn *GetNextMachineInsn() const
    {
        for (Insn *returnInsn = next; returnInsn != nullptr; returnInsn = returnInsn->next) {
            CHECK_FATAL(returnInsn->bb == bb, "insn and it's next insn must have same bb");
            if (returnInsn->IsMachineInstruction()) {
                return returnInsn;
            }
        }
        return nullptr;
    }

    void SetPrev(Insn *prev)
    {
        this->prev = prev;
    }

    Insn *GetPrev()
    {
        return prev;
    }

    const Insn *GetPrev() const
    {
        return prev;
    }

    void SetNext(Insn *next)
    {
        this->next = next;
    }

    Insn *GetNext() const
    {
        return next;
    }

    void SetBB(BB *bb)
    {
        this->bb = bb;
    }

    BB *GetBB()
    {
        return bb;
    }

    const BB *GetBB() const
    {
        return bb;
    }

    void SetId(uint32 id)
    {
        this->id = id;
    }

    uint32 GetId() const
    {
        return id;
    }

    void SetAddress(uint32 addr)
    {
        address = addr;
    }

    uint32 GetAddress() const
    {
        return address;
    }

    void SetNopNum(uint32 num)
    {
        nopNum = num;
    }

    uint32 GetNopNum() const
    {
        return nopNum;
    }

    void SetNeedSplit(bool flag)
    {
        needSplit = flag;
    }

    bool IsNeedSplit() const
    {
        return needSplit;
    }

    void SetIsThrow(bool isThrow)
    {
        this->isThrow = isThrow;
    }

    bool GetIsThrow() const
    {
        return isThrow;
    }

    void SetDoNotRemove(bool doNotRemove)
    {
        this->doNotRemove = doNotRemove;
    }

    bool GetDoNotRemove() const
    {
        return doNotRemove;
    }

    void SetIsSpill()
    {
        this->isSpill = true;
    }

    bool GetIsSpill() const
    {
        return isSpill;
    }

    void SetIsReload()
    {
        this->isReload = true;
    }

    bool GetIsReload() const
    {
        return isReload;
    }

    bool IsSpillInsn() const
    {
        return (isSpill || isReload);
    }

    void SetIsCallReturnUnsigned(bool unSigned)
    {
        DEBUG_ASSERT(IsCall(), "Insn should be a call.");
        this->isCallReturnUnsigned = unSigned;
    }

    bool GetIsCallReturnUnsigned() const
    {
        DEBUG_ASSERT(IsCall(), "Insn should be a call.");
        return isCallReturnUnsigned;
    }

    bool GetIsCallReturnSigned() const
    {
        DEBUG_ASSERT(IsCall(), "Insn should be a call.");
        return !isCallReturnUnsigned;
    }

    void SetClearStackOffset(short index, int64 offset)
    {
        CHECK_FATAL(index < kMaxStackOffsetSize, "out of clearStackOffset's range");
        clearStackOffset[index] = offset;
    }

    int64 GetClearStackOffset(short index) const
    {
        CHECK_FATAL(index < kMaxStackOffsetSize, "out of clearStackOffset's range");
        return clearStackOffset[index];
    }

    /* if function name is MCC_ClearLocalStackRef or MCC_DecRefResetPair, will clear designate stack slot */
    bool IsClearDesignateStackCall() const
    {
        return clearStackOffset[0] != -1 || clearStackOffset[1] != -1;
    }

    void SetDepNode(DepNode &depNode)
    {
        this->depNode = &depNode;
    }

    DepNode *GetDepNode()
    {
        return depNode;
    }

    const DepNode *GetDepNode() const
    {
        return depNode;
    }

    void SetIsPhiMovInsn(bool val)
    {
        isPhiMovInsn = val;
    }

    bool IsPhiMovInsn() const
    {
        return isPhiMovInsn;
    }

    void SetInsnDescrption(const InsnDesc &newMD)
    {
        md = &newMD;
    }

    const InsnDesc *GetDesc() const
    {
        return md;
    }

    void AddRegBinding(uint32 regA, uint32 regB)
    {
        (void)registerBinding.emplace(regA, regB);
    }

    const MapleMap<uint32, uint32> &GetRegBinding() const
    {
        return registerBinding;
    }

    void SetReferenceOsts(MemDefUse *memDefUse)
    {
        referenceOsts = memDefUse;
    }

    const MemDefUse *GetReferenceOsts() const
    {
        return referenceOsts;
    }

    void MergeReferenceOsts(Insn &rhs)
    {
        if (referenceOsts == nullptr) {
            SetReferenceOsts(rhs.referenceOsts);
        } else if (rhs.referenceOsts != nullptr) {
            referenceOsts->MergeOthers(*rhs.referenceOsts);
        }
    }

    bool Equals(const Insn &rhs) const
    {
        if (&rhs == this) {
            return true;
        }
        if (mOp != rhs.mOp || opnds.size() != rhs.opnds.size()) {
            return false;
        }
        for (uint32 i = 0; i < opnds.size(); ++i) {
            if (!opnds[i]->Equals(*rhs.opnds[i])) {
                return false;
            }
        }
        return true;
    }

    void AddDeoptBundleInfo(int32 deoptVreg, Operand &opnd)
    {
        if (stackMap == nullptr) {
            stackMap = localAlloc.New<StackMap>(localAlloc);
        }
        stackMap->GetDeoptInfo().AddDeoptBundleInfo(deoptVreg, opnd);
    }

    StackMap *GetStackMap()
    {
        return stackMap;
    }

    const StackMap *GetStackMap() const
    {
        return stackMap;
    }

    void InitStackMapInfo()
    {
        if (stackMap != nullptr) {
            return;
        }
        stackMap = localAlloc.New<StackMap>(localAlloc);
    }

    void SetRefSkipIdx(int32 index)
    {
        refSkipIdx = index;
    }

    /* Get Size of memory write/read by insn */
    uint32 GetMemoryByteSize() const;

    /* return ture if register appears */
    virtual bool ScanReg(regno_t regNO) const;

    virtual bool IsRegDefined(regno_t regNO) const;

    virtual std::set<uint32> GetDefRegs() const;

    void SetStackMapDef(SparseDataInfo &def)
    {
        stackMapDef = &def;
    }

    SparseDataInfo *GetStackMapDef()
    {
        return stackMapDef;
    }

    void SetStackMapUse(SparseDataInfo &use)
    {
        stackMapUse = &use;
    }

    SparseDataInfo *GetStackMapUse()
    {
        return stackMapUse;
    }

    void SetStackMapLiveIn(SparseDataInfo &liveIn)
    {
        liveIn.GetInfo().ConvertToSet(stackMapLiveIn);
    }

    const MapleSet<regno_t> &GetStackMapLiveIn() const
    {
        return stackMapLiveIn;
    }

    void ClearStackMapDefUse()
    {
        stackMapDef = nullptr;
        stackMapUse = nullptr;
    }

protected:
    MOperator mOp;
    MapleAllocator localAlloc;
    MapleVector<Operand *> opnds;
    Insn *prev = nullptr;
    Insn *next = nullptr;
    BB *bb = nullptr; /* BB to which this insn belongs */
    uint32 flags = 0;
    bool isPhiMovInsn = false;

private:
    MapleMap<uint32, uint32> registerBinding; /* used for inline asm only */
    StackMap *stackMap = nullptr;
    enum OpKind : uint32 {
        kOpUnknown = 0,
        kOpCondDef = 0x1,
        kOpAccessRefField = (1ULL << 30),            /* load-from/store-into a ref flag-fieldGetMachineOpcode() */
        kOpDassignToSaveRetValToLocal = (1ULL << 31) /* save return value to local flag */
    };

    uint32 id = 0;
    uint32 address = 0;
    uint32 nopNum = 0;
    uint32 retSize = 0; /* Byte size of the return value if insn is a call. */
    /* record the stack cleared by MCC_ClearLocalStackRef or MCC_DecRefResetPair */
    int64 clearStackOffset[kMaxStackOffsetSize] = {-1, -1};
    DepNode *depNode = nullptr; /* For dependence analysis, pointing to a dependence node. */
    const MapleString *debugComment = nullptr;
    MapleString comment;
    bool isThrow = false;
    bool doNotRemove = false;          /* caller reg cross call */
    bool isCallReturnUnsigned = false; /* for call insn only. false: signed, true: unsigned */
    bool isSpill = false;              /* used as hint for optimization */
    bool isReload = false;             /* used as hint for optimization */
    bool isFrameDef = false;
    bool isStackDef = false;     // def sp in prolog
    bool isStackRevert = false;  // revert sp in epilog
    bool asmDefCondCode = false;
    bool asmModMem = false;
    bool needSplit = false;

    /* for dynamic language to mark reference counting */
    int32 refSkipIdx = -1;

    /* for multiple architecture */
    const InsnDesc *md = nullptr;
    MemDefUse *referenceOsts = nullptr;
    SparseDataInfo *stackMapDef = nullptr;
    SparseDataInfo *stackMapUse = nullptr;
    MapleSet<regno_t> stackMapLiveIn;
};

struct InsnIdCmp {
    bool operator()(const Insn *lhs, const Insn *rhs) const
    {
        CHECK_FATAL(lhs != nullptr, "lhs is nullptr in InsnIdCmp");
        CHECK_FATAL(rhs != nullptr, "rhs is nullptr in InsnIdCmp");
        return lhs->GetId() < rhs->GetId();
    }
};
using InsnSet = std::set<Insn *, InsnIdCmp>;
using InsnMapleSet = MapleSet<Insn *, InsnIdCmp>;
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_INSN_H */
