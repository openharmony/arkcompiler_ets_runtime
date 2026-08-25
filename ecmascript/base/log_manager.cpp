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

#include "ecmascript/base/log_manager.h"

namespace panda::ecmascript::base {

bool LogManager::ShouldPrintByRateLimit(uint64_t hashKey)
{
    std::lock_guard<std::mutex> lock(rateLimitMutex_);
    auto it = rateLimitLruMap_.find(hashKey);
    if (it != rateLimitLruMap_.end()) {
        // Key exists: increment count and move to front (most recently used)
        it->second->count++;
        uint64_t count = it->second->count;
        rateLimitLruList_.splice(rateLimitLruList_.begin(), rateLimitLruList_, it->second);
        // Print only when count reaches multiples of printInterval
        if (count % printInterval_ != 0) {
            return false;
        }
        return true;
    }
    // New key: evict LRU if at capacity
    if (rateLimitLruList_.size() >= maxLruEntries_) {
        rateLimitLruMap_.erase(rateLimitLruList_.back().hashKey);
        rateLimitLruList_.pop_back();
    }
    rateLimitLruList_.push_front({hashKey, 1});
    rateLimitLruMap_[hashKey] = rateLimitLruList_.begin();
    // First occurrence: always print
    return true;
}

uint64_t LogManager::ComputeHashKey(const std::string& keyStr)
{
    return std::hash<std::string>{}(keyStr);
}

void LogManager::SetRateLimitParams(size_t maxEntries, uint64_t printInterval)
{
    maxLruEntries_ = maxEntries;
    printInterval_ = printInterval;
}

void LogManager::ClearRateLimitCache()
{
    std::lock_guard<std::mutex> lock(rateLimitMutex_);
    rateLimitLruList_.clear();
    rateLimitLruMap_.clear();
}

}  // namespace panda::ecmascript::base
