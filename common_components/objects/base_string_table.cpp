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

#include "common_interfaces/objects/base_string_table.h"

#include "common_components/base/globals.h"
#include "common_interfaces/objects/composite_base_class.h"
#include "common_components/objects/string_table/hashtriemap.h"
#include "common_components/objects/string_table/hashtriemap-inl.h"
#include "common_components/objects/string_table_internal.h"
#include "common_interfaces/objects/base_string.h"
#include "common_interfaces/thread/thread_holder.h"
#include "common_interfaces/thread/thread_state_transition.h"
#include "heap/heap_allocator.h"

namespace common {
BaseString* BaseStringTableImpl::AllocateLineStringObject(size_t size)
{
    size = AlignUp(size, ALIGN_OBJECT);
    BaseString* str;
    if (size > MAX_REGULAR_HEAP_OBJECT_SIZE) {
        str = reinterpret_cast<BaseString*>(HeapAllocator::AllocateInHuge(size, LanguageType::DYNAMIC));
    } else {
        str = reinterpret_cast<BaseString*>(HeapAllocator::AllocateInOld(size, LanguageType::DYNAMIC));
    }
    BaseClass* cls = BaseRuntime::GetInstance()->GetBaseClassRoots().GetBaseClass(CommonType::LINE_STRING);
    str->SetFullBaseClassWithoutBarrier(cls);
    return str;
}

BaseString* BaseStringTableImpl::GetOrInternFlattenString(ThreadHolder* holder, const HandleCreator& handleCreator,
                                                          BaseString* string)
{
    ASSERT(string->NotTreeString());
    if (string->IsInternString()) {
        return string;
    }
    auto readBarrier = [](void* obj, size_t offset)-> BaseObject* {
        return BaseObject::Cast(
            reinterpret_cast<MAddress>(BaseRuntime::ReadBarrier(
                obj, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(obj) + offset))));
    };
    uint32_t hashcode = string->GetHashcode(readBarrier);
    // Strings in string table should not be in the young space.
    auto loadResult = stringTable_.Load(readBarrier, hashcode, string);
    if (loadResult.value != nullptr) {
        return loadResult.value;
    }
    ReadOnlyHandle<BaseString> stringHandle = handleCreator(holder, string);
    BaseString* result = stringTable_.StoreOrLoad(
        holder, readBarrier, hashcode, loadResult, stringHandle);
    ASSERT(result != nullptr);
    return result;
}

BaseString* BaseStringTableImpl::GetOrInternStringFromCompressedSubString(ThreadHolder* holder,
                                                                          const HandleCreator& handleCreator,
                                                                          const ReadOnlyHandle<BaseString>& string,
                                                                          uint32_t offset, uint32_t utf8Len)
{
    const uint8_t* utf8Data = string->GetDataUtf8() + offset;
    uint32_t hashcode = BaseString::ComputeHashcodeUtf8(utf8Data, utf8Len, true);
    auto readBarrier = [](void* obj, size_t offset)-> BaseObject* {
        return BaseObject::Cast(
            reinterpret_cast<MAddress>(BaseRuntime::ReadBarrier(
                obj, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(obj) + offset))));
    };
    auto loadResult = stringTable_.Load(readBarrier, hashcode, string, offset, utf8Len);
    if (loadResult.value != nullptr) {
        return loadResult.value;
    }
    auto allocator = [this](size_t size, CommonType type)-> BaseString* {
        ASSERT(type == CommonType::LINE_STRING);
        return AllocateLineStringObject(size);
    };
    BaseString* result = stringTable_.StoreOrLoad(
        holder, hashcode, loadResult,
        [holder, string, offset, utf8Len, hashcode, handleCreator, allocator]() {
            BaseString* str = BaseString::CreateFromUtf8CompressedSubString(
                std::move(allocator), string, offset, utf8Len);
            str->SetRawHashcode(hashcode);
            ASSERT(!str->IsInternString());
            ASSERT(str->NotTreeString());
            // Strings in string table should not be in the young space.
            ReadOnlyHandle<BaseString> strHandle = handleCreator(holder, str);
            return strHandle;
        },
        [utf8Len, string, offset](const BaseString* foundString) {
            const uint8_t* utf8Data = string->GetDataUtf8() + offset;
            auto readBarrier = [](void* obj, size_t offset)-> BaseObject* {
                return BaseObject::Cast(
                    reinterpret_cast<MAddress>(BaseRuntime::ReadBarrier(
                        obj, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(obj) + offset))));
            };
            return BaseString::StringIsEqualUint8Data(readBarrier, foundString, utf8Data, utf8Len, true);
        });
    ASSERT(result != nullptr);
    return result;
}

BaseString* BaseStringTableImpl::GetOrInternString(ThreadHolder* holder,
                                                   const HandleCreator& handleCreator, const uint8_t* utf8Data,
                                                   uint32_t utf8Len,
                                                   bool canBeCompress)
{
    uint32_t hashcode = BaseString::ComputeHashcodeUtf8(utf8Data, utf8Len, canBeCompress);
    auto allocator = [this](size_t size, CommonType type)-> BaseString* {
        ASSERT(type == CommonType::LINE_STRING);
        return AllocateLineStringObject(size);
    };
    BaseString* result = stringTable_.LoadOrStore<true>(
        holder, hashcode,
        [holder, hashcode, utf8Data, utf8Len, canBeCompress, handleCreator, allocator]() {
            BaseString* value = BaseString::CreateFromUtf8(std::move(allocator), utf8Data, utf8Len, canBeCompress);
            value->SetRawHashcode(hashcode);
            ASSERT(!value->IsInternString());
            ASSERT(value->NotTreeString());
            ReadOnlyHandle<BaseString> stringHandle = handleCreator(holder, value);
            return stringHandle;
        },
        [utf8Data, utf8Len, canBeCompress](BaseString* foundString) {
            auto readBarrier = [](void* obj, size_t offset)-> BaseObject* {
                return BaseObject::Cast(
                    reinterpret_cast<MAddress>(BaseRuntime::ReadBarrier(
                        obj, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(obj) + offset))));
            };
            return BaseString::StringIsEqualUint8Data(readBarrier, foundString, utf8Data, utf8Len,
                                                      canBeCompress);
        });
    ASSERT(result != nullptr);
    return result;
}

BaseString* BaseStringTableImpl::GetOrInternString(ThreadHolder* holder, const HandleCreator& handleCreator,
                                                   const uint16_t* utf16Data, uint32_t utf16Len,
                                                   bool canBeCompress)
{
    uint32_t hashcode = BaseString::ComputeHashcodeUtf16(const_cast<uint16_t*>(utf16Data), utf16Len);
    auto allocator = [this](size_t size, CommonType type)-> BaseString* {
        ASSERT(type == CommonType::LINE_STRING);
        return AllocateLineStringObject(size);
    };
    BaseString* result = stringTable_.LoadOrStore<true>(
        holder, hashcode,
        [holder, utf16Data, utf16Len, canBeCompress, hashcode, handleCreator, allocator]() {
            BaseString* value = BaseString::CreateFromUtf16(std::move(allocator), utf16Data, utf16Len,
                                                            canBeCompress);
            value->SetRawHashcode(hashcode);
            ASSERT(!value->IsInternString());
            ASSERT(value->NotTreeString());
            // Strings in string table should not be in the young space.
            ReadOnlyHandle<BaseString> stringHandle = handleCreator(holder, value);
            return stringHandle;
        },
        [utf16Data, utf16Len](BaseString* foundString) {
            auto readBarrier = [](void* obj, size_t offset)-> BaseObject* {
                return BaseObject::Cast(
                    reinterpret_cast<MAddress>(BaseRuntime::ReadBarrier(
                        obj, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(obj) + offset))));
            };
            return BaseString::StringsAreEqualUtf16(readBarrier, foundString, utf16Data, utf16Len);
        });
    ASSERT(result != nullptr);
    return result;
}

BaseString* BaseStringTableImpl::TryGetInternString(const ReadOnlyHandle<BaseString>& string)
{
    auto readBarrier = [](void* obj, size_t offset)-> BaseObject* {
        return BaseObject::Cast(
            reinterpret_cast<MAddress>(BaseRuntime::ReadBarrier(
                obj, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(obj) + offset))));
    };
    uint32_t hashcode = string->GetHashcode(readBarrier);
    return stringTable_.Load<false>(readBarrier, hashcode, *string);
}
}
