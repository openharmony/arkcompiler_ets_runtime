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
#include <unistd.h>

#include "ecmascript/base/config.h"
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#include "ecmascript/dfx/cpu_profiler/samples_record.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/log_wrapper.h"

#include <sys/time.h>

namespace panda::ecmascript {
const int USEC_PER_SEC = 1000 * 1000;
const int NSEC_PER_USEC = 1000;
SamplingProcessor::SamplingProcessor(SamplesRecord *generator, int interval, bool outToFile)
{
    generator_ = generator;
    interval_ = interval;
    pid_ = pthread_self();
    outToFile_ = outToFile;
}
SamplingProcessor::~SamplingProcessor() {}

bool SamplingProcessor::Run([[maybe_unused]] uint32_t threadIndex)
{
    uint64_t startTime = GetMicrosecondsTimeStamp();
    uint64_t endTime = GetMicrosecondsTimeStamp();
    generator_->SetThreadStartTime(startTime);
    while (generator_->GetIsStart()) {
#if ECMASCRIPT_ENABLE_ACTIVE_CPUPROFILER
        JSThread *thread = generator_->GetAssociatedJSThread();
        SamplesRecord::staticGcState_ = thread->GetGcState();
        if (!SamplesRecord::staticGcState_) {
            thread->SetGetStackSignal(true);
            if (generator_->SemWait(0) != 0) {
                LOG_ECMA(ERROR) << "sem_[0] wait failed";
            }
        }
#else
        if (pthread_kill(pid_, SIGPROF) != 0) {
            LOG(ERROR, RUNTIME) << "pthread_kill signal failed";
            return false;
        }
        if (generator_->SemWait(0) != 0) {
            LOG_ECMA(ERROR) << "sem_[0] wait failed";
            return false;
        }
#endif
        startTime = GetMicrosecondsTimeStamp();
        int64_t ts = interval_ - static_cast<int64_t>(startTime - endTime);
        endTime = startTime;
        if (ts > 0) {
            usleep(ts);
            endTime = GetMicrosecondsTimeStamp();
        }
        if (generator_->GetMethodNodeCount() + generator_->GetframeStackLength() >= MAX_NODE_COUNT) {
            break;
        }
        generator_->AddSample(endTime, outToFile_);
        if (outToFile_) {
            if (generator_->GetMethodNodeCount() >= 10 || // 10:Number of nodes currently stored
                generator_->GetSamples().size() == 100) { // 100:Number of Samples currently stored
                generator_->WriteMethodsAndSampleInfo(false);
            }
        }
        if (outToFile_) {
            if (collectCount_ % 50 == 0) { // 50:The sampling times reached 50 times.
                WriteSampleDataToFile();
            }
            collectCount_++;
        }
        generator_->SetSampleFlag(false);
    }
    uint64_t stopTime = GetMicrosecondsTimeStamp();
    generator_->SetThreadStopTime(stopTime);
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

void SamplingProcessor::WriteSampleDataToFile()
{
    if (!generator_->fileHandle_.is_open() || generator_->GetSampleData().size() < 1_MB) { // 1M
        return;
    }
    if (firstWrite_) {
        generator_->fileHandle_ << generator_->GetSampleData();
        generator_->ClearSampleData();
        generator_->fileHandle_.close();
        generator_->fileHandle_.open(generator_->GetFileName().c_str(), std::ios::app);
        firstWrite_ = false;
        return;
    }
    generator_->fileHandle_ << generator_->GetSampleData();
    generator_->ClearSampleData();
}
} // namespace panda::ecmascript
