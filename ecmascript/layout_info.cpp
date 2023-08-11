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

#include "ecmascript/layout_info-inl.h"

#include "ecmascript/ecma_string.h"
#include "ecmascript/filter_helper.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/mem/assert_scope.h"

namespace panda::ecmascript {
void LayoutInfo::Initialize(const JSThread *thread, int num)
{
    SetExtraLength(num);
    int propNum = GetPropertiesCapacity();
    auto attr = PropertyAttributes();
    for (int i = 0; i < propNum; i++) {
        SetPropertyInit(thread, i, JSTaggedValue::Hole(), attr);
    }
}

void LayoutInfo::AddKey(const JSThread *thread, [[maybe_unused]] int index, const JSTaggedValue &key,
                        const PropertyAttributes &attr)
{
    DISALLOW_GARBAGE_COLLECTION;
    int number = NumberOfElements();
    ASSERT(attr.GetOffset() == static_cast<uint32_t>(number));
    ASSERT(number + 1 <= GetPropertiesCapacity());
    ASSERT(number == index);
    SetNumberOfElements(thread, number + 1);
    SetPropertyInit(thread, number, key, attr);

    uint32_t keyHash = key.GetKeyHashCode();
    int insertIndex = number;
    for (; insertIndex > 0; --insertIndex) {
        JSTaggedValue prevKey = GetSortedKey(insertIndex - 1);
        if (prevKey.GetKeyHashCode() <= keyHash) {
            break;
        }
        SetSortedIndex(thread, insertIndex, GetSortedIndex(insertIndex - 1));
    }
    SetSortedIndex(thread, insertIndex, number);
}

void LayoutInfo::GetAllKeys(const JSThread *thread, int end, int offset, TaggedArray *keyArray,
                            const JSHandle<JSObject> object)
{
    ASSERT(end <= NumberOfElements());
    ASSERT_PRINT(offset + end <= static_cast<int>(keyArray->GetLength()),
                 "keyArray capacity is not enough for dictionary");

    DISALLOW_GARBAGE_COLLECTION;
    int enumKeys = 0;
    for (int i = 0; i < end; i++) {
        JSTaggedValue key = GetKey(i);
        if (key.IsString()) {
            if (IsUninitializedProperty(object, i)) {
                continue;
            }
            keyArray->Set(thread, enumKeys + offset, key);
            enumKeys++;
        }
    }

    if (enumKeys < end) {
        for (int i = 0; i < end; i++) {
            JSTaggedValue key = GetKey(i);
            if (key.IsSymbol()) {
                keyArray->Set(thread, enumKeys + offset, key);
                enumKeys++;
            }
        }
    }
}
void LayoutInfo::GetAllKeysByFilter(const JSThread *thread, uint32_t numberOfProps, uint32_t &keyArrayEffectivelength,
                                    TaggedArray *keyArray, const JSHandle<JSObject> object, uint32_t filter)
{
    ASSERT(numberOfProps <= static_cast<uint32_t>(NumberOfElements()));
    ASSERT_PRINT(keyArrayEffectivelength + numberOfProps <= keyArray->GetLength(),
                 "keyArray capacity is not enough for dictionary");

    DISALLOW_GARBAGE_COLLECTION;
    uint32_t enumKeys = 0;
    for (uint32_t i = 0; i < numberOfProps; i++) {
        JSTaggedValue key = GetKey(static_cast<int>(i));
        if (key.IsString() && !(filter & NATIVE_KEY_SKIP_STRINGS)) {
            if (IsUninitializedProperty(object, i)) {
                continue;
            }
            PropertyAttributes attr = GetAttr(static_cast<int>(i));
            bool bIgnore = FilterHelper::IgnoreKeyByFilter<PropertyAttributes>(attr, filter);
            if (bIgnore) {
                continue;
            }
            keyArray->Set(thread, keyArrayEffectivelength, key);
            keyArrayEffectivelength++;
            enumKeys++;
        }
    }

    if (enumKeys < numberOfProps) {
        for (uint32_t i = 0; i < numberOfProps; i++) {
            JSTaggedValue key = GetKey(static_cast<int>(i));
            if (key.IsSymbol() && !(filter & NATIVE_KEY_SKIP_SYMBOLS)) {
                PropertyAttributes attr = GetAttr(static_cast<int>(i));
                bool bIgnore = FilterHelper::IgnoreKeyByFilter<PropertyAttributes>(attr, filter);
                if (bIgnore) {
                    continue;
                }
                keyArray->Set(thread, keyArrayEffectivelength, key);
                keyArrayEffectivelength++;
            }
        }
    }
}

void LayoutInfo::GetAllKeys(int end, std::vector<JSTaggedValue> &keyVector, const JSHandle<JSObject> object)
{
    ASSERT(end <= NumberOfElements());
    for (int i = 0; i < end; i++) {
        JSTaggedValue key = GetKey(i);
        if (key.IsString() || key.IsSymbol()) {
            if (IsUninitializedProperty(object, i)) {
                continue;
            }
            keyVector.emplace_back(key);
        }
    }
}

void LayoutInfo::GetAllEnumKeys(const JSThread *thread, int end, int offset, TaggedArray *keyArray,
                                uint32_t *keys, const JSHandle<JSObject> object)
{
    ASSERT(end <= NumberOfElements());
    ASSERT_PRINT(offset + end <= static_cast<int>(keyArray->GetLength()),
                 "keyArray capacity is not enough for dictionary");

    DISALLOW_GARBAGE_COLLECTION;
    int enumKeys = 0;
    for (int i = 0; i < end; i++) {
        JSTaggedValue key = GetKey(i);
        if (key.IsString() && GetAttr(i).IsEnumerable()) {
            if (IsUninitializedProperty(object, i)) {
                continue;
            }
            keyArray->Set(thread, enumKeys + offset, key);
            enumKeys++;
        }
    }
    *keys += enumKeys;
}

bool LayoutInfo::IsUninitializedProperty(const JSHandle<JSObject> object, uint32_t index)
{
    PropertyAttributes attr = GetAttr(index);
    if (!attr.IsInlinedProps()) {
        return false;
    }

    JSTaggedValue val = object->GetPropertyInlinedProps(attr.GetOffset());
    return val.IsHole();
}

void LayoutInfo::DumpFieldIndexForProfile(int index, PGOHClassLayoutDesc &desc, PGOObjKind kind)
{
    auto key = GetKey(index);
    if (key.IsString()) {
        auto attr = GetAttr(index);
        TrackType type = attr.GetTrackType();
        bool isAccessor = attr.IsAccessor();
        auto keyString = EcmaStringAccessor(key).ToCString();
        desc.UpdateKeyAndDesc(keyString.c_str(), PGOHandler(type, isAccessor), kind);
    }
}
}  // namespace panda::ecmascript
