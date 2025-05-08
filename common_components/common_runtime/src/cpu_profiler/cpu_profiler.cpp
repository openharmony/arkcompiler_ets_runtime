/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "common_components/common_runtime/src/cpu_profiler/cpu_profiler.h"

#include "common_components/common_runtime/src/mutator/mutator_manager.h"

namespace panda {
CpuProfiler::~CpuProfiler()
{
    TryStopSampling();
}

bool CpuProfiler::StartCpuProfilerForFile()
{
    if (generator_.GetIsStart()) {
        LOG_COMMON(ERROR) << "Start CpuProfiler repeatedly.";
        return false;
    }
    generator_.SetIsStart(true);
    tid_ = std::thread(CpuProfiler::SamplingThread, std::ref(generator_));
    if (!tid_.joinable()) {
        LOG_COMMON(ERROR) << "Failed to create sampling thread.";
        return false;
    }
    return true;
}

bool CpuProfiler::StopCpuProfilerForFile(const int fd)
{
    if (!generator_.GetIsStart()) {
        LOG_COMMON(ERROR) << "CpuProfiler is not in profiling";
        return false;
    }
    bool ret = generator_.OpenFile(fd);
    if (!ret) {
        LOG_COMMON(ERROR) << "Open file failed";
    }
    TryStopSampling();
    return ret;
}

void CpuProfiler::TryStopSampling()
{
    if (!generator_.GetIsStart()) {
        return;
    }
    generator_.SetIsStart(false);
    if (tid_.joinable()) {
        tid_.join();
    }
}

void CpuProfiler::SamplingThread(SamplesRecord& generator)
{
    generator.InitProfileInfo();
    uint32_t interval = generator.GetSamplingInterval();
    uint64_t startTime = SamplesRecord::GetMicrosecondsTimeStamp();
    generator.SetThreadStartTime(startTime);
    uint64_t endTime = startTime;
    while (generator.GetIsStart()) {
        startTime = SamplesRecord::GetMicrosecondsTimeStamp();
        int64_t ts = static_cast<int64_t>(interval) - static_cast<int64_t>(startTime - endTime);
        endTime = startTime;
        if (ts > 0) {
            usleep(ts);
            endTime = SamplesRecord::GetMicrosecondsTimeStamp();
        }
        DoSampleStack();
        generator.ParseSampleData(endTime);
        generator.DoSingleTask(endTime);
    }
    generator.RunTaskLoop();
    generator.SetSampleStopTime(SamplesRecord::GetMicrosecondsTimeStamp());
    generator.DumpProfileInfo();
    generator.ReleaseProfileInfo();
}

void CpuProfiler::DoSampleStack()
{
    MutatorManager::Instance().TransitionAllMutatorsToCpuProfile();
}
}
