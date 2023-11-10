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

#ifndef MAPLEBE_INCLUDE_CG_LOCALO_H
#define MAPLEBE_INCLUDE_CG_LOCALO_H

#include "cg_phase.h"
#include "cgbb.h"
#include "live.h"
#include "loop.h"
#include "cg.h"

namespace maplebe {
class LocalOpt {
public:
    LocalOpt(MemPool &memPool, CGFunc &func, ReachingDefinition &rd)
        : localoMp(&memPool), cgFunc(&func), reachindDef(&rd)
    {
    }

    virtual ~LocalOpt() = default;

    void DoLocalCopyPropOptmize();

protected:
    ReachingDefinition *GetRDInfo()
    {
        return reachindDef;
    }
    MemPool *localoMp;
    CGFunc *cgFunc;
    ReachingDefinition *reachindDef;

private:
    virtual void DoLocalCopyProp() = 0;
};

class LocalOptimizeManager {
public:
    LocalOptimizeManager(CGFunc &cgFunc, ReachingDefinition &rd) : cgFunc(cgFunc), reachingDef(&rd) {}
    ~LocalOptimizeManager() = default;
    template <typename LocalPropOptimizePattern>
    void Optimize()
    {
        LocalPropOptimizePattern optPattern(cgFunc, *reachingDef);
        optPattern.Run();
    }

private:
    CGFunc &cgFunc;
    ReachingDefinition *reachingDef;
};

class LocalPropOptimizePattern {
public:
    LocalPropOptimizePattern(CGFunc &cgFunc, ReachingDefinition &rd) : cgFunc(cgFunc), reachingDef(&rd) {}
    virtual ~LocalPropOptimizePattern() = default;
    virtual bool CheckCondition(Insn &insn) = 0;
    virtual void Optimize(BB &bb, Insn &insn) = 0;
    void Run();

protected:
    std::string PhaseName() const
    {
        return "localopt";
    }
    CGFunc &cgFunc;
    ReachingDefinition *reachingDef;
};

class RedundantDefRemove : public LocalPropOptimizePattern {
public:
    RedundantDefRemove(CGFunc &cgFunc, ReachingDefinition &rd) : LocalPropOptimizePattern(cgFunc, rd) {}
    ~RedundantDefRemove() override = default;
    bool CheckCondition(Insn &insn) final;
};

MAPLE_FUNC_PHASE_DECLARE(LocalCopyProp, maplebe::CGFunc)
} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_LOCALO_H */
