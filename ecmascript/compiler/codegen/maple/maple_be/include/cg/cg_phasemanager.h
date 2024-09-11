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

#ifndef MAPLEBE_INCLUDE_CG_CG_PHASEMANAGER_H
#define MAPLEBE_INCLUDE_CG_CG_PHASEMANAGER_H
#include <vector>
#include <string>
#include <sys/stat.h>
#include "mempool.h"
#include "mempool_allocator.h"
#include "mir_module.h"
#include "mir_lower.h"
#include "lower.h"
#include "constantfold.h"
#include "cgfunc.h"
#include "cg_phase.h"
#include "cg_option.h"
#include "target_select.h"
#include "target_registry.h"
namespace maplebe {
using cgFuncOptTy = MapleFunctionPhase<CGFunc>;

/* =================== new phase manager ===================  */
class CgFuncPM : public FunctionPM {
public:
    explicit CgFuncPM(MemPool *mp) : FunctionPM(mp, &id) {}
    PHASECONSTRUCTOR(CgFuncPM);
    std::string PhaseName() const override;
    ~CgFuncPM() override
    {
        cgOptions = nullptr;
        cg = nullptr;
        beCommon = nullptr;
        if (CGOptions::IsEnableTimePhases()) {
            DumpPhaseTime();
        }
    }
    bool PhaseRun(MIRModule &m) override;

    void SetCGOptions(CGOptions *curCGOptions)
    {
        cgOptions = curCGOptions;
    }

    CG *GetCG()
    {
        return cg;
    }
    BECommon *GetBECommon()
    {
        return beCommon;
    }

private:
    bool FuncLevelRun(CGFunc &cgFunc, AnalysisDataManager &serialADM);
    void GenerateOutPutFile(MIRModule &m);
    void CreateCGAndBeCommon(MIRModule &m, const Target *T);
    void PrepareLower(MIRModule &m);
    void PostOutPut(MIRModule &m);
    void DoFuncCGLower(const MIRModule &m, MIRFunction &mirFunc);
    /* Tool functions */
    void DumpFuncCGIR(const CGFunc &f, const std::string &phaseName) const;
    /* For Emit */
    void EmitGlobalInfo(MIRModule &m) const;
    void EmitDuplicatedAsmFunc(MIRModule &m) const;
    bool IsFramework(MIRModule &m) const;

    CG *cg = nullptr;
    BECommon *beCommon = nullptr;
    MIRLower *mirLower = nullptr;
    CGLowerer *cgLower = nullptr;
    /* module options */
    CGOptions *cgOptions = nullptr;
};
} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_CG_PHASEMANAGER_H */
