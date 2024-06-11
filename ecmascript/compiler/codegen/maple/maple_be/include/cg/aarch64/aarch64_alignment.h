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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ALIGNMENT_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ALIGNMENT_H

#include "alignment.h"
#include "aarch64_cgfunc.h"

namespace maplebe {
constexpr uint32 kAlignRegionPower = 4;
constexpr uint32 kAlignInsnLength = 4;
constexpr uint32 kAlignMaxNopNum = 1;

struct AArch64AlignInfo {
    /* if bb size in (16byte, 96byte) , the bb need align */
    uint32 alignMinBBSize = 16;
    uint32 alignMaxBBSize = 96;
    /* default loop & jump align power, related to the target machine.  eg. 2^5 */
    uint32 loopAlign = 4;
    uint32 jumpAlign = 5;
    /* record func_align_power in CGFunc */
};

class AArch64AlignAnalysis : public AlignAnalysis {
public:
    AArch64AlignAnalysis(CGFunc &func, MemPool &memPool, LoopAnalysis &loop) : AlignAnalysis(func, memPool, loop)
    {
        aarFunc = static_cast<AArch64CGFunc *>(&func);
    }
    ~AArch64AlignAnalysis() override = default;

    void FindLoopHeader() override;
    void FindJumpTarget() override;
    void ComputeLoopAlign() override;
    void ComputeJumpAlign() override;
    void ComputeCondBranchAlign() override;
    bool MarkShortBranchSplit();
    void UpdateInsnId();
    uint32 GetAlignRange(uint32 alignedVal, uint32 addr) const;

    /* filter condition */
    bool IsIncludeCall(BB &bb) override;
    bool IsInSizeRange(BB &bb) override;
    bool HasFallthruEdge(BB &bb) override;
    bool IsInSameAlignedRegion(uint32 addr1, uint32 addr2, uint32 alignedRegionSize) const;

private:
    AArch64CGFunc *aarFunc = nullptr;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ALIGNMENT_H */
