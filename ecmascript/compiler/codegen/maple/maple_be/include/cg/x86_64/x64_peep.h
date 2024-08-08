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

#ifndef MAPLEBE_INCLUDE_CG_X64_X64_PEEP_H
#define MAPLEBE_INCLUDE_CG_X64_X64_PEEP_H

#include <vector>
#include "peep.h"

namespace maplebe {
class X64CGPeepHole : public CGPeepHole {
public:
    /* normal constructor */
    X64CGPeepHole(CGFunc &f, MemPool *memPool) : CGPeepHole(f, memPool) {};
    ~X64CGPeepHole() = default;
    void Run() override;
    void DoNormalOptimize(BB &bb, Insn &insn) override;
};

class X64RemoveMovingtoSameRegPattern : public CGPeepPattern {
public:
    X64RemoveMovingtoSameRegPattern(CGFunc &cgFunc,
        BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn)
    {
    }
    ~X64RemoveMovingtoSameRegPattern() override = default;
    void Run(BB &bb, Insn &insn) override;
    bool CheckCondition(Insn &insn) override;
    std::string GetPatternName() override
    {
        return "X64RemoveMovingtoSameRegPattern";
    }
};

} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_PEEP_H */
