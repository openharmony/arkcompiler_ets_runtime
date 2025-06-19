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

#ifndef COMMON_COMPONENTS_OBJECTS_STRING_TABLE_HASHTRIEMAP_H
#define COMMON_COMPONENTS_OBJECTS_STRING_TABLE_HASHTRIEMAP_H

#include "common_components/heap/heap.h"
#include "common_components/log/log.h"
#include "common_interfaces/objects/readonly_handle.h"
#include "common_interfaces/objects/base_string.h"

namespace panda::ecmascript {
class TaggedObject;
}

namespace common {
class TrieMapConfig {
public:
    static constexpr uint32_t N_CHILDREN_LOG2 = 3U;
    static constexpr uint32_t TOTAL_HASH_BITS = 32U;

    static constexpr uint32_t N_CHILDREN = 1 << N_CHILDREN_LOG2;
    static constexpr uint32_t N_CHILDREN_MASK = N_CHILDREN - 1U;

    static constexpr uint32_t INDIRECT_SIZE = 8U; // 8: 2^3
    static constexpr uint32_t INDIRECT_MASK = INDIRECT_SIZE - 1U;

    enum SlotBarrier {
        NeedSlotBarrier,
        NoSlotBarrier,
    };
};

class HashTrieMapEntry;
class HashTrieMapIndirect;

class HashTrieMapNode {
public:
    explicit HashTrieMapNode(bool isEntry) : isEntry_(isEntry) {}

    bool IsEntry() const
    {
        return isEntry_;
    }

    HashTrieMapEntry* AsEntry();
    HashTrieMapIndirect* AsIndirect();

private:
    const bool isEntry_;
};

class HashTrieMapEntry final : public HashTrieMapNode {
public:
    HashTrieMapEntry(uint32_t k, BaseString* v) : HashTrieMapNode(true), key_(k), value_(v), overflow_(nullptr) {}

    uint32_t Key() const
    {
        return key_;
    }

    template <TrieMapConfig::SlotBarrier SlotBarrier>
    BaseString* Value() const
    {
        if constexpr (SlotBarrier == TrieMapConfig::NoSlotBarrier) {
            return value_;
        }
        return reinterpret_cast<BaseString*>(Heap::GetBarrier().ReadStringTableStaticRef(
            *reinterpret_cast<RefField<false>*>((void*)(&value_))));
    }

    void SetValue(BaseString* v)
    {
        value_ = v;
    }

    std::atomic<HashTrieMapEntry*>& Overflow()
    {
        return overflow_;
    }

private:
    uint32_t key_;
    BaseString* value_;
    std::atomic<HashTrieMapEntry*> overflow_;
};

class HashTrieMapIndirect final : public HashTrieMapNode {
public:
    std::array<std::atomic<HashTrieMapNode*>, TrieMapConfig::INDIRECT_SIZE> children_{};
    HashTrieMapIndirect* parent_;

    explicit HashTrieMapIndirect(HashTrieMapIndirect* p = nullptr) : HashTrieMapNode(false), parent_(p) {};

    ~HashTrieMapIndirect()
    {
        for (std::atomic<HashTrieMapNode*>& child : children_) {
            HashTrieMapNode* node = child.exchange(nullptr, std::memory_order_relaxed);
            if (node == nullptr) {
                continue;
            }
            if (!node->IsEntry()) {
                delete node->AsIndirect();
                continue;
            }
            HashTrieMapEntry* e = node->AsEntry();
            // Clear overflow chain
            for (HashTrieMapEntry* current = e->Overflow().exchange(nullptr, std::memory_order_relaxed); current !=
                 nullptr
                 ;) {
                // atom get the next node and break the link
                HashTrieMapEntry* next = current->Overflow().exchange(nullptr, std::memory_order_relaxed);
                // deletion of the current node
                delete current;
                // move to the next node
                current = next;
            }
            delete e;
        }
    }
};

struct HashTrieMapLoadResult {
    BaseString* value;
    HashTrieMapIndirect* current;
    uint32_t hashShift;
    std::atomic<HashTrieMapNode*>* slot;
};

inline HashTrieMapEntry* HashTrieMapNode::AsEntry()
{
    ASSERT(isEntry_ && "HashTrieMap: called entry on non-entry node");
    return static_cast<HashTrieMapEntry*>(this);
}

inline HashTrieMapIndirect* HashTrieMapNode::AsIndirect()
{
    ASSERT(!isEntry_ && "HashTrieMap: called indirect on entry node");
    return static_cast<HashTrieMapIndirect*>(this);
}

template <typename Mutex, typename ThreadHolder, TrieMapConfig::SlotBarrier SlotBarrier>
class HashTrieMap {
public:
    using WeakRefFieldVisitor = std::function<bool(RefField<>&)>;
    using WeakRootVisitor = std::function<panda::ecmascript::TaggedObject *(panda::ecmascript::TaggedObject* p)>;
    using Node = HashTrieMapNode;
    using Indirect = HashTrieMapIndirect;
    using Entry = HashTrieMapEntry;
    using LoadResult = HashTrieMapLoadResult;
    HashTrieMap()
    {
        root_.store(new Indirect(nullptr), std::memory_order_relaxed);
    }

    ~HashTrieMap()
    {
        Clear();
    };

    template <typename ReadBarrier>
    LoadResult Load(ReadBarrier&& readBarrier, const uint32_t key, BaseString* value);

    template <bool threadState = true, typename ReadBarrier, typename HandleType>
    BaseString* StoreOrLoad(ThreadHolder* holder, ReadBarrier&& readBarrier, const uint32_t key, LoadResult loadResult,
                            HandleType str);
    template <typename ReadBarrier>
    LoadResult Load(ReadBarrier&& readBarrier, const uint32_t key, const ReadOnlyHandle<BaseString>& string,
                    uint32_t offset, uint32_t utf8Len);
    template <typename LoaderCallback, typename EqualsCallback>
    BaseString* StoreOrLoad(ThreadHolder* holder, const uint32_t key, LoadResult loadResult,
                            LoaderCallback loaderCallback, EqualsCallback equalsCallback);

    template <bool IsCheck, typename ReadBarrier>
    BaseString* Load(ReadBarrier&& readBarrier, const uint32_t key, BaseString* value);
    template <bool IsLock, typename LoaderCallback, typename EqualsCallback>
    BaseString* LoadOrStore(ThreadHolder* holder, const uint32_t key, LoaderCallback loaderCallback,
                            EqualsCallback equalsCallback);

    template <typename LoaderCallback, typename EqualsCallback>
    BaseString* LoadOrStoreForJit(ThreadHolder* holder, const uint32_t key, LoaderCallback loaderCallback,
                                  EqualsCallback equalsCallback);

    // All other threads have stopped due to the gc and Clear phases.
    // Therefore, the operations related to atoms in ClearNodeFromGc and Clear use std::memory_order_relaxed,
    // which ensures atomicity but does not guarantee memory order
    template <TrieMapConfig::SlotBarrier Barrier = SlotBarrier,
              std::enable_if_t<Barrier == TrieMapConfig::NeedSlotBarrier, int>  = 0>
    bool ClearNodeFromGC(Indirect* parent, int index, const WeakRefFieldVisitor& visitor,
                         std::vector<Entry*>& waitDeleteEntries);

    template <TrieMapConfig::SlotBarrier Barrier = SlotBarrier,
              std::enable_if_t<Barrier == TrieMapConfig::NoSlotBarrier, int>  = 0>
    bool ClearNodeFromGC(Indirect* parent, int index, const WeakRefFieldVisitor& visitor);

    template <TrieMapConfig::SlotBarrier Barrier = SlotBarrier,
              std::enable_if_t<Barrier == TrieMapConfig::NoSlotBarrier, int>  = 0>
    bool ClearNodeFromGC(Indirect* parent, int index, const WeakRootVisitor& visitor);
    // Iterator
    template <typename ReadBarrier>
    void Range(ReadBarrier&& readBarrier, bool& isValid)
    {
        Iter(std::forward<ReadBarrier>(readBarrier), root_.load(std::memory_order_relaxed), isValid);
    }

    void Clear()
    {
        // The atom replaces the root node with nullptr and obtains the old root node
        Indirect* oldRoot = root_.exchange(nullptr, std::memory_order_relaxed);
        if (oldRoot != nullptr) {
            // Clear the entire HashTreeMap based on the Indirect destructor
            delete oldRoot;
        }
    }

    // ut used
    const std::atomic<Indirect*>& GetRoot() const
    {
        return root_;
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

    template <TrieMapConfig::SlotBarrier Barrier = SlotBarrier,
              std::enable_if_t<Barrier == TrieMapConfig::NeedSlotBarrier, int>  = 0>
    void CleanUp()
    {
        for (const HashTrieMapEntry* entry : waitFreeEntries_) {
            delete entry;
        }
        waitFreeEntries_.clear();
    }

    void StartSweeping()
    {
        GetMutex().Lock();
        isSweeping = true;
        GetMutex().Unlock();
    }

    void FinishSweeping()
    {
        GetMutex().Lock();
        isSweeping = false;
        GetMutex().Unlock();
    }
    std::atomic<Indirect*> root_;
private:
    Mutex mu_;
    std::vector<Entry*> waitFreeEntries_{};
    std::atomic<uint32_t> inuseCount_{0};
    bool isSweeping{false};
    template <bool IsLock>
    Node* Expand(Entry* oldEntry, Entry* newEntry, uint32_t newHash, uint32_t hashShift, Indirect* parent);
    template <typename ReadBarrier>
    void Iter(ReadBarrier&& readBarrier, Indirect* node, bool& isValid);
    bool CheckWeakRef(const WeakRefFieldVisitor& visitor, Entry* entry);
    bool CheckWeakRef(const WeakRootVisitor& visitor, Entry* entry);

    template <typename ReadBarrier>
    bool CheckValidity(ReadBarrier&& readBarrier, BaseString* value, bool& isValid);

    constexpr static bool IsNull(BaseString* value)
    {
        if constexpr (SlotBarrier == TrieMapConfig::NoSlotBarrier) {
            return false;
        }
        return value == nullptr;
    }

    constexpr Entry* PruneHead(Entry* entry)
    {
        if constexpr (SlotBarrier == TrieMapConfig::NoSlotBarrier) {
            return entry;
        }
        if (entry == nullptr || isSweeping) {
            // can't be pruned when sweeping.
            return entry;
        }
        if (entry->Value<SlotBarrier>() == nullptr) {
            waitFreeEntries_.push_back(entry);
            return entry->Overflow();
        }
        return entry;
    }
};

template <typename Mutex, typename ThreadHolder, TrieMapConfig::SlotBarrier SlotBarrier>
class HashTrieMapInUseScope {
public:
    HashTrieMapInUseScope(HashTrieMap<Mutex, ThreadHolder, SlotBarrier>* hashTrieMap) : hashTrieMap_(hashTrieMap)
    {
        hashTrieMap_->IncreaseInuseCount();
    }

    ~HashTrieMapInUseScope()
    {
        hashTrieMap_->DecreaseInuseCount();
    }

    NO_COPY_SEMANTIC(HashTrieMapInUseScope);
    NO_MOVE_SEMANTIC(HashTrieMapInUseScope);

private:
    HashTrieMap<Mutex, ThreadHolder, SlotBarrier>* hashTrieMap_;
};
}
#endif //COMMON_COMPONENTS_OBJECTS_STRING_TABLE_HASHTRIEMAP_H
