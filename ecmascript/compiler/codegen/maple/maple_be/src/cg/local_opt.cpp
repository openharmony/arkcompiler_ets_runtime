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

#include "local_opt.h"
#include "cg.h"
#include "mpl_logging.h"
#if defined TARGX86_64
#include "x64_reaching.h"
#endif
/*
 * this phase does optimization on local level(single bb or super bb)
 * this phase requires liveanalysis
 */
namespace maplebe {
void LocalOpt::DoLocalCopyPropOptmize()
{
    DoLocalCopyProp();
}

void LocalPropOptimizePattern::Run()
{
    FOR_ALL_BB(bb, &cgFunc) {
        FOR_BB_INSNS(insn, bb) {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            if (!CheckCondition(*insn)) {
                continue;
            }
            Optimize(*bb, *insn);
        }
    }
}

bool LocalCopyProp::PhaseRun(maplebe::CGFunc &f)
{
    MemPool *mp = GetPhaseMemPool();
    auto *reachingDef = f.GetCG()->CreateReachingDefinition(*mp, f);
    LocalOpt *localOpt = f.GetCG()->CreateLocalOpt(*mp, f, *reachingDef);
    localOpt->DoLocalCopyPropOptmize();
    return false;
}

void LocalCopyProp::GetAnalysisDependence(maple::AnalysisDep &aDep) const
{
    aDep.AddRequired<CgLiveAnalysis>();
    aDep.SetPreservedAll();
}

bool RedundantDefRemove::CheckCondition(Insn &insn)
{
    uint32 opndNum = insn.GetOperandSize();
    const InsnDesc *md = insn.GetDesc();
    std::vector<Operand *> defOpnds;
    for (uint32 i = 0; i < opndNum; ++i) {
        Operand &opnd = insn.GetOperand(i);
        auto *opndDesc = md->opndMD[i];
        if (opndDesc->IsDef() && opndDesc->IsUse()) {
            return false;
        }
        if (opnd.IsList()) {
            continue;
        }
        if (opndDesc->IsDef()) {
            defOpnds.emplace_back(&opnd);
        }
    }
    if (defOpnds.size() != 1 || !defOpnds[0]->IsRegister()) {
        return false;
    }
    auto &regDef = static_cast<RegOperand &>(*defOpnds[0]);
    auto &liveOutRegSet = insn.GetBB()->GetLiveOutRegNO();
    if (liveOutRegSet.find(regDef.GetRegisterNumber()) != liveOutRegSet.end()) {
        return false;
    }
    return true;
}

MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(LocalCopyProp, localcopyprop)
} /* namespace maplebe */
