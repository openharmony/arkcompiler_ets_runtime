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

#ifndef MAPLEBE_INCLUDE_CG_REG_INFO_H
#define MAPLEBE_INCLUDE_CG_REG_INFO_H

#include "isa.h"
#include "insn.h"

namespace maplebe {
constexpr size_t kSpillMemOpndNum = 4;
constexpr uint32 kBaseVirtualRegNO = 200; /* avoid conflicts between virtual and physical */
constexpr uint32 kRegIncrStepLen = 80;    /* reg number increate step length */

class VirtualRegNode {
public:
    VirtualRegNode() = default;

    VirtualRegNode(RegType type, uint32 size) : regType(type), size(size), regNO(kInvalidRegNO) {}

    virtual ~VirtualRegNode() = default;

    void AssignPhysicalRegister(regno_t phyRegNO)
    {
        regNO = phyRegNO;
    }

    RegType GetType() const
    {
        return regType;
    }

    uint32 GetSize() const
    {
        return size;
    }

private:
    RegType regType = kRegTyUndef;
    uint32 size = 0;               /* size in bytes */
    regno_t regNO = kInvalidRegNO; /* physical register assigned by register allocation */
};

class VregInfo {
public:
    /* Only one place to allocate vreg within cg.
       'static' can be removed and initialized here if only allocation is from only one source.  */
    static uint32 virtualRegCount;
    static uint32 maxRegCount;
    static std::vector<VirtualRegNode> vRegTable;
    static std::unordered_map<regno_t, RegOperand *> vRegOperandTable;
    static bool initialized;

    VregInfo()
    {
        if (initialized) {
            initialized = false;
            return;
        }
        initialized = true;
        virtualRegCount = kBaseVirtualRegNO;
        maxRegCount = kBaseVirtualRegNO;
        vRegTable.clear();
        vRegOperandTable.clear();
    }

    ~VregInfo() = default;

    uint32 GetNextVregNO(RegType type, uint32 size)
    {
        /* when vReg reach to maxRegCount, maxRegCount limit adds 80 every time */
        /* and vRegTable increases 80 elements. */
        if (virtualRegCount >= maxRegCount) {
            DEBUG_ASSERT(virtualRegCount < maxRegCount + 1, "MAINTAIN FAILED");
            maxRegCount += kRegIncrStepLen;
            VRegTableResize(maxRegCount);
        }
#if TARGAARCH64 || TARGX86_64 || TARGRISCV64
        if (size < k4ByteSize) {
            size = k4ByteSize;
        }
        if (Triple::GetTriple().IsAarch64BeOrLe()) {
            /* cannot handle 128 size register */
            if (type == kRegTyInt && size > k8ByteSize) {
                size = k8ByteSize;
            }
        }
        DEBUG_ASSERT(size == k4ByteSize || size == k8ByteSize || size == k16ByteSize, "check size");
#endif
        VRegTableValuesSet(virtualRegCount, type, size);

        uint32 temp = virtualRegCount;
        ++virtualRegCount;
        return temp;
    }
    void Inc(uint32 v) const
    {
        virtualRegCount += v;
    }
    uint32 GetCount() const
    {
        return virtualRegCount;
    }
    void SetCount(uint32 v) const
    {
        /* Vreg number can only increase. */
        if (virtualRegCount < v) {
            virtualRegCount = v;
        }
    }

    /* maxRegCount related stuff */
    uint32 GetMaxRegCount() const
    {
        return maxRegCount;
    }
    void SetMaxRegCount(uint32 num)
    {
        maxRegCount = num;
    }
    void IncMaxRegCount(uint32 num) const
    {
        maxRegCount += num;
    }

    /* vRegTable related stuff */
    void VRegTableResize(uint32 sz)
    {
        vRegTable.resize(sz);
    }
    uint32 VRegTableSize() const
    {
        return static_cast<uint32>(vRegTable.size());
    }
    uint32 VRegTableGetSize(uint32 idx) const
    {
        return vRegTable[idx].GetSize();
    }
    RegType VRegTableGetType(uint32 idx) const
    {
        return vRegTable[idx].GetType();
    }
    VirtualRegNode &VRegTableElementGet(uint32 idx) const
    {
        return vRegTable[idx];
    }
    void VRegTableElementSet(uint32 idx, VirtualRegNode *node)
    {
        vRegTable[idx] = *node;
    }
    void VRegTableValuesSet(uint32 idx, RegType rt, uint32 sz) const
    {
        new (&vRegTable[idx]) VirtualRegNode(rt, sz);
    }
    void VRegOperandTableSet(regno_t regNO, RegOperand *rp) const
    {
        vRegOperandTable[regNO] = rp;
    }
};
using VRegNo = regno_t;
using PRegNo = regno_t;
using RegNoPair = std::pair<VRegNo, PRegNo>;

class RegisterInfo {
public:
    explicit RegisterInfo(MapleAllocator &mallocator)
        : allIntRegs(mallocator.Adapter()), allFpRegs(mallocator.Adapter()), allregs(mallocator.Adapter())
    {
    }

    virtual ~RegisterInfo()
    {
        cgFunc = nullptr;
    }

    virtual void Init() = 0;
    virtual void Fini() = 0;
    void AddToAllRegs(regno_t regNo)
    {
        (void)allregs.insert(regNo);
    }
    const MapleSet<regno_t> &GetAllRegs() const
    {
        return allregs;
    }
    void AddToIntRegs(regno_t regNo)
    {
        (void)allIntRegs.insert(regNo);
    }
    const MapleSet<regno_t> &GetIntRegs() const
    {
        return allIntRegs;
    }
    void AddToFpRegs(regno_t regNo)
    {
        (void)allFpRegs.insert(regNo);
    }
    const MapleSet<regno_t> &GetFpRegs() const
    {
        return allFpRegs;
    }
    void SetCurrFunction(CGFunc &func)
    {
        cgFunc = &func;
    }
    CGFunc *GetCurrFunction() const
    {
        return cgFunc;
    }
    // When some registers are allocated to the callee, the caller stores a part of the registers
    // and the callee stores another part of the registers.
    // For these registers, it is a better choice to assign them as caller-save registers.
    virtual bool IsPrefCallerSaveRegs(RegType type, uint32 size) const
    {
        return false;
    }
    virtual bool IsCallerSavePartRegister(regno_t regNO, uint32 size) const
    {
        return false;
    }
    virtual RegOperand *GetOrCreatePhyRegOperand(regno_t regNO, uint32 size, RegType kind, uint32 flag = 0) = 0;
    virtual bool IsGPRegister(regno_t regNO) const = 0;
    virtual uint32 GetIntParamRegIdx(regno_t regNO) const = 0;
    virtual uint32 GetFpParamRegIdx(regno_t regNO) const = 0;
    virtual bool IsAvailableReg(regno_t regNO) const = 0;
    virtual bool IsCalleeSavedReg(regno_t regno) const = 0;
    virtual bool IsYieldPointReg(regno_t regNO) const = 0;
    virtual bool IsUnconcernedReg(regno_t regNO) const = 0;
    virtual bool IsVirtualRegister(const RegOperand &regOpnd) = 0;
    virtual bool IsVirtualRegister(regno_t regno) = 0;
    virtual uint32 GetIntRegsParmsNum() = 0;
    virtual uint32 GetIntRetRegsNum() = 0;
    virtual uint32 GetFpRetRegsNum() = 0;
    virtual uint32 GetFloatRegsParmsNum() = 0;
    virtual regno_t GetLastParamsIntReg() = 0;
    virtual regno_t GetLastParamsFpReg() = 0;
    virtual regno_t GetIntRetReg(uint32 idx) = 0;
    virtual regno_t GetFpRetReg(uint32 idx) = 0;
    virtual uint32 GetReservedSpillReg() = 0;
    virtual uint32 GetSecondReservedSpillReg() = 0;
    virtual uint32 GetAllRegNum() = 0;
    virtual uint32 GetNormalUseOperandNum() = 0;
    virtual regno_t GetInvalidReg() = 0;
    virtual bool IsSpillRegInRA(regno_t regNO, bool has3RegOpnd) = 0;
    virtual Insn *BuildStrInsn(uint32 regSize, PrimType stype, RegOperand &phyOpnd, MemOperand &memOpnd) = 0;
    virtual Insn *BuildLdrInsn(uint32 regSize, PrimType stype, RegOperand &phyOpnd, MemOperand &memOpnd) = 0;
    virtual bool IsMovFromRegtoReg(MOperator mOp, Insn &insn) = 0;
    virtual MemOperand *GetOrCreatSpillMem(regno_t vrNum, uint32 bitSize) = 0;
    virtual MemOperand *AdjustMemOperandIfOffsetOutOfRange(MemOperand *memOpnd, const RegNoPair &regNoPair, bool isDest,
                                                           Insn &insn, bool &isOutOfRange) = 0;
    virtual void FreeSpillRegMem(regno_t vrNum) = 0;

private:
    MapleSet<regno_t> allIntRegs;
    MapleSet<regno_t> allFpRegs;
    MapleSet<regno_t> allregs;
    CGFunc *cgFunc = nullptr;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_REG_INFO_H */
