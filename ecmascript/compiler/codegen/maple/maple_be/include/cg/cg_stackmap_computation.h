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

#ifndef MAPLEBE_CG_INCLUDE_CG_STACKMAP_COMPUTATION_H
#define MAPLEBE_CG_INCLUDE_CG_STACKMAP_COMPUTATION_H

#include "cgfunc.h"
#include "cg_phase.h"
#include "stackmap.h"

namespace maplebe {
class StackmapComputation {
public:
    explicit StackmapComputation(CGFunc &func, MemPool &memPool)
        : cgFunc(func),
          alloc(&memPool),
          derivedRef2Base(alloc.Adapter()),
          regInfo(func.GetTargetRegInfo())
    {
        regInfo->Init();
    }

    ~StackmapComputation() {};
    void run();
    void SpillOperand(Insn &insn, regno_t regNO);
    void SetStackmapDerivedInfo();
    void RelocateStackmapInfo();
    void CollectReferenceMap();
    void CollectDeoptInfo();
    void SolveMemOpndDeoptInfo(const MemOperand &memOpnd, DeoptInfo &deoptInfo, int32 deoptVregNO) const;
    void SolveRegOpndDeoptInfo(const RegOperand &regOpnd, DeoptInfo &deoptInfo, int32 deoptVregNO) const;
    void LoadOperand(Insn &insn, regno_t);
    MemOperand *GetSpillMem(uint32 vRegNO, bool isDest, Insn &insn, regno_t regNO, bool &isOutOfRange, uint32 bitSize);

    std::string PhaseName() const
    {
        return "stackmapcomputation";
    }

    void SetNeedDump(bool dump)
    {
        needDump = dump;
    }

protected:
    CGFunc &cgFunc;
    MapleAllocator alloc;
    MapleUnorderedMap<regno_t, RegOperand*> derivedRef2Base;
    bool needDump = false;
    RegisterInfo *regInfo = nullptr;
};

MAPLE_FUNC_PHASE_DECLARE(CgStackmapComputation, maplebe::CGFunc)
}  // namespace maplebe

#endif  // MAPLEBE_CG_INCLUDE_CG_STACKMAP_COMPUTATION_H
