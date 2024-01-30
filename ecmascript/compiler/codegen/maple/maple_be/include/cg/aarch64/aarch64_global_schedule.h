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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_GLOBAL_SCHEDULE_H
#define MAPLEBE_INCLUDE_CG_AARCH64_GLOBAL_SCHEDULE_H

#include "global_schedule.h"

namespace maplebe {
class AArch64GlobalSchedule : public GlobalSchedule {
public:
    AArch64GlobalSchedule(MemPool &mp, CGFunc &f, ControlDepAnalysis &cdAna, DataDepAnalysis &dda)
        : GlobalSchedule(mp, f, cdAna, dda)
    {
    }
    ~AArch64GlobalSchedule() override = default;

    /* Verify global scheduling */
    void VerifyingSchedule(CDGRegion &region) override;

protected:
    void FinishScheduling(CDGNode &cdgNode) override;
};
} /* namespace maplebe */

#endif  // MAPLEBE_INCLUDE_CG_AARCH64_GLOBAL_SCHEDULE_H
