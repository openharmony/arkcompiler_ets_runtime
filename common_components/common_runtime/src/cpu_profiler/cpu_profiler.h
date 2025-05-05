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
#ifndef ARK_COMMON_CPU_PROFILER_H
#define ARK_COMMON_CPU_PROFILER_H

#include <thread>

namespace panda {
class CpuProfiler {
public:
    static CpuProfiler& GetInstance()
    {
        static CpuProfiler instance;
        return instance;
    }
    bool StartCpuProfilerForFile();
    bool StopCpuProfilerForFile(const int fd);
    void TryStopSampling();

private:
    CpuProfiler() {}
    ~CpuProfiler();
    static void DoSampleStack();
    std::thread tid_;
};
} // namespace panda
#endif
