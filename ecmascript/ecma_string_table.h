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

#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
class EcmaString;
class EcmaVM;
class JSPandaFile;

class EcmaStringTable {
public:
    explicit EcmaStringTable(const EcmaVM *vm);
    virtual ~EcmaStringTable()
    {
        table_.clear();
    }

    void InternEmptyString(EcmaString *emptyStr);
    EcmaString *GetOrInternString(const JSHandle<EcmaString> &firstString, const JSHandle<EcmaString> &secondString);
    EcmaString *GetOrInternString(const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress);
    EcmaString *CreateAndInternStringNonMovable(const uint8_t *utf8Data, uint32_t utf8Len);
    EcmaString *GetOrInternString(const uint16_t *utf16Data, uint32_t utf16Len, bool canBeCompress);
    EcmaString *GetOrInternString(EcmaString *string);
    EcmaString *GetOrInternCompressedSubString(const JSHandle<EcmaString> &string, uint32_t offset, uint32_t utf8Len);
    EcmaString *GetOrInternStringWithSpaceType(const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress,
                                               MemSpaceType type, bool isConstantString, uint32_t idOffset);
    EcmaString *GetOrInternStringWithSpaceType(const uint8_t *utf8Data, uint32_t utf16Len,
                                               MemSpaceType type);
    EcmaString *TryGetInternString(EcmaString *string);
    EcmaString *InsertStringToTable(const JSHandle<EcmaString> &strHandle);

    void SweepWeakReference(const WeakRootVisitor &visitor);
    bool CheckStringTableValidity();
    void RelocateConstantData(const JSPandaFile *jsPandaFile);
private:
    NO_COPY_SEMANTIC(EcmaStringTable);
    NO_MOVE_SEMANTIC(EcmaStringTable);

    std::pair<EcmaString *, uint32_t> GetString(
        const JSHandle<EcmaString> &firstString, const JSHandle<EcmaString> &secondString) const;
    std::pair<EcmaString *, uint32_t> GetString(const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress) const;
    std::pair<EcmaString *, uint32_t> GetString(const uint16_t *utf16Data, uint32_t utf16Len) const;
    EcmaString *GetString(EcmaString *string) const;

    void InternString(EcmaString *string);

    void InsertStringIfNotExist(EcmaString *string)
    {
        EcmaString *str = GetString(string);
        if (str == nullptr) {
            InternString(string);
        }
    }

    CUnorderedMultiMap<uint32_t, EcmaString *> table_;
    const EcmaVM *vm_{nullptr};
    friend class SnapshotProcessor;
    friend class BaseDeserializer;
};

class SingleCharTable : public TaggedArray {
public:
    static SingleCharTable *Cast(TaggedObject *object)
    {
        return reinterpret_cast<SingleCharTable*>(object);
    }
    static void CreateSingleCharTable(JSThread *thread);
    JSTaggedValue GetStringFromSingleCharTable(int32_t ch)
    {
        return Get(ch);
    }
private:
    static constexpr uint32_t MAX_ONEBYTE_CHARCODE = 128; // 0X00-0X7F
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_STRING_TABLE_H
