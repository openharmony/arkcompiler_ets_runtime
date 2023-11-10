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

#ifndef MAPLE_UTIL_INCLUDE_THREAD_STATUS_H
#define MAPLE_UTIL_INCLUDE_THREAD_STATUS_H
#include <thread>
#include <mutex>
#include <unordered_map>

namespace maple {
class ThreadEnv {
public:
    static bool IsMeParallel()
    {
        return threadEnv.isMeParallel;
    }

    static void SetMeParallel(bool status)
    {
        threadEnv.isMeParallel = status;
    }

    static size_t GetThreadIndex(const std::thread::id &tid)
    {
        static std::mutex mtx;
        std::lock_guard<std::mutex> guard(mtx);
        auto it = threadEnv.threadIndexMap.find(tid);
        if (it != threadEnv.threadIndexMap.end()) {
            return it->second;
        }
        return 0;  // the index of main thread is 0
    }

    static void InitThreadIndex(const std::thread::id &tid)
    {
        static std::mutex mtx;
        std::lock_guard<std::mutex> guard(mtx);
        size_t currThreadCnt = threadEnv.threadIndexMap.size();
        threadEnv.threadIndexMap[tid] = (currThreadCnt + 1);
    }

private:
    static ThreadEnv threadEnv;
    bool isMeParallel = false;  // whether me is under multithreading (optimize separate functions in parallel)
    // thread index begins from 1. 0 is reserved for main thread, which is not in the map
    std::unordered_map<std::thread::id, size_t> threadIndexMap;
};

class ParallelGuard {
public:
    explicit ParallelGuard(std::mutex &mtxInput, bool cond = true) : mtx(mtxInput), condition(cond)
    {
        if (condition) {
            mtx.lock();
        }
    }

    ParallelGuard(const ParallelGuard &) = delete;
    ParallelGuard &operator=(const ParallelGuard &) = delete;

    ~ParallelGuard()
    {
        if (condition) {
            mtx.unlock();
        }
    }

private:
    std::mutex &mtx;
    const bool condition;
};
}  // namespace maple
#endif  // MAPLE_UTIL_INCLUDE_THREAD_STATUS_H
