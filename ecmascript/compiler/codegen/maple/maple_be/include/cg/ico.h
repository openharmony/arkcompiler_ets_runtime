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

#ifndef MAPLEBE_INCLUDE_CG_ICO_H
#define MAPLEBE_INCLUDE_CG_ICO_H
#include "optimize_common.h"
#include "live.h"

namespace maplebe {
class IfConversionOptimizer : public Optimizer {
public:
    IfConversionOptimizer(CGFunc &func, MemPool &memPool) : Optimizer(func, memPool)
    {
        name = "ICO";
    }

    ~IfConversionOptimizer() override = default;
};

/* If-Then-Else pattern */
class ICOPattern : public OptimizationPattern {
public:
    explicit ICOPattern(CGFunc &func) : OptimizationPattern(func)
    {
        dotColor = kIcoIte;
        patternName = "If-Then-Else";
    }
    ~ICOPattern() override = default;
    static constexpr int kThreshold = 2;

protected:
    Insn *FindLastCmpInsn(BB &bb) const;
    std::vector<LabelOperand *> GetLabelOpnds(Insn &insn) const;
};

MAPLE_FUNC_PHASE_DECLARE(CgIco, maplebe::CGFunc)
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_ICO_H */
