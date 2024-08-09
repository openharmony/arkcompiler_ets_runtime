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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CFGO_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CFGO_H

#include "cfgo.h"
#include "aarch64_isa.h"

namespace maplebe {
class AArch64CFGOptimizer : public CFGOptimizer {
public:
    AArch64CFGOptimizer(CGFunc &func, MemPool &memPool, LoopAnalysis &loop) : CFGOptimizer(func, memPool, loop) {}
    ~AArch64CFGOptimizer() override = default;
    void InitOptimizePatterns() override;
};

class AArch64FlipBRPattern : public FlipBRPattern {
public:
    explicit AArch64FlipBRPattern(CGFunc &func, LoopAnalysis &loop) : FlipBRPattern(func, loop) {}
    ~AArch64FlipBRPattern() override = default;

private:
    uint32 GetJumpTargetIdx(const Insn &insn) override;
    MOperator FlipConditionOp(MOperator flippedOp) override;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CFGO_H */
