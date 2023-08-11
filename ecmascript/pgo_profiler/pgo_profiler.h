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

#ifndef ECMASCRIPT_PGO_PROFILER_H
#define ECMASCRIPT_PGO_PROFILER_H

#include <chrono>
#include <memory>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/pgo_profiler/pgo_profiler_info.h"

namespace panda::ecmascript {
class PGOProfiler {
public:
    NO_COPY_SEMANTIC(PGOProfiler);
    NO_MOVE_SEMANTIC(PGOProfiler);

    void ProfileCall(JSTaggedType func, JSTaggedType callTarget, int32_t pcOffset = -1,
                     SampleMode mode = SampleMode::CALL_MODE, int32_t incCount = 1);
    void ProfileOpType(JSTaggedType func, int32_t offset, uint32_t type);
    void ProfileCreateObject(JSTaggedType func, int32_t offset, JSTaggedType newObj, int32_t traceId);
    void ProfileDefineClass(JSThread *thread, JSTaggedType func, int32_t offset, JSTaggedType ctor);
    void ProfileObjLayout(JSThread *thread, JSTaggedType func, int32_t offset, JSTaggedType object, bool store);
    void ProfileObjIndex(JSThread *thread, JSTaggedType func, int32_t offset, JSTaggedType object);

    void SetSaveTimestamp(std::chrono::system_clock::time_point timestamp)
    {
        saveTimestamp_ = timestamp;
    }

    int32_t InsertLiteralId(JSTaggedType hclass, int32_t traceId);
    void ProcessReferences(const WeakRootVisitor &visitor);

private:
    static constexpr uint32_t MERGED_EVERY_COUNT = 20;
    static constexpr auto MERGED_MIN_INTERVAL = std::chrono::milliseconds(15);

    PGOProfiler([[maybe_unused]] EcmaVM *vm, bool isEnable) : isEnable_(isEnable)
    {
        if (isEnable_) {
            recordInfos_ = std::make_unique<PGORecordDetailInfos>(0);
        }
    };

    virtual ~PGOProfiler()
    {
        Reset(false);
    }

    void Reset(bool isEnable)
    {
        isEnable_ = isEnable;
        methodCount_ = 0;
        if (recordInfos_) {
            recordInfos_->Clear();
        } else {
            if (isEnable_) {
                recordInfos_ = std::make_unique<PGORecordDetailInfos>(0);
            }
        }
    }

    bool isEnable_ {false};
    uint32_t methodCount_ {0};
    std::chrono::system_clock::time_point saveTimestamp_;
    CMap<JSTaggedType, int32_t> traceIds_;
    std::unique_ptr<PGORecordDetailInfos> recordInfos_;
    friend class PGOProfilerManager;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_PGO_PROFILER_H
