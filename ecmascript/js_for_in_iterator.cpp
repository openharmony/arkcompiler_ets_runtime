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

#include "ecmascript/js_for_in_iterator.h"

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/js_object-inl.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
bool JSForInIterator::IsEnumCacheValid(JSTaggedValue receiver, JSTaggedValue cachedHclass, EnumCacheKind kind)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHClass *hclass = receiver.GetTaggedObject()->GetClass();
    if (JSTaggedValue(hclass) != cachedHclass) {
        return false;
    }
    if (kind == EnumCacheKind::SIMPLE) {
        return true;
    }
    ASSERT(kind == EnumCacheKind::PROTOCHAIN);
    JSTaggedValue proto = hclass->GetPrototype();
    if (!proto.IsECMAObject()) {
        return false;
    }
    JSTaggedValue protoChangeMarker = proto.GetTaggedObject()->GetClass()->GetProtoChangeMarker();
    if (protoChangeMarker.IsProtoChangeMarker()) {
        if (!ProtoChangeMarker::Cast(protoChangeMarker.GetTaggedObject())->GetHasChanged()) {
            return true;
        }
    }
    return false;
}

bool JSForInIterator::NeedCheckProperty(JSTaggedValue receiver)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue current = receiver;
    while (current.IsHeapObject()) {
        if (!current.IsJSObject() || current.GetTaggedObject()->GetClass()->HasDeleteProperty()) {
            return true;
        }
        current = JSObject::GetPrototype(current);
    }
    return false;
}

bool JSForInIterator::HasProperty(JSThread *thread, JSHandle<JSTaggedValue> receiver, JSHandle<JSTaggedValue> key)
{
    JSMutableHandle<JSTaggedValue> current(thread, receiver.GetTaggedValue());
    while (current->IsHeapObject()) {
        PropertyDescriptor desc(thread);
        bool has = JSTaggedValue::GetOwnProperty(thread, current, key, desc);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        if (has && desc.IsEnumerable()) {
            return true;
        }
        current.Update(JSTaggedValue::GetPrototype(thread, current));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    }
    return false;
}

JSTaggedValue JSForInIterator::NextInternal(JSThread *thread, const JSHandle<JSForInIterator> &it)
{
    uint32_t length = it->GetLength();
    uint32_t index = it->GetIndex();
    if (index >= length) {
        return JSTaggedValue::Undefined();
    }
    JSTaggedValue taggedKeys = it->GetKeys();
    JSTaggedValue receiver = it->GetObject();
    EnumCacheKind kind = JSObject::GetEnumCacheKind(thread, taggedKeys);
    TaggedArray *keys = TaggedArray::Cast(taggedKeys.GetTaggedObject());
    if (IsEnumCacheValid(receiver, it->GetCachedHclass(), kind)) {
        JSTaggedValue key = keys->Get(index);
        index++;
        it->SetIndex(index);
        return key;
    }

    if (!NeedCheckProperty(receiver)) {
        JSTaggedValue key = keys->Get(index);
        index++;
        it->SetIndex(index);
        return key;
    }
    // slow path
    return NextInternalSlowpath(thread, it);
}

JSTaggedValue JSForInIterator::NextInternalSlowpath(JSThread *thread, const JSHandle<JSForInIterator> &it)
{
    uint32_t length = it->GetLength();
    uint32_t index = it->GetIndex();
    if (index >= length) {
        return JSTaggedValue::Undefined();
    }
    JSHandle<TaggedArray> keysHandle(thread, it->GetKeys());
    JSHandle<JSTaggedValue> receiverHandle(thread, it->GetObject());
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    bool has = false;
    while (index < length) {
        keyHandle.Update(keysHandle->Get(index));
        if (keyHandle->IsUndefined()) {
            has = false;
            break;
        }
        has = HasProperty(thread, receiverHandle, keyHandle);
        if (has) {
            break;
        }
        index++;
    }
    if (!has) {
        it->SetIndex(index);
        return JSTaggedValue::Undefined();
    }

    JSTaggedValue key = keysHandle->Get(index);
    index++;
    it->SetIndex(index);
    return key;
}

// 13.7.5.16.2.1 %ForInIteratorPrototype%.next ( )
JSTaggedValue JSForInIterator::Next(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSForInIterator> it(BuiltinsBase::GetThis(msg));
    ASSERT(JSHandle<JSTaggedValue>(it)->IsForinIterator());
    JSTaggedValue res = NextInternal(thread, it);
    return res;
}
}  // namespace panda::ecmascript
