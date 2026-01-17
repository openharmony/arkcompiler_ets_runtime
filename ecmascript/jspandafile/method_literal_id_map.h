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

namespace panda::ecmascript {
/**
 * Open addressing hash table with linear probing for mapping uint32_t keys to MethodLiteral pointers.
 * This container is optimized for known maximum size and provides O(1) average case for insertion and lookup.
 * Key features:
 * - Fixed capacity allocated upfront via Reserve()
 * - No rehashing or resizing operations
 * - Linear probing for collision resolution
 * - Fast bitwise modulo operations (capacity is power of two)
 * - Iterator support for range-based for loops with structured binding
 */
class MethodLiteralIDMap {
public:
    MethodLiteralIDMap() : table_(nullptr), capacity_(0), size_(0) {}

    ~MethodLiteralIDMap() { Clear(); }

    struct Entry {
        MethodLiteral* value_;
        uint32_t key_;
        bool occupied_;
        
        Entry() : value_(nullptr), key_(0), occupied_(false) {}
    };

    class Iterator {
    public:
        Iterator(Entry* table, size_t capacity, size_t index = 0)
            : table_(table), capacity_(capacity), current_index_(index)
        {
            while (current_index_ < capacity_ && !table_[current_index_].occupied_) {
                ++current_index_;
            }
        }

        std::pair<uint32_t, MethodLiteral*> operator*() const
        {
            ASSERT(table_ != nullptr);
            ASSERT(current_index_ < capacity_);
            return {table_[current_index_].key_, table_[current_index_].value_};
        }

        Iterator& operator++()
        {
            ++current_index_;
            while (current_index_ < capacity_ && !table_[current_index_].occupied_) {
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
        size_t capacity_;
        size_t current_index_;
    };

    class ConstIterator {
    public:
        ConstIterator(const Entry* table, size_t capacity, size_t index = 0)
            : table_(table), capacity_(capacity), current_index_(index)
        {
            while (current_index_ < capacity_ && !table_[current_index_].occupied_) {
                ++current_index_;
            }
        }

        std::pair<const uint32_t, MethodLiteral*> operator*() const
        {
            ASSERT(table_ != nullptr);
            ASSERT(current_index_ < capacity_);
            return {table_[current_index_].key_, table_[current_index_].value_};
        }

        ConstIterator& operator++()
        {
            ++current_index_;
            while (current_index_ < capacity_ && !table_[current_index_].occupied_) {
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
        size_t capacity_;
        size_t current_index_;
    };

    Iterator begin()
    {
        return Iterator(table_, capacity_, 0);
    }

    Iterator end()
    {
        return Iterator(table_, capacity_, capacity_);
    }

    ConstIterator begin() const
    {
        return ConstIterator(table_, capacity_, 0);
    }

    ConstIterator end() const
    {
        return ConstIterator(table_, capacity_, capacity_);
    }

    // Reserve space for expected number of elements
    void Reserve(size_t expectedSize)
    {
        AllocateTable(CalculateCapacity(expectedSize));
    }

    // Insert key-value pair, returns true if inserted, false if key exists or table is full
    bool Insert(uint32_t key, MethodLiteral* value)
    {
        ASSERT(table_ != nullptr);
        size_t index = Hash(key);
        size_t start = index;
        // Linear probing to find empty slot or existing key
        do {
            if (!table_[index].occupied_) {
                // Found empty slot, insert here
                table_[index].key_ = key;
                table_[index].value_ = value;
                table_[index].occupied_ = true;
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

    // Find value by key, returns nullptr if not found
    MethodLiteral* Find(uint32_t key) const
    {
        ASSERT(table_ != nullptr);
        size_t index = Hash(key);
        size_t start = index;
        do {
            if (!table_[index].occupied_) {
                return nullptr;  // Empty slot found, key doesn't exist
            }
            if (table_[index].key_ == key) {
                return table_[index].value_;  // Found key
            }
            index = (index + 1) & (capacity_ - 1);  // Using bitwise AND for modulo
        } while (index != start);
        
        return nullptr;  // Table is full and key not found
    }

    // Clear all entries by deleting and recreating table
    void Clear()
    {
        if (table_ != nullptr) {
            delete[] table_;
            table_ = nullptr;
        }
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
        : table_(other.table_), capacity_(other.capacity_), size_(other.size_)
    {
        other.table_ = nullptr;
        other.capacity_ = 0;
        other.size_ = 0;
    }

    // Move assignment
    MethodLiteralIDMap& operator=(MethodLiteralIDMap&& other) noexcept
    {
        if (this != &other) {
            Clear();
            table_ = other.table_;
            capacity_ = other.capacity_;
            size_ = other.size_;
            other.table_ = nullptr;
            other.capacity_ = 0;
            other.size_ = 0;
        }
        return *this;
    }
private:
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
        size_t capacity = 1;
        while (capacity < baseCapacity) {
            capacity <<= 1;
        }
        // Ensure minimum capacity
        const size_t MIN_CAPACITY = 16;
        return capacity < MIN_CAPACITY ? MIN_CAPACITY : capacity;
    }

    // Allocate new table
    void AllocateTable(size_t capacity)
    {
        if (UNLIKELY(capacity == 0)) {
            capacity_ = 0;
            table_ = nullptr;
            size_ = 0;
            return;
        }
        
        capacity_ = capacity;
        table_ = new Entry[capacity_];
        size_ = 0;
    }

    Entry* table_;
    size_t capacity_;
    size_t size_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JSPANDAFILE_METHOD_LITERAL_H