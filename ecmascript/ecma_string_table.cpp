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
#include "ecmascript/mem/c_string.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
EcmaStringTable::EcmaStringTable(const EcmaVM *vm) : vm_(vm) {}

EcmaString *EcmaStringTable::GetString(const JSHandle<EcmaString> &firstString,
                                       const JSHandle<EcmaString> &secondString) const
{
    ASSERT(EcmaStringAccessor(firstString).IsLineOrConstantString());
    ASSERT(EcmaStringAccessor(secondString).IsLineOrConstantString());
    uint32_t hashCode = EcmaStringAccessor(firstString).GetHashcode();
    hashCode = EcmaStringAccessor(secondString).ComputeHashcode(hashCode);
    auto range = table_.equal_range(hashCode);
    for (auto item = range.first; item != range.second; ++item) {
        auto foundString = item->second;
        if (EcmaStringAccessor(foundString).EqualToSplicedString(*firstString, *secondString)) {
            return foundString;
        }
    }
    return nullptr;
}

EcmaString *EcmaStringTable::GetString(const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress) const
{
    uint32_t hashCode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, canBeCompress);
    auto range = table_.equal_range(hashCode);
    for (auto item = range.first; item != range.second; ++item) {
        auto foundString = item->second;
        if (EcmaStringAccessor::StringsAreEqualUtf8(foundString, utf8Data, utf8Len, canBeCompress)) {
            return foundString;
        }
    }
    return nullptr;
}

EcmaString *EcmaStringTable::GetString(const uint16_t *utf16Data, uint32_t utf16Len) const
{
    uint32_t hashCode = EcmaStringAccessor::ComputeHashcodeUtf16(const_cast<uint16_t *>(utf16Data), utf16Len);
    auto range = table_.equal_range(hashCode);
    for (auto item = range.first; item != range.second; ++item) {
        auto foundString = item->second;
        if (EcmaStringAccessor::StringsAreEqualUtf16(foundString, utf16Data, utf16Len)) {
            return foundString;
        }
    }
    return nullptr;
}

EcmaString *EcmaStringTable::GetString(EcmaString *string) const
{
    ASSERT(EcmaStringAccessor(string).IsLineOrConstantString());
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
    ASSERT(EcmaStringAccessor(string).IsLineOrConstantString());
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
    EcmaString *concatString = GetString(firstFlat, secondFlat);
    if (concatString != nullptr) {
        return concatString;
    }
    JSHandle<EcmaString> concatHandle(vm_->GetJSThread(),
        EcmaStringAccessor::Concat(vm_, firstFlat, secondFlat, MemSpaceType::OLD_SPACE));
    concatString = EcmaStringAccessor::Flatten(vm_, concatHandle, MemSpaceType::OLD_SPACE);
    InternString(concatString);
    return concatString;
}

EcmaString *EcmaStringTable::GetOrInternString(const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress)
{
    EcmaString *result = GetString(utf8Data, utf8Len, canBeCompress);
    if (result != nullptr) {
        return result;
    }

    result = EcmaStringAccessor::CreateFromUtf8(vm_, utf8Data, utf8Len, canBeCompress, MemSpaceType::OLD_SPACE);
    InternString(result);
    return result;
}

/*
    This function is used to create global constant strings from non-movable sapce only.
    It only inserts string into string-table and provides no string-table validity check.
*/
EcmaString *EcmaStringTable::CreateAndInternStringNonMovable(const uint8_t *utf8Data, uint32_t utf8Len)
{
    EcmaString *result = GetString(utf8Data, utf8Len, true);
    if (result != nullptr) {
        return result;
    }

    result = EcmaStringAccessor::CreateFromUtf8(vm_, utf8Data, utf8Len, true, MemSpaceType::NON_MOVABLE);
    InternString(result);
    return result;
}

EcmaString *EcmaStringTable::GetOrInternString(const uint16_t *utf16Data, uint32_t utf16Len, bool canBeCompress)
{
    EcmaString *result = GetString(utf16Data, utf16Len);
    if (result != nullptr) {
        return result;
    }

    result = EcmaStringAccessor::CreateFromUtf16(vm_, utf16Data, utf16Len, canBeCompress, MemSpaceType::OLD_SPACE);
    InternString(result);
    return result;
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

    if (EcmaStringAccessor(strFlat).IsLineOrConstantString()) {
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

EcmaString *EcmaStringTable::GetOrInternStringWithSpaceType(const uint8_t *utf8Data, uint32_t utf8Len,
                                                            bool canBeCompress, MemSpaceType type,
                                                            bool isConstantString, uint32_t idOffset)
{
    EcmaString *result = GetString(utf8Data, utf8Len, canBeCompress);
    if (result != nullptr) {
        return result;
    }
    type = type == MemSpaceType::NON_MOVABLE ? MemSpaceType::NON_MOVABLE : MemSpaceType::OLD_SPACE;
    if (canBeCompress) {
        // Constant string will be created in this branch.
        result = EcmaStringAccessor::CreateFromUtf8(vm_, utf8Data, utf8Len, canBeCompress, type, isConstantString,
            idOffset);
    } else {
        result = EcmaStringAccessor::CreateFromUtf8(vm_, utf8Data, utf8Len, canBeCompress, type);
    }
    InternString(result);
    return result;
}

EcmaString *EcmaStringTable::GetOrInternStringWithSpaceType(const uint16_t *utf16Data, uint32_t utf16Len,
                                                            bool canBeCompress, MemSpaceType type)
{
    EcmaString *result = GetString(utf16Data, utf16Len);
    if (result != nullptr) {
        return result;
    }
    type = type == MemSpaceType::NON_MOVABLE ? MemSpaceType::NON_MOVABLE : MemSpaceType::OLD_SPACE;
    result = EcmaStringAccessor::CreateFromUtf16(vm_, utf16Data, utf16Len, canBeCompress, type);
    InternString(result);
    return result;
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
            table_.erase(it++);
        } else if (fwd != object) {
            it->second = static_cast<EcmaString *>(fwd);
            ++it;
            LOG_ECMA(VERBOSE) << "StringTable: forward " << std::hex << object << " -> " << fwd;
        } else {
            ++it;
        }
    }
}

bool EcmaStringTable::CheckStringTableValidity()
{
    for (auto itemOuter = table_.begin(); itemOuter != table_.end(); ++itemOuter) {
        auto outerString = itemOuter->second;
        if (!EcmaStringAccessor(outerString).IsLineOrConstantString()) {
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
}  // namespace panda::ecmascript
