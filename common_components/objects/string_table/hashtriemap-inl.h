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

#ifndef COMMON_COMPONENTS_OBJECTS_STRING_TABLE_HASHTRIEMAP_INL_H
#define COMMON_COMPONENTS_OBJECTS_STRING_TABLE_HASHTRIEMAP_INL_H

#include "common_components/log/log.h"
#include "common_interfaces/objects/readonly_handle.h"
#include "common_interfaces/objects/base_string.h"
#include "common_components/objects/string_table/integer_cache.h"

namespace common {
// Expand to get oldEntry and newEntry, with hash conflicts from 32 bits up to
// hashShift and Generate a subtree of indirect nodes to hold two new entries.
template <typename Mutex, typename ThreadHolder>
template <bool IsLock>
typename HashTrieMap<Mutex, ThreadHolder>::Node* HashTrieMap<Mutex, ThreadHolder>::Expand(
    Entry* oldEntry, Entry* newEntry, uint32_t newHash, uint32_t hashShift, Indirect* parent)
{
    uint32_t oldHash = oldEntry->Key();
    // Check for hash conflicts.
    if (oldHash == newHash) {
        // Store the old entry in the overflow list of the new entry, and then store
        // the new entry.
        newEntry->Overflow().store(oldEntry, std::memory_order_release);
        return newEntry;
    }

    // must add an indirect node. may need to add more than one.
    Indirect* newIndirect = new Indirect(parent);
    Indirect* top = newIndirect;

    while (true) {
#ifndef NDEBUG
        if (hashShift == TOTAL_HASH_BITS) {
            if constexpr (IsLock) {
                GetMutex().Unlock();
            }
            LOG_COMMON(FATAL) << "StringTable: ran out of hash bits while inserting";
            UNREACHABLE();
        }
#endif

        hashShift += N_CHILDREN_LOG2; // hashShift is the level at which the parent
        // is located. Need to go deeper.
        uint32_t oldIdx = (oldHash >> hashShift) & N_CHILDREN_MASK;
        uint32_t newIdx = (newHash >> hashShift) & N_CHILDREN_MASK;
        if (oldIdx != newIdx) {
            newIndirect->children_[oldIdx].store(oldEntry, std::memory_order_release);
            newIndirect->children_[newIdx].store(newEntry, std::memory_order_release);
            break;
        }
        Indirect* nextIndirect = new Indirect(newIndirect);

        newIndirect->children_[oldIdx].store(nextIndirect, std::memory_order_release);
        newIndirect = nextIndirect;
    }
    return top;
}

// Load returns the value of the key stored in the mapping, or nullptr value if not
template <typename Mutex, typename ThreadHolder>
template <bool IsCheck, typename ReadBarrier>
BaseString* HashTrieMap<Mutex, ThreadHolder>::Load(ReadBarrier&& readBarrier, const uint32_t key, BaseString* value)
{
    uint32_t hash = key;
    Indirect* current = root_.load(std::memory_order_relaxed);

    for (uint32_t hashShift = 0; hashShift < TOTAL_HASH_BITS; hashShift += N_CHILDREN_LOG2) {
        size_t index = (hash >> hashShift) & N_CHILDREN_MASK;

        std::atomic<Node*>* slot = &current->children_[index];
        Node* node = slot->load(std::memory_order_acquire);
        if (node == nullptr) {
            return nullptr;
        }
        if (!node->IsEntry()) {
            current = node->AsIndirect();
            continue;
        }
        for (Entry* currentEntry = node->AsEntry(); currentEntry != nullptr;
             currentEntry = currentEntry->Overflow().load(std::memory_order_acquire)) {
            if (currentEntry->Key() == key) {
                bool valuesEqual = BaseString::StringsAreEqual(std::forward<ReadBarrier>(readBarrier),
                                                               currentEntry->Value(), value);
                if constexpr (IsCheck) {
                    if (!valuesEqual) {
                        return currentEntry->Value();
                    }
                } else if (valuesEqual) {
                    return currentEntry->Value();
                }
            }
        }
        return nullptr;
    }

    LOG_COMMON(FATAL) << "StringTable: ran out of hash bits while iterating";
    UNREACHABLE();
}

// LoadOrStore returns the existing value of the key, if it exists.
// Otherwise, call the callback function to create a value,
// store the value, and return the value
template <typename Mutex, typename ThreadHolder>
template <bool IsLock, typename LoaderCallback, typename EqualsCallback>
BaseString* HashTrieMap<Mutex, ThreadHolder>::LoadOrStore(ThreadHolder* holder, const uint32_t key,
                                                          LoaderCallback loaderCallback,
                                                          EqualsCallback equalsCallback)
{
    HashTrieMapInUseScope mapInUse(this);
    uint32_t hash = key;
    uint32_t hashShift = 0;
    std::atomic<Node*>* slot = nullptr;
    Node* node = nullptr;
    [[maybe_unused]] bool haveInsertPoint = false;
    ReadOnlyHandle<BaseString> str;
    bool isStrCreated = false; // Flag to track whether an object has been created
    Indirect* current = root_.load(std::memory_order_acquire);
    while (true) {
        haveInsertPoint = false;
        // find the key or insert the candidate position.
        for (; hashShift < TOTAL_HASH_BITS; hashShift += N_CHILDREN_LOG2) {
            size_t index = (hash >> hashShift) & N_CHILDREN_MASK;
            slot = &current->children_[index];
            node = slot->load(std::memory_order_acquire);
            if (node == nullptr) {
                haveInsertPoint = true;
                break;
            }
            // Entry, Search in overflow
            if (!node->IsEntry()) {
                // Indirect, Next level Continue to search
                current = node->AsIndirect();
                continue;
            }
            for (Entry* currentEntry = node->AsEntry(); currentEntry != nullptr;
                 currentEntry = currentEntry->Overflow().load(std::memory_order_acquire)) {
                if (currentEntry->Key() == key &&
                    std::invoke(std::forward<EqualsCallback>(equalsCallback), currentEntry->Value())) {
                    return currentEntry->Value();
                }
            }
            haveInsertPoint = true;
            break;
        }
#ifndef NDEBUG
        if (!haveInsertPoint) {
            LOG_COMMON(FATAL) << "StringTable: ran out of hash bits while iterating";
            UNREACHABLE();
        }
#endif
        // invoke the callback to create str
        if (!isStrCreated) {
            str = std::invoke(std::forward<LoaderCallback>(loaderCallback));
            isStrCreated = true; // Tag str created
        }
        // lock and double-check
        if constexpr (IsLock) {
            GetMutex().LockWithThreadState(holder);
        }

        ASSERT(slot != nullptr);
        node = slot->load(std::memory_order_acquire);
        if (node == nullptr || node->IsEntry()) {
            // see is still real, so can continue to insert.
            break;
        }
        if constexpr (IsLock) {
            GetMutex().Unlock();
        }
        current = node->AsIndirect();
        hashShift += N_CHILDREN_LOG2;
    }

    Entry* oldEntry = nullptr;
    if (node != nullptr) {
        oldEntry = node->AsEntry();
        for (Entry* currentEntry = oldEntry; currentEntry;
             currentEntry = currentEntry->Overflow().load(std::memory_order_acquire)) {
            if (currentEntry->Key() == key &&
                std::invoke(std::forward<EqualsCallback>(equalsCallback), currentEntry->Value())) {
                if constexpr (IsLock) {
                    GetMutex().Unlock();
                }
                return currentEntry->Value();
            }
        }
    }

    BaseString* value = *str;
    value->SetIsInternString();
    IntegerCache::InitIntegerCache(value);
    Entry* newEntry = new Entry(key, value);
    if (oldEntry == nullptr) {
        // The simple case: Create a new entry and store it.
        slot->store(newEntry, std::memory_order_release);
    } else {
        // Expand an existing entry to one or more new nodes.
        // Release the node, which will make both oldEntry and newEntry visible
        auto expandedNode = Expand<IsLock>(oldEntry, newEntry, hash, hashShift, current);
        slot->store(expandedNode, std::memory_order_release);
    }
    if constexpr (IsLock) {
        GetMutex().Unlock();
    }
    return value;
}

// LoadOrStorForJit need to lock the object before creating the object.
// returns the existing value of the key, if it exists.
// Otherwise, call the callback function to create a value,
// store the value, and return the value
template <typename Mutex, typename ThreadHolder>
template <typename LoaderCallback, typename EqualsCallback>
BaseString* HashTrieMap<Mutex, ThreadHolder>::LoadOrStoreForJit(ThreadHolder* holder, const uint32_t key,
                                                                LoaderCallback loaderCallback,
                                                                EqualsCallback equalsCallback)
{
    HashTrieMapInUseScope mapInUse(this);
    uint32_t hash = key;
    uint32_t hashShift = 0;
    std::atomic<Node*>* slot = nullptr;
    Node* node = nullptr;
    [[maybe_unused]] bool haveInsertPoint = false;
    BaseString* value = nullptr;
    Indirect* current = root_.load(std::memory_order_acquire);
    while (true) {
        haveInsertPoint = false;
        // find the key or insert the candidate position.
        for (; hashShift < TOTAL_HASH_BITS; hashShift += N_CHILDREN_LOG2) {
            size_t index = (hash >> hashShift) & N_CHILDREN_MASK;
            slot = &current->children_[index];
            node = slot->load(std::memory_order_acquire);
            if (node == nullptr) {
                haveInsertPoint = true;
                break;
            }
            // Entry, Search in overflow
            if (!node->IsEntry()) {
                // Indirect, Next level Continue to search
                current = node->AsIndirect();
                continue;
            }
            for (Entry* currentEntry = node->AsEntry(); currentEntry != nullptr;
                 currentEntry = currentEntry->Overflow().load(std::memory_order_acquire)) {
                if (currentEntry->Key() == key &&
                    std::invoke(std::forward<EqualsCallback>(equalsCallback), currentEntry->Value())) {
                    return currentEntry->Value();
                }
            }
            haveInsertPoint = true;
            break;
        }
#ifndef NDEBUG
        if (!haveInsertPoint) {
            LOG_COMMON(FATAL) << "StringTable: ran out of hash bits while iterating";
            UNREACHABLE();
        }
#endif
        // Jit need to lock the object before creating the object
        GetMutex().LockWithThreadState(holder);
        // invoke the callback to create str
        value = std::invoke(std::forward<LoaderCallback>(loaderCallback));
        ASSERT(slot != nullptr);
        node = slot->load(std::memory_order_acquire);
        if (node == nullptr || node->IsEntry()) {
            // see is still real, so can continue to insert.
            break;
        }

        GetMutex().Unlock();
        current = node->AsIndirect();
        hashShift += N_CHILDREN_LOG2;
    }

    Entry* oldEntry = nullptr;
    if (node != nullptr) {
        oldEntry = node->AsEntry();
        for (Entry* currentEntry = oldEntry; currentEntry;
             currentEntry = currentEntry->Overflow().load(std::memory_order_acquire)) {
            if (currentEntry->Key() == key &&
                std::invoke(std::forward<EqualsCallback>(equalsCallback), currentEntry->Value())) {
                GetMutex().Unlock();
                return currentEntry->Value();
            }
        }
    }

    value->SetIsInternString();
    IntegerCache::InitIntegerCache(value);
    Entry* newEntry = new Entry(key, value);
    if (oldEntry == nullptr) {
        // The simple case: Create a new entry and store it.
        slot->store(newEntry, std::memory_order_release);
    } else {
        // Expand an existing entry to one or more new nodes.
        // Release the node, which will make both oldEntry and newEntry visible
        auto expandedNode = Expand<true>(oldEntry, newEntry, hash, hashShift, current);
        slot->store(expandedNode, std::memory_order_release);
    }
    GetMutex().Unlock();
    return value;
}

// Based on the loadResult, try the store first
// StoreOrLoad returns the existing value of the key, store the value, and return the value
template <typename Mutex, typename ThreadHolder>
template <typename LoaderCallback, typename EqualsCallback>
BaseString* HashTrieMap<Mutex, ThreadHolder>::StoreOrLoad(ThreadHolder* holder, const uint32_t key,
                                                          HashTrieMapLoadResult loadResult,
                                                          LoaderCallback loaderCallback,
                                                          EqualsCallback equalsCallback)
{
    HashTrieMapInUseScope mapInUse(this);
    uint32_t hash = key;
    uint32_t hashShift = loadResult.hashShift;
    std::atomic<Node*>* slot = loadResult.slot;
    Node* node = nullptr;
    [[maybe_unused]] bool haveInsertPoint = true;
    Indirect* current = loadResult.current;
    ReadOnlyHandle<BaseString> str = std::invoke(std::forward<LoaderCallback>(loaderCallback));
    // lock and double-check
    GetMutex().LockWithThreadState(holder);
    node = slot->load(std::memory_order_acquire);
    if (node != nullptr && !node->IsEntry()) {
        GetMutex().Unlock();
        current = node->AsIndirect();
        hashShift += N_CHILDREN_LOG2;
        while (true) {
            haveInsertPoint = false;
            // find the key or insert the candidate position.
            for (; hashShift < TOTAL_HASH_BITS; hashShift += N_CHILDREN_LOG2) {
                size_t index = (hash >> hashShift) & N_CHILDREN_MASK;
                slot = &current->children_[index];
                node = slot->load(std::memory_order_acquire);
                if (node == nullptr) {
                    haveInsertPoint = true;
                    break;
                }
                // Entry, Search in overflow
                if (node->IsEntry()) {
                    for (Entry* currentEntry = node->AsEntry(); currentEntry != nullptr;
                         currentEntry = currentEntry->Overflow().load(std::memory_order_acquire)) {
                        if (currentEntry->Key() == key &&
                            std::invoke(std::forward<EqualsCallback>(equalsCallback), currentEntry->Value())) {
                            return currentEntry->Value();
                        }
                    }
                    haveInsertPoint = true;
                    break;
                }
                // Indirect, Next level Continue to search
                current = node->AsIndirect();
            }
#ifndef NDEBUG
            if (!haveInsertPoint) {
                LOG_COMMON(FATAL) << "StringTable: ran out of hash bits while iterating";
                UNREACHABLE();
            }
#endif
            // lock and double-check
            GetMutex().LockWithThreadState(holder);
            node = slot->load(std::memory_order_acquire);
            if (node == nullptr || node->IsEntry()) {
                // see is still real, so can continue to insert.
                break;
            }
            GetMutex().Unlock();
            current = node->AsIndirect();
            hashShift += N_CHILDREN_LOG2;
        }
    }
    Entry* oldEntry = nullptr;
    if (node != nullptr) {
        oldEntry = node->AsEntry();
        for (Entry* currentEntry = oldEntry; currentEntry;
             currentEntry = currentEntry->Overflow().load(std::memory_order_acquire)) {
            if (currentEntry->Key() == key &&
                std::invoke(std::forward<EqualsCallback>(equalsCallback), currentEntry->Value())) {
                GetMutex().Unlock();
                return currentEntry->Value();
            }
        }
    }

    BaseString* value = *str;
    value->SetIsInternString();
    IntegerCache::InitIntegerCache(value);
    Entry* newEntry = new Entry(key, value);
    if (oldEntry == nullptr) {
        // The simple case: Create a new entry and store it.
        slot->store(newEntry, std::memory_order_release);
    } else {
        // Expand an existing entry to one or more new nodes.
        // Release the node, which will make both oldEntry and newEntry visible
        auto expandedNode = Expand<true>(oldEntry, newEntry, hash, hashShift, current);
        slot->store(expandedNode, std::memory_order_release);
    }

    GetMutex().Unlock();
    return value;
}

// Load returns the value of the key stored in the mapping, or HashTrieMapLoadResult for StoreOrLoad
template <typename Mutex, typename ThreadHolder>
template <typename ReadBarrier>
typename HashTrieMap<Mutex, ThreadHolder>::HashTrieMapLoadResult HashTrieMap<Mutex, ThreadHolder>::Load(
    ReadBarrier&& readBarrier,
    const uint32_t key, BaseString* value)
{
    uint32_t hash = key;
    Indirect* current = root_.load(std::memory_order_relaxed);
    for (uint32_t hashShift = 0; hashShift < TOTAL_HASH_BITS; hashShift += N_CHILDREN_LOG2) {
        size_t index = (hash >> hashShift) & N_CHILDREN_MASK;

        std::atomic<Node*>* slot = &current->children_[index];
        Node* node = slot->load(std::memory_order_acquire);
        if (node == nullptr) {
            return {nullptr, current, hashShift, slot};
        }
        if (node->IsEntry()) {
            for (Entry* currentEntry = node->AsEntry(); currentEntry != nullptr;
                 currentEntry = currentEntry->Overflow().load(std::memory_order_acquire)) {
                if (currentEntry->Key() == key &&
                    BaseString::StringsAreEqual(std::forward<ReadBarrier>(readBarrier), currentEntry->Value(),
                                                value)) {
                    return {currentEntry->Value(), nullptr, hashShift, nullptr};
                }
            }
            return {nullptr, current, hashShift, slot};
        }
        current = node->AsIndirect();
    }

    LOG_COMMON(FATAL) << "StringTable: ran out of hash bits while iterating";
    UNREACHABLE();
}

// Load returns the value of the key stored in the mapping, or HashTrieMapLoadResult for StoreOrLoad
template <typename Mutex, typename ThreadHolder>
template <typename ReadBarrier>
typename HashTrieMap<Mutex, ThreadHolder>::HashTrieMapLoadResult HashTrieMap<Mutex, ThreadHolder>::Load(
    ReadBarrier&& readBarrier,
    const uint32_t key, const ReadOnlyHandle<BaseString>& string, uint32_t offset, uint32_t utf8Len)
{
    uint32_t hash = key;
    Indirect* current = root_.load(std::memory_order_relaxed);
    const uint8_t* utf8Data = string->GetDataUtf8() + offset;
    for (uint32_t hashShift = 0; hashShift < TOTAL_HASH_BITS; hashShift += N_CHILDREN_LOG2) {
        size_t index = (hash >> hashShift) & N_CHILDREN_MASK;

        std::atomic<Node*>* slot = &current->children_[index];
        Node* node = slot->load(std::memory_order_acquire);
        if (node == nullptr) {
            return {nullptr, current, hashShift, slot};
        }
        if (!node->IsEntry()) {
            current = node->AsIndirect();
            continue;
        }
        for (Entry* currentEntry = node->AsEntry(); currentEntry != nullptr;
             currentEntry = currentEntry->Overflow().load(std::memory_order_acquire)) {
            if (currentEntry->Key() == key && BaseString::StringIsEqualUint8Data(
                std::forward<ReadBarrier>(readBarrier), currentEntry->Value(), utf8Data, utf8Len, true)) {
                return {currentEntry->Value(), nullptr, hashShift, nullptr};
            }
        }
        return {nullptr, current, hashShift, slot};
    }

    LOG_COMMON(FATAL) << "StringTable: ran out of hash bits while iterating";
    UNREACHABLE();
}

// Based on the loadResult, try the store first
// StoreOrLoad returns the existing value of the key, store the value, and return the value
template <typename Mutex, typename ThreadHolder>
template <bool threadState, typename ReadBarrier, typename HandleType>
BaseString* HashTrieMap<Mutex, ThreadHolder>::StoreOrLoad(ThreadHolder* holder, ReadBarrier&& readBarrier,
                                                          const uint32_t key, HashTrieMapLoadResult loadResult,
                                                          HandleType str)
{
    HashTrieMapInUseScope mapInUse(this);
    uint32_t hash = key;
    uint32_t hashShift = loadResult.hashShift;
    std::atomic<Node*>* slot = loadResult.slot;
    Node* node = nullptr;
    [[maybe_unused]] bool haveInsertPoint = true;
    Indirect* current = loadResult.current;
    if constexpr (threadState) {
        GetMutex().LockWithThreadState(holder);
    } else {
        GetMutex().Lock();
    }
    node = slot->load(std::memory_order_acquire);
    if (node != nullptr && !node->IsEntry()) {
        GetMutex().Unlock();
        current = node->AsIndirect();
        hashShift += N_CHILDREN_LOG2;
        while (true) {
            haveInsertPoint = false;
            for (; hashShift < TOTAL_HASH_BITS; hashShift += N_CHILDREN_LOG2) {
                size_t index = (hash >> hashShift) & N_CHILDREN_MASK;
                slot = &current->children_[index];
                node = slot->load(std::memory_order_acquire);
                if (node == nullptr) {
                    haveInsertPoint = true;
                    break;
                }
                // Entry, Search in overflow
                if (!node->IsEntry()) {
                    // Indirect, Next level Continue to search
                    current = node->AsIndirect();
                    continue;
                }
                for (Entry* currentEntry = node->AsEntry(); currentEntry != nullptr;
                     currentEntry = currentEntry->Overflow().load(std::memory_order_acquire)) {
                    if (currentEntry->Key() == key &&
                        BaseString::StringsAreEqual(std::forward<ReadBarrier>(readBarrier), currentEntry->Value(),
                                                    *str)) {
                        return currentEntry->Value();
                    }
                }
                haveInsertPoint = true;
                break;
            }
#ifndef NDEBUG
            if (!haveInsertPoint) {
                LOG_COMMON(FATAL) << "StringTable: ran out of hash bits while iterating";
                UNREACHABLE();
            }
#endif
            // lock and double-check
            if constexpr (threadState) {
                GetMutex().LockWithThreadState(holder);
            } else {
                GetMutex().Lock();
            }
            node = slot->load(std::memory_order_acquire);
            if (node == nullptr || node->IsEntry()) {
                // see is still real, so can continue to insert.
                break;
            }
            GetMutex().Unlock();
            current = node->AsIndirect();
            hashShift += N_CHILDREN_LOG2;
        }
    }

    Entry* oldEntry = nullptr;
    if (node != nullptr) {
        oldEntry = node->AsEntry();
        for (Entry* currentEntry = oldEntry; currentEntry;
             currentEntry = currentEntry->Overflow().load(std::memory_order_acquire)) {
            if (currentEntry->Key() == key && BaseString::StringsAreEqual(
                std::forward<ReadBarrier>(readBarrier), currentEntry->Value(), *str)) {
                GetMutex().Unlock();
                return currentEntry->Value();
            }
        }
    }

    BaseString* value = *str;
    value->SetIsInternString();
    IntegerCache::InitIntegerCache(value);
    Entry* newEntry = new Entry(key, value);
    if (oldEntry == nullptr) {
        // The simple case: Create a new entry and store it.
        slot->store(newEntry, std::memory_order_release);
    } else {
        // Expand an existing entry to one or more new nodes.
        // Release the node, which will make both oldEntry and newEntry visible
        auto expandedNode = Expand<true>(oldEntry, newEntry, hash, hashShift, current);
        slot->store(expandedNode, std::memory_order_release);
    }
    GetMutex().Unlock();
    return value;
}


template <typename Mutex, typename ThreadHolder>
bool HashTrieMap<Mutex, ThreadHolder>::CheckWeakRef(const WeakRootVisitor& visitor, Entry* entry)
{
    panda::ecmascript::TaggedObject* object = reinterpret_cast<panda::ecmascript::TaggedObject*>(entry->Value());
    auto fwd = visitor(object);
    if (fwd == nullptr) {
        LOG_COMMON(VERBOSE) << "StringTable: delete string " << std::hex << object;
        return true;
    } else if (fwd != object) {
        entry->SetValue(reinterpret_cast<BaseString*>(fwd));
        LOG_COMMON(VERBOSE) << "StringTable: forward " << std::hex << object << " -> " << fwd;
    }
    return false;
}

template <typename Mutex, typename ThreadHolder>
template <typename ReadBarrier>
bool HashTrieMap<Mutex, ThreadHolder>::CheckValidity(ReadBarrier&& readBarrier, BaseString* value, bool& isValid)
{
    if (!value->NotTreeString()) {
        isValid = false;
        return false;
    }
    uint32_t hashcode = value->GetHashcode(std::forward<ReadBarrier>(readBarrier));
    if (this->template Load<true>(std::forward<ReadBarrier>(readBarrier), hashcode, value) != nullptr) {
        isValid = false;
        return false;
    }
    return true;
}

template <typename Mutex, typename ThreadHolder>
template <typename ReadBarrier>
void HashTrieMap<Mutex, ThreadHolder>::Iter(ReadBarrier&& readBarrier, Indirect* node, bool& isValid)
{
    if (node == nullptr)
        return;

    for (std::atomic<Node*>& child : node->children_) {
        Node* childNode = child.load(std::memory_order_relaxed);
        if (childNode == nullptr)
            continue;

        if (!(childNode->IsEntry())) {
            // Recursive traversal of indirect nodes
            Iter(std::forward<ReadBarrier>(readBarrier), childNode->AsIndirect(), isValid);
            continue;
        }

        for (Entry* e = childNode->AsEntry(); e != nullptr; e = e->Overflow().load(std::memory_order_relaxed)) {
            if (!CheckValidity(std::forward<ReadBarrier>(readBarrier), e->Value(), isValid)) {
                return; // 用户终止迭代
            }
        }
    }
}

template <typename Mutex, typename ThreadHolder>
bool HashTrieMap<Mutex, ThreadHolder>::CheckWeakRef(const WeakRefFieldVisitor& visitor, HashTrieMap::Entry* entry)
{
    // RefField only support 64-bit value, so could not cirectly pass `Entry::Value` to WeakRefFieldVisitor
    // int 32-bit machine
    uint64_t str = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(entry->Value()));
    bool isAlive = visitor(reinterpret_cast<RefField<> &>(str));
    if (!isAlive) {
        return true;
    }
    entry->SetValue(reinterpret_cast<BaseString *>(static_cast<uintptr_t>(str)));
    return false;
}

template <typename Mutex, typename ThreadHolder>
bool HashTrieMap<Mutex, ThreadHolder>::ClearNodeFromGC(Indirect* parent, int index, const WeakRefFieldVisitor& visitor)
{
    // load sub-nodes
    Node* child = parent->children_[index].load(std::memory_order_relaxed);
    if (child == nullptr)
        return true;

    if (child->IsEntry()) {
        Entry* entry = child->AsEntry();
        // Processing the overflow linked list
        for (Entry *prev = nullptr, *current = entry->Overflow().load(std::memory_order_relaxed); current != nullptr
             ;) {
            Entry* next = current->Overflow().load(std::memory_order_relaxed);
            if (CheckWeakRef(visitor, current)) {
                // Remove and remove a node from a linked list
                if (prev) {
                    prev->Overflow().store(next, std::memory_order_relaxed);
                } else {
                    entry->Overflow().store(next, std::memory_order_relaxed);
                }
                delete current;
            } else {
                prev = current;
            }
            current = next;
        }
        // processing entry node
        if (CheckWeakRef(visitor, entry)) {
            Entry* e = entry->Overflow().load(std::memory_order_relaxed);
            if (e == nullptr) {
                // Delete the empty Entry node and update the parent reference
                delete entry;
                parent->children_[index].store(nullptr, std::memory_order_relaxed);
                return true;
            }
            // Delete the Entry node and update the parent reference
            delete entry;
            parent->children_[index].store(e, std::memory_order_relaxed);
        }
        return false;
    } else {
        // Recursive processing of the Indirect node
        Indirect* indirect = child->AsIndirect();
        uint32_t cleanCount = 0;
        for (uint32_t i = 0; i < INDIRECT_SIZE; ++i) {
            if (ClearNodeFromGC(indirect, i, visitor)) {
                cleanCount += 1;
            }
        }
        // Check whether the indirect node is empty
        if (cleanCount == INDIRECT_SIZE) {
            // Remove the empty Indirect and update the parent reference
            delete indirect;
            parent->children_[index].store(nullptr, std::memory_order_relaxed);
            return true;
        }
        return false;
    }
}

template <typename Mutex, typename ThreadHolder>
bool HashTrieMap<Mutex, ThreadHolder>::ClearNodeFromGC(Indirect* parent, int index, const WeakRootVisitor& visitor)
{
    // load sub-nodes
    Node* child = parent->children_[index].load(std::memory_order_relaxed);
    if (child == nullptr)
        return true;

    if (child->IsEntry()) {
        Entry* entry = child->AsEntry();
        // Processing the overflow linked list
        for (Entry *prev = nullptr, *current = entry->Overflow().load(std::memory_order_relaxed); current != nullptr;) {
            Entry* next = current->Overflow().load(std::memory_order_relaxed);
            if (CheckWeakRef(visitor, current)) {
                // Remove and remove a node from a linked list
                if (prev != nullptr) {
                    prev->Overflow().store(next, std::memory_order_relaxed);
                } else {
                    entry->Overflow().store(next, std::memory_order_relaxed);
                }
                delete current;
            } else {
                prev = current;
            }
            current = next;
        }
        // processing entry node
        if (CheckWeakRef(visitor, entry)) {
            Entry* e = entry->Overflow().load(std::memory_order_relaxed);
            if (e == nullptr) {
                // Delete the empty Entry node and update the parent reference
                delete entry;
                parent->children_[index].store(nullptr, std::memory_order_relaxed);
                return true;
            }
            // Delete the Entry node and update the parent reference
            delete entry;
            parent->children_[index].store(e, std::memory_order_relaxed);
        }
        return false;
    } else {
        // Recursive processing of the Indirect node
        Indirect* indirect = child->AsIndirect();
        uint32_t cleanCount = 0;
        for (uint32_t i = 0; i < INDIRECT_SIZE; ++i) {
            if (ClearNodeFromGC(indirect, i, visitor)) {
                cleanCount += 1;
            }
        }
        // Check whether the indirect node is empty
        if (cleanCount == INDIRECT_SIZE && inuseCount_ == 0) {
            // Remove the empty Indirect and update the parent reference
            delete indirect;
            parent->children_[index].store(nullptr, std::memory_order_relaxed);
            return true;
        }
        return false;
    }
}
}
#endif //COMMON_COMPONENTS_OBJECTS_STRING_TABLE_HASHTRIEMAP_INL_H
