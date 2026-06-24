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

#ifndef ECMASCRIPT_STRING_TABLE_CHAINED_HASH_MAP_H
#define ECMASCRIPT_STRING_TABLE_CHAINED_HASH_MAP_H

#include <memory>
#include <new>

#include "base/common.h"
#include "common_components/heap/heap.h"
#include "common_components/log/log.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/string/base_string-inl.h"
#include "ecmascript/string/base_string.h"
#include "objects/readonly_handle.h"

namespace panda::ecmascript {
class TaggedObject;
class ChainedHashMapConfig {
public:
    static constexpr uint32_t SWEEP_PARTITION_BIT = 11U;
    static constexpr uint32_t SWEEP_PARTITION_COUNT = (1 << SWEEP_PARTITION_BIT);
    // MAP_BIT (bucket-count exponent) is configurable at runtime via the system
    // parameter "const.ark.string_table_map_bit"; see GetConfiguredMapBit().
    // MIN_MAP_BIT == SWEEP_PARTITION_BIT keeps MAP_SIZE a multiple of
    // SWEEP_PARTITION_COUNT so sweeping can split the map evenly.
    // MAX_MAP_BIT bounds the array to 1 << 24 buckets and keeps the index within uint32_t.
    static constexpr uint32_t DEFAULT_MAP_BIT = 16U;
    static constexpr uint32_t MIN_MAP_BIT = SWEEP_PARTITION_BIT;
    static constexpr uint32_t MAX_MAP_BIT = 24U;
    enum SlotBarrier { NeedSlotBarrier, NeedSlotBarrierCMC, NoSlotBarrier, NoSlotBarrierDynamic };
};

// Defined in chained_hash_map.cpp to keep the OHOS parameter dependency out of
// this widely-included header.
uint32_t GetConfiguredMapBit();

class ChainedHashMapEntry final {
public:
    // Do not use 57-64bits, HWAsan uses 57-64 bits as pointer tag
    static constexpr uint64_t POINTER_LENGTH = 48;
    // Used by CMC concurrent sweep to tag entries inserted during sweeping.
    // The tag is cleared during sweep finalization.
    static constexpr uint64_t TO_SPACE_TAG_MASK = 1ULL << POINTER_LENGTH;

    using Pointer = common::BitField<uint64_t, 0, POINTER_LENGTH>;
    // BitField view of the CMC concurrent-sweep to-space tag.
    using ToSpaceBit = Pointer::NextFlag;

    ChainedHashMapEntry(BaseString* v): overflow_(nullptr)
    {
        // CMC GC assumes strings are non-young objects and skips them during young GC.
        ASSERT_LOGF(common::Heap::GetHeap().IsHeapAddress(v) ? common::Heap::GetHeap().InRecentSpace(v) == false
                                                             : true,
                    "Violate CMC-GC assumption: should not be young object");
        bitField_ = reinterpret_cast<uint64_t>(v);
    }

    // Build an entry with an optional CMC concurrent-sweep to-space tag.
    ChainedHashMapEntry(BaseString* v, bool toSpaceTag): overflow_(nullptr)
    {
        if (toSpaceTag) {
            bitField_ = (TO_SPACE_TAG_MASK | reinterpret_cast<uint64_t>(v));
        } else {
            bitField_ = reinterpret_cast<uint64_t>(v);
        }
    }

    template<ChainedHashMapConfig::SlotBarrier SlotBarrier>
    BaseString* Value() const
    {
        uint64_t value = Pointer::Decode(bitField_);
        if constexpr (SlotBarrier == ChainedHashMapConfig::NoSlotBarrier ||
                      SlotBarrier == ChainedHashMapConfig::NoSlotBarrierDynamic) {
            return reinterpret_cast<BaseString*>(static_cast<uintptr_t>(value));
        }
        if constexpr (SlotBarrier == ChainedHashMapConfig::NeedSlotBarrier) {
            return reinterpret_cast<BaseString*>(
                static_cast<uintptr_t>(Barriers::ReadBarrierForStringTableSlot(value)));
        }
        return reinterpret_cast<BaseString*>(common::Heap::GetBarrier().ReadStringTableStaticRef(
            *reinterpret_cast<common::RefField<false>*>((void*)(&value))));
    }

    void SetValue(BaseString* v)
    {
        // CMC GC assumes strings are non-young objects and skips them during young GC.
        ASSERT_LOGF(common::Heap::GetHeap().IsHeapAddress(v) ? common::Heap::GetHeap().InRecentSpace(v) == false
                                                             : true,
                    "Violate CMC-GC assumption: should not be young object");
        bitField_ = reinterpret_cast<uint64_t>(v);
    }

    // Clear the CMC concurrent-sweep to-space tag during sweep finalization.
    void ClearToSpaceTag()
    {
        bitField_ = bitField_ & (~TO_SPACE_TAG_MASK);
    }

    std::atomic<ChainedHashMapEntry*>& Overflow()
    {
        return overflow_;
    }

    // Query whether an entry was inserted during CMC concurrent sweeping.
    bool IsToSpaceObject() const
    {
        return ToSpaceBit::Decode(bitField_);
    }

private:
    uint64_t bitField_;
    std::atomic<ChainedHashMapEntry*> overflow_;
};

using ChainedHashMapSlotCheckInfo = std::tuple<uint32_t, ChainedHashMapEntry*>;

template<typename Mutex>
class ChainedHashMap {
public:
    using Entry = ChainedHashMapEntry;
    ChainedHashMap()
        : mapBit_(GetConfiguredMapBit()),
          mapSize_(1U << mapBit_),
          mapBitMask_(mapSize_ - 1U),
          sweepPartitionToMapShift_(mapBit_ - ChainedHashMapConfig::SWEEP_PARTITION_BIT),
          buckets_(nullptr)
    {
        // MIN_MAP_BIT == SWEEP_PARTITION_BIT guarantees this sweep invariant.
        ASSERT(mapBit_ >= ChainedHashMapConfig::SWEEP_PARTITION_BIT);

        buckets_.reset(new (std::nothrow) std::atomic<ChainedHashMapEntry*>[mapSize_]);
        if (buckets_ == nullptr) {
            LOG_COMMON(FATAL) << "ChainedHashMap: failed to allocate bucket array";
            UNREACHABLE_CC();
        }
        for (uint32_t i = 0; i < mapSize_; i++) {
            buckets_[i].store(nullptr, std::memory_order_relaxed);
        }
    }

    ~ChainedHashMap()
    {
        Clear();
    };

    uint32_t GetMapSize() const
    {
        return mapSize_;
    }

    uint32_t GetMapBitMask() const
    {
        return mapBitMask_;
    }

    // Calls fn(bucketHead, bucketIndex) for every non-empty bucket in the given
    // sweep partition. Bucket heads are loaded relaxed: sweep callers run either
    // stop-the-world or under the deferred-reclamation protocol.
    template<typename Fn>
    void ForEachBucketHeadInPartition(uint32_t partitionID, Fn&& fn);

    void Clear()
    {
        for (uint32_t i = 0; i < mapSize_; i++) {
            // Atomically replace the bucket head with nullptr and obtain the old head.
            ChainedHashMapEntry* oldBucketHead = buckets_[i].exchange(nullptr, std::memory_order_relaxed);
            DeleteEntryList(oldBucketHead);
        }
    }

    // Access bucket heads for production sweep paths and tests.
    const std::atomic<ChainedHashMapEntry*>& GetBucket(uint32_t index) const
    {
        DCHECK_CC(index < mapSize_);
        return buckets_[index];
    }

    // Caller must hold the mutex (or be in a single-threaded GC/debugger context),
    // and oldEntry must have been re-read from the bucket after the caller's last
    // GC-capable operation: the STW string-table sweep frees entries and rewrites
    // buckets without taking the mutex, so Entry pointers do not survive a safepoint.
    // Performs two sequential release stores without CAS.
    void Insert(const uint32_t mapIndex, Entry* oldEntry, Entry* newEntry)
    {
        newEntry->Overflow().store(oldEntry, std::memory_order_release);
        buckets_[mapIndex].store(newEntry, std::memory_order_release);
    }

    void StoreBucket(uint32_t mapIndex, Entry* newEntry)
    {
        buckets_[mapIndex].store(newEntry, std::memory_order_relaxed);
    }

    uint32_t GetInuseCount() const
    {
        return inuseCount_.load();
    }

    void IncreaseInuseCount()
    {
        inuseCount_.fetch_add(1);
    }

    void DecreaseInuseCount()
    {
        inuseCount_.fetch_sub(1);
    }

    Mutex& GetMutex()
    {
        return mu_;
    }

    void AddWaitFreeEntry(Entry* entry)
    {
        waitFreeEntries_.push_back(entry);
    }

    void CleanUp()
    {
        for (const ChainedHashMapEntry* entry: waitFreeEntries_) {
            delete entry;
        }
        waitFreeEntries_.clear();
    }

    void CheckAndFreeHeadEntries(std::vector<ChainedHashMapSlotCheckInfo>& waitCheckAndFreeHeadEntries)
    {
        for (auto info: waitCheckAndFreeHeadEntries) {
            uint32_t index = std::get<0>(info); // 0: index
            ChainedHashMapEntry* current = buckets_[index].load(std::memory_order_relaxed);
            ChainedHashMapEntry* oldHead = std::get<1>(info); // 1: oldHead
            ASSERT(oldHead->Value<ChainedHashMapConfig::NoSlotBarrier>() == nullptr);
            if (current == oldHead) {
                buckets_[index].store(oldHead->Overflow().load(std::memory_order_relaxed),
                                      std::memory_order_relaxed);
                delete oldHead;
            }
        }
        waitCheckAndFreeHeadEntries.clear();
    }

    // CMC GC only: can be removed after CMC GC is adapted.
    void AddWaitClearToSpaceTagEntry(Entry* entry)
    {
        waitClearToSpaceTagEntries_.push_back(entry);
    }

    // CMC GC only: can be removed after CMC GC is adapted.
    void ClearToSpaceTagForFreshEntries()
    {
        for (ChainedHashMapEntry* entry: waitClearToSpaceTagEntries_) {
            entry->ClearToSpaceTag();
        }
        waitClearToSpaceTagEntries_.clear();
    }

    bool IsSweeping() const
    {
        return isSweeping_;
    }

    void StartSweeping()
    {
        if (g_isEnableCMCGC) {
            GetMutex().Lock();
            isSweeping_ = true;
            GetMutex().Unlock();
        } else {
            isSweeping_ = true;
        }
    }

    void FinishSweeping()
    {
        if (g_isEnableCMCGC) {
            GetMutex().Lock();
            isSweeping_ = false;
            GetMutex().Unlock();
        } else {
            isSweeping_ = false;
        }
    }

private:
    // Bucket-count exponent and derived sizes, fixed at construction from
    // GetConfiguredMapBit(). Declared before buckets_ so the constructor's member
    // initialization order is well-defined.
    const uint32_t mapBit_;
    const uint32_t mapSize_;
    const uint32_t mapBitMask_;
    const uint32_t sweepPartitionToMapShift_;
    std::unique_ptr<std::atomic<ChainedHashMapEntry*>[]> buckets_;
    static void DeleteEntryList(Entry* entry)
    {
        while (entry != nullptr) {
            Entry* next = entry->Overflow().exchange(nullptr, std::memory_order_relaxed);
            delete entry;
            entry = next;
        }
    }

    Mutex mu_;
    std::vector<Entry*> waitFreeEntries_ {};
    // CMC GC only: can be removed after CMC GC is adapted.
    std::vector<Entry*> waitClearToSpaceTagEntries_ {};
    std::atomic<uint32_t> inuseCount_ {0};
    bool isSweeping_ {false};
};

template<typename Mutex, typename ThreadHolder, ChainedHashMapConfig::SlotBarrier SlotBarrier>
class ChainedHashMapOperation {
public:
    using WeakRefFieldVisitor = std::function<bool(common::RefField<>&)>;
    using WeakRootVisitor = std::function<panda::ecmascript::TaggedObject*(panda::ecmascript::TaggedObject* p)>;
    using Entry = ChainedHashMapEntry;
    explicit ChainedHashMapOperation(ChainedHashMap<Mutex>* chainedHashMap): chainedHashMap_(chainedHashMap) { }

    ~ChainedHashMapOperation() { }

#if ECMASCRIPT_ENABLE_TRACE_STRING_TABLE
    class StringTableTracer {
    public:
        static constexpr uint32_t DUMP_THRESHOLD = 40000;
        static StringTableTracer& GetInstance()
        {
            static StringTableTracer tracer;
            return tracer;
        }

        NO_COPY_SEMANTIC_CC(StringTableTracer);
        NO_MOVE_SEMANTIC_CC(StringTableTracer);

        void TraceFindSuccess(uint32_t chainDepth)
        {
            totalDepth_.fetch_add(chainDepth, std::memory_order_relaxed);
            uint64_t currentSuccess = totalSuccessNum_.fetch_add(1, std::memory_order_relaxed) + 1;
            if (currentSuccess >= lastDumpPoint_.load(std::memory_order_relaxed) + DUMP_THRESHOLD) {
                DumpWithLock(currentSuccess);
            }
        }

        void TraceFindFail()
        {
            totalFailNum_.fetch_add(1, std::memory_order_relaxed);
        }

    private:
        StringTableTracer() = default;

        void DumpWithLock(uint64_t triggerPoint)
        {
            std::lock_guard<std::mutex> lock(mu_);

            if (triggerPoint >= lastDumpPoint_.load(std::memory_order_relaxed) + DUMP_THRESHOLD) {
                lastDumpPoint_ = triggerPoint;
                DumpInfo();
            }
        }

        void DumpInfo() const
        {
            uint64_t depth = totalDepth_.load(std::memory_order_relaxed);
            uint64_t success = totalSuccessNum_.load(std::memory_order_relaxed);
            uint64_t fail = totalFailNum_.load(std::memory_order_relaxed);

            double avgDepth = (static_cast<double>(depth) / success);

            LOG_COMMON(INFO) << "------------------------------------------------------------"
                             << "---------------------------------------------------------";
            LOG_COMMON(INFO) << "StringTableTotalSuccessFindNum: " << success;
            LOG_COMMON(INFO) << "StringTableTotalInsertNum: " << fail;
            LOG_COMMON(INFO) << "StringTableAverageDepth: " << avgDepth;
            LOG_COMMON(INFO) << "------------------------------------------------------------"
                             << "---------------------------------------------------------";
        }

        std::mutex mu_;
        std::atomic<uint64_t> totalDepth_ {0};
        std::atomic<uint64_t> totalSuccessNum_ {0};
        std::atomic<uint64_t> totalFailNum_ {0};
        std::atomic<uint64_t> lastDumpPoint_ {0};
    };

    void TraceFindSuccessDepth(uint32_t chainDepth)
    {
        StringTableTracer::GetInstance().TraceFindSuccess(chainDepth);
    }

    void TraceFindFail()
    {
        StringTableTracer::GetInstance().TraceFindFail();
    }
#endif
    template<typename ReadBarrier>
    BaseString* Load(ReadBarrier&& readBarrier, uint32_t key, BaseString* value);

    template<bool threadState = true, typename ReadBarrier, typename HandleType>
    BaseString* StoreOrLoad(ThreadHolder* holder, ReadBarrier&& readBarrier, uint32_t key, HandleType str);

    template<typename ReadBarrier>
    BaseString* Load(ReadBarrier&& readBarrier,
                     uint32_t key,
                     const common::ReadOnlyHandle<BaseString>& string,
                     uint32_t offset,
                     uint32_t utf8Len);

    template<typename LoaderCallback, typename EqualsCallback>
    BaseString* StoreOrLoad(ThreadHolder* holder,
                            uint32_t key,
                            LoaderCallback loaderCallback,
                            EqualsCallback equalsCallback);

    template<bool IsCheck, typename ReadBarrier>
    BaseString* Load(ReadBarrier&& readBarrier, uint32_t key, BaseString* value);

    template<bool IsLock, typename LoaderCallback, typename EqualsCallback>
    BaseString* LoadOrStore(ThreadHolder* holder,
                            uint32_t key,
                            LoaderCallback loaderCallback,
                            EqualsCallback equalsCallback);

    template<typename LoaderCallback, typename EqualsCallback>
    BaseString* LoadOrStoreForJit(ThreadHolder* holder,
                                  uint32_t key,
                                  LoaderCallback loaderCallback,
                                  EqualsCallback equalsCallback);

    static constexpr uint32_t ID_SHIFT_RIGHT = 11;
    static constexpr uint32_t ID_SHIFT_LEFT = 9;

    uint32_t GetIDFromHash(uint32_t hash) const
    {
        hash ^= hash >> ID_SHIFT_RIGHT;
        hash ^= hash << ID_SHIFT_LEFT;
        return (hash & chainedHashMap_->GetMapBitMask());
    }

    inline Entry* GetBucketHead(uint32_t mapIndex)
    {
        return chainedHashMap_->GetBucket(mapIndex).load(std::memory_order_acquire);
    }

    // All other threads have stopped due to the gc and Clear phases.
    // Therefore, the operations related to atoms in ClearNodeFromGc and Clear use std::memory_order_relaxed,
    // which ensures atomicity but does not guarantee memory order
    template<ChainedHashMapConfig::SlotBarrier Barrier = SlotBarrier,
             std::enable_if_t<Barrier == ChainedHashMapConfig::NeedSlotBarrierCMC, int> = 0>
    bool ClearNodeFromGC(Entry* parent, const WeakRefFieldVisitor& visitor, std::vector<Entry*>& waitDeleteEntries);

    template<ChainedHashMapConfig::SlotBarrier Barrier = SlotBarrier,
             std::enable_if_t<Barrier == ChainedHashMapConfig::NoSlotBarrier, int> = 0>
    bool ClearNodeFromGC(Entry* parent, uint32_t index, const WeakRefFieldVisitor& visitor);

    template<ChainedHashMapConfig::SlotBarrier Barrier = SlotBarrier,
             std::enable_if_t<Barrier == ChainedHashMapConfig::NeedSlotBarrier, int> = 0>
    bool ClearNodeFromGC(Entry* parent,
                         uint32_t index,
                         const WeakRootVisitor& visitor,
                         std::vector<Entry*>& waitDeleteEntries,
                         std::vector<ChainedHashMapSlotCheckInfo>& waitCheckAndFreeHeadEntries);

    template<ChainedHashMapConfig::SlotBarrier Barrier = SlotBarrier,
             std::enable_if_t<Barrier == ChainedHashMapConfig::NoSlotBarrier ||
                                  Barrier == ChainedHashMapConfig::NoSlotBarrierDynamic,
                              int> = 0>
    bool ClearNodeFromGC(Entry* parent, uint32_t index, const WeakRootVisitor& visitor);

    // Iterator
    template<typename ReadBarrier>
    void Range(ReadBarrier&& readBarrier, bool& isValid)
    {
        std::function<bool(Entry*)> iter = [readBarrier, &isValid, this](Entry* node) {
            BaseString* value = node->Value<SlotBarrier>();
            if (!IsNull(value) && !this->CheckValidity(readBarrier, value, isValid)) {
                return false;
            }
            return true;
        };
        Range(iter);
    }

    void Range(std::function<bool(Entry*)>& iter)
    {
        for (uint32_t i = 0; i < chainedHashMap_->GetMapSize(); i++) {
            if (!Iter(chainedHashMap_->GetBucket(i).load(std::memory_order_acquire), iter)) {
                return;
            }
        }
    }

private:
    ChainedHashMap<Mutex>* chainedHashMap_ {nullptr};

    // Walks the bucket chain from head, calling pred on every live entry whose
    // hash equals key; returns the first entry pred accepts, or nullptr.
    // Lock-free traversal: safe with or without the map mutex held.
    template<typename Pred>
    BaseString* FindInChain(Entry* head, uint32_t key, Pred&& pred);

    // Publishes value as a new interned entry at the head of its bucket.
    // Caller must hold the map mutex; the bucket head is re-read here, and no
    // code on this path can reach a GC safepoint, so the Insert() freshness
    // precondition holds by construction.
    void InsertWithLockHeld(uint32_t mapIndex, BaseString* value);

    bool Iter(Entry* node, std::function<bool(Entry*)>& iter);

    template<ChainedHashMapConfig::SlotBarrier Barrier = SlotBarrier,
             std::enable_if_t<Barrier != ChainedHashMapConfig::NeedSlotBarrier, int> = 0>
    bool CheckWeakRef(const WeakRefFieldVisitor& visitor, Entry* entry);

    template<ChainedHashMapConfig::SlotBarrier Barrier = SlotBarrier,
             std::enable_if_t<Barrier != ChainedHashMapConfig::NeedSlotBarrierCMC, int> = 0>
    bool CheckWeakRef(const WeakRootVisitor& visitor, Entry* entry);

    template<typename ReadBarrier>
    bool CheckValidity(ReadBarrier&& readBarrier, BaseString* value, bool& isValid);

    BaseString* GetValue(Entry* entry) const
    {
        if constexpr (SlotBarrier == ChainedHashMapConfig::NeedSlotBarrier) {
            if (!chainedHashMap_->IsSweeping()) {
                return entry->Value<ChainedHashMapConfig::NoSlotBarrier>();
            }
            // Fresh entries inserted during concurrent sweep are already to-space tagged.
            // Read them without a slot barrier until sweep finalization clears the tag.
            if (entry->IsToSpaceObject()) {
                return entry->Value<ChainedHashMapConfig::NoSlotBarrier>();
            }
        }
        return entry->Value<SlotBarrier>();
    }

    Entry* NewEntry(BaseString* value)
    {
        if constexpr (SlotBarrier != ChainedHashMapConfig::NeedSlotBarrier) {
            return new Entry(value);
        }
        if (!chainedHashMap_->IsSweeping()) {
            return new Entry(value);
        }
        // Fresh entries inserted during concurrent sweep carry the to-space tag
        // until sweep finalization.
        Entry* newEntry = new Entry(value, true);
        chainedHashMap_->AddWaitClearToSpaceTagEntry(newEntry);
        return newEntry;
    }

    constexpr static bool IsNull(BaseString* value)
    {
        if constexpr (SlotBarrier == ChainedHashMapConfig::NoSlotBarrier) {
            return false;
        }
        return value == nullptr;
    }

    constexpr Entry* PruneHead(Entry* entry)
    {
        if constexpr (SlotBarrier != ChainedHashMapConfig::NeedSlotBarrierCMC) {
            return entry;
        }
        if (chainedHashMap_->IsSweeping()) {
            return entry;
        }
        if (entry == nullptr) {
            return entry;
        }
        if (entry->Value<SlotBarrier>() == nullptr) {
            chainedHashMap_->AddWaitFreeEntry(entry);
            return entry->Overflow().load(std::memory_order_acquire);
        }
        return entry;
    }
};

template<typename Mutex>
class ChainedHashMapInUseScope {
public:
    ChainedHashMapInUseScope(ChainedHashMap<Mutex>* chainedHashMap): chainedHashMap_(chainedHashMap)
    {
        chainedHashMap_->IncreaseInuseCount();
    }

    ~ChainedHashMapInUseScope()
    {
        chainedHashMap_->DecreaseInuseCount();
    }

    NO_COPY_SEMANTIC_CC(ChainedHashMapInUseScope);
    NO_MOVE_SEMANTIC_CC(ChainedHashMapInUseScope);

private:
    ChainedHashMap<Mutex>* chainedHashMap_;
};
}
#endif //ECMASCRIPT_STRING_TABLE_CHAINED_HASH_MAP_H
