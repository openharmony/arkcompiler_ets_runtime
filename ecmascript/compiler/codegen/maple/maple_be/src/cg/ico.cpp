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

#include "ico.h"
#include "cg_option.h"
#ifdef TARGAARCH64
#include "aarch64_ico.h"
#include "aarch64_isa.h"
#include "aarch64_insn.h"
#elif TARGRISCV64
#include "riscv64_ico.h"
#include "riscv64_isa.h"
#include "riscv64_insn.h"
#elif TARGARM32
#include "arm32_ico.h"
#include "arm32_isa.h"
#include "arm32_insn.h"
#endif
#include "cg.h"

/*
 * This phase implements if-conversion optimization,
 * which tries to convert conditional branches into cset/csel instructions
 */
#define ICO_DUMP_NEWPM CG_DEBUG_FUNC(f)
namespace maplebe {
Insn *ICOPattern::FindLastCmpInsn(BB &bb) const
{
    if (bb.GetKind() != BB::kBBIf) {
        return nullptr;
    }
    FOR_BB_INSNS_REV(insn, (&bb)) {
        if (cgFunc->GetTheCFG()->GetInsnModifier()->IsCompareInsn(*insn)) {
            return insn;
        }
    }
    return nullptr;
}

std::vector<LabelOperand *> ICOPattern::GetLabelOpnds(Insn &insn) const
{
    std::vector<LabelOperand *> labelOpnds;
    for (uint32 i = 0; i < insn.GetOperandSize(); i++) {
        if (insn.GetOperand(i).IsLabelOpnd()) {
            labelOpnds.emplace_back(static_cast<LabelOperand *>(&insn.GetOperand(i)));
        }
    }
    return labelOpnds;
}

bool CgIco::PhaseRun(maplebe::CGFunc &f)
{
    LiveAnalysis *live = GET_ANALYSIS(CgLiveAnalysis, f);
    if (ICO_DUMP_NEWPM) {
        DotGenerator::GenerateDot("ico-before", f, f.GetMirModule());
    }
    MemPool *memPool = GetPhaseMemPool();
    IfConversionOptimizer *ico = nullptr;
#if TARGAARCH64 || TARGRISCV64
    ico = memPool->New<AArch64IfConversionOptimizer>(f, *memPool);
#endif
#if TARGARM32
    ico = memPool->New<Arm32IfConversionOptimizer>(f, *memPool);
#endif
    const std::string &funcClass = f.GetFunction().GetBaseClassName();
    const std::string &funcName = f.GetFunction().GetBaseFuncName();
    std::string name = funcClass + funcName;
    ico->Run(name);
    if (ICO_DUMP_NEWPM) {
        DotGenerator::GenerateDot("ico-after", f, f.GetMirModule());
    }
    /* the live range info may changed, so invalid the info. */
    if (live != nullptr) {
        live->ClearInOutDataInfo();
    }
    return false;
}
void CgIco::GetAnalysisDependence(maple::AnalysisDep &aDep) const
{
    aDep.AddRequired<CgLiveAnalysis>();
    aDep.PreservedAllExcept<CgLiveAnalysis>();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgIco, ico)
} /* namespace maplebe */
