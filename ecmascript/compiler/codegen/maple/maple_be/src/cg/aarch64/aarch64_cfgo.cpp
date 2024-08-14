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

#include "aarch64_cfgo.h"

namespace maplebe {
/* Initialize cfg optimization patterns */
void AArch64CFGOptimizer::InitOptimizePatterns()
{
    diffPassPatterns.emplace_back(memPool->New<ChainingPattern>(*cgFunc));
    auto *brOpt = memPool->New<AArch64FlipBRPattern>(*cgFunc, loopInfo);
    if (GetPhase() == kCfgoPostRegAlloc) {
        brOpt->SetPhase(kCfgoPostRegAlloc);
    }
    diffPassPatterns.emplace_back(brOpt);
    diffPassPatterns.emplace_back(memPool->New<UnreachBBPattern>(*cgFunc));
    diffPassPatterns.emplace_back(memPool->New<EmptyBBPattern>(*cgFunc));
}

uint32 AArch64FlipBRPattern::GetJumpTargetIdx(const Insn &insn)
{
    return AArch64isa::GetJumpTargetIdx(insn);
}

MOperator AArch64FlipBRPattern::FlipConditionOp(MOperator flippedOp)
{
    return AArch64isa::FlipConditionOp(flippedOp);
}
}  // namespace maplebe
