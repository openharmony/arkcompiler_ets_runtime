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

#ifndef MAPLE_PHASE_INCLUDE_PHASEDRIVER_H
#define MAPLE_PHASE_INCLUDE_PHASEDRIVER_H
#include "mir_module.h"
#include "mir_function.h"
#include "mpl_scheduler.h"

namespace maple {
class PhaseDriverImpl : public MplTaskParam {
public:
    PhaseDriverImpl() = default;
    virtual ~PhaseDriverImpl() = default;

    virtual void GlobalInit() {}

    virtual void LocalInit() {}

    virtual void ProcessRun(uint32, void *, void *) {}

    virtual void ProcessFinish(uint32, void *, void *) {}
};

class PhaseDriver : public MplScheduler {
public:
    class Task : public MplTask {
    public:
        Task(void *currTarget, void *currParamEx = nullptr) : target(currTarget), paramException(currParamEx) {}

        ~Task() = default;

    protected:
        int RunImpl(MplTaskParam *param) override
        {
            CHECK_NULL_FATAL(param);
            static_cast<PhaseDriverImpl *>(param)->ProcessRun(taskId, target, paramException);
            return 0;
        }

        int FinishImpl(MplTaskParam *param) override
        {
            CHECK_NULL_FATAL(param);
            static_cast<PhaseDriverImpl *>(param)->ProcessFinish(taskId, target, paramException);
            return 0;
        }
        void *target;
        void *paramException;
    };

    explicit PhaseDriver(const std::string &phaseName)
        : MplScheduler(phaseName), module(nullptr), phaseImpl(nullptr), phaseName(phaseName) {}
    virtual ~PhaseDriver() = default;

    virtual PhaseDriverImpl *NewPhase() = 0;
    virtual void RegisterTasks() = 0;

protected:
    virtual MplTaskParam *CallbackGetTaskRunParam() const
    {
        return phaseImplLocal;
    }

    virtual MplTaskParam *CallbackGetTaskFinishParam() const
    {
        return phaseImplLocal;
    }

    MIRModule *module;
    PhaseDriverImpl *phaseImpl;
    thread_local static PhaseDriverImpl *phaseImplLocal;
    std::string phaseName;
};
}  // namespace maple
#endif  // MAPLE_PHASE_INCLUDE_PHASEDRIVER_H
