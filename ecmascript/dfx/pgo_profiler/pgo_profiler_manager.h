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
 * "recordName1:methodId1/methodCount,methodId2/methodCount,methodId3/methodCount......"
 * "recordName2:methodId1/methodCount,methodId2/methodCount,methodId3/methodCount......"
 *                                 "......"
 * */
class PGOProfiler {
public:
    PGOProfiler(bool isEnable) : isEnable_(isEnable) {};
    virtual ~PGOProfiler() = default;

    NO_COPY_SEMANTIC(PGOProfiler);
    NO_MOVE_SEMANTIC(PGOProfiler);

    void Sample(JSTaggedType value);
private:
    bool isEnable_ {false};
    std::unordered_map<CString, std::unordered_map<EntityId, uint32_t>> profilerMap_;
    friend class PGOProfilerManager;
};

class PGOProfilerManager {
public:
    static PGOProfilerManager *GetInstance()
    {
        static PGOProfilerManager instance;
        return &instance;
    }

    void Initialize(bool isEnable, const std::string &outDir);

    void Destroy()
    {
        if (!isEnable_) {
            return;
        }
        SaveProfiler();
    }

    PGOProfiler *Build(bool isEnable)
    {
        return new PGOProfiler(isEnable && isEnable_);
    }

    void Destroy(PGOProfiler *profiler)
    {
        if (profiler != nullptr) {
            Merge(profiler);
            delete profiler;
        }
    }

private:
    static constexpr uint32_t MIN_COUNT = 1;

    void Merge(PGOProfiler *profile);
    void SaveProfiler();
    std::string ProcessProfile();

    os::memory::Mutex mutex_;
    bool isEnable_ {false};
    std::string realOutPath_;
    std::unordered_map<CString, std::unordered_map<EntityId, uint32_t>> globalProfilerMap_;
};

} // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_PGO_PROFILER_MANAGER_H
