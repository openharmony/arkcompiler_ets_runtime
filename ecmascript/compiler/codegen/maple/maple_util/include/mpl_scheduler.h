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

#ifndef MAPLE_UTIL_INCLUDE_MPLSCHEDULER_H
#define MAPLE_UTIL_INCLUDE_MPLSCHEDULER_H

#include <vector>
#include <set>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <inttypes.h>
#include <pthread.h>
#include <inttypes.h>
#include <iostream>
#include <string>
#include "types_def.h"

namespace maple {
class MplTaskParam {
public:
    MplTaskParam() = default;
    virtual ~MplTaskParam() = default;
};

class MplTask {
public:
    MplTask() : taskId(0) {}

    virtual ~MplTask() = default;

    void SetTaskId(uint32 id)
    {
        taskId = id;
    }

    uint32 GetTaskId() const
    {
        return taskId;
    }

    int Run(MplTaskParam *param = nullptr)
    {
        return RunImpl(param);
    }

    int Finish(MplTaskParam *param = nullptr)
    {
        return FinishImpl(param);
    }

protected:
    virtual int RunImpl(MplTaskParam *)
    {
        return 0;
    }

    virtual int FinishImpl(MplTaskParam *)
    {
        return 0;
    }

    uint32 taskId;
};

class MplSchedulerParam {
public:
    MplSchedulerParam() = default;
    ~MplSchedulerParam() = default;
};

class MplScheduler {
public:
    explicit MplScheduler(const std::string &name)
        : schedulerName(name),
          taskIdForAdd(0),
          taskIdToRun(0),
          taskIdExpected(0),
          numberTasks(0),
          numberTasksFinish(0),
          isSchedulerSeq(false),
          dumpTime(false),
          statusFinish(kThreadRun)
{
}
    virtual ~MplScheduler() = default;

    virtual MplSchedulerParam *EncodeThreadMainEnvironment(uint32)
    {
        return nullptr;
    }

    virtual void DecodeThreadMainEnvironment(MplSchedulerParam *) {}

    virtual MplSchedulerParam *EncodeThreadFinishEnvironment()
    {
        return nullptr;
    }

    virtual void DecodeThreadFinishEnvironment(MplSchedulerParam *) {}

    void GlobalLock()
    {
        pthread_mutex_lock(&mutexGlobal);
    }

    void GlobalUnlock()
    {
        pthread_mutex_unlock(&mutexGlobal);
    }

protected:
    std::string schedulerName;
    std::vector<MplTask *> tbTasks;
    std::set<uint32> tbTaskIdsToFinish;
    uint32 taskIdForAdd;
    uint32 taskIdToRun;
    uint32 taskIdExpected;
    uint32 numberTasks;
    uint32 numberTasksFinish;
    pthread_mutex_t mutexTaskIdsToRun;
    pthread_mutex_t mutexTaskIdsToFinish;
    pthread_mutex_t mutexTaskFinishProcess;
    pthread_mutex_t mutexGlobal;
    pthread_cond_t conditionFinishProcess;
    bool isSchedulerSeq;
    bool dumpTime;

    enum ThreadStatus { kThreadStop, kThreadRun, kThreadPause };

    ThreadStatus statusFinish;
    void ThreadFinishNoSequence(MplSchedulerParam *env);
    void ThreadFinishSequence(MplSchedulerParam *env);
    // Callback Function
    virtual void CallbackThreadMainStart() {}

    virtual void CallbackThreadMainEnd() {}

    virtual void CallbackThreadFinishStart() {}

    virtual void CallbackThreadFinishEnd() {}

    virtual MplTaskParam *CallbackGetTaskRunParam() const
    {
        return nullptr;
    }

    virtual MplTaskParam *CallbackGetTaskFinishParam() const
    {
        return nullptr;
    }
};
}  // namespace maple
#endif  // MAPLE_UTIL_INCLUDE_MPLSCHEDULER_H
