/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_STRING_TABLE_H
#define ECMASCRIPT_STRING_TABLE_H

#include <array>
#include "common_components/taskpool/task.h"
#include "ecmascript/daemon/daemon_thread.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/string/base_string_table.h"
#include "ecmascript/string/base_string.h"
#include "ecmascript/string/hashtriemap.h"
#include "ecmascript/string/string_table_internal.h"

namespace panda::ecmascript {
#if ENABLE_NEXT_OPTIMIZATION
class EcmaString;
class EcmaVM;
class JSPandaFile;
class JSThread;

class EcmaStringTable;
class EcmaStringTableCleaner {
public:
    using IteratorPtr = std::shared_ptr<std::atomic<uint32_t>>;
    EcmaStringTableCleaner(EcmaStringTable *stringTable) : stringTable_(stringTable) {}
    ~EcmaStringTableCleaner()
    {
        stringTable_ = nullptr;
    }
    void PostSweepWeakRefTask(const WeakRootVisitor &visitor);
    void JoinAndWaitSweepWeakRefTask(const WeakRootVisitor &visitor);
    void WaitSweepWeakRefTaskFinished();

    void PostConcurrentSweepWeakRefTask(const WeakRootVisitor &visitor);
    void WaitConcurrentSweepWeakRefTaskAndSuspend(JSThread *thread);
    void WaitConcurrentSweepWeakRefTaskAndSuspendByDaemonThread(DaemonThread *dThread);

    bool IsEnableConcurrentSweep() const
    {
        return enableConcurrentSweep_;
    }

    void SetEnableConcurrentSweep(bool enableConcurrentSweep)
    {
        enableConcurrentSweep_ = enableConcurrentSweep;
    }
private:
    NO_COPY_SEMANTIC(EcmaStringTableCleaner);
    NO_MOVE_SEMANTIC(EcmaStringTableCleaner);
    static void ProcessSweepWeakRef(IteratorPtr &iter, EcmaStringTableCleaner *cleaner, const WeakRootVisitor &visitor);
    static void ProcessConcurrentSweepWeakRef(IteratorPtr &iter, EcmaStringTableCleaner *cleaner,
                                              const WeakRootVisitor &visitor);
    void StartSweepWeakRefTask();
    void WaitSweepWeakRefTask();
    void WaitConcurrentSweepWeakRefTask();
    void SignalSweepWeakRefTaskFinish();
    void SignalSweepWeakRefTaskPending();
    void SuspendAllAndFinishSweeping(JSThread *thread);
    void SuspendAllAndFinishSweepingByDaemonThread(DaemonThread *dThread);

    static inline uint32_t GetNextIndexId(IteratorPtr &iter)
    {
        return iter->fetch_add(1U, std::memory_order_relaxed);
    }

    static inline bool ReduceCountAndCheckFinish(EcmaStringTableCleaner *cleaner)
    {
        return (cleaner->PendingTaskCount_.fetch_sub(1U, std::memory_order_relaxed) == 1U);
    }

    class SweepWeakRefTask : public common::Task {
    public:
        SweepWeakRefTask(IteratorPtr iter, EcmaStringTableCleaner *cleaner, const WeakRootVisitor &visitor)
            : common::Task(0), iter_(iter), cleaner_(cleaner), visitor_(visitor)
        {
        }
        ~SweepWeakRefTask() = default;

        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(SweepWeakRefTask);
        NO_MOVE_SEMANTIC(SweepWeakRefTask);

    private:
        IteratorPtr iter_;
        EcmaStringTableCleaner *cleaner_;
        const WeakRootVisitor &visitor_;
    };

    class ConcurrentSweepWeakRefTask : public common::Task {
    public:
        ConcurrentSweepWeakRefTask(IteratorPtr iter, EcmaStringTableCleaner *cleaner, const WeakRootVisitor &visitor)
            : common::Task(0), iter_(iter), cleaner_(cleaner), visitor_(visitor)
        {
        }
        ~ConcurrentSweepWeakRefTask() = default;

        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(ConcurrentSweepWeakRefTask);
        NO_MOVE_SEMANTIC(ConcurrentSweepWeakRefTask);

    private:
        IteratorPtr iter_;
        EcmaStringTableCleaner *cleaner_;
        WeakRootVisitor visitor_;
    };

    enum class SweepState {
        SWEEPING,
        PENDING,
        FINISHED
    };

    IteratorPtr iter_;
    EcmaStringTable *stringTable_;
    std::atomic<uint32_t> PendingTaskCount_ {0U};
    bool enableConcurrentSweep_ {false};
    std::array<std::vector<HashTrieMapEntry *>, TrieMapConfig::ROOT_SIZE> waitFreeEntries_ {};
    Mutex sweepWeakRefMutex_;
    SweepState sweepWeakRefFinished_ { SweepState::FINISHED };
    ConditionVariable sweepWeakRefCV_;
};

class EcmaStringTableMutex {
public:
    explicit EcmaStringTableMutex(bool is_init = true) : mtx_(is_init)
    {
    }

    void LockWithThreadState(JSThread* thread);

    void Lock()
    {
        return mtx_.Lock();
    }

    void Unlock()
    {
        return mtx_.Unlock();
    }

private:
    Mutex mtx_;
};

struct EnableCMCGCTrait {
    using StringTableInterface = BaseStringTableInterface<BaseStringTableImpl>;
    using HashTrieMapType = HashTrieMap<BaseStringTableMutex>;
    using HashTrieMapInUseScopeType = HashTrieMapInUseScope<BaseStringTableMutex>;
#ifndef GC_STW_STRINGTABLE
    using HashTrieMapOperationType = HashTrieMapOperation<BaseStringTableMutex, common::ThreadHolder,
                                                          TrieMapConfig::NeedSlotBarrierCMC>;
    static constexpr bool ConcurrentSweep = true;
#else
    using HashTrieMapOperationType = HashTrieMapOperation<BaseStringTableMutex, common::ThreadHolder,
                                                          TrieMapConfig::NoSlotBarrier>;
    static constexpr bool ConcurrentSweep = false;
#endif
    using ThreadType = common::ThreadHolder;
    static constexpr bool EnableCMCGC = true;
    static common::ReadOnlyHandle<BaseString> CreateHandle(ThreadType* holder, BaseString* string)
    {
        return JSHandle<EcmaString>(holder->GetJSThread(), EcmaString::FromBaseString(string));
    }
};

struct DisableCMCGCNormalTrait {
    struct DummyStringTableInterface {}; // placeholder for consistent type
    using StringTableInterface = DummyStringTableInterface;
    using HashTrieMapType = HashTrieMap<EcmaStringTableMutex>;
    using HashTrieMapInUseScopeType = HashTrieMapInUseScope<EcmaStringTableMutex>;
    using HashTrieMapOperationType = HashTrieMapOperation<EcmaStringTableMutex, JSThread,
                                                          TrieMapConfig::NoSlotBarrierDynamic>;
    using ThreadType = JSThread;
    static constexpr bool EnableCMCGC = false;
    static constexpr bool ConcurrentSweep = false;
    static common::ReadOnlyHandle<BaseString> CreateHandle(ThreadType* holder, BaseString* string)
    {
        return JSHandle<EcmaString>(holder, EcmaString::FromBaseString(string));
    }
};

struct DisableCMCGCConcurrentSweepTrait {
    struct DummyStringTableInterface {}; // placeholder for consistent type
    using StringTableInterface = DummyStringTableInterface;
    using HashTrieMapType = HashTrieMap<EcmaStringTableMutex>;
    using HashTrieMapInUseScopeType = HashTrieMapInUseScope<EcmaStringTableMutex>;
    using HashTrieMapOperationType = HashTrieMapOperation<EcmaStringTableMutex, JSThread,
                                                          TrieMapConfig::NeedSlotBarrier>;
    using ThreadType = JSThread;
    static constexpr bool EnableCMCGC = false;
    static constexpr bool ConcurrentSweep = true;
    static common::ReadOnlyHandle<BaseString> CreateHandle(ThreadType* holder, BaseString* string)
    {
        return JSHandle<EcmaString>(holder, EcmaString::FromBaseString(string));
    }
};

static_assert(std::is_same_v<DisableCMCGCNormalTrait::HashTrieMapType,
                             DisableCMCGCConcurrentSweepTrait::HashTrieMapType>);

class EcmaStringTableImpl final {
public:
    EcmaStringTableImpl() {}

#ifdef USE_CMC_GC
    void Initialize(void* hashTrieMap, void* itf)
    {
        hashTrieMap_ = hashTrieMap;
        stringTableItf_ = itf;
    }
#endif

    template <typename Traits>
    EcmaString *GetOrInternFlattenString(EcmaVM *vm, EcmaString *string);
    template <typename Traits>
    EcmaString *GetOrInternFlattenStringNoGC(EcmaVM *vm, EcmaString *string);
    template <typename Traits>
    EcmaString *GetOrInternStringFromCompressedSubString(EcmaVM *vm, const JSHandle<EcmaString> &string,
                                                         uint32_t offset, uint32_t utf8Len);
    template <typename Traits>
    EcmaString *GetOrInternString(EcmaVM *vm, EcmaString *string);

    template <typename Traits, typename LoaderCallback, typename EqualsCallback>
    EcmaString *GetOrInternString(EcmaVM *vm, uint32_t hashcode, LoaderCallback loaderCallback,
                                  EqualsCallback equalsCallback);
    template <typename Traits>
    EcmaString *GetOrInternString(EcmaVM *vm, const JSHandle<EcmaString> &firstString,
                                  const JSHandle<EcmaString> &secondString);
    template <typename Traits>
    EcmaString *GetOrInternString(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress,
                                  MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE);
    template <typename Traits>
    EcmaString *GetOrInternString(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf16Len, MemSpaceType type);
    template <typename Traits>
    EcmaString *GetOrInternString(EcmaVM *vm, const uint16_t *utf16Data, uint32_t utf16Len, bool canBeCompress);
    // This is ONLY for JIT Thread, since JIT could not create JSHandle so need to allocate String with holding
    // lock_ --- need to support JSHandle
    template <typename Traits>
    EcmaString *GetOrInternStringWithoutJSHandleForJit(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf16Len,
                                                       MemSpaceType type);
    template <typename Traits>
    EcmaString *GetOrInternStringWithoutJSHandleForJit(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len,
                                                       bool canBeCompress, MemSpaceType type);
    template <typename Traits>
    EcmaString *TryGetInternString(JSThread *thread, const JSHandle<EcmaString> &string);

    template <typename Traits, std::enable_if_t<!Traits::ConcurrentSweep, int> = 0>
    void SweepWeakRef(const WeakRootVisitor &visitor, uint32_t rootID);

    template <typename Traits, std::enable_if_t<Traits::ConcurrentSweep, int> = 0>
    void ConcurrentSweepWeakRef(const WeakRootVisitor &visitor, uint32_t rootID,
                                std::vector<HashTrieMapEntry*>& waitDeleteEntries);

    template <typename Traits>
    bool CheckStringTableValidity(JSThread *thread);

    NO_COPY_SEMANTIC(EcmaStringTableImpl);
    NO_MOVE_SEMANTIC(EcmaStringTableImpl);

    /**
     *
     * These are some "incorrect" functions, which need to fix the call chain to be removed.
     *
     */
    // This should only call in Debugger Signal, and need to fix and remove
    template <typename Traits>
    EcmaString *GetOrInternStringThreadUnsafe(EcmaVM *vm, const JSHandle<EcmaString> firstString,
                                              const JSHandle<EcmaString> secondString);
    // This should only call in Debugger Signal, and need to fix and remove
    template <typename Traits>
    EcmaString* GetOrInternStringThreadUnsafe(EcmaVM* vm, const uint8_t* utf8Data, uint32_t utf8Len,
                                              bool canBeCompress);

#ifdef USE_CMC_GC
    void *GetHashTrieMap() const
    {
        return hashTrieMap_;
    }
#else
    DisableCMCGCNormalTrait::HashTrieMapType *GetHashTrieMap() const
    {
        return const_cast<DisableCMCGCNormalTrait::HashTrieMapType *>(&hashTrieMap_);
    }
#endif
private:
    template <typename Traits>
    typename Traits::ThreadType* GetThreadHolder(JSThread* thread);

#ifdef USE_CMC_GC
    void* hashTrieMap_ = nullptr;
    [[maybe_unused]] void* stringTableItf_ = nullptr;
#else
    DisableCMCGCNormalTrait::HashTrieMapType hashTrieMap_;
#endif
};


class EcmaStringTable final {
public:
    EcmaStringTable(bool enableCMC, [[maybe_unused]] void* itf = nullptr, [[maybe_unused]] void* map = nullptr)
        : enableCMCGC_(enableCMC)
    {
#ifdef USE_CMC_GC
        if (enableCMC) {
            ASSERT(map != nullptr);
            impl_.Initialize(map, itf);
        } else {
            if (map == nullptr) {
                map = new DisableCMCGCNormalTrait::HashTrieMapType();
                needDeleteHashTrieMap_ = true;
            }
            impl_.Initialize(map, nullptr);
            cleaner_ = new EcmaStringTableCleaner(this);
        }
#else
        cleaner_ = new EcmaStringTableCleaner(this);
#endif
    }

    ~EcmaStringTable()
    {
        if (cleaner_ != nullptr) {
            delete cleaner_;
            cleaner_ = nullptr;
        }
#ifdef USE_CMC_GC
        if (needDeleteHashTrieMap_) {
            ASSERT(!enableCMCGC_);
            delete reinterpret_cast<DisableCMCGCNormalTrait::HashTrieMapType *>(impl_.GetHashTrieMap());
        }
#endif
    }

    EcmaString *GetOrInternFlattenString(EcmaVM *vm, EcmaString *string);
    EcmaString *GetOrInternFlattenStringNoGC(EcmaVM *vm, EcmaString *string);
    EcmaString *GetOrInternStringFromCompressedSubString(EcmaVM *vm, const JSHandle<EcmaString> &string,
                                                         uint32_t offset, uint32_t utf8Len);
    EcmaString *GetOrInternString(EcmaVM *vm, EcmaString *string);

    template <typename LoaderCallback, typename EqualsCallback>
    EcmaString *GetOrInternString(EcmaVM *vm, uint32_t hashcode, LoaderCallback loaderCallback,
                                  EqualsCallback equalsCallback);
    EcmaString *GetOrInternString(EcmaVM *vm, const JSHandle<EcmaString> &firstString,
                                  const JSHandle<EcmaString> &secondString);
    EcmaString *GetOrInternString(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress,
                                  MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE);
    EcmaString *GetOrInternString(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf16Len, MemSpaceType type);
    EcmaString *GetOrInternString(EcmaVM *vm, const uint16_t *utf16Data, uint32_t utf16Len, bool canBeCompress);
    // This is ONLY for JIT Thread, since JIT could not create JSHandle so need to allocate String with holding
    // lock_ --- need to support JSHandle
    EcmaString *GetOrInternStringWithoutJSHandleForJit(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf16Len,
                                                       MemSpaceType type);
    EcmaString *GetOrInternStringWithoutJSHandleForJit(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len,
                                                       bool canBeCompress, MemSpaceType type);
    EcmaString *TryGetInternString(JSThread *thread, const JSHandle<EcmaString> &string);

    void SweepWeakRef(const WeakRootVisitor &visitor, uint32_t index);
    void ConcurrentSweepWeakRef(const WeakRootVisitor &visitor, uint32_t index,
                                std::vector<HashTrieMapEntry*>& waitDeleteEntries);

    bool CheckStringTableValidity(JSThread *thread);

    EcmaStringTableCleaner *GetCleaner()
    {
        ASSERT(!enableCMCGC_ && "EcmaStringTableCleaner should not be used when cmcgc enabled");
        return cleaner_;
    }

    void WaitSweepWeakRefTaskFinished()
    {
#ifdef USE_CMC_GC
        if (enableCMCGC_) {
            return;
        }
#endif
        cleaner_->WaitSweepWeakRefTaskFinished();
    }

    void TransferToNativeAndWaitSweepWeakRefTaskFinished(JSThread *thread);

    bool IsSweeping() const
    {
#ifdef USE_CMC_GC
        if (enableCMCGC_) {
            return reinterpret_cast<EnableCMCGCTrait::HashTrieMapType *>(impl_.GetHashTrieMap())->IsSweeping();
        }
#endif
        return reinterpret_cast<DisableCMCGCNormalTrait::HashTrieMapType *>(impl_.GetHashTrieMap())->IsSweeping();
    }

    bool IsInUse() const
    {
#ifdef USE_CMC_GC
        if (enableCMCGC_) {
            return reinterpret_cast<EnableCMCGCTrait::HashTrieMapType *>(impl_.GetHashTrieMap())->GetInuseCount() > 0;
        }
#endif
        return reinterpret_cast<DisableCMCGCNormalTrait::HashTrieMapType *>(
            impl_.GetHashTrieMap())->GetInuseCount() > 0;
    }

    void *GetHashTrieMap() const
    {
        return impl_.GetHashTrieMap();
    }

private:
    NO_COPY_SEMANTIC(EcmaStringTable);
    NO_MOVE_SEMANTIC(EcmaStringTable);

    /**
     *
     * These are some "incorrect" functions, which need to fix the call chain to be removed.
     *
     */
    // This should only call in Debugger Signal, and need to fix and remove
    EcmaString *GetOrInternStringThreadUnsafe(EcmaVM *vm, const JSHandle<EcmaString> firstString,
                                              const JSHandle<EcmaString> secondString);
    // This should only call in Debugger Signal, and need to fix and remove
    EcmaString* GetOrInternStringThreadUnsafe(EcmaVM* vm, const uint8_t* utf8Data, uint32_t utf8Len,
                                              bool canBeCompress);

    EcmaStringTableImpl impl_;

    EcmaStringTableCleaner *cleaner_ = nullptr;
    bool enableCMCGC_ = false;
#ifdef USE_CMC_GC
    bool needDeleteHashTrieMap_ = false;
#endif
    friend class SnapshotProcessor;
    friend class BaseDeserializer;
};

#else
class EcmaString;
class EcmaVM;
class JSPandaFile;
class JSThread;

class EcmaStringTable;

class EcmaStringTableCleaner {
public:
    using IteratorPtr = std::shared_ptr<std::atomic<uint32_t>>;
    EcmaStringTableCleaner(EcmaStringTable* stringTable) : stringTable_(stringTable) {}
    ~EcmaStringTableCleaner() { stringTable_ = nullptr; }

    void PostSweepWeakRefTask(const WeakRootVisitor &visitor);
    void JoinAndWaitSweepWeakRefTask(const WeakRootVisitor &visitor);

private:
    NO_COPY_SEMANTIC(EcmaStringTableCleaner);
    NO_MOVE_SEMANTIC(EcmaStringTableCleaner);

    static void ProcessSweepWeakRef(IteratorPtr& iter, EcmaStringTableCleaner *cleaner, const WeakRootVisitor &visitor);
    void StartSweepWeakRefTask();
    void WaitSweepWeakRefTask();
    void SignalSweepWeakRefTask();

    static inline uint32_t GetNextTableId(IteratorPtr& iter)
    {
        return iter->fetch_add(1U, std::memory_order_relaxed);
    }

    static inline bool ReduceCountAndCheckFinish(EcmaStringTableCleaner* cleaner)
    {
        return (cleaner->PendingTaskCount_.fetch_sub(1U, std::memory_order_relaxed) == 1U);
    }

    class SweepWeakRefTask : public common::Task {
    public:
        SweepWeakRefTask(IteratorPtr iter, EcmaStringTableCleaner* cleaner, const WeakRootVisitor& visitor)
            : common::Task(0), iter_(iter), cleaner_(cleaner), visitor_(visitor) {}
        ~SweepWeakRefTask() = default;

        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(SweepWeakRefTask);
        NO_MOVE_SEMANTIC(SweepWeakRefTask);

    private:
        IteratorPtr iter_;
        EcmaStringTableCleaner* cleaner_;
        const WeakRootVisitor& visitor_;
    };

    IteratorPtr iter_;
    EcmaStringTable* stringTable_;
    std::atomic<uint32_t> PendingTaskCount_ {0U};
    Mutex sweepWeakRefMutex_;
    bool sweepWeakRefFinished_ {true};
    ConditionVariable sweepWeakRefCV_;
};

class EcmaStringTable {
public:
    EcmaStringTable() : cleaner_(new EcmaStringTableCleaner(this))
    {
        stringTable_.fill(Segment());
    }
    virtual ~EcmaStringTable()
    {
        if (cleaner_ != nullptr) {
            delete cleaner_;
            cleaner_ = nullptr;
        }
        for (auto &seg : stringTable_) {
            seg.table_.clear();
        }
    }

    static inline uint32_t GetTableId(uint32_t hashcode)
    {
        return hashcode & SEGMENT_MASK;
    }
    EcmaString *GetOrInternFlattenString(EcmaVM *vm, EcmaString *string);
    EcmaString *GetOrInternFlattenStringNoGC(EcmaVM *vm, EcmaString *string);
    EcmaString *GetOrInternStringFromCompressedSubString(EcmaVM *vm, const JSHandle<EcmaString> &string,
        uint32_t offset, uint32_t utf8Len);
    EcmaString *GetOrInternString(EcmaVM *vm, EcmaString *string);
    EcmaString *GetOrInternString(EcmaVM *vm,
                                  const JSHandle<EcmaString> &firstString, const JSHandle<EcmaString> &secondString);
    EcmaString *GetOrInternString(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress,
                                  MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE);
    EcmaString *GetOrInternString(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf16Len, MemSpaceType type);
    EcmaString *GetOrInternString(EcmaVM *vm, const uint16_t *utf16Data, uint32_t utf16Len, bool canBeCompress);
    // This is ONLY for JIT Thread, since JIT could not create JSHandle so need to allocate String with holding
    // lock_ --- need to support JSHandle
    EcmaString *GetOrInternStringWithoutJSHandleForJit(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf16Len,
                                                       MemSpaceType type);
    EcmaString *GetOrInternStringWithoutJSHandleForJit(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len,
                                                       bool canBeCompress, MemSpaceType type);
    EcmaString *TryGetInternString(JSThread *thread, const JSHandle<EcmaString> &string);
    EcmaString *InsertStringToTable(EcmaVM *vm, const JSHandle<EcmaString> &strHandle);

    void SweepWeakRef(const WeakRootVisitor &visitor);
    void SweepWeakRef(const WeakRootVisitor &visitor, uint32_t tableId);

    bool CheckStringTableValidity(JSThread *thread);
    void RelocateConstantData(EcmaVM *vm, const JSPandaFile *jsPandaFile);
    void IterWeakRoot(WeakVisitor &visitor);

    EcmaStringTableCleaner* GetCleaner()
    {
        return cleaner_;
    }
    static constexpr uint32_t SEGMENT_COUNT = 16U; // 16: 2^4
    static constexpr uint32_t SEGMENT_MASK = SEGMENT_COUNT - 1U;
private:
    NO_COPY_SEMANTIC(EcmaStringTable);
    NO_MOVE_SEMANTIC(EcmaStringTable);

    EcmaString *GetStringThreadUnsafe(JSThread *thread, EcmaString *string, uint32_t hashcode) const;
    void InternStringThreadUnsafe(EcmaString *string, uint32_t hashcode);
    EcmaString *AtomicGetOrInternStringImpl(JSThread *thread, const JSHandle<EcmaString> string, uint32_t hashcode);
    EcmaString *AtomicGetOrInternStringImplNoGC(JSThread *thread, EcmaString *string, uint32_t hashcode);

    EcmaString *GetStringFromCompressedSubString(JSThread *thread, const JSHandle<EcmaString> string, uint32_t offset,
                                                 uint32_t utf8Len, uint32_t hashcode);
    EcmaString *GetString(JSThread *thread, const JSHandle<EcmaString> string, uint32_t hashcode);
    EcmaString *GetString(JSThread *thread, const JSHandle<EcmaString> firstString,
                          const JSHandle<EcmaString> secondString, uint32_t hashcode);
    // utf8Data MUST NOT on JSHeap
    EcmaString *GetString(JSThread *thread, const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress,
                          uint32_t hashcode);
    // utf16Data MUST NOT on JSHeap
    EcmaString *GetString(JSThread *thread, const uint16_t *utf16Data, uint32_t utf16Len, uint32_t hashcode);

    // This used only for SnapShot.
    void InsertStringToTableWithHashThreadUnsafe(JSThread *thread, EcmaString *string, uint32_t hashcode);
    /**
     *
     * These are some "incorrect" functions, which need to fix the call chain to be removed.
     *
    */
    // This should only call in Debugger Signal, and need to fix and remove
    EcmaString *GetOrInternStringThreadUnsafe(EcmaVM *vm,
                                              const JSHandle<EcmaString> firstString,
                                              const JSHandle<EcmaString> secondString);
    // This should only call in Debugger Signal, and need to fix and remove
    EcmaString *GetOrInternStringThreadUnsafe(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len,
                                              bool canBeCompress);
    // This should only call in Debugger Signal, and need to fix and remove
    EcmaString *GetStringThreadUnsafe(JSThread *thread, const JSHandle<EcmaString> firstString,
                                      const JSHandle<EcmaString> secondString, uint32_t hashcode) const;
    // This should only call in Debugger Signal or from JIT, and need to fix and remove
    EcmaString *GetStringThreadUnsafe(JSThread *thread, const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress,
                                      uint32_t hashcode) const;
    // This should only call in JIT Thread, and need to fix and remove
    EcmaString *GetStringThreadUnsafe(JSThread *thread, const uint16_t *utf16Data, uint32_t utf16Len,
                                      uint32_t hashcode) const;

    struct Segment {
        CUnorderedMultiMap<uint32_t, EcmaString *> table_;
        Mutex mutex_;
    };

    std::array<Segment, SEGMENT_COUNT> stringTable_;
    EcmaStringTableCleaner* cleaner_;

    friend class SnapshotProcessor;
    friend class BaseDeserializer;
};
#endif

class SingleCharTable : public TaggedArray {
public:
    static SingleCharTable *Cast(TaggedObject *object)
    {
        return reinterpret_cast<SingleCharTable*>(object);
    }
    static JSTaggedValue CreateSingleCharTable(JSThread *thread);
    JSTaggedValue GetStringFromSingleCharTable(JSThread *thread, int32_t ch)
    {
        return Get(thread, ch);
    }
private:
    SingleCharTable() = default;
    ~SingleCharTable() = default;
    NO_COPY_SEMANTIC(SingleCharTable);
    NO_MOVE_SEMANTIC(SingleCharTable);
    static constexpr uint32_t MAX_ONEBYTE_CHARCODE = 128; // 0X00-0X7F
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_STRING_TABLE_H
