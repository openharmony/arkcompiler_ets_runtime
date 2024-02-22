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
#ifndef MAPLEBE_INCLUDE_CG_GLOBAL_SCHEDULE_H
#define MAPLEBE_INCLUDE_CG_GLOBAL_SCHEDULE_H

#include "base_schedule.h"

namespace maplebe {
#define GLOBAL_SCHEDULE_DUMP CG_DEBUG_FUNC(cgFunc)

class GlobalSchedule : public BaseSchedule {
public:
    GlobalSchedule(MemPool &mp, CGFunc &f, ControlDepAnalysis &cdAna, DataDepAnalysis &dda)
        : BaseSchedule(mp, f, cdAna), interDDA(dda)
    {
    }
    ~GlobalSchedule() override = default;

    std::string PhaseName() const
    {
        return "globalschedule";
    }
    void Run() override;
    bool CheckCondition(CDGRegion &region);
    // Region-based global scheduling entry, using the list scheduling algorithm for scheduling insns in bb
    void DoGlobalSchedule(CDGRegion &region);

    // Verifying the Correctness of Global Scheduling
    virtual void VerifyingSchedule(CDGRegion &region) = 0;

protected:
    void InitInCDGNode(CDGRegion &region, CDGNode &cdgNode, MemPool &cdgNodeMp);
    void PrepareCommonSchedInfo(CDGRegion &region, CDGNode &cdgNode, MemPool &cdgNodeMp);
    virtual void FinishScheduling(CDGNode &cdgNode) = 0;
    void ClearCDGNodeInfo(CDGRegion &region, CDGNode &cdgNode, MemPool *cdgNodeMp);

    DataDepAnalysis &interDDA;
};

MAPLE_FUNC_PHASE_DECLARE(CgGlobalSchedule, maplebe::CGFunc)
}  // namespace maplebe

#endif  // MAPLEBE_INCLUDE_CG_GLOBAL_SCHEDULE_H
