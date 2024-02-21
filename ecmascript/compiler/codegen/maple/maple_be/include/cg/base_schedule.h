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
#ifndef MAPLEBE_INCLUDE_CG_BASE_SCHEDULE_H
#define MAPLEBE_INCLUDE_CG_BASE_SCHEDULE_H

#include "cgfunc.h"
#include "control_dep_analysis.h"
#include "data_dep_analysis.h"
#include "list_scheduler.h"

namespace maplebe {
class BaseSchedule {
public:
    BaseSchedule(MemPool &mp, CGFunc &f, ControlDepAnalysis &cdAna, bool doDelay = false)
        : schedMP(mp), schedAlloc(&mp), cgFunc(f), cda(cdAna), doDelayHeu(doDelay)
    {
    }
    virtual ~BaseSchedule()
    {
        listScheduler = nullptr;
    }

    virtual void Run() = 0;
    void InitInRegion(CDGRegion &region) const;
    void DoLocalSchedule(CDGRegion &region);
    bool DoDelayHeu() const
    {
        return doDelayHeu;
    }
    void SetDelayHeu()
    {
        doDelayHeu = true;
    }
    void SetUnitTest(bool flag)
    {
        isUnitTest = flag;
    }

protected:
    void InitInsnIdAndLocInsn();
    // Using total number of machine instructions to control the end of the scheduling process
    void InitMachineInsnNum(CDGNode &cdgNode) const;
    uint32 CaculateOriginalCyclesOfBB(CDGNode &cdgNode) const;
    void DumpRegionInfoBeforeSchedule(CDGRegion &region) const;
    void DumpCDGNodeInfoBeforeSchedule(CDGNode &cdgNode) const;
    void DumpCDGNodeInfoAfterSchedule(CDGNode &cdgNode) const;
    void DumpInsnInfoByScheduledOrder(CDGNode &cdgNode) const;

    MemPool &schedMP;
    MapleAllocator schedAlloc;
    CGFunc &cgFunc;
    ControlDepAnalysis &cda;
    CommonScheduleInfo *commonSchedInfo = nullptr;
    ListScheduler *listScheduler = nullptr;
    bool doDelayHeu = false;
    bool isUnitTest = false;
};
}  // namespace maplebe

#endif  // MAPLEBE_INCLUDE_CG_BASE_SCHEDULE_H
