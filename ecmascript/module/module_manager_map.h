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

#include <memory>
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
#if ENABLE_LATEST_OPTIMIZATION
template <class Key, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
class ModuleManagerMap {
private:
    // Use 2 bits for three states
    // State bits are the highest 2 bits, hash value is in the lower 62 bits
    static constexpr size_t STATE_BITS = 2;
    static constexpr size_t HASH_BITS = sizeof(size_t) * 8 - STATE_BITS;
    static constexpr size_t HASH_MASK = (static_cast<size_t>(1) << HASH_BITS) - 1;
    static constexpr size_t STATE_MASK = ~HASH_MASK;

    // State definitions
    static constexpr size_t EMPTY_STATE = 0ull << HASH_BITS;    // 00
    static constexpr size_t DELETED_STATE = 1ull << HASH_BITS;  // 01
    static constexpr size_t OCCUPIED_STATE = 2ull << HASH_BITS; // 10

    struct Entry {
        size_t hashState;  // Highest 2 bits for state, lower 62 bits for hash
        Key key;
        GCRoot value;

        Entry() : hashState(EMPTY_STATE) {}

        // Move constructor
        Entry(Entry&& other) noexcept
            : hashState(other.hashState),
              key(std::move(other.key)),
              value(std::move(other.value))
        {
            other.hashState = EMPTY_STATE;
        }

        // Move assignment operator
        Entry& operator=(Entry&& other) noexcept
        {
            if (this != &other) {
                hashState = other.hashState;
                key = std::move(other.key);
                value = std::move(other.value);
                other.hashState = EMPTY_STATE;
            }
            return *this;
        }

        // Disable copy
        Entry(const Entry&) = delete;
        Entry& operator=(const Entry&) = delete;

        // State checks
        bool IsEmpty() const { return (hashState & STATE_MASK) == EMPTY_STATE; }
        bool IsDeleted() const { return (hashState & STATE_MASK) == DELETED_STATE; }
        bool IsOccupied() const { return (hashState & STATE_MASK) == OCCUPIED_STATE; }

        // Get hash value (without state bits)
        size_t GetHash() const { return hashState & HASH_MASK; }

        // Set states
        void SetEmpty() { hashState = EMPTY_STATE; }
        void SetDeleted() { hashState = (hashState & HASH_MASK) | DELETED_STATE; }
        void SetOccupied(size_t hash) { hashState = (hash & HASH_MASK) | OCCUPIED_STATE; }

        // Fast comparison: first compare hash, then key
        template <typename K>
        bool FastEqual(const K& otherKey, size_t otherHash, const KeyEqual& equal) const
        {
            if (GetHash() != otherHash) {
                return false;
            }
            return equal(key, otherKey);
        }
    };

    // Hash table parameters
    static constexpr size_t INITIAL_CAPACITY = 16;
    static constexpr size_t MIN_CAPACITY = 16;
    static constexpr size_t GROW_FACTOR = 2;
    static constexpr double MAX_LOAD_FACTOR = 0.75;
    static constexpr double MIN_LOAD_FACTOR = 0.125;
    static constexpr double DELETION_THRESHOLD = 0.3;
    static constexpr size_t EMPTY_MARK = static_cast<size_t>(-1);

    mutable std::mutex lock_;
    std::unique_ptr<Entry[]> table_;
    size_t capacity_;
    size_t size_;
    size_t deletedCount_;
    Hash hasher_;
    KeyEqual keyEqual_;

    // Find position of key - must skip deleted states
    template <typename K>
    size_t FindPosition(const K& key) const
    {
        if (UNLIKELY(size_ == 0)) {
            return EMPTY_MARK;
        }
        size_t hash = hasher_(key) & HASH_MASK;
        size_t index = hash % capacity_;
        size_t startIndex = index;
        do {
            Entry& entry = table_[index];
            if (entry.IsOccupied() && entry.FastEqual(key, hash, keyEqual_)) {
                return index;
            }
            // Empty state means key doesn't exist
            if (entry.IsEmpty()) {
                break;
            }
            // Deleted state, continue probing
            index = (index + 1) % capacity_;
        } while (index != startIndex);
        return EMPTY_MARK;
    }

    // Find insertion position - can insert into deleted positions
    template <typename K>
    size_t FindInsertPosition(const K& key, size_t hash, bool& found) const
    {
        found = false;
        size_t index = hash % capacity_;
        size_t startIndex = index;
        size_t firstDeleted = EMPTY_MARK;
        do {
            Entry& entry = table_[index];
            if (entry.IsOccupied()) {
                if (entry.FastEqual(key, hash, keyEqual_)) {
                    found = true;
                    return index;
                }
            } else if (entry.IsDeleted()) {
                // Record first deleted position
                if (firstDeleted == EMPTY_MARK) {
                    firstDeleted = index;
                }
            } else if (entry.IsEmpty()) {
                // Prefer deleted position, otherwise use empty position
                return (firstDeleted != EMPTY_MARK) ? firstDeleted : index;
            }
            index = (index + 1) % capacity_;
        } while (index != startIndex);
        // Table full (shouldn't happen due to resizing)
        return EMPTY_MARK;
    }

    // Rehash to new capacity
    void Rehash(size_t newCapacity)
    {
        // Round up to power of two
        size_t roundedCapacity = 1;
        while (roundedCapacity < newCapacity) {
            roundedCapacity <<= 1;
        }
        newCapacity = roundedCapacity;
        // Ensure minimum capacity
        if (newCapacity < MIN_CAPACITY) {
            newCapacity = MIN_CAPACITY;
        }
        auto newTable = std::make_unique<Entry[]>(newCapacity);
        // Reinsert occupied elements (skip deleted states)
        for (size_t i = 0; i < capacity_; ++i) {
            Entry& oldEntry = table_[i];
            if (oldEntry.IsOccupied()) {
                size_t hash = oldEntry.GetHash();
                ASSERT(newCapacity != 0);
                size_t newIndex = hash % newCapacity;
                while (newTable[newIndex].IsOccupied()) {
                    newIndex = (newIndex + 1) % newCapacity;
                }
                newTable[newIndex] = std::move(oldEntry);
            }
        }
        table_ = std::move(newTable);
        capacity_ = newCapacity;
        deletedCount_ = 0;
    }

    // Check if we should shrink the table
    bool ShouldShrink() const
    {
        // Don't shrink below minimum capacity
        if (capacity_ <= MIN_CAPACITY * GROW_FACTOR) {
            return false;
        }
        double loadFactor = static_cast<double>(size_) / capacity_;
        return loadFactor < MIN_LOAD_FACTOR;
    }

    // Check if we need to rehash due to too many deletions
    bool TooManyDeletions() const
    {
        if (capacity_ == 0) return false;
        double deletionRatio = static_cast<double>(deletedCount_) / capacity_;
        return deletionRatio > DELETION_THRESHOLD;
    }

public:
    ModuleManagerMap()
        : table_(std::make_unique<Entry[]>(INITIAL_CAPACITY)),
          capacity_(INITIAL_CAPACITY),
          size_(0),
          deletedCount_(0),
          hasher_(),
          keyEqual_() {}

    explicit ModuleManagerMap(const Hash& hasher, const KeyEqual& keyEqual = KeyEqual())
        : table_(std::make_unique<Entry[]>(INITIAL_CAPACITY)),
          capacity_(INITIAL_CAPACITY),
          size_(0),
          deletedCount_(0),
          hasher_(hasher),
          keyEqual_(keyEqual) {}

    ~ModuleManagerMap() = default;

    ModuleManagerMap(const ModuleManagerMap&) = delete;
    ModuleManagerMap& operator=(const ModuleManagerMap&) = delete;

    ModuleManagerMap(ModuleManagerMap&& other) noexcept
        : lock_(),
          table_(std::move(other.table_)),
          capacity_(other.capacity_),
          size_(other.size_),
          deletedCount_(other.deletedCount_),
          hasher_(std::move(other.hasher_)),
          keyEqual_(std::move(other.keyEqual_))
    {
        other.capacity_ = INITIAL_CAPACITY;
        other.size_ = 0;
        other.deletedCount_ = 0;
        other.table_ = std::make_unique<Entry[]>(INITIAL_CAPACITY);
    }

    ModuleManagerMap& operator=(ModuleManagerMap&& other) noexcept
    {
        if (this != &other) {
            std::lock_guard<std::mutex> lock(lock_);
            table_ = std::move(other.table_);
            capacity_ = other.capacity_;
            size_ = other.size_;
            deletedCount_ = other.deletedCount_;
            hasher_ = std::move(other.hasher_);
            keyEqual_ = std::move(other.keyEqual_);

            other.capacity_ = INITIAL_CAPACITY;
            other.size_ = 0;
            other.deletedCount_ = 0;
            other.table_ = std::make_unique<Entry[]>(INITIAL_CAPACITY);
        }
        return *this;
    }

    /**
      * Inserts or updates a key-value pair.
      * If the key already exists, replaces the existing value.
      */
    void Insert(const Key &key, JSTaggedValue value)
    {
        std::lock_guard<std::mutex> lock(lock_);
        // Check if expansion is needed
        if ((size_ + 1) > capacity_ * MAX_LOAD_FACTOR) {
            Rehash(capacity_ * GROW_FACTOR);
        }
        size_t hash = hasher_(key) & HASH_MASK;
        bool found = false;
        size_t pos = FindInsertPosition(key, hash, found);
        if (pos != EMPTY_MARK) {
            Entry& entry = table_[pos];
            if (found) {
                entry.value = GCRoot(value);
            } else {
                // If inserting into deleted position, decrease deletion count
                if (entry.IsDeleted()) {
                    deletedCount_--;
                }
                entry.SetOccupied(hash);
                entry.key = key;
                entry.value = GCRoot(value);
                size_++;
            }
        }
    }

    /**
      * Inserts a key-value pair only if the key doesn't exist.
      * If the key already exists, the operation is a no-op.
      * The try_emplace ensure a GCRoot constructor is only trigger
      * if the insertion actually happen.
      *
      * Note: Returns bool instead of <iterator,bool> for thread safety.
      * Exposing iterators would be unsafe as they become invalid
      * once the lock is released.
      */
    template <typename K>
    bool Emplace(const K &key, JSTaggedValue value)
    {
        std::lock_guard<std::mutex> lock(lock_);
        if ((size_ + 1) > capacity_ * MAX_LOAD_FACTOR) {
            Rehash(capacity_ * GROW_FACTOR);
        }
        size_t hash = hasher_(key) & HASH_MASK;
        bool found = false;
        size_t pos = FindInsertPosition(key, hash, found);
        if (found || pos == EMPTY_MARK) {
            return false;
        }
        Entry& entry = table_[pos];
        if (entry.IsDeleted()) {
            deletedCount_--;
        }
        entry.SetOccupied(hash);
        entry.key = key;
        entry.value = GCRoot(value);
        size_++;
        return true;
    }

    /**
      * Safely retrieves a value by key with readbarrier.
      *
      * The returned value is obtained via GCRoot::Read(), ensuring proper
      * read barriers are applied for concurrent GC safety. The value is
      * returned by copy to avoid dangling references after lock release.
      */
    template <typename K>
    std::optional<JSTaggedValue> Find(const K& key)
    {
        std::lock_guard<std::mutex> lock(lock_);
        size_t pos = FindPosition(key);
        if (pos != EMPTY_MARK) {
            return std::make_optional(table_[pos].value.Read());
        }
        return std::nullopt;
    }

    /**
     * Applies a function to each key-value pair.
     */
    template <typename Func>
    void ForEach(Func &&fn)
    {
        std::lock_guard<std::mutex> lock(lock_);
        for (size_t i = 0; i < capacity_; ++i) {
            Entry& entry = table_[i];
            if (entry.IsOccupied()) {
                fn(entry.key, entry.value);
            }
        }
    }

    /**
     * Applies a function to each value.
     */
    template <typename Func>
    void ForEachValue(Func &&fn)
    {
        std::lock_guard<std::mutex> lock(lock_);
        for (size_t i = 0; i < capacity_; ++i) {
            Entry& entry = table_[i];
            if (entry.IsOccupied()) {
                fn(entry.value);
            }
        }
    }

    /**
     * Removes a key-value pair from the map.
     */
    void Erase(const Key &key)
    {
        std::lock_guard<std::mutex> lock(lock_);
        size_t pos = FindPosition(key);
        if (pos != EMPTY_MARK) {
            Entry& entry = table_[pos];
            entry.SetDeleted();
            entry.key = Key();
            entry.value = GCRoot();
            size_--;
            deletedCount_++;
            
            // Check if we should shrink
            if (ShouldShrink()) {
                Rehash(capacity_ / GROW_FACTOR); // Check if need to clean up deletions
            } else if (TooManyDeletions()) {
                Rehash(capacity_);
            }
        }
    }

    /**
     * Returns the number of occupied entries.
     */
    size_t Size() const
    {
        std::lock_guard<std::mutex> lock(lock_);
        return size_;
    }

    /**
     * Clears all entries from the map and shrinks to initial capacity.
     */
    void Clear()
    {
        std::lock_guard<std::mutex> lock(lock_);
        // Always shrink to initial capacity when clearing
        if (capacity_ != INITIAL_CAPACITY) {
            table_ = std::make_unique<Entry[]>(INITIAL_CAPACITY);
            capacity_ = INITIAL_CAPACITY;
        } else {
            // If already at initial capacity, just clear entries
            for (size_t i = 0; i < capacity_; ++i) {
                table_[i].SetEmpty();
            }
        }
        size_ = 0;
        deletedCount_ = 0;
    }
};
#else
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
#endif // ENABLE_LATEST_OPTIMIZATION
}  // namespace panda::ecmascript
#endif
