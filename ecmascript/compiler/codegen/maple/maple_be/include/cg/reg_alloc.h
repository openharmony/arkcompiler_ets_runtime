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

#ifndef MAPLEBE_INCLUDE_CG_REG_ALLOC_H
#define MAPLEBE_INCLUDE_CG_REG_ALLOC_H

#include <map>
#include <stack>
#include "isa.h"
#include "cg_phase.h"
#include "maple_phase_manager.h"

namespace maplebe {
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

class RegAllocator {
public:
    RegAllocator(CGFunc &tempCGFunc, MemPool &memPool) : cgFunc(&tempCGFunc), memPool(&memPool), alloc(&memPool) {}

    virtual ~RegAllocator() = default;

    virtual bool AllocateRegisters() = 0;

    bool IsYieldPointReg(regno_t regNO) const;
    bool IsUntouchableReg(regno_t regNO) const;

    virtual std::string PhaseName() const
    {
        return "regalloc";
    }

protected:
    CGFunc *cgFunc;
    MemPool *memPool;
    MapleAllocator alloc;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgRegAlloc, CGFunc)
OVERRIDE_DEPENDENCE
MAPLE_FUNC_PHASE_DECLARE_END
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_REG_ALLOC_H */
