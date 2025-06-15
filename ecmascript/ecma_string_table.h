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
#include "common_components/objects/string_table/hashtriemap.h"
#include "common_components/objects/string_table_internal.h"
#include "common_components/taskpool/task.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/tagged_array.h"
#include "common_interfaces/objects/base_string_table.h"
#include "common_interfaces/objects/string/base_string_declare.h"

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

private:
    NO_COPY_SEMANTIC(EcmaStringTableCleaner);
    NO_MOVE_SEMANTIC(EcmaStringTableCleaner);

    static void ProcessSweepWeakRef(IteratorPtr &iter, EcmaStringTableCleaner *cleaner, const WeakRootVisitor &visitor);
    void StartSweepWeakRefTask();
    void WaitSweepWeakRefTask();
    void SignalSweepWeakRefTask();

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

    IteratorPtr iter_;
    EcmaStringTable *stringTable_;
    std::atomic<uint32_t> PendingTaskCount_ {0U};
    Mutex sweepWeakRefMutex_;
    bool sweepWeakRefFinished_ {true};
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
    using StringTableInterface = common::BaseStringTableInterface<common::BaseStringTableImpl>;
    using HashTrieMapImpl = common::HashTrieMap<common::BaseStringTableMutex, common::ThreadHolder>;
    using ThreadType = common::ThreadHolder;
    static constexpr bool EnableCMCGC = true;
    static common::ReadOnlyHandle<BaseString> CreateHandle(ThreadType* holder, BaseString* string)
    {
        return JSHandle<EcmaString>(holder->GetJSThread(), EcmaString::FromBaseString(string));
    }
};

struct DisableCMCGCTrait {
    struct DummyStringTableInterface {}; // placeholder for consistent type
    using StringTableInterface = DummyStringTableInterface;
    using HashTrieMapImpl = common::HashTrieMap<EcmaStringTableMutex, JSThread>;
    using ThreadType = JSThread;
    static constexpr bool EnableCMCGC = false;
    static common::ReadOnlyHandle<BaseString> CreateHandle(ThreadType* holder, BaseString* string)
    {
        return JSHandle<EcmaString>(holder, EcmaString::FromBaseString(string));
    }
};

template <typename Traits>
class EcmaStringTableImpl final {
public:
    using StringTableInterface = typename Traits::StringTableInterface;
    using HashTrieMapImpl = typename Traits::HashTrieMapImpl;
    using ThreadType = typename Traits::ThreadType;
    // CMC constructor
    template <typename T = Traits, std::enable_if_t<T::EnableCMCGC, int> = 0>
    EcmaStringTableImpl(StringTableInterface* itf, HashTrieMapImpl& map)
        : stringTable_(map), stringTableItf_(itf) {}

    // Non-CMC constructor
    template <typename T = Traits, std::enable_if_t<!T::EnableCMCGC, int> = 0>
    EcmaStringTableImpl() {}

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
    EcmaString *TryGetInternString(const JSHandle<EcmaString> &string);

    void IterWeakRoot(const common::WeakRefFieldVisitor &visitor);
    void IterWeakRoot(const common::WeakRefFieldVisitor &visitor, uint32_t index);
    void SweepWeakRef(const WeakRootVisitor &visitor);
    void SweepWeakRef(const WeakRootVisitor &visitor, uint32_t index);

    bool CheckStringTableValidity();

    NO_COPY_SEMANTIC(EcmaStringTableImpl);
    NO_MOVE_SEMANTIC(EcmaStringTableImpl);

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

private:
    static ThreadType* GetThreadHolder(JSThread* thread);

    std::conditional_t<Traits::EnableCMCGC, HashTrieMapImpl&, HashTrieMapImpl> stringTable_;
    StringTableInterface* stringTableItf_ = nullptr;
};


class EcmaStringTable final {
public:
    using StringTableInterface = common::BaseStringTableInterface<common::BaseStringTableImpl>;
    using HashTrieMapImpl = common::HashTrieMap<common::BaseStringTableMutex, common::ThreadHolder>;

    EcmaStringTable(bool enableCMC, void* itf = nullptr, void* map = nullptr)
    {
        cleaner_ = new EcmaStringTableCleaner(this);
        if (enableCMC) {
            impl_.emplace<EcmaStringTableImpl<EnableCMCGCTrait>>(
                static_cast<EnableCMCGCTrait::StringTableInterface*>(itf),
                *static_cast<EnableCMCGCTrait::HashTrieMapImpl*>(map));
        } else {
            impl_.emplace<EcmaStringTableImpl<DisableCMCGCTrait>>();
        }
    }

    ~EcmaStringTable()
    {
        if (cleaner_ != nullptr) {
            delete cleaner_;
            cleaner_ = nullptr;
        }
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
    EcmaString *TryGetInternString(const JSHandle<EcmaString> &string);

    void IterWeakRoot(const common::WeakRefFieldVisitor &visitor);
    void IterWeakRoot(const common::WeakRefFieldVisitor &visitor, uint32_t index);
    void SweepWeakRef(const WeakRootVisitor &visitor);
    void SweepWeakRef(const WeakRootVisitor &visitor, uint32_t index);

    bool CheckStringTableValidity();

    EcmaStringTableCleaner *GetCleaner()
    {
        return cleaner_;
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

    template <typename Fn>
    decltype(auto) visitImpl(Fn&& fn)
    {
        return std::visit(std::forward<Fn>(fn), impl_);
    }

    std::variant<
        EcmaStringTableImpl<DisableCMCGCTrait>,
        EcmaStringTableImpl<EnableCMCGCTrait>
    > impl_;

    EcmaStringTableCleaner *cleaner_;

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

    EcmaString *GetStringThreadUnsafe(EcmaString *string, uint32_t hashcode) const;
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
    void InsertStringToTableWithHashThreadUnsafe(EcmaString* string, uint32_t hashcode);
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
    EcmaString *GetStringThreadUnsafe(const JSHandle<EcmaString> firstString, const JSHandle<EcmaString> secondString,
                                      uint32_t hashcode) const;
    // This should only call in Debugger Signal or from JIT, and need to fix and remove
    EcmaString *GetStringThreadUnsafe(const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress,
                                      uint32_t hashcode) const;
    // This should only call in JIT Thread, and need to fix and remove
    EcmaString *GetStringThreadUnsafe(const uint16_t *utf16Data, uint32_t utf16Len, uint32_t hashcode) const;

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
    JSTaggedValue GetStringFromSingleCharTable(int32_t ch)
    {
        return Get(ch);
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
