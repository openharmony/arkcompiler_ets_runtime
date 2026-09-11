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

#ifndef ECMASCRIPT_JSPANDAFILE_METHOD_LITERAL_ID_MAP_H
#define ECMASCRIPT_JSPANDAFILE_METHOD_LITERAL_ID_MAP_H

#include "ecmascript/jspandafile/method_literal.h"
#include "common_components/base/config.h"

namespace panda::ecmascript {
/**
 * Open addressing hash table with linear probing for mapping uint32_t keys to MethodLiteral
 * pointers. The base of the owner's contiguous MethodLiteral array is injected through
 * Reserve(); the map does not own it.
 * This container is optimized for known maximum size and provides O(1) average case for
 * insertion and lookup.
 * Key features:
 * - Fixed capacity allocated upfront via Reserve(expectedSize, methodLiterals)
 * - No rehashing or resizing operations
 * - Linear probing for collision resolution
 * - Fast bitwise modulo operations (capacity is power of two)
 * - Iterator support for range-based for loops with structured binding
 * The storage strategy is selected by the single ENABLE_LATEST_OPTIMIZATION block over the
 * nested Entry definition in the private section; probing, capacity and iteration logic are
 * written once against Entry's uniform IsEmpty()/Set()/Value() interface:
 * - ENABLE_LATEST_OPTIMIZATION = 1: Entry stores a uint32_t index into the array bound by
 *   Reserve() (8 bytes per slot); pointers passed to Insert() must point into that array.
 * - ENABLE_LATEST_OPTIMIZATION = 0: Entry stores the MethodLiteral* directly and ignores the
 *   bound base.
 */
class MethodLiteralIDMap {
private:
    struct Entry;

public:
    MethodLiteralIDMap() : table_(nullptr), methodLiterals_(nullptr), capacity_(0), size_(0) {}

    ~MethodLiteralIDMap() { Clear(); }

    class Iterator {
    public:
        Iterator(Entry* table, MethodLiteral* methodLiterals, size_t capacity, size_t index = 0)
            : table_(table), methodLiterals_(methodLiterals), capacity_(capacity), current_index_(index)
        {
            while (current_index_ < capacity_ && table_[current_index_].IsEmpty()) {
                ++current_index_;
            }
        }

        std::pair<uint32_t, MethodLiteral*> operator*() const
        {
            ASSERT(table_ != nullptr);
            ASSERT(current_index_ < capacity_);
            return {table_[current_index_].key_, table_[current_index_].Value(methodLiterals_)};
        }

        Iterator& operator++()
        {
            ++current_index_;
            while (current_index_ < capacity_ && table_[current_index_].IsEmpty()) {
                ++current_index_;
            }
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const Iterator& other) const
        {
            return current_index_ == other.current_index_;
        }

        bool operator!=(const Iterator& other) const
        {
            return !(*this == other);
        }

    private:
        Entry* table_;
        MethodLiteral* methodLiterals_;
        size_t capacity_;
        size_t current_index_;
    };

    class ConstIterator {
    public:
        // A const view of the container still yields mutable MethodLiteral* values, mirroring
        // the original Entry-based implementation (a const Entry* exposed a non-const
        // MethodLiteral*).
        ConstIterator(const Entry* table, MethodLiteral* methodLiterals, size_t capacity, size_t index = 0)
            : table_(table), methodLiterals_(methodLiterals), capacity_(capacity), current_index_(index)
        {
            while (current_index_ < capacity_ && table_[current_index_].IsEmpty()) {
                ++current_index_;
            }
        }

        std::pair<const uint32_t, MethodLiteral*> operator*() const
        {
            ASSERT(table_ != nullptr);
            ASSERT(current_index_ < capacity_);
            return {table_[current_index_].key_, table_[current_index_].Value(methodLiterals_)};
        }

        ConstIterator& operator++()
        {
            ++current_index_;
            while (current_index_ < capacity_ && table_[current_index_].IsEmpty()) {
                ++current_index_;
            }
            return *this;
        }

        ConstIterator operator++(int)
        {
            ConstIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const ConstIterator& other) const
        {
            return current_index_ == other.current_index_;
        }

        bool operator!=(const ConstIterator& other) const
        {
            return !(*this == other);
        }

    private:
        const Entry* table_;
        MethodLiteral* methodLiterals_;
        size_t capacity_;
        size_t current_index_;
    };

    Iterator begin()
    {
        return Iterator(table_, methodLiterals_, capacity_, 0);
    }

    Iterator end()
    {
        return Iterator(table_, methodLiterals_, capacity_, capacity_);
    }

    ConstIterator begin() const
    {
        return ConstIterator(table_, methodLiterals_, capacity_, 0);
    }

    ConstIterator end() const
    {
        return ConstIterator(table_, methodLiterals_, capacity_, capacity_);
    }

    // Reserve space for expected number of elements and bind the MethodLiteral array base that
    // stored indices are resolved against (ignored by the pointer-based Entry layout). The map
    // does not own `methodLiterals`.
    void Reserve(size_t expectedSize, MethodLiteral* methodLiterals)
    {
        Clear();
        capacity_ = CalculateCapacity(expectedSize);
        ASSERT(capacity_ > 0);
        table_ = new Entry[capacity_];
        methodLiterals_ = methodLiterals;
        size_ = 0;
    }

    // Insert (key -> value) pair, returns true if inserted, false if key exists or table is full.
    // With the index-based Entry layout, value must point into the array bound by Reserve().
    bool Insert(uint32_t key, MethodLiteral* value)
    {
        ASSERT(table_ != nullptr);
        ASSERT(value != nullptr);
        size_t index = Hash(key);
        size_t start = index;
        // Linear probing to find empty slot or existing key
        do {
            if (table_[index].IsEmpty()) {
                // Found empty slot, insert here
                table_[index].Set(key, value, methodLiterals_);
                size_++;
                return true;
            }
            if (table_[index].key_ == key) {
                return false;  // Key already exists
            }
            index = (index + 1) & (capacity_ - 1);  // Using bitwise AND for modulo
        } while (index != start);
        return false;  // Table is full
    }

    // Find value by key, returns nullptr if not found.
    MethodLiteral* Find(uint32_t key) const
    {
        ASSERT(table_ != nullptr);
        size_t index = Hash(key);
        size_t start = index;
        do {
            if (table_[index].IsEmpty()) {
                return nullptr;  // Empty slot found, key doesn't exist
            }
            if (table_[index].key_ == key) {
                return table_[index].Value(methodLiterals_);  // Found key
            }
            index = (index + 1) & (capacity_ - 1);  // Using bitwise AND for modulo
        } while (index != start);

        return nullptr;  // Table is full and key not found
    }

    // Clear all entries by deleting and recreating table. Also drops the bound array base; a
    // subsequent Reserve() must re-bind it before the map can be used again.
    void Clear()
    {
        if (table_ != nullptr) {
            delete[] table_;
            table_ = nullptr;
        }
        methodLiterals_ = nullptr;
        capacity_ = 0;
        size_ = 0;
    }

    // Get current size
    size_t Size() const
    {
        return size_;
    }

    // Disable copy constructor and assignment
    MethodLiteralIDMap(const MethodLiteralIDMap&) = delete;
    MethodLiteralIDMap& operator=(const MethodLiteralIDMap&) = delete;

    // Move constructor
    MethodLiteralIDMap(MethodLiteralIDMap&& other) noexcept
        : table_(other.table_), methodLiterals_(other.methodLiterals_),
          capacity_(other.capacity_), size_(other.size_)
    {
        other.table_ = nullptr;
        other.methodLiterals_ = nullptr;
        other.capacity_ = 0;
        other.size_ = 0;
    }

    // Move assignment
    MethodLiteralIDMap& operator=(MethodLiteralIDMap&& other) noexcept
    {
        if (this != &other) {
            Clear();
            table_ = other.table_;
            methodLiterals_ = other.methodLiterals_;
            capacity_ = other.capacity_;
            size_ = other.size_;
            other.table_ = nullptr;
            other.methodLiterals_ = nullptr;
            other.capacity_ = 0;
            other.size_ = 0;
        }
        return *this;
    }

private:
#if ENABLE_LATEST_OPTIMIZATION
    // Sentinel stored in Entry::valueIndex_ to mark an empty slot (no separate occupied flag).
    static constexpr uint32_t INVALID_INDEX = UINT32_MAX;

    // Index-based slot: 8 bytes, stores an index into the array bound by Reserve().
    struct Entry {
        uint32_t key_;
        uint32_t valueIndex_;  // index into the bound methodLiterals array
        Entry() : key_(0), valueIndex_(INVALID_INDEX) {}
        bool IsEmpty() const
        {
            return valueIndex_ == INVALID_INDEX;
        }
        void Set(uint32_t key, MethodLiteral* value, MethodLiteral* base)
        {
            ASSERT(base != nullptr);
            uint32_t index = static_cast<uint32_t>(value - base);
            ASSERT(index != INVALID_INDEX);  // value outside the bound array or overflow
            key_ = key;
            valueIndex_ = index;
        }
        MethodLiteral* Value(MethodLiteral* base) const
        {
            return base + valueIndex_;
        }
    };
    static_assert(sizeof(Entry) == 8, "MethodLiteralIDMap::Entry must pack to 8 bytes");
#else
    // Pointer-based slot: nullptr marks an empty slot (Insert rejects null values).
    struct Entry {
        MethodLiteral* value_;
        uint32_t key_;
        Entry() : value_(nullptr), key_(0) {}
        bool IsEmpty() const
        {
            return value_ == nullptr;
        }
        void Set(uint32_t key, MethodLiteral* value, MethodLiteral* /*base*/)
        {
            key_ = key;
            value_ = value;
        }
        MethodLiteral* Value(MethodLiteral* /*base*/) const
        {
            return value_;
        }
    };
#endif

    // Hash function
    size_t Hash(uint32_t key) const
    {
        // Use bitwise AND for better performance when capacity is power of two
        return key & (capacity_ - 1);
    }

    // Calculate capacity based on expected size
    size_t CalculateCapacity(size_t expectedSize) const
    {
        // Use load factor of 0.75 for good performance
        // Add extra space to handle collisions
        const double MAX_LOAD_FACTOR = 0.75;
        size_t baseCapacity = static_cast<size_t>(expectedSize / MAX_LOAD_FACTOR) + 1;
        // Round up to next power of two for better hash distribution
        // and to use bitwise AND for modulo operation
        size_t capacity = 16; // 16: Ensure minimum capacity
        while (capacity < baseCapacity) {
            capacity <<= 1;
        }
        return capacity;
    }

    Entry* table_;
    MethodLiteral* methodLiterals_;  // array base bound by Reserve(); resolves stored indices
    size_t capacity_;
    size_t size_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JSPANDAFILE_METHOD_LITERAL_ID_MAP_H
