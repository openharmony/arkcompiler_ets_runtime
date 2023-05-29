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

#include "ecmascript/dfx/cpu_profiler/sampling_processor.h"

#include <csignal>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#include "ecmascript/base/config.h"
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#include "ecmascript/dfx/cpu_profiler/samples_record.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript {
const int USEC_PER_SEC = 1000 * 1000;
const int NSEC_PER_USEC = 1000;
SamplingProcessor::SamplingProcessor(int32_t id, SamplesRecord *generator, int interval) : Task(id)
{
    generator_ = generator;
    interval_ = static_cast<uint32_t>(interval);
    pid_ = pthread_self();
}
SamplingProcessor::~SamplingProcessor() {}

bool SamplingProcessor::Run([[maybe_unused]] uint32_t threadIndex)
{
    pthread_t tid = pthread_self();
    pthread_setname_np(tid, "SamplingThread");
    uint64_t startTime = GetMicrosecondsTimeStamp();
    uint64_t endTime = startTime;
    generator_->SetThreadStartTime(startTime);
    while (generator_->GetIsStart()) {
        if (pthread_kill(pid_, SIGPROF) != 0) {
            LOG(ERROR, RUNTIME) << "pthread_kill signal failed";
            return false;
        }
        if (generator_->SemWait(0) != 0) {
            LOG_ECMA(ERROR) << "sem_[0] wait failed";
            return false;
        }
        startTime = GetMicrosecondsTimeStamp();
        int64_t ts = static_cast<int64_t>(interval_) - static_cast<int64_t>(startTime - endTime);
        endTime = startTime;
        if (ts > 0) {
            usleep(ts);
            endTime = GetMicrosecondsTimeStamp();
        }
        if (generator_->GetMethodNodeCount() + generator_->GetframeStackLength() >= MAX_NODE_COUNT) {
            break;
        }
        if (generator_->samplesQueue_->IsEmpty()) {
            uint64_t sampleTimeStamp = SamplingProcessor::GetMicrosecondsTimeStamp();
            generator_->AddEmptyStackSample(sampleTimeStamp);
        } else {
            while (!generator_->samplesQueue_->IsEmpty()) {
                FrameStackAndInfo *frame = generator_->samplesQueue_->PopFrame();
                generator_->AddSample(frame);
            }
        }
    }
    generator_->SetThreadStopTime();
    pthread_setname_np(tid, "GC_WorkerThread");
    if (generator_->SemPost(1) != 0) {
        LOG_ECMA(ERROR) << "sem_[1] post failed";
        return false;
    }
    return true;
}

uint64_t SamplingProcessor::GetMicrosecondsTimeStamp()
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time.tv_sec * USEC_PER_SEC + time.tv_nsec / NSEC_PER_USEC;
}
} // namespace panda::ecmascript
