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

#ifndef MAPLEBE_INCLUDE_CG_REG_ALLOC_H
#define MAPLEBE_INCLUDE_CG_REG_ALLOC_H

#include "cgfunc.h"
#include "maple_phase_manager.h"

namespace maplebe {
class RATimerManager {
public:
    RATimerManager(const RATimerManager&) = delete;
    RATimerManager& operator=(const RATimerManager&) = delete;

    static MPLTimerManager &GetInstance()
    {
        static RATimerManager raTimerM{};
        return raTimerM.timerM;
    }

    void PrintAllTimerAndClear(const std::string &funcName)
    {
        LogInfo::MapleLogger() << "Func[" << funcName << "] Reg Alloc Time:\n";
        LogInfo::MapleLogger() << timerM.ConvertAllTimer2Str() << std::endl;
        timerM.Clear();
    }
private:
    RATimerManager() = default;
    ~RATimerManager() = default;

    MPLTimerManager timerM;
};

// RA time statistics marco. If defined, RA time consumed will print.
#ifdef REG_ALLOC_TIME_STATISTICS
#define RA_TIMER_REGISTER(timerName, str) MPLTimerRegister timerName##Timer(RATimerManager::GetInstance(), str)
#define RA_TIMER_STOP(timerName) timerName##Timer.Stop()
#define RA_TIMER_PRINT(funcName) RATimerManager::GetInstance().PrintAllTimerAndClear(funcName)
#else
#define RA_TIMER_REGISTER(name, str)
#define RA_TIMER_STOP(name)
#define RA_TIMER_PRINT(funcName)
#endif

class RegAllocator {
public:
    RegAllocator(CGFunc &tempCGFunc, MemPool &memPool)
        : cgFunc(&tempCGFunc),
          memPool(&memPool),
          alloc(&memPool),
          regInfo(tempCGFunc.GetTargetRegInfo())
    {
        regInfo->Init();
    }

    virtual ~RegAllocator() = default;

    virtual bool AllocateRegisters() = 0;

    virtual std::string PhaseName() const
    {
        return "regalloc";
    }

    void SetNeedDump(bool dump)
    {
        needDump = dump;
    }
protected:
    CGFunc *cgFunc = nullptr;
    MemPool *memPool = nullptr;
    MapleAllocator alloc;
    RegisterInfo *regInfo = nullptr;
    bool needDump = false;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgRegAlloc, CGFunc)
OVERRIDE_DEPENDENCE
MAPLE_FUNC_PHASE_DECLARE_END
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_REG_ALLOC_H */
