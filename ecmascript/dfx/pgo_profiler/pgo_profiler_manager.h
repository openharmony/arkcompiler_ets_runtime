/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DFX_PGO_PROFILER_MANAGER_H
#define ECMASCRIPT_DFX_PGO_PROFILER_MANAGER_H

#include <unordered_map>

#include "ecmascript/ecma_macros.h"
#include "ecmascript/jspandafile/method_literal.h"

namespace panda::ecmascript {
/*
 * Support statistics of JS Function call heat. Save the method ID whose calls are less than MIN_COUNT.
 *
 * The saving format is as follows:
 * "recordName1:[methodId/count/name,methodId/count/name......]"
 * "recordName2:[methodId/count/name,methodId/count/name,methodId/count/name......]"
 *                                 "......"
 * */
class MethodProfilerInfo {
public:
    MethodProfilerInfo(uint32_t count, const std::string &methodName) : count_(count), methodName_(methodName) {}

    void IncreaseCount()
    {
        count_++;
    }

    void AddCount(uint32_t count)
    {
        count_ += count;
    }

    uint32_t GetCount() const
    {
        return count_;
    }
    const std::string &GetMethodName() const
    {
        return methodName_;
    }

    NO_COPY_SEMANTIC(MethodProfilerInfo);
    NO_MOVE_SEMANTIC(MethodProfilerInfo);

private:
    uint32_t count_ {0};
    std::string methodName_;
};

class PGOProfiler {
public:
    NO_COPY_SEMANTIC(PGOProfiler);
    NO_MOVE_SEMANTIC(PGOProfiler);

    void Sample(JSTaggedType value);
private:
    PGOProfiler(bool isEnable) : isEnable_(isEnable) {};
    virtual ~PGOProfiler() = default;

    bool isEnable_ {false};
    std::unordered_map<CString, std::unordered_map<EntityId, MethodProfilerInfo *>> profilerMap_;
    friend class PGOProfilerManager;
};

class PGOProfilerManager {
public:
    static PGOProfilerManager *GetInstance()
    {
        static PGOProfilerManager instance;
        return &instance;
    }

    PGOProfilerManager() = default;

    NO_COPY_SEMANTIC(PGOProfilerManager);
    NO_MOVE_SEMANTIC(PGOProfilerManager);

    void Initialize(uint32_t hotnessThreshold, const std::string &outDir);

    void Destroy()
    {
        if (!isEnable_) {
            return;
        }
        SaveProfiler();
    }

    PGOProfiler *Build(bool isEnable)
    {
        if (isEnable) {
            isEnable_ = true;
        }
        return new PGOProfiler(isEnable);
    }

    void Destroy(PGOProfiler *profiler)
    {
        if (profiler != nullptr) {
            Merge(profiler);
            delete profiler;
        }
    }

private:
    void Merge(PGOProfiler *profile);
    void SaveProfiler();
    std::string ProcessProfile();

    os::memory::Mutex mutex_;
    bool isEnable_ {false};
    uint32_t hotnessThreshold_ {2};
    std::string outDir_;
    std::unordered_map<CString, std::unordered_map<EntityId, MethodProfilerInfo *>> globalProfilerMap_;
};

} // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_PGO_PROFILER_MANAGER_H
