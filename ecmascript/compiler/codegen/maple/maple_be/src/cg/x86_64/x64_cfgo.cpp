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

#include "x64_cfgo.h"
#include "x64_isa.h"

namespace maplebe {
/* Initialize cfg optimization patterns */
void X64CFGOptimizer::InitOptimizePatterns()
{
    diffPassPatterns.emplace_back(memPool->New<X64FlipBRPattern>(*cgFunc, loopInfo));
    diffPassPatterns.emplace_back(memPool->New<UnreachBBPattern>(*cgFunc));
    diffPassPatterns.emplace_back(memPool->New<EmptyBBPattern>(*cgFunc));
}

uint32 X64FlipBRPattern::GetJumpTargetIdx(const Insn &insn)
{
    return x64::GetJumpTargetIdx(insn);
}
MOperator X64FlipBRPattern::FlipConditionOp(MOperator flippedOp)
{
    return x64::FlipConditionOp(flippedOp);
}
}  // namespace maplebe
