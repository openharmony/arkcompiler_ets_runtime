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
#ifndef MAPLEBE_INCLUDE_CG_LOCAL_SCHEDULE_H
#define MAPLEBE_INCLUDE_CG_LOCAL_SCHEDULE_H

#include "base_schedule.h"

namespace maplebe {
#define LOCAL_SCHEDULE_DUMP CG_DEBUG_FUNC(cgFunc)

class LocalSchedule : public BaseSchedule {
public:
    LocalSchedule(MemPool &mp, CGFunc &f, ControlDepAnalysis &cdAna, DataDepAnalysis &dda)
        : BaseSchedule(mp, f, cdAna), intraDDA(dda)
    {
    }
    ~LocalSchedule() override = default;

    std::string PhaseName() const
    {
        return "localschedule";
    }
    void Run() override;
    bool CheckCondition(CDGRegion &region) const;
    void DoLocalScheduleForRegion(CDGRegion &region);
    using BaseSchedule::DoLocalSchedule;
    void DoLocalSchedule(CDGNode &cdgNode);

protected:
    void InitInCDGNode(CDGNode &cdgNode);
    virtual void FinishScheduling(CDGNode &cdgNode) = 0;

    DataDepAnalysis &intraDDA;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgLocalSchedule, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}  // namespace maplebe

#endif  // MAPLEBE_INCLUDE_CG_LOCAL_SCHEDULE_H
