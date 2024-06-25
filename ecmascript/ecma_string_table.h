/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
class EcmaString;
class EcmaVM;
class JSPandaFile;

class EcmaStringTable {
public:
    EcmaStringTable() = default;
    virtual ~EcmaStringTable()
    {
        table_.clear();
    }

    void InternEmptyString(JSThread *thread, EcmaString *emptyStr);
    EcmaString *GetOrInternString(EcmaVM *vm,
                                  const JSHandle<EcmaString> &firstString,
                                  const JSHandle<EcmaString> &secondString);
    EcmaString *GetOrInternString(EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress);
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

    void SweepWeakReference(const WeakRootVisitor &visitor);
    bool CheckStringTableValidity(JSThread *thread);
    void RelocateConstantData(EcmaVM *vm, const JSPandaFile *jsPandaFile);
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
