/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DFX_CPU_PROFILER_SAMPLING_PROCESSOR_H
#define ECMASCRIPT_DFX_CPU_PROFILER_SAMPLING_PROCESSOR_H

#include <csignal>
#include <cstdint>

#include "ecmascript/taskpool/task.h"

#include "libpandabase/macros.h"

namespace panda::ecmascript {
class SamplesRecord;
class JSThread;
class EcmaVM;
class SamplingProcessor : public Task {
public:
    static uint64_t GetMicrosecondsTimeStamp();

    SamplingProcessor(int32_t id, SamplesRecord *generator, int interval);
    virtual ~SamplingProcessor();

    bool Run(uint32_t threadIndex) override;

    NO_COPY_SEMANTIC(SamplingProcessor);
    NO_MOVE_SEMANTIC(SamplingProcessor);
private:
    SamplesRecord *generator_ = nullptr;
    uint32_t interval_ = 0;
    pthread_t pid_ = 0;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_DFX_CPU_PROFILER_SAMPLING_PROCESSOR_H
