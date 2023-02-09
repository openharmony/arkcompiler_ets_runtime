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

#include <memory>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/pgo_profiler/pgo_profiler_info.h"

namespace panda::ecmascript {
class PGOProfiler {
public:
    NO_COPY_SEMANTIC(PGOProfiler);
    NO_MOVE_SEMANTIC(PGOProfiler);

    void Sample(JSTaggedType value, SampleMode mode = SampleMode::CALL_MODE);
private:
    static constexpr uint32_t MERGED_EVERY_COUNT = 10;

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
    std::unique_ptr<PGORecordDetailInfos> recordInfos_;
    friend class PGOProfilerManager;
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_PGO_PROFILER_H
