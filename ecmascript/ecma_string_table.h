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
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/taskpool/task.h"

namespace panda::ecmascript {
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

    class SweepWeakRefTask : public Task {
    public:
        SweepWeakRefTask(IteratorPtr iter, EcmaStringTableCleaner* cleaner, const WeakRootVisitor& visitor)
            : Task(0), iter_(iter), cleaner_(cleaner), visitor_(visitor) {}
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
    EcmaString *GetOrInternStringFromCompressedSubString(EcmaVM *vm, const JSHandle<EcmaString> &string,
        uint32_t offset, uint32_t utf8Len);
    EcmaString *GetOrInternString(EcmaVM *vm, EcmaString *string);
    EcmaString *GetOrInternString(EcmaVM *vm,
                                  const JSHandle<EcmaString> &firstString, const JSHandle<EcmaString> &secondString);
    EcmaString *GetOrInternString(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress,
                                  MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE);
    EcmaString *GetOrInternString(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress,
                                  MemSpaceType type, bool isConstantString, uint32_t idOffset);
    EcmaString *GetOrInternString(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf16Len,  MemSpaceType type);
    EcmaString *GetOrInternString(EcmaVM *vm, const uint16_t *utf16Data, uint32_t utf16Len, bool canBeCompress);
    // This is ONLY for JIT Thread, since JIT could not create JSHandle so need to allocate String with holding
    // lock_ --- need to support JSHandle
    EcmaString *GetOrInternStringWithoutJSHandleForJit(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf16Len,
                                                       MemSpaceType type);
    EcmaString *GetOrInternStringWithoutJSHandleForJit(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len,
        bool canBeCompress, MemSpaceType type, bool isConstantString, uint32_t idOffset);
    EcmaString *TryGetInternString(JSThread *thread, const JSHandle<EcmaString> &string);
    EcmaString *InsertStringToTable(EcmaVM *vm, const JSHandle<EcmaString> &strHandle);

    void SweepWeakRef(const WeakRootVisitor &visitor);
    void SweepWeakRef(const WeakRootVisitor &visitor, uint32_t tableId);

    bool CheckStringTableValidity(JSThread *thread);
    void RelocateConstantData(EcmaVM *vm, const JSPandaFile *jsPandaFile);

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
     * These are some "incorrect" functions, whcih need to fix the call chain to be removed.
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
