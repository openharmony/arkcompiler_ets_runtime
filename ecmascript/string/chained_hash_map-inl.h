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

#ifndef ECMASCRIPT_STRING_TABLE_CHAINED_HASH_MAP_INL_H
#define ECMASCRIPT_STRING_TABLE_CHAINED_HASH_MAP_INL_H

#include "chained_hash_map.h"
#include "common_components/log/log.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/string/base_string.h"
#include "ecmascript/string/integer_cache.h"
#include "ecmascript/string/line_string-inl.h"
#include "objects/readonly_handle.h"

namespace panda::ecmascript {
template<typename Mutex>
template<typename Fn>
void ChainedHashMap<Mutex>::ForEachBucketHeadInPartition(uint32_t partitionID, Fn&& fn)
{
    DCHECK_CC(partitionID < ChainedHashMapConfig::SWEEP_PARTITION_COUNT);
    uint32_t begin = partitionID << sweepPartitionToMapShift_;
    uint32_t end = (partitionID + 1) << sweepPartitionToMapShift_;
    for (uint32_t index = begin; index < end; ++index) {
        ChainedHashMapEntry* bucketHead = buckets_[index].load(std::memory_order_relaxed);
        if (bucketHead == nullptr) {
            continue;
        }
        fn(bucketHead, index);
    }
}

template<typename Mutex, typename ThreadHolder, ChainedHashMapConfig::SlotBarrier SlotBarrier>
template<typename Pred>
BaseString* ChainedHashMapOperation<Mutex, ThreadHolder, SlotBarrier>::FindInChain(Entry* head,
                                                                                   uint32_t key,
                                                                                   Pred&& pred)
{
    for (Entry* current = head; current != nullptr; current = current->Overflow().load(std::memory_order_acquire)) {
        auto oldValue = GetValue(current);
        if (IsNull(oldValue)) {
            continue;
        }
        if (oldValue->GetMixHashcode() != key) {
            continue;
        }
        if (std::invoke(pred, oldValue)) {
            return oldValue;
        }
    }
    return nullptr;
}

template<typename Mutex, typename ThreadHolder, ChainedHashMapConfig::SlotBarrier SlotBarrier>
void ChainedHashMapOperation<Mutex, ThreadHolder, SlotBarrier>::InsertWithLockHeld(uint32_t mapIndex,
                                                                                   BaseString* value)
{
    DCHECK_CC(value != nullptr);
    value->SetIsInternString();
    IntegerCache::InitIntegerCache(value);
    Entry* newEntry = NewEntry(value);
    Entry* oldEntry = PruneHead(GetBucketHead(mapIndex));
    chainedHashMap_->Insert(mapIndex, oldEntry, newEntry);
}

// Load<false> returns an equal interned string if present.
// Load<true> is used by validity checks and returns a hash-colliding non-equal string if present.
template<typename Mutex, typename ThreadHolder, ChainedHashMapConfig::SlotBarrier SlotBarrier>
template<bool IsCheck, typename ReadBarrier>
BaseString* ChainedHashMapOperation<Mutex, ThreadHolder, SlotBarrier>::Load(ReadBarrier&& readBarrier,
                                                                            uint32_t key,
                                                                            BaseString* value)
{
    uint32_t mapIndex = GetIDFromHash(key);
    return FindInChain(GetBucketHead(mapIndex), key, [&](BaseString* oldValue) {
        bool equal = BaseString::StringsAreEqual(std::forward<ReadBarrier>(readBarrier), oldValue, value);
        if constexpr (IsCheck) {
            return !equal;
        } else {
            return equal;
        }
    });
}

// LoadOrStore returns the existing value of the key, if it exists.
// Otherwise, call the callback function to create a value,
// store the value, and return the value
template<typename Mutex, typename ThreadHolder, ChainedHashMapConfig::SlotBarrier SlotBarrier>
template<bool IsLock, typename LoaderCallback, typename EqualsCallback>
BaseString* ChainedHashMapOperation<Mutex, ThreadHolder, SlotBarrier>::LoadOrStore(ThreadHolder* holder,
                                                                                   uint32_t key,
                                                                                   LoaderCallback loaderCallback,
                                                                                   EqualsCallback equalsCallback)
{
    uint32_t mapIndex = GetIDFromHash(key);
    [[maybe_unused]] uint32_t depth = 0;
    BaseString* existing = FindInChain(GetBucketHead(mapIndex), key, [&](BaseString* oldValue) {
        ++depth;
        return std::invoke(equalsCallback, oldValue);
    });
    if (existing != nullptr) {
#if ECMASCRIPT_ENABLE_TRACE_STRING_TABLE
        TraceFindSuccessDepth(depth);
#endif
        return existing;
    }

#if ECMASCRIPT_ENABLE_TRACE_STRING_TABLE
    TraceFindFail();
#endif
    common::ReadOnlyHandle<BaseString> str = std::invoke(std::forward<LoaderCallback>(loaderCallback));
    // lock and double-check
    if constexpr (IsLock) {
        chainedHashMap_->GetMutex().LockWithThreadState(holder);
    }

    // Re-scan unconditionally: the unlocked scan above cannot prove absence —
    // entries freed by a GC sweep while this thread was unlocked (loader call,
    // lock wait) can be reallocated at the same address (ABA), hiding a racing
    // insertion of an equal string.
    existing = FindInChain(GetBucketHead(mapIndex), key, equalsCallback);
    if (existing != nullptr) {
        if constexpr (IsLock) {
            chainedHashMap_->GetMutex().Unlock();
        }
        return existing;
    }

    BaseString* value = *str;
    InsertWithLockHeld(mapIndex, value);
    if constexpr (IsLock) {
        chainedHashMap_->GetMutex().Unlock();
    }
    return value;
}

// JIT interning paths must lock the hash map before creating the string.
template<typename Mutex, typename ThreadHolder, ChainedHashMapConfig::SlotBarrier SlotBarrier>
template<typename LoaderCallback, typename EqualsCallback>
BaseString* ChainedHashMapOperation<Mutex, ThreadHolder, SlotBarrier>::LoadOrStoreForJit(
    ThreadHolder* holder, uint32_t key, LoaderCallback loaderCallback, EqualsCallback equalsCallback)
{
    uint32_t mapIndex = GetIDFromHash(key);
    BaseString* existing = FindInChain(GetBucketHead(mapIndex), key, equalsCallback);
    if (existing != nullptr) {
        return existing;
    }
    // JIT needs to lock before creating the object.
    chainedHashMap_->GetMutex().LockWithThreadState(holder);

    // Re-scan unconditionally: the unlocked scan above cannot prove absence —
    // entries freed by a GC sweep while this thread was unlocked can be
    // reallocated at the same address (ABA), hiding a racing insertion of an
    // equal string.
    existing = FindInChain(GetBucketHead(mapIndex), key, equalsCallback);
    if (existing != nullptr) {
        chainedHashMap_->GetMutex().Unlock();
        return existing;
    }
    BaseString* value = std::invoke(std::forward<LoaderCallback>(loaderCallback));
    // The loader allocates and may park at a GC safepoint while the mutex is
    // held; the STW string-table sweep mutates chains and frees entries without
    // taking the mutex, so every Entry* captured above is stale now. Re-scan
    // from a fresh bucket head before inserting.
    existing = FindInChain(GetBucketHead(mapIndex), key, equalsCallback);
    if (existing != nullptr) {
        chainedHashMap_->GetMutex().Unlock();
        return existing;
    }
    InsertWithLockHeld(mapIndex, value);
    chainedHashMap_->GetMutex().Unlock();
    return value;
}

// StoreOrLoad creates the value via loaderCallback BEFORE taking the mutex:
// the loader allocates and may park at a GC safepoint, and the STW string-table
// sweep mutates bucket chains and frees entries without taking this mutex, so
// neither the lock nor any Entry* may be held across the loader call. It then
// locks, re-checks the bucket, and inserts (the created string is discarded if
// a racing thread interned an equal one first).
template<typename Mutex, typename ThreadHolder, ChainedHashMapConfig::SlotBarrier SlotBarrier>
template<typename LoaderCallback, typename EqualsCallback>
BaseString* ChainedHashMapOperation<Mutex, ThreadHolder, SlotBarrier>::StoreOrLoad(ThreadHolder* holder,
                                                                                   uint32_t key,
                                                                                   LoaderCallback loaderCallback,
                                                                                   EqualsCallback equalsCallback)
{
    uint32_t mapIndex = GetIDFromHash(key);
    common::ReadOnlyHandle<BaseString> str = std::invoke(std::forward<LoaderCallback>(loaderCallback));
    // lock and double-check
    chainedHashMap_->GetMutex().LockWithThreadState(holder);

    BaseString* existing = FindInChain(GetBucketHead(mapIndex), key, equalsCallback);
    if (existing != nullptr) {
        chainedHashMap_->GetMutex().Unlock();
        return existing;
    }

    BaseString* value = *str;
    InsertWithLockHeld(mapIndex, value);
    chainedHashMap_->GetMutex().Unlock();
    return value;
}

// Returns the matched interned string, or nullptr if no equal string exists.
template<typename Mutex, typename ThreadHolder, ChainedHashMapConfig::SlotBarrier SlotBarrier>
template<typename ReadBarrier>
BaseString* ChainedHashMapOperation<Mutex, ThreadHolder, SlotBarrier>::Load(ReadBarrier&& readBarrier,
                                                                            uint32_t key,
                                                                            BaseString* value)
{
    uint32_t mapIndex = GetIDFromHash(key);
    return FindInChain(GetBucketHead(mapIndex), key, [&](BaseString* oldValue) {
        return BaseString::StringsAreEqual(std::forward<ReadBarrier>(readBarrier), oldValue, value);
    });
}

// Looks up an interned string equal to string[offset, offset + utf8Len).
template<typename Mutex, typename ThreadHolder, ChainedHashMapConfig::SlotBarrier SlotBarrier>
template<typename ReadBarrier>
BaseString* ChainedHashMapOperation<Mutex, ThreadHolder, SlotBarrier>::Load(
    ReadBarrier&& readBarrier,
    uint32_t key,
    const common::ReadOnlyHandle<BaseString>& string,
    uint32_t offset,
    uint32_t utf8Len)
{
    uint32_t mapIndex = GetIDFromHash(key);
    DCHECK_CC(!string.IsEmpty());
    const uint8_t* utf8Data = common::ReadOnlyHandle<LineString>::Cast(string)->GetDataUtf8() + offset;
    return FindInChain(GetBucketHead(mapIndex), key, [&](BaseString* oldValue) {
        return BaseString::StringIsEqualUint8Data(
            std::forward<ReadBarrier>(readBarrier), oldValue, utf8Data, utf8Len, true);
    });
}

// StoreOrLoad recomputes mapIndex from key and obtains the mutex, then
// re-checks the bucket under the lock before inserting.
template<typename Mutex, typename ThreadHolder, ChainedHashMapConfig::SlotBarrier SlotBarrier>
template<bool threadState, typename ReadBarrier, typename HandleType>
BaseString* ChainedHashMapOperation<Mutex, ThreadHolder, SlotBarrier>::StoreOrLoad(ThreadHolder* holder,
                                                                                   ReadBarrier&& readBarrier,
                                                                                   uint32_t key,
                                                                                   HandleType str)
{
    uint32_t mapIndex = GetIDFromHash(key);
    if constexpr (threadState) {
        chainedHashMap_->GetMutex().LockWithThreadState(holder);
    } else {
        chainedHashMap_->GetMutex().Lock();
    }

    BaseString* existing = FindInChain(GetBucketHead(mapIndex), key, [&](BaseString* oldValue) {
        return BaseString::StringsAreEqual(std::forward<ReadBarrier>(readBarrier), oldValue, *str);
    });
    if (existing != nullptr) {
        chainedHashMap_->GetMutex().Unlock();
        return existing;
    }

    BaseString* value = *str;
    InsertWithLockHeld(mapIndex, value);
    chainedHashMap_->GetMutex().Unlock();
    return value;
}

template<typename Mutex, typename ThreadHolder, ChainedHashMapConfig::SlotBarrier SlotBarrier>
template<ChainedHashMapConfig::SlotBarrier Barrier,
         std::enable_if_t<Barrier != ChainedHashMapConfig::NeedSlotBarrierCMC, int>>
bool ChainedHashMapOperation<Mutex, ThreadHolder, SlotBarrier>::CheckWeakRef(const WeakRootVisitor& visitor,
                                                                             Entry* entry)
{
    // Fresh entries inserted during concurrent sweep are to-space tagged and must not be swept.
    if (entry->IsToSpaceObject()) {
        return false;
    }
    panda::ecmascript::TaggedObject* object = reinterpret_cast<panda::ecmascript::TaggedObject*>(
        entry->Value<ChainedHashMapConfig::NoSlotBarrier>());
    auto fwd = visitor(object);
    if (fwd == nullptr) {
        if constexpr (SlotBarrier == ChainedHashMapConfig::NeedSlotBarrier) {
            entry->SetValue(nullptr);
        }
        LOG_COMMON(VERBOSE) << "StringTable: delete string " << std::hex << object;
        return true;
    } else if (fwd != object) {
        entry->SetValue(reinterpret_cast<BaseString*>(fwd));
        LOG_COMMON(VERBOSE) << "StringTable: forward " << std::hex << object << " -> " << fwd;
    }
    return false;
}

template<typename Mutex, typename ThreadHolder, ChainedHashMapConfig::SlotBarrier SlotBarrier>
template<typename ReadBarrier>
bool ChainedHashMapOperation<Mutex, ThreadHolder, SlotBarrier>::CheckValidity(ReadBarrier&& readBarrier,
                                                                              BaseString* value,
                                                                              bool& isValid)
{
    if (value->IsTreeString()) {
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

template<typename Mutex, typename ThreadHolder, ChainedHashMapConfig::SlotBarrier SlotBarrier>
bool ChainedHashMapOperation<Mutex, ThreadHolder, SlotBarrier>::Iter(Entry* node, std::function<bool(Entry*)>& iter)
{
    if (node == nullptr) {
        return true;
    }
    for (Entry* e = node; e != nullptr; e = e->Overflow().load(std::memory_order_acquire)) {
        if (!iter(e)) {
            return false;
        }
    }
    return true;
}

template<typename Mutex, typename ThreadHolder, ChainedHashMapConfig::SlotBarrier SlotBarrier>
template<ChainedHashMapConfig::SlotBarrier Barrier,
         std::enable_if_t<Barrier != ChainedHashMapConfig::NeedSlotBarrier, int>>
bool ChainedHashMapOperation<Mutex, ThreadHolder, SlotBarrier>::CheckWeakRef(const WeakRefFieldVisitor& visitor,
                                                                             Entry* entry)
{
    // RefFieldVisitor expects RefField-sized storage, so Entry::Value cannot be
    // passed directly on 32-bit machines. Copy it through a uint64_t slot.
    if (SlotBarrier == ChainedHashMapConfig::NeedSlotBarrierCMC) {
        uint64_t str = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(entry->Value<SlotBarrier>()));
        bool isAlive = visitor(reinterpret_cast<common::RefField<>&>(str));
        entry->SetValue(reinterpret_cast<BaseString*>(static_cast<uintptr_t>(str)));
        return isAlive;
    }
    uint64_t str = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(entry->Value<SlotBarrier>()));
    bool isAlive = visitor(reinterpret_cast<common::RefField<>&>(str));
    if (!isAlive) {
        return true;
    }
    entry->SetValue(reinterpret_cast<BaseString*>(static_cast<uintptr_t>(str)));
    return false;
}

template<typename Mutex, typename ThreadHolder, ChainedHashMapConfig::SlotBarrier SlotBarrier>
template<ChainedHashMapConfig::SlotBarrier Barrier,
         std::enable_if_t<Barrier == ChainedHashMapConfig::NeedSlotBarrierCMC, int>>
bool ChainedHashMapOperation<Mutex, ThreadHolder, SlotBarrier>::ClearNodeFromGC(
    Entry* parent, const WeakRefFieldVisitor& visitor, std::vector<Entry*>& waitDeleteEntries)
{
    // Processing the overflow linked list
    for (Entry *prev = nullptr, *current = parent; current != nullptr;
         current = current->Overflow().load(std::memory_order_acquire)) {
        if (!CheckWeakRef(visitor, current) && prev != nullptr) {
            prev->Overflow().store(current->Overflow().load(std::memory_order_acquire), std::memory_order_release);
            waitDeleteEntries.push_back(current);
        } else {
            prev = current;
        }
    }
    return false;
}

template<typename Mutex, typename ThreadHolder, ChainedHashMapConfig::SlotBarrier SlotBarrier>
template<ChainedHashMapConfig::SlotBarrier Barrier,
         std::enable_if_t<Barrier == ChainedHashMapConfig::NoSlotBarrier, int>>
bool ChainedHashMapOperation<Mutex, ThreadHolder, SlotBarrier>::ClearNodeFromGC(Entry* parent,
                                                                                uint32_t index,
                                                                                const WeakRefFieldVisitor& visitor)
{
    Entry* entry = parent;
    // Process the overflow linked list.
    for (Entry *prev = nullptr, *current = entry->Overflow().load(std::memory_order_relaxed); current != nullptr;) {
        Entry* next = current->Overflow().load(std::memory_order_relaxed);
        if (CheckWeakRef(visitor, current)) {
            // Remove the node from the linked list.
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
    // Process the head entry node.
    if (CheckWeakRef(visitor, entry)) {
        Entry* e = entry->Overflow().load(std::memory_order_relaxed);
        if (e == nullptr) {
            // Delete the empty entry node and update the bucket head.
            delete entry;
            chainedHashMap_->StoreBucket(index, nullptr);
            return true;
        }
        // Delete the entry node and update the bucket head.
        delete entry;
        chainedHashMap_->StoreBucket(index, e);
    }
    return false;
}

template<typename Mutex, typename ThreadHolder, ChainedHashMapConfig::SlotBarrier SlotBarrier>
template<ChainedHashMapConfig::SlotBarrier Barrier,
         std::enable_if_t<Barrier == ChainedHashMapConfig::NeedSlotBarrier, int>>
bool ChainedHashMapOperation<Mutex, ThreadHolder, SlotBarrier>::ClearNodeFromGC(
    Entry* parent,
    uint32_t index,
    const WeakRootVisitor& visitor,
    std::vector<Entry*>& waitDeleteEntries,
    std::vector<ChainedHashMapSlotCheckInfo>& waitCheckAndFreeHeadEntries)
{
    Entry* entry = parent;
    // Processing the overflow linked list
    for (Entry *prev = nullptr, *current = entry; current != nullptr;
         current = current->Overflow().load(std::memory_order_acquire)) {
        if (CheckWeakRef(visitor, current) && prev != nullptr) {
            prev->Overflow().store(current->Overflow().load(std::memory_order_acquire), std::memory_order_release);
            waitDeleteEntries.push_back(current);
        } else {
            prev = current;
        }
    }
    if (entry->Value<ChainedHashMapConfig::NoSlotBarrier>() == nullptr) {
        waitCheckAndFreeHeadEntries.push_back(std::make_tuple(index, entry));
    }
    return false;
}

template<typename Mutex, typename ThreadHolder, ChainedHashMapConfig::SlotBarrier SlotBarrier>
template<ChainedHashMapConfig::SlotBarrier Barrier,
         std::enable_if_t<Barrier == ChainedHashMapConfig::NoSlotBarrier
                              || Barrier == ChainedHashMapConfig::NoSlotBarrierDynamic,
                          int>>
bool ChainedHashMapOperation<Mutex, ThreadHolder, SlotBarrier>::ClearNodeFromGC(Entry* parent,
                                                                                uint32_t index,
                                                                                const WeakRootVisitor& visitor)
{
    Entry* entry = parent;
    // Process the overflow linked list.
    for (Entry *prev = nullptr, *current = entry->Overflow().load(std::memory_order_relaxed); current != nullptr;) {
        Entry* next = current->Overflow().load(std::memory_order_relaxed);
        if (CheckWeakRef(visitor, current)) {
            // Remove the node from the linked list.
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
    // Process the head entry node.
    if (CheckWeakRef(visitor, entry)) {
        Entry* e = entry->Overflow().load(std::memory_order_relaxed);
        if (e == nullptr) {
            // Delete the empty entry node and update the bucket head.
            delete entry;
            chainedHashMap_->StoreBucket(index, nullptr);
            return true;
        }
        // Delete the entry node and update the bucket head.
        delete entry;
        chainedHashMap_->StoreBucket(index, e);
    }
    return false;
}
}
#endif //ECMASCRIPT_STRING_TABLE_CHAINED_HASH_MAP_INL_H
