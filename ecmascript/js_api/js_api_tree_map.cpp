/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/js_api/js_api_tree_map.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/tagged_tree.h"

namespace panda::ecmascript {
using ContainerError = containers::ContainerError;
using ErrorFlag = containers::ErrorFlag;
void JSAPITreeMap::Set(JSThread *thread, const JSHandle<JSAPITreeMap> &map, const JSHandle<JSTaggedValue> &key,
                       const JSHandle<JSTaggedValue> &value)
{
    if (!TaggedTreeMap::IsKey(key.GetTaggedValue())) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, key.GetTaggedValue());
        RETURN_IF_ABRUPT_COMPLETION(thread);
        CString errorMsg =
            "The type of \"key\" must be not null. Received value is: " + ConvertToString(thread, *result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN(thread, error);
    }
    JSHandle<TaggedTreeMap> mapHandle(thread, TaggedTreeMap::Cast(map->GetTreeMap(thread).GetTaggedObject()));

    JSTaggedValue newMap = TaggedTreeMap::Set(thread, mapHandle, key, value);
    RETURN_IF_ABRUPT_COMPLETION(thread);
    map->SetTreeMap(thread, newMap);
}

JSTaggedValue JSAPITreeMap::Get(JSThread *thread, const JSHandle<JSAPITreeMap> &map, const JSHandle<JSTaggedValue> &key)
{
    JSHandle<TaggedTreeMap> mapHandle(thread, TaggedTreeMap::Cast(map->GetTreeMap(thread).GetTaggedObject()));
    return TaggedTreeMap::Get(thread, mapHandle, key);
}

int JSAPITreeMap::GetSize(const JSThread *thread) const
{
    return TaggedTreeMap::Cast(GetTreeMap(thread).GetTaggedObject())->NumberOfElements();
}

JSTaggedValue JSAPITreeMap::GetKey(const JSThread *thread, int entry) const
{
    ASSERT_PRINT(entry < GetSize(thread), "entry must less than capacity");
    JSTaggedValue key = TaggedTreeMap::Cast(GetTreeMap(thread).GetTaggedObject())->GetKey(thread, entry);
    return key.IsHole() ? JSTaggedValue::Undefined() : key;
}

JSTaggedValue JSAPITreeMap::GetValue(const JSThread *thread, int entry) const
{
    ASSERT_PRINT(entry < GetSize(thread), "entry must less than capacity");
    JSTaggedValue value = TaggedTreeMap::Cast(GetTreeMap(thread).GetTaggedObject())->GetValue(thread, entry);
    return value.IsHole() ? JSTaggedValue::Undefined() : value;
}

JSTaggedValue JSAPITreeMap::Delete(JSThread *thread, const JSHandle<JSAPITreeMap> &map,
                                   const JSHandle<JSTaggedValue> &key)
{
    JSHandle<TaggedTreeMap> mapHandle(thread, TaggedTreeMap::Cast(map->GetTreeMap(thread).GetTaggedObject()));
    int entry = TaggedTreeMap::FindEntry(thread, mapHandle, key);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (entry < 0) {
        return JSTaggedValue::Undefined();
    }
    JSHandle<JSTaggedValue> value(thread, mapHandle->GetValue(thread, entry));
    JSTaggedValue newMap = TaggedTreeMap::Delete(thread, mapHandle, entry);
    map->SetTreeMap(thread, newMap);
    return value.GetTaggedValue();
}

bool JSAPITreeMap::HasKey(JSThread *thread, const JSHandle<JSAPITreeMap> &map, const JSHandle<JSTaggedValue> &key)
{
    JSHandle<TaggedTreeMap> mapHandle(thread, TaggedTreeMap::Cast(map->GetTreeMap(thread).GetTaggedObject()));
    return TaggedTreeMap::FindEntry(thread, mapHandle, key) >= 0;
}

bool JSAPITreeMap::HasValue(JSThread *thread, const JSHandle<JSTaggedValue> &value) const
{
    JSHandle<TaggedTreeMap> mapHandle(thread, TaggedTreeMap::Cast(GetTreeMap(thread).GetTaggedObject()));
    return mapHandle->HasValue(thread, value.GetTaggedValue());
}

void JSAPITreeMap::Clear(const JSThread *thread, const JSHandle<JSAPITreeMap> &map)
{
    int cap = map->GetSize(thread);
    if (cap == 0) {
        return;
    }
    JSTaggedValue fn = TaggedTreeMap::Cast(map->GetTreeMap(thread).GetTaggedObject())->GetCompare(thread);
    JSHandle<JSTaggedValue> compareFn = JSHandle<JSTaggedValue>(thread, fn);
    cap = std::max(cap, TaggedTreeMap::MIN_CAPACITY);
    JSTaggedValue internal = TaggedTreeMap::Create(thread, cap);
    if (!compareFn->IsUndefined() && !compareFn->IsNull()) {
        TaggedTreeMap::Cast(internal.GetTaggedObject())->SetCompare(thread, compareFn.GetTaggedValue());
    }
    map->SetTreeMap(thread, internal);
}

bool JSAPITreeMap::Replace(JSThread *thread, const JSHandle<JSAPITreeMap> &map, const JSHandle<JSTaggedValue> &key,
                           const JSHandle<JSTaggedValue> &value)
{
    JSHandle<TaggedTreeMap> mapHandle(thread, TaggedTreeMap::Cast(map->GetTreeMap(thread).GetTaggedObject()));
    int index = TaggedTreeMap::FindEntry(thread, mapHandle, key);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (index < 0) {
        return false;
    }
    mapHandle->SetValue(thread, index, value.GetTaggedValue());
    return true;
}
}  // namespace panda::ecmascript
