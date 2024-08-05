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
    EcmaStringTableCleaner(EcmaStringTable* stringTable) : stringTable_(stringTable) {}
    ~EcmaStringTableCleaner() { stringTable_ = nullptr; }

    uint32_t PostSweepWeakRefTask(const WeakRootVisitor &visitor);
    void TakeOrWaitSweepWeakRefTask(const WeakRootVisitor &visitor, uint32_t curSweepCount);

private:
    NO_COPY_SEMANTIC(EcmaStringTableCleaner);
    NO_MOVE_SEMANTIC(EcmaStringTableCleaner);

    void StartSweepWeakRefTask();
    void WaitSweepWeakRefTask();
    void SignalSweepWeakRefTask();

    inline bool CheckAndSwitchRunningState(uint32_t curSweepCount)
    {
        uint32_t expected = curSweepCount;
        // When the uint32_t overflows, it wraps around, which does not affect the correctness.
        return sweepWeakRefCount_.compare_exchange_strong(expected, curSweepCount + 1U, // 1: Running state
                                                          std::memory_order_relaxed, std::memory_order_relaxed);
    }

    inline void SwitchFinishState(uint32_t curSweepCount)
    {
        // When the uint32_t overflows, it wraps around, which does not affect the correctness.
        sweepWeakRefCount_.store(curSweepCount + 2U, std::memory_order_relaxed); // 2: Finish state
    }

    class SweepWeakRefTask : public Task {
    public:
        SweepWeakRefTask(EcmaStringTableCleaner* cleaner, const WeakRootVisitor& visitor, uint32_t curSweepCount)
            : Task(0), cleaner_(cleaner), visitor_(visitor), curSweepCount_(curSweepCount) {}
        ~SweepWeakRefTask() = default;
        
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(SweepWeakRefTask);
        NO_MOVE_SEMANTIC(SweepWeakRefTask);

    private:
        EcmaStringTableCleaner* cleaner_;
        const WeakRootVisitor& visitor_;
        uint32_t curSweepCount_;
    };

    EcmaStringTable* stringTable_;
    std::atomic<uint32_t> sweepWeakRefCount_ {0U};
    Mutex sweepWeakRefMutex_;
    bool sweepWeakRefFinished_;
    ConditionVariable sweepWeakRefCV_;
};

class EcmaStringTable {
public:
    EcmaStringTable() : cleaner_(new EcmaStringTableCleaner(this)) {}
    virtual ~EcmaStringTable()
    {
        if (cleaner_ != nullptr) {
            delete cleaner_;
            cleaner_ = nullptr;
        }
        table_.clear();
    }

    void InternEmptyString(JSThread *thread, EcmaString *emptyStr);
    EcmaString *GetOrInternString(EcmaVM *vm,
                                  const JSHandle<EcmaString> &firstString,
                                  const JSHandle<EcmaString> &secondString);
    EcmaString *GetOrInternStringWithoutLock(JSThread *thread,
                                             const JSHandle<EcmaString> &firstString,
                                             const JSHandle<EcmaString> &secondString);
    EcmaString *GetOrInternString(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress);
    EcmaString *GetOrInternStringWithoutLock(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len,
                                             bool canBeCompress);
    EcmaString *CreateAndInternStringNonMovable(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len);
    EcmaString *CreateAndInternStringReadOnly(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len,
                                              bool canBeCompress);
    EcmaString *GetOrInternString(EcmaVM *vm, const uint16_t *utf16Data, uint32_t utf16Len, bool canBeCompress);
    EcmaString *GetOrInternString(EcmaVM *vm, EcmaString *string);
    EcmaString *GetOrInternCompressedSubString(EcmaVM *vm, const JSHandle<EcmaString> &string,
        uint32_t offset, uint32_t utf8Len);
    EcmaString *GetOrInternStringWithSpaceType(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len,
        bool canBeCompress, MemSpaceType type, bool isConstantString, uint32_t idOffset);
    EcmaString *GetOrInternStringWithSpaceType(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf16Len,
                                               MemSpaceType type);
    EcmaString *TryGetInternString(JSThread *thread, const JSHandle<EcmaString> &string);
    void InsertStringToTableWithHashThreadUnsafe(EcmaString* string, uint32_t hashcode);
    EcmaString *InsertStringToTable(EcmaVM *vm, const JSHandle<EcmaString> &strHandle);

    void SweepWeakRef(const WeakRootVisitor &visitor);

    bool CheckStringTableValidity(JSThread *thread);
    void RelocateConstantData(EcmaVM *vm, const JSPandaFile *jsPandaFile);

    EcmaStringTableCleaner* GetCleaner()
    {
        return cleaner_;
    }
private:
    NO_COPY_SEMANTIC(EcmaStringTable);
    NO_MOVE_SEMANTIC(EcmaStringTable);

    std::pair<EcmaString *, uint32_t> GetStringThreadUnsafe(const JSHandle<EcmaString> &firstString,
                                                            const JSHandle<EcmaString> &secondString) const;
    std::pair<EcmaString *, uint32_t> GetStringThreadUnsafe(const uint8_t *utf8Data, uint32_t utf8Len,
                                                            bool canBeCompress) const;
    std::pair<EcmaString *, uint32_t> GetStringThreadUnsafe(const uint16_t *utf16Data, uint32_t utf16Len) const;
    EcmaString *GetStringWithHashThreadUnsafe(EcmaString *string, uint32_t hashcode) const;
    EcmaString *GetStringThreadUnsafe(EcmaString *string) const;

    void InternStringThreadUnsafe(EcmaString *string);
    EcmaString *GetOrInternStringThreadUnsafe(EcmaVM *vm, EcmaString *string);

    void InsertStringIfNotExistThreadUnsafe(EcmaString *string)
    {
        EcmaString *str = GetStringThreadUnsafe(string);
        if (str == nullptr) {
            InternStringThreadUnsafe(string);
        }
    }

    CUnorderedMultiMap<uint32_t, EcmaString *> table_;
    Mutex mutex_;

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
