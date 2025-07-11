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

#ifndef ECMASCRIPT_MODULE_MANAGER_MAP_H
#define ECMASCRIPT_MODULE_MANAGER_MAP_H

#include <optional>
#include "ecmascript/mem/gc_root.h"
#include "ecmascript/mem/c_containers.h"

namespace panda::ecmascript {
/**
 * A concurrent-safe hash map for use as a GC root container.
 * All stored values are wrapped in GCRoot with CMCGC.
 *
 * Safe for concurrent access between GC threads and mutator threads
 *
 * IMPORTANT: Do not attempt to cache iterators or references outside
 * of the provided interface - they can become invalid after the lock is released.
 */
template <class Key, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
class ModuleManagerMap {
    using UnderlyingMap = CUnorderedMap<Key, GCRoot, Hash, KeyEqual>;
    mutable std::mutex lock_;
    // Underlying storage with GCRoot values
    UnderlyingMap map_;

public:
    /**
     * Inserts or updates a key-value pair.
     * If the key already exists, replaces the existing value.
     */
    void Insert(const Key &key, JSTaggedValue value)
    {
        std::lock_guard<std::mutex> lock(lock_);
        map_[key] = GCRoot(value);
    }

    /**
     * Inserts a key-value pair only if the key doesn't exist.
     * If the key already exists, the operation is a no-op.
     * The try_emplace ensure a GCRoot constructor is only trigger
     * if the insertion actually happen
     *
     * Note: Returns bool instead of <iterator,bool> for thread safety.
     * Exposing iterators would be unsafe as they become invalid
     * once the lock is released.
     */
    template <typename K>
    bool Emplace(const K &key, JSTaggedValue value)
    {
        std::lock_guard<std::mutex> lock(lock_);
        return map_.try_emplace(key, value).second;
    }

    /**
     * Safely retrieves a value by key with readbarrier.
     *
     * The returned value is obtained via GCRoot::Read(), ensuring proper
     * read barriers are applied for concurrent GC safety. The value is
     * returned by copy to avoid dangling references after lock release.
     */
    template <typename K>
    std::optional<JSTaggedValue> Find(const K &key)
    {
        std::lock_guard<std::mutex> lock(lock_);
        auto it = map_.find(key);
        return it == map_.end() ? std::nullopt : std::make_optional(it->second.Read());
    }

    /**
     * Applies a function to each key-value pair while holding the lock.
     */
    template <typename Func>
    void ForEach(Func &&fn)
    {
        std::lock_guard<std::mutex> lock(lock_);
        for (auto it = map_.begin(); it != map_.end(); ++it) {
            fn(it);
        }
    }

    void Erase(const Key &key)
    {
        std::lock_guard<std::mutex> lock(lock_);
        map_.erase(key);
    }

    size_t Size() const
    {
        std::lock_guard<std::mutex> lock(lock_);
        return map_.size();
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(lock_);
        map_.clear();
    }
};

}  // namespace panda::ecmascript
#endif
