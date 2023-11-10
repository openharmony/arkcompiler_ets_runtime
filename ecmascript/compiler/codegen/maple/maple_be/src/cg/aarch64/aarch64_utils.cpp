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

#include "aarch64_utils.h"
#include "cg_option.h"

namespace maplebe {

MemOperand *GetOrCreateMemOperandForNewMOP(CGFunc &cgFunc, const Insn &loadIns, MOperator newLoadMop)
{
    MemPool &memPool = *cgFunc.GetMemoryPool();
    auto *memOp = static_cast<MemOperand *>(loadIns.GetMemOpnd());
    MOperator loadMop = loadIns.GetMachineOpcode();

    DEBUG_ASSERT(loadIns.IsLoad() && AArch64CG::kMd[newLoadMop].IsLoad(), "ins and Mop must be load");

    MemOperand *newMemOp = memOp;

    uint32 memSize = AArch64CG::kMd[loadMop].GetOperandSize();
    uint32 newMemSize = AArch64CG::kMd[newLoadMop].GetOperandSize();

    if (newMemSize == memSize) {
        // if sizes are the same just return old memory operand
        return newMemOp;
    }

    newMemOp = memOp->Clone(memPool);
    newMemOp->SetSize(newMemSize);

    if (!CGOptions::IsBigEndian()) {
        return newMemOp;
    }

    // for big-endian it's necessary to adjust offset if it's present
    if (memOp->GetAddrMode() != MemOperand::kAddrModeBOi || newMemSize > memSize) {
        // currently, it's possible to adjust an offset only for immediate offset
        // operand if new size is less than the original one
        return nullptr;
    }

    auto *newOffOp = static_cast<OfstOperand *>(memOp->GetOffsetImmediate()->Clone(memPool));

    newOffOp->AdjustOffset(static_cast<int32>((memSize - newMemSize) >> kLog2BitsPerByte));
    newMemOp->SetOffsetOperand(*newOffOp);

    DEBUG_ASSERT(memOp->IsOffsetMisaligned(memSize) || !newMemOp->IsOffsetMisaligned(newMemSize),
                 "New offset value is misaligned!");

    return newMemOp;
}

}  // namespace maplebe
