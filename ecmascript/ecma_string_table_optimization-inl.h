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

#ifndef ECMASCRIPT_STRING_TABLE_OPTIMIZ_INL_H
#define ECMASCRIPT_STRING_TABLE_OPTIMIZ_INL_H

#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/runtime_lock.h"

namespace panda::ecmascript {
#if ENABLE_NEXT_OPTIMIZATION

// Expand to get oldEntry and newEntry, with hash conflicts from 32 bits up to
// hashShift and Generate a subtree of indirect nodes to hold two new entries.
template <bool IsLock>
HashTrieMap::Node *HashTrieMap::Expand(Entry *oldEntry, Entry *newEntry, uint32_t newHash, uint32_t hashShift,
                                       Indirect *parent)
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
    Indirect *newIndirect = new Indirect(parent);
    Indirect *top = newIndirect;

    while (true) {
#ifndef NDEBUG
        if (hashShift == TOTAL_HASH_BITS) {
            if constexpr (IsLock) {
                RuntimeUnLock(parent->GetMutex());
            }
            LOG_ECMA(FATAL) << "StringTable: ran out of hash bits while inserting";
            UNREACHABLE();
        }
#endif

        hashShift += N_CHILDREN_LOG2;  // hashShift is the level at which the parent
                                       // is located. Need to go deeper.
        uint32_t oldIdx = (oldHash >> hashShift) & N_CHILDREN_MASK;
        uint32_t newIdx = (newHash >> hashShift) & N_CHILDREN_MASK;
        if (oldIdx != newIdx) {
            newIndirect->children_[oldIdx].store(oldEntry, std::memory_order_release);
            newIndirect->children_[newIdx].store(newEntry, std::memory_order_release);
            break;
        }
        Indirect *nextIndirect = new Indirect(newIndirect);

        newIndirect->children_[oldIdx].store(nextIndirect, std::memory_order_release);
        newIndirect = nextIndirect;
    }
    return top;
}

// Load returns the value of the key stored in the mapping, or nullptr value if not
template <bool IsCheck>
EcmaString *HashTrieMap::Load(const uint32_t key, EcmaString *value)
{
    uint32_t hash = key;
    Indirect *current = root_.load(std::memory_order_relaxed);

    for (uint32_t hashShift = 0; hashShift < TOTAL_HASH_BITS; hashShift += N_CHILDREN_LOG2) {
        size_t index = (hash >> hashShift) & N_CHILDREN_MASK;

        std::atomic<Node *> *slot = &current->children_[index];
        Node *node = slot->load(std::memory_order_acquire);
        if (node == nullptr) {
            return nullptr;
        }
        if (node->IsEntry()) {
            for (Entry *currentEntry = node->AsEntry(); currentEntry != nullptr;
                 currentEntry = currentEntry->Overflow().load(std::memory_order_acquire)) {
                if (currentEntry->Key() == key) {
                    bool valuesEqual = EcmaStringAccessor::StringsAreEqual(currentEntry->Value(), value);
                    if constexpr (IsCheck) {
                        if (!valuesEqual) {
                            return currentEntry->Value();
                        }
                    } else {
                        if (valuesEqual) {
                            return currentEntry->Value();
                        }
                    }
                }
            }
            return nullptr;
        }
        current = node->AsIndirect();
    }

    LOG_ECMA(FATAL) << "StringTable: ran out of hash bits while iterating";
    UNREACHABLE();
}

// LoadOrStore returns the existing value of the key, if it exists.
// Otherwise, call the callback function to create a value,
// store the value, and return the value
template <bool IsLock, typename LoaderCallback, typename EqualsCallback>
EcmaString *HashTrieMap::LoadOrStore(EcmaVM *vm, const uint32_t key, LoaderCallback loaderCallback,
                                     EqualsCallback equalsCallback)
{
    HashTrieMapInUseScope mapInUse(this);
    uint32_t hash = key;
    uint32_t hashShift = 0;
    std::atomic<Node *> *slot = nullptr;
    Node *node = nullptr;
    bool haveInsertPoint = false;
    JSHandle<EcmaString> str;
    bool isStrCreated = false;  // Flag to track whether an object has been created
    Indirect *current = root_.load(std::memory_order_acquire);
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
                for (Entry *currentEntry = node->AsEntry(); currentEntry != nullptr;
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
            LOG_ECMA(FATAL) << "StringTable: ran out of hash bits while iterating";
            UNREACHABLE();
        }
#endif
        // invoke the callback to create str
        if (!isStrCreated) {
            str = std::invoke(std::forward<LoaderCallback>(loaderCallback));
            isStrCreated = true;  // Tag str created
        }
        // lock and double-check
        if constexpr (IsLock) {
            RuntimeLock(vm->GetJSThread(), current->GetMutex());
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
            if (vm->IsCollectingScopeLockStats()) {
                vm->IncreaseStringTableLockCount();
            }
#endif
        }

        node = slot->load(std::memory_order_acquire);
        if (node == nullptr || node->IsEntry()) {
            // see is still real, so can continue to insert.
            break;
        }
        if constexpr (IsLock) {
            RuntimeUnLock(current->GetMutex());
        }
        current = node->AsIndirect();
        hashShift += N_CHILDREN_LOG2;
    }

    Entry *oldEntry = nullptr;
    if (node != nullptr) {
        oldEntry = node->AsEntry();
        for (Entry *currentEntry = oldEntry; currentEntry;
             currentEntry = currentEntry->Overflow().load(std::memory_order_acquire)) {
            if (currentEntry->Key() == key &&
                std::invoke(std::forward<EqualsCallback>(equalsCallback), currentEntry->Value())) {
                if constexpr (IsLock) {
                    RuntimeUnLock(current->GetMutex());
                }
                return currentEntry->Value();
            }
        }
    }

    EcmaString *value = *str;
    EcmaStringAccessor(value).SetInternString();
    Entry *newEntry = new Entry(key, value);
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
        RuntimeUnLock(current->GetMutex());
    }
    return value;
}

// LoadOrStorForJit need to lock the object before creating the object.
// returns the existing value of the key, if it exists.
// Otherwise, call the callback function to create a value,
// store the value, and return the value
template <typename LoaderCallback, typename EqualsCallback>
EcmaString *HashTrieMap::LoadOrStoreForJit(EcmaVM *vm, const uint32_t key, LoaderCallback loaderCallback,
                                           EqualsCallback equalsCallback)
{
    HashTrieMapInUseScope mapInUse(this);
    uint32_t hash = key;
    uint32_t hashShift = 0;
    std::atomic<Node *> *slot = nullptr;
    Node *node = nullptr;
    bool haveInsertPoint = false;
    EcmaString *value = nullptr;
    Indirect *current = root_.load(std::memory_order_acquire);
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
                for (Entry *currentEntry = node->AsEntry(); currentEntry != nullptr;
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
            LOG_ECMA(FATAL) << "StringTable: ran out of hash bits while iterating";
            UNREACHABLE();
        }
#endif
        // Jit need to lock the object before creating the object
        RuntimeLock(vm->GetJSThread(), current->GetMutex());
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
        if (vm->IsCollectingScopeLockStats()) {
            vm->IncreaseStringTableLockCount();
        }
#endif
        // invoke the callback to create str
        value = std::invoke(std::forward<LoaderCallback>(loaderCallback));
        node = slot->load(std::memory_order_acquire);
        if (node == nullptr || node->IsEntry()) {
            // see is still real, so can continue to insert.
            break;
        }

        RuntimeUnLock(current->GetMutex());
        current = node->AsIndirect();
        hashShift += N_CHILDREN_LOG2;
    }

    Entry *oldEntry = nullptr;
    if (node != nullptr) {
        oldEntry = node->AsEntry();
        for (Entry *currentEntry = oldEntry; currentEntry;
             currentEntry = currentEntry->Overflow().load(std::memory_order_acquire)) {
            if (currentEntry->Key() == key &&
                std::invoke(std::forward<EqualsCallback>(equalsCallback), currentEntry->Value())) {
                RuntimeUnLock(current->GetMutex());
                return currentEntry->Value();
            }
        }
    }

    EcmaStringAccessor(value).SetInternString();
    Entry *newEntry = new Entry(key, value);
    if (oldEntry == nullptr) {
        // The simple case: Create a new entry and store it.
        slot->store(newEntry, std::memory_order_release);
    } else {
        // Expand an existing entry to one or more new nodes.
        // Release the node, which will make both oldEntry and newEntry visible
        auto expandedNode = Expand<true>(oldEntry, newEntry, hash, hashShift, current);
        slot->store(expandedNode, std::memory_order_release);
    }
    RuntimeUnLock(current->GetMutex());
    return value;
}

// Based on the loadResult, try the store first
// StoreOrLoad returns the existing value of the key, store the value, and return the value
template <typename LoaderCallback, typename EqualsCallback>
EcmaString *HashTrieMap::StoreOrLoad(EcmaVM *vm, const uint32_t key, HashTrieMapLoadResult loadResult,
                                     LoaderCallback loaderCallback, EqualsCallback equalsCallback)
{
    HashTrieMapInUseScope mapInUse(this);
    uint32_t hash = key;
    uint32_t hashShift = loadResult.hashShift;
    std::atomic<Node *> *slot = loadResult.slot;
    Node *node = nullptr;
    bool haveInsertPoint = true;
    Indirect *current = loadResult.current;
    JSHandle<EcmaString> str = std::invoke(std::forward<LoaderCallback>(loaderCallback));
    // lock and double-check
    RuntimeLock(vm->GetJSThread(), current->GetMutex());
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    if (vm->IsCollectingScopeLockStats()) {
        vm->IncreaseStringTableLockCount();
    }
#endif
    node = slot->load(std::memory_order_acquire);
    if (node != nullptr && !node->IsEntry()) {
        RuntimeUnLock(current->GetMutex());
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
                    for (Entry *currentEntry = node->AsEntry(); currentEntry != nullptr;
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
                LOG_ECMA(FATAL) << "StringTable: ran out of hash bits while iterating";
                UNREACHABLE();
            }
#endif
            // lock and double-check
            RuntimeLock(vm->GetJSThread(), current->GetMutex());
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
            if (vm->IsCollectingScopeLockStats()) {
                vm->IncreaseStringTableLockCount();
            }
#endif

            node = slot->load(std::memory_order_acquire);
            if (node == nullptr || node->IsEntry()) {
                // see is still real, so can continue to insert.
                break;
            }

            RuntimeUnLock(current->GetMutex());
            current = node->AsIndirect();
            hashShift += N_CHILDREN_LOG2;
        }
    }
    Entry *oldEntry = nullptr;
    if (node != nullptr) {
        oldEntry = node->AsEntry();
        for (Entry *currentEntry = oldEntry; currentEntry;
             currentEntry = currentEntry->Overflow().load(std::memory_order_acquire)) {
            if (currentEntry->Key() == key &&
                std::invoke(std::forward<EqualsCallback>(equalsCallback), currentEntry->Value())) {
                RuntimeUnLock(current->GetMutex());
                return currentEntry->Value();
            }
        }
    }

    EcmaString *value = *str;
    EcmaStringAccessor(value).SetInternString();
    Entry *newEntry = new Entry(key, value);
    if (oldEntry == nullptr) {
        // The simple case: Create a new entry and store it.
        slot->store(newEntry, std::memory_order_release);
    } else {
        // Expand an existing entry to one or more new nodes.
        // Release the node, which will make both oldEntry and newEntry visible
        auto expandedNode = Expand<true>(oldEntry, newEntry, hash, hashShift, current);
        slot->store(expandedNode, std::memory_order_release);
    }

    RuntimeUnLock(current->GetMutex());
    return value;
}

template <typename LoaderCallback, typename EqualsCallback>
EcmaString *EcmaStringTable::GetOrInternString(EcmaVM *vm, uint32_t hashcode, LoaderCallback loaderCallback,
                                               EqualsCallback equalsCallback)
{
    EcmaString *result = stringTable_.LoadOrStore<true>(vm, hashcode, loaderCallback, equalsCallback);
    ASSERT(result != nullptr);
    return result;
}
#endif
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_STRING_TABLE_OPTIMIZ_INL_H
