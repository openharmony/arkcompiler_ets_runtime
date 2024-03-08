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

#include "ecmascript/ecma_string_table.h"

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
EcmaStringTable::EcmaStringTable(const EcmaVM *vm) : vm_(vm) {}

std::pair<EcmaString *, uint32_t> EcmaStringTable::GetString(const JSHandle<EcmaString> &firstString,
                                                             const JSHandle<EcmaString> &secondString) const
{
    ASSERT(EcmaStringAccessor(firstString).NotTreeString());
    ASSERT(EcmaStringAccessor(secondString).NotTreeString());
    auto [hashCode, isInteger] = EcmaStringAccessor(firstString).ComputeRawHashcode();
    hashCode = EcmaStringAccessor(secondString).ComputeHashcode(hashCode, isInteger);

    auto range = table_.equal_range(hashCode);
    for (auto item = range.first; item != range.second; ++item) {
        auto foundString = item->second;
        if (EcmaStringAccessor(foundString).EqualToSplicedString(*firstString, *secondString)) {
            return std::make_pair(foundString, hashCode);
        }
    }
    return std::make_pair(nullptr, hashCode);
}

std::pair<EcmaString *, uint32_t> EcmaStringTable::GetString(const uint8_t *utf8Data,
                                                             uint32_t utf8Len, bool canBeCompress) const
{
    uint32_t hashCode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, canBeCompress);
    auto range = table_.equal_range(hashCode);
    for (auto item = range.first; item != range.second; ++item) {
        auto foundString = item->second;
        if (EcmaStringAccessor::StringIsEqualUint8Data(foundString, utf8Data, utf8Len, canBeCompress)) {
            return std::make_pair(foundString, hashCode);
        }
    }
    return std::make_pair(nullptr, hashCode);
}

std::pair<EcmaString *, uint32_t> EcmaStringTable::GetString(const uint16_t *utf16Data, uint32_t utf16Len) const
{
    uint32_t hashCode = EcmaStringAccessor::ComputeHashcodeUtf16(const_cast<uint16_t *>(utf16Data), utf16Len);
    auto range = table_.equal_range(hashCode);
    for (auto item = range.first; item != range.second; ++item) {
        auto foundString = item->second;
        if (EcmaStringAccessor::StringsAreEqualUtf16(foundString, utf16Data, utf16Len)) {
            return std::make_pair(foundString, hashCode);
        }
    }
    return std::make_pair(nullptr, hashCode);
}

EcmaString *EcmaStringTable::GetString(EcmaString *string) const
{
    auto hashcode = EcmaStringAccessor(string).GetHashcode();
    auto range = table_.equal_range(hashcode);
    for (auto item = range.first; item != range.second; ++item) {
        auto foundString = item->second;
        if (EcmaStringAccessor::StringsAreEqual(foundString, string)) {
            return foundString;
        }
    }
    return nullptr;
}

void EcmaStringTable::InternString(EcmaString *string)
{
    if (EcmaStringAccessor(string).IsInternString()) {
        return;
    }
    // Strings in string table should not be in the young space.
    ASSERT(!Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(string))->InYoungSpace());
    ASSERT(EcmaStringAccessor(string).NotTreeString());
    auto hashcode = EcmaStringAccessor(string).GetHashcode();
    table_.emplace(hashcode, string);
    EcmaStringAccessor(string).SetInternString();
}

void EcmaStringTable::InternEmptyString(EcmaString *emptyStr)
{
    InternString(emptyStr);
}

EcmaString *EcmaStringTable::GetOrInternString(const JSHandle<EcmaString> &firstString,
                                               const JSHandle<EcmaString> &secondString)
{
    auto firstFlat = JSHandle<EcmaString>(vm_->GetJSThread(), EcmaStringAccessor::Flatten(vm_, firstString));
    auto secondFlat = JSHandle<EcmaString>(vm_->GetJSThread(), EcmaStringAccessor::Flatten(vm_, secondString));
    std::pair<EcmaString *, uint32_t> result = GetString(firstFlat, secondFlat);
    if (result.first != nullptr) {
        return result.first;
    }
    JSHandle<EcmaString> concatHandle(vm_->GetJSThread(),
        EcmaStringAccessor::Concat(vm_, firstFlat, secondFlat, MemSpaceType::OLD_SPACE));
    EcmaString *concatString = EcmaStringAccessor::Flatten(vm_, concatHandle, MemSpaceType::OLD_SPACE);
    concatString->SetMixHashcode(result.second);
    InternString(concatString);
    return concatString;
}

EcmaString *EcmaStringTable::GetOrInternString(const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress)
{
    std::pair<EcmaString *, uint32_t> result = GetString(utf8Data, utf8Len, canBeCompress);
    if (result.first != nullptr) {
        return result.first;
    }

    EcmaString *str =
        EcmaStringAccessor::CreateFromUtf8(vm_, utf8Data, utf8Len, canBeCompress, MemSpaceType::OLD_SPACE);
    str->SetMixHashcode(result.second);
    InternString(str);
    return str;
}

EcmaString *EcmaStringTable::GetOrInternCompressedSubString(const JSHandle<EcmaString> &string,
    uint32_t offset, uint32_t utf8Len)
{
    auto *utf8Data = EcmaStringAccessor(string).GetDataUtf8() + offset;
    std::pair<EcmaString *, uint32_t> result = GetString(utf8Data, utf8Len, true);
    if (result.first != nullptr) {
        return result.first;
    }

    EcmaString *str = EcmaStringAccessor::CreateFromUtf8CompressedSubString(
        vm_, string, offset, utf8Len, MemSpaceType::OLD_SPACE);
    str->SetMixHashcode(result.second);
    InternString(str);
    return str;
}

/*
    This function is used to create global constant strings from non-movable sapce only.
    It only inserts string into string-table and provides no string-table validity check.
*/
EcmaString *EcmaStringTable::CreateAndInternStringNonMovable(const uint8_t *utf8Data, uint32_t utf8Len)
{
    std::pair<EcmaString *, uint32_t> result  = GetString(utf8Data, utf8Len, true);
    if (result.first != nullptr) {
        return result.first;
    }

    EcmaString *str = EcmaStringAccessor::CreateFromUtf8(vm_, utf8Data, utf8Len, true, MemSpaceType::NON_MOVABLE);
    str->SetMixHashcode(result.second);
    InternString(str);
    return str;
}

EcmaString *EcmaStringTable::GetOrInternString(const uint16_t *utf16Data, uint32_t utf16Len, bool canBeCompress)
{
    std::pair<EcmaString *, uint32_t> result = GetString(utf16Data, utf16Len);
    if (result.first != nullptr) {
        return result.first;
    }

    EcmaString *str =
        EcmaStringAccessor::CreateFromUtf16(vm_, utf16Data, utf16Len, canBeCompress, MemSpaceType::OLD_SPACE);
    str->SetMixHashcode(result.second);
    InternString(str);
    return str;
}

EcmaString *EcmaStringTable::GetOrInternString(EcmaString *string)
{
    if (EcmaStringAccessor(string).IsInternString()) {
        return string;
    }
    JSHandle<EcmaString> strHandle(vm_->GetJSThread(), string);
    // may gc
    auto strFlat = EcmaStringAccessor::Flatten(vm_, strHandle, MemSpaceType::OLD_SPACE);
    if (EcmaStringAccessor(strFlat).IsInternString()) {
        return strFlat;
    }
    EcmaString *result = GetString(strFlat);
    if (result != nullptr) {
        return result;
    }

    if (EcmaStringAccessor(strFlat).NotTreeString()) {
        Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(strFlat));
        if (objectRegion->InYoungSpace()) {
            JSHandle<EcmaString> resultHandle(vm_->GetJSThread(), strFlat);
            strFlat = EcmaStringAccessor::CopyStringToOldSpace(vm_,
                resultHandle, EcmaStringAccessor(strFlat).GetLength(), EcmaStringAccessor(strFlat).IsUtf8());
        }
    }
    InternString(strFlat);
    return strFlat;
}

EcmaString *EcmaStringTable::InsertStringToTable(const JSHandle<EcmaString> &strHandle)
{
    auto strFlat = EcmaStringAccessor::Flatten(vm_, strHandle, MemSpaceType::OLD_SPACE);
    if (EcmaStringAccessor(strFlat).NotTreeString()) {
        Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(strFlat));
        if (objectRegion->InYoungSpace()) {
            JSHandle<EcmaString> resultHandle(vm_->GetJSThread(), strFlat);
            strFlat = EcmaStringAccessor::CopyStringToOldSpace(vm_,
                resultHandle, EcmaStringAccessor(strFlat).GetLength(), EcmaStringAccessor(strFlat).IsUtf8());
        }
    }
    InternString(strFlat);
    return strFlat;
}

EcmaString *EcmaStringTable::TryGetInternString(EcmaString *string)
{
    return GetString(string);
}

EcmaString *EcmaStringTable::GetOrInternStringWithSpaceType(const uint8_t *utf8Data, uint32_t utf8Len,
                                                            bool canBeCompress, MemSpaceType type,
                                                            bool isConstantString, uint32_t idOffset)
{
    std::pair<EcmaString *, uint32_t> result = GetString(utf8Data, utf8Len, canBeCompress);
    if (result.first != nullptr) {
        return result.first;
    }
    type = type == MemSpaceType::NON_MOVABLE ? MemSpaceType::NON_MOVABLE : MemSpaceType::OLD_SPACE;
    EcmaString *str;
    if (canBeCompress) {
        // Constant string will be created in this branch.
        str = EcmaStringAccessor::CreateFromUtf8(vm_, utf8Data, utf8Len, canBeCompress, type, isConstantString,
            idOffset);
    } else {
        str = EcmaStringAccessor::CreateFromUtf8(vm_, utf8Data, utf8Len, canBeCompress, type);
    }
    str->SetMixHashcode(result.second);
    InternString(str);
    return str;
}

EcmaString *EcmaStringTable::GetOrInternStringWithSpaceType(const uint8_t *utf8Data, uint32_t utf16Len,
                                                            MemSpaceType type)
{
    type = type == MemSpaceType::NON_MOVABLE ? MemSpaceType::NON_MOVABLE : MemSpaceType::OLD_SPACE;
    EcmaString *str = EcmaStringAccessor::CreateUtf16StringFromUtf8(vm_, utf8Data, utf16Len, type);
    EcmaString *result = GetString(str);
    if (result != nullptr) {
        return result;
    }
    InternString(str);
    return str;
}

void EcmaStringTable::SweepWeakReference(const WeakRootVisitor &visitor)
{
    for (auto it = table_.begin(); it != table_.end();) {
        // Strings in string table should not be in the young space. Only old gc will sweep string table.
        auto *object = it->second;
        auto fwd = visitor(object);
        ASSERT(!Region::ObjectAddressToRange(object)->InYoungSpace());
        if (fwd == nullptr) {
            LOG_ECMA(VERBOSE) << "StringTable: delete string " << std::hex << object;
            it = table_.erase(it);
        } else if (fwd != object) {
            it->second = static_cast<EcmaString *>(fwd);
            ++it;
            LOG_ECMA(VERBOSE) << "StringTable: forward " << std::hex << object << " -> " << fwd;
        } else {
            ++it;
        }
    }
}

void EcmaStringTable::RelocateConstantData(const JSPandaFile *jsPandaFile)
{
    auto thread = vm_->GetJSThread();
    for (auto it = table_.begin(); it != table_.end();) {
        auto *object = it->second;
        if (!EcmaStringAccessor(object).IsConstantString()) {
            ++it;
            continue;
        }
        auto constantStr = ConstantString::Cast(object);
        if (constantStr->GetEntityId() < 0 || !jsPandaFile->Contain(constantStr->GetConstantData())) {
            // EntityId is -1, which means this str has been relocated. Or the data is not in pandafile.
            ++it;
            continue;
        }
        uint32_t id = constantStr->GetEntityIdU32();
        panda_file::File::StringData sd = jsPandaFile->GetStringData(EntityId(id));
        if (constantStr->GetConstantData() == sd.data) {
            uint32_t strLen = sd.utf16_length;
            if (UNLIKELY(strLen == 0)) {
                it->second = *(vm_->GetFactory()->GetEmptyString());
            }
            size_t byteLength = sd.is_ascii ? 1 : sizeof(uint16_t);
            JSHandle<ByteArray> newData = vm_->GetFactory()->NewByteArray(
                strLen, byteLength, reinterpret_cast<void *>(const_cast<uint8_t *>(sd.data)),
                MemSpaceType::NON_MOVABLE);
            constantStr->SetRelocatedData(thread, newData.GetTaggedValue());
            constantStr->SetConstantData(static_cast<uint8_t *>(newData->GetData()));
            constantStr->SetEntityId(-1);
        } else {
            LOG_ECMA(ERROR) << "ConstantString data pointer is inconsistent with sd.data";
        }
        ++it;
    }
}

bool EcmaStringTable::CheckStringTableValidity()
{
    for (auto itemOuter = table_.begin(); itemOuter != table_.end(); ++itemOuter) {
        auto outerString = itemOuter->second;
        if (!EcmaStringAccessor(outerString).NotTreeString()) {
            return false;
        }
        int counter = 0;
        auto hashcode = EcmaStringAccessor(outerString).GetHashcode();
        auto range = table_.equal_range(hashcode);
        auto it = range.first;
        for (; it != range.second; ++it) {
            auto foundString = it->second;
            if (EcmaStringAccessor::StringsAreEqual(foundString, outerString)) {
                ++counter;
            }
        }
        if (counter > 1) {
            return false;
        }
    }
    return true;
}

void SingleCharTable::CreateSingleCharTable(JSThread *thread)
{
    auto table = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(MAX_ONEBYTE_CHARCODE,
        JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    for (uint32_t i = 1; i < MAX_ONEBYTE_CHARCODE; ++i) {
        std::string tmp(1, i + 0X00); // 1: size
        table->Set(thread, i, factory->NewFromASCIINonMovable(tmp).GetTaggedValue());
    }
    thread->SetSingleCharTable((table.GetTaggedValue()));
}
}  // namespace panda::ecmascript
