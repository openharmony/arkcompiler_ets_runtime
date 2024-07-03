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
SamplingProcessor::~SamplingProcessor() {}

void *SamplingProcessor::Run(void *arg)
{
    LOG_ECMA(INFO) << "SamplingProcessor::Run start";
    RunParams params = *reinterpret_cast<RunParams *>(arg);
    SamplesRecord *generator = params.generator_;
    uint32_t interval = params.interval_;
    pthread_t pid = params.tid_;
    pthread_t tid = pthread_self();
    pthread_setname_np(tid, "SamplingThread");
    uint64_t startTime = generator->GetThreadStartTime();
    uint64_t endTime = startTime;
    generator->AddStartTraceEvent();
    while (generator->GetIsStart()) {
        if (pthread_kill(pid, SIGPROF) != 0) {
            LOG(ERROR, RUNTIME) << "pthread_kill signal failed";
            return nullptr;
        }
        if (generator->SemWait(0) != 0) {
            LOG_ECMA(ERROR) << "sem_[0] wait failed";
            return nullptr;
        }
        startTime = GetMicrosecondsTimeStamp();
        int64_t ts = static_cast<int64_t>(interval) - static_cast<int64_t>(startTime - endTime);
        endTime = startTime;
        if (ts > 0) {
            usleep(ts);
            endTime = GetMicrosecondsTimeStamp();
        }
        if (generator->GetMethodNodeCount() + generator->GetframeStackLength() >= MAX_NODE_COUNT) {
            LOG_ECMA(ERROR) << "SamplingProcessor::Run, exceed MAX_NODE_COUNT";
            break;
        }
        if (generator->samplesQueue_->IsEmpty()) {
            uint64_t sampleTimeStamp = SamplingProcessor::GetMicrosecondsTimeStamp();
            generator->AddEmptyStackSample(sampleTimeStamp);
        } else {
            while (!generator->samplesQueue_->IsEmpty()) {
                FrameStackAndInfo *frame = generator->samplesQueue_->PopFrame();
                generator->AddSample(frame);
            }
        }
        generator->AddTraceEvent(false);
    }
    generator->SetThreadStopTime();
    generator->AddTraceEvent(true);
    return PostSemAndLogEnd(generator, tid);
}

void *SamplingProcessor::PostSemAndLogEnd(SamplesRecord *generator, pthread_t tid)
{
    pthread_setname_np(tid, "OS_GC_Thread");
    if (generator->SemPost(1) != 0) {
        LOG_ECMA(ERROR) << "sem_[1] post failed, errno = " << errno;
        return nullptr;
    }
    LOG_ECMA(INFO) << "SamplingProcessor::Run end";
    return nullptr;
}

uint64_t SamplingProcessor::GetMicrosecondsTimeStamp()
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time.tv_sec * USEC_PER_SEC + time.tv_nsec / NSEC_PER_USEC;
}
} // namespace panda::ecmascript
