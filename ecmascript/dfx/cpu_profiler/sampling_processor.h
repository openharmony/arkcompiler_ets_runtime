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
#include <pthread.h>

#include "libpandabase/macros.h"

namespace panda::ecmascript {
class SamplesRecord;
class JSThread;
class EcmaVM;
class SamplingProcessor {
public:
    static void AddSample(SamplesRecord *generator);
    static uint64_t GetMicrosecondsTimeStamp();
    static void *PostSemAndLogEnd(SamplesRecord *generator, pthread_t tid);
    virtual ~SamplingProcessor();

    static void *Run(void *arg);

    NO_COPY_SEMANTIC(SamplingProcessor);
    NO_MOVE_SEMANTIC(SamplingProcessor);
};

struct RunParams {
    SamplesRecord *generator_;
    uint32_t interval_;
    pthread_t tid_;
    void *taskHandle_;

    RunParams(SamplesRecord *generator, uint32_t interval, pthread_t tid, void *taskHandle)
        :generator_(generator), interval_(interval), tid_(tid), taskHandle_(taskHandle) {};
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_DFX_CPU_PROFILER_SAMPLING_PROCESSOR_H
