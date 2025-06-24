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

#ifndef COMMON_COMPONENTS_HEAP_HEURISTIC_GC_POLICY_H
#define COMMON_COMPONENTS_HEAP_HEURISTIC_GC_POLICY_H

#include "common_components/base/globals.h"
#include "common_components/taskpool/taskpool.h"
#include "common_components/log/log.h"

namespace common {
enum class StartupStatus: uint8_t {
    BEFORE_STARTUP,
    COLD_STARTUP,
    COLD_STARTUP_PARTIALLY_FINISH,
    COLD_STARTUP_FINISH,
};

class StartupStatusManager {
public:
    static std::atomic<StartupStatus> startupStatus_;
    static constexpr size_t STARTUP_DURATION_MS = 2000;
    static constexpr size_t STARTUP_FINISH_DURATION_MS = 8000;

    static void OnAppStartup();

    static void SetStartupStatus(StartupStatus status)
    {
        startupStatus_ = status;
    }

    static StartupStatus GetStartupStatus()
    {
        return startupStatus_.load(std::memory_order_relaxed);
    }

private:
    class StartupFinishTask : public common::Task {
    public:
        StartupFinishTask(uint32_t id) : Task(id) {}
        bool Run([[maybe_unused]] uint32_t threadIndex) override
        {
            StartupStatusManager::SetStartupStatus(StartupStatus::COLD_STARTUP_FINISH);
            return true;
        }
    };

    class StartupTask : public common::Task {
    public:
        StartupTask(uint32_t id, Taskpool *pool, size_t delay)
            : Task(id), threadPool_(pool), delay_(delay) {}
        bool Run([[maybe_unused]] uint32_t threadIndex) override
        {
            StartupStatusManager::SetStartupStatus(StartupStatus::COLD_STARTUP_PARTIALLY_FINISH);
            size_t startupFinishDelay = STARTUP_FINISH_DURATION_MS - delay_;
            threadPool_->PostDelayedTask(std::make_unique<StartupFinishTask>(0), startupFinishDelay);
            return true;
        }

    private:
        Taskpool *threadPool_ {nullptr};
        size_t delay_ {0};
    };
};

static constexpr size_t NATIVE_MULTIPLIER = 2;
// Use interval to avoid gc request too frequent.
static constexpr size_t NOTIFY_NATIVE_INTERVAL = 32;
#if defined(PANDA_TARGET_32)
    static constexpr size_t NATIVE_INIT_THRESHOLD = 100 * MB;
    static constexpr size_t MAX_GLOBAL_NATIVE_LIMIT = 512 * MB;
    static constexpr size_t MAX_NATIVE_STEP = 64 * MB;
    static constexpr size_t MAX_NATIVE_SIZE_INC = 256 * MB;
    static constexpr size_t NATIVE_IMMEDIATE_THRESHOLD = 300 * KB;
#else
    static constexpr size_t NATIVE_INIT_THRESHOLD = 200 * MB;
    static constexpr size_t MAX_GLOBAL_NATIVE_LIMIT = 2 * GB;
    static constexpr size_t MAX_NATIVE_STEP = 300 * MB;
    static constexpr size_t MAX_NATIVE_SIZE_INC = 1 * GB;
    static constexpr size_t NATIVE_IMMEDIATE_THRESHOLD = 2 * MB;
#endif
static constexpr size_t URGENCY_NATIVE_LIMIT = (MAX_NATIVE_SIZE_INC + MAX_GLOBAL_NATIVE_LIMIT) / 2;

class HeuristicGCPolicy {
public:
    static constexpr double COLD_STARTUP_GC_THRESHOLD_RATIO = 0.25;
    void Init();

    bool ShouldRestrainGCOnStartup();

    void TryHeuristicGC();

    void NotifyNativeAllocation(size_t bytes);

    void NotifyNativeFree(size_t bytes);

    void NotifyNativeReset(size_t oldBytes, size_t newBytes);

    size_t GetNotifiedNativeSize() const;

    void SetNativeHeapThreshold(size_t newThreshold);

    size_t GetNativeHeapThreshold() const;

private:
    void CheckGCForNative();
    uint64_t heapSize_ {0};

    std::atomic<size_t> notifiedNativeSize_ = 0;
    std::atomic<size_t> nativeHeapThreshold_ = NATIVE_INIT_THRESHOLD;
    std::atomic<size_t> nativeHeapObjects_ = 0;
};
} // namespace common

#endif // COMMON_COMPONENTS_HEAP_HEURISTIC_GC_POLICY_H
