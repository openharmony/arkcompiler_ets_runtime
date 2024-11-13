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

#include "ecmascript/js_object-inl.h"
#include "pgo_profiler/pgo_profiler_layout.h"

namespace panda::ecmascript {
using PGOHandler = pgo::PGOHandler;
void LayoutInfo::Initialize(const JSThread *thread, int num)
{
    SetExtraLength(num);
    int propNum = GetPropertiesCapacity();
    auto attr = PropertyAttributes();
    for (int i = 0; i < propNum; i++) {
        SetPropertyInit(thread, i, JSTaggedValue::Hole(), attr);
    }
}

void LayoutInfo::GetAllKeys(const JSThread *thread, int end, int offset, TaggedArray *keyArray)
{
    ASSERT(end <= NumberOfElements());
    ASSERT_PRINT(offset + end <= static_cast<int>(keyArray->GetLength()),
                 "keyArray capacity is not enough for dictionary");

    DISALLOW_GARBAGE_COLLECTION;
    int enumKeys = 0;
    for (int i = 0; i < end; i++) {
        JSTaggedValue key = GetKey(i);
        if (key.IsString()) {
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
                                    TaggedArray *keyArray, uint32_t filter)
{
    ASSERT(numberOfProps <= static_cast<uint32_t>(NumberOfElements()));
    ASSERT_PRINT(keyArrayEffectivelength + numberOfProps <= keyArray->GetLength(),
                 "keyArray capacity is not enough for dictionary");

    DISALLOW_GARBAGE_COLLECTION;
    uint32_t enumKeys = 0;
    for (uint32_t i = 0; i < numberOfProps; i++) {
        JSTaggedValue key = GetKey(static_cast<int>(i));
        if (key.IsString() && !(filter & NATIVE_KEY_SKIP_STRINGS)) {
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

void LayoutInfo::GetAllKeysForSerialization(int end, std::vector<JSTaggedValue> &keyVector)
{
    ASSERT(end <= NumberOfElements());
    for (int i = 0; i < end; i++) {
        JSTaggedValue key = GetKey(i);
        if (key.IsString() || key.IsSymbol()) {
            keyVector.emplace_back(key);
        }
    }
}

std::pair<uint32_t, uint32_t> LayoutInfo::GetNumOfEnumKeys(int end) const
{
    ASSERT(end <= NumberOfElements());
    uint32_t enumKeys = 0;
    uint32_t shadowKeys = 0;
    for (int i = 0; i < end; i++) {
        JSTaggedValue key = GetKey(i);
        if (!key.IsString()) {
            continue;
        }
        if (GetAttr(i).IsEnumerable()) {
            enumKeys++;
        } else {
            shadowKeys++;
        }
    }
    return std::make_pair(enumKeys, shadowKeys);
}

void LayoutInfo::GetAllEnumKeys(JSThread *thread, int end, int offset, JSHandle<TaggedArray> keyArray,
                                uint32_t *keys, JSHandle<TaggedQueue> shadowQueue, int32_t lastLength)
{
    ASSERT(end <= NumberOfElements());
    ASSERT_PRINT(offset <= static_cast<int>(keyArray->GetLength()),
                 "keyArray capacity is not enough for dictionary");
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    int enumKeys = 0;
    for (int i = 0; i < end; i++) {
        keyHandle.Update(GetKey(i));
        if (!keyHandle->IsString()) {
            continue;
        }
        if (GetAttr(i).IsEnumerable()) {
            bool isDuplicated = JSObject::IsDepulicateKeys(thread, keyArray, lastLength, shadowQueue, keyHandle);
            if (isDuplicated) {
                continue;
            }
            keyArray->Set(thread, enumKeys + offset, keyHandle);
            enumKeys++;
        } else {
            TaggedQueue::PushFixedQueue(thread, shadowQueue, keyHandle);
        }
    }
    *keys += enumKeys;
}

void LayoutInfo::GetAllEnumKeys(JSThread *thread, int end, int offset, JSHandle<TaggedArray> keyArray, uint32_t *keys)
{
    ASSERT(end <= NumberOfElements());
    ASSERT_PRINT(offset <= static_cast<int>(keyArray->GetLength()),
                 "keyArray capacity is not enough for dictionary");
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    int enumKeys = 0;
    for (int i = 0; i < end; i++) {
        keyHandle.Update(GetKey(i));
        if (keyHandle->IsString() && GetAttr(i).IsEnumerable()) {
            keyArray->Set(thread, enumKeys + offset, keyHandle);
            enumKeys++;
        }
    }
    *keys += enumKeys;
}

CString LayoutInfo::GetSymbolKeyString(JSTaggedValue key)
{
    auto symbol = JSSymbol::Cast(key);
    if (!symbol->HasId()) {
        return "";
    }
    auto id = symbol->GetPrivateId();
    auto symbolDesc = symbol->GetDescription();
    if (symbolDesc.IsUndefined()) {
        return ToCString(id);
    }
    if (!symbolDesc.IsString()) {
        return "";
    }
    CString str = EcmaStringAccessor(symbolDesc).ToCString();
    if (str != "method") {
        return "";
    }
    return str.append("_").append(ToCString(id));
}

void LayoutInfo::DumpFieldIndexByPGO(int index, pgo::HClassLayoutDesc* desc)
{
    auto key = GetKey(index);
    auto attr = GetAttr(index);
    SetIsPGODumped(index);
    TrackType type = attr.GetTrackType();
    int propertyMeta = attr.GetPropertyMetaData();
    if (key.IsString()) {
        auto keyString = EcmaStringAccessor(key).ToCString();
        desc->InsertKeyAndDesc(keyString, PGOHandler(type, propertyMeta, false));
    } else if (key.IsSymbol()) {
        auto keyString = GetSymbolKeyString(key);
        if (keyString.empty()) {
            return;
        }
        desc->InsertKeyAndDesc(keyString, PGOHandler(type, propertyMeta, true));
    }
}

bool LayoutInfo::UpdateFieldIndexByPGO(int index, pgo::HClassLayoutDesc* desc)
{
    auto key = GetKey(index);
    auto attr = GetAttr(index);
    if (attr.IsPGODumped()) {
        return true;
    }
    SetIsPGODumped(index);
    TrackType type = attr.GetTrackType();
    int propertyMeta = attr.GetPropertyMetaData();
    if (key.IsString()) {
        auto keyString = EcmaStringAccessor(key).ToCString();
        return desc->UpdateKeyAndDesc(keyString, PGOHandler(type, propertyMeta, false));
    } else if (key.IsSymbol()) {
        auto keyString = GetSymbolKeyString(key);
        if (keyString.empty()) {
            return false;
        }
        return desc->UpdateKeyAndDesc(keyString, PGOHandler(type, propertyMeta, true));
    }
    return false;
}
}  // namespace panda::ecmascript
