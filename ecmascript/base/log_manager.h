/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_BASE_LOG_MANAGER_H
#define ECMASCRIPT_BASE_LOG_MANAGER_H

#include <cstdint>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>

namespace panda::ecmascript::base {

class LogManager {
public:
    LogManager() = default;
    ~LogManager() = default;
    LogManager(const LogManager &) = delete;
    LogManager& operator=(const LogManager &) = delete;
    LogManager(LogManager &&) = delete;
    LogManager& operator=(LogManager &&) = delete;

    // LRU rate-limiting: returns true if this key should be printed
    // (first occurrence, or count reaches multiples of printInterval)
    bool ShouldPrintByRateLimit(uint64_t hashKey);

    // Hash utility: computes std::hash<std::string> on the input
    static uint64_t ComputeHashKey(const std::string& keyStr);

    // Configure LRU parameters (call before first use)
    void SetRateLimitParams(size_t maxEntries, uint64_t printInterval);

    // Clear LRU cache
    void ClearRateLimitCache();

private:
    struct RateLimitEntry {
        uint64_t hashKey;
        uint64_t count;
    };
    std::list<RateLimitEntry> rateLimitLruList_;
    std::unordered_map<uint64_t, std::list<RateLimitEntry>::iterator> rateLimitLruMap_;
    std::mutex rateLimitMutex_;
    size_t maxLruEntries_ = 30;
    uint64_t printInterval_ = 1000;
};

}  // namespace panda::ecmascript::base

#endif  // ECMASCRIPT_BASE_LOG_MANAGER_H
