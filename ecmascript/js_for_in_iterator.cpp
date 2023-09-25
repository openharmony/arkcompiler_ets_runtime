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
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/tagged_queue.h"
#include "ecmascript/tagged_queue.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;

bool JSForInIterator::CheckObjProto(const JSThread *thread, const JSHandle<JSForInIterator> &it)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> object(thread, it->GetObject());
    if (!object->IsJSObject()) {
        return false;
    }
    auto *hclass = object->GetTaggedObject()->GetClass();
    JSType jsType = hclass->GetObjectType();
    if (jsType != JSType::JS_OBJECT) {
        return false;
    }
    JSTaggedValue proto = hclass->GetPrototype();
    if (!proto.IsJSObject()) {
        return false;
    }
    return hclass->GetPrototype().GetTaggedObject()->GetClass() ==
           env->GetObjectFunctionPrototypeClass().GetTaggedValue().GetTaggedObject()->GetClass();
}

void JSForInIterator::GetAllEnumKeys(JSThread *thread, const JSHandle<JSForInIterator> &it,
                                     const JSHandle<JSTaggedValue> &object)
{
    ASSERT(object->IsHeapObject());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSMutableHandle<TaggedQueue> remaining(thread, thread->GlobalConstants()->GetEmptyTaggedQueue());
    if (object->IsJSProxy()) {
        JSHandle<TaggedArray> proxyArr = JSProxy::OwnPropertyKeys(thread, JSHandle<JSProxy>(object));
        RETURN_IF_ABRUPT_COMPLETION(thread);
        uint32_t length = proxyArr->GetLength();
        for (uint32_t i = 0; i < length; i++) {
            value.Update(proxyArr->Get(i));
            PropertyDescriptor desc(thread);
            JSProxy::GetOwnProperty(thread, JSHandle<JSProxy>(object), value, desc);
            RETURN_IF_ABRUPT_COMPLETION(thread);
            if (desc.IsEnumerable()) {
                TaggedQueue *newQueue = TaggedQueue::Push(thread, remaining, value);
                remaining.Update(JSTaggedValue(newQueue));
            }
        }
    } else {
        JSHandle<TaggedArray> arr = JSTaggedValue::GetOwnEnumPropertyKeys(thread, object);
        uint32_t len = arr->GetLength();
        for (uint32_t i = 0; i < len; i++) {
            value.Update(arr->Get(i));
            if (value->IsString()) {
                TaggedQueue *newQueue = TaggedQueue::Push(thread, remaining, value);
                remaining.Update(JSTaggedValue(newQueue));
            }
        }
    }
    if (it->GetHasVisitObjs()) {
        JSMutableHandle<TaggedQueue> remained(thread, thread->GlobalConstants()->GetEmptyTaggedQueue());
        JSMutableHandle<TaggedQueue> visited(thread, it->GetVisitedObjs());
        uint32_t size = visited->Size();
        while (!remaining->Empty()) {
            JSHandle<JSTaggedValue> key(thread, remaining->Pop(thread));
            bool has = false;
            for (uint32_t i = 0; i < size; i++) {
                value.Update(visited->Get(i));
                PropertyDescriptor desc(thread);
                has = JSTaggedValue::GetOwnProperty(thread, value, key, desc);
                RETURN_IF_ABRUPT_COMPLETION(thread);
                if (has) {
                    break;
                }
            }
            if (!has) {
                TaggedQueue *newQueue = TaggedQueue::Push(thread, remained, key);
                remained.Update(JSTaggedValue(newQueue));
            }
        }
        it->SetRemainingKeys(thread, remained);
    } else {
        it->SetRemainingKeys(thread, remaining);
    }
    it->SetWasVisited(true);
    object->GetTaggedObject()->GetClass()->SetHasDeleteProperty(false);
}

std::pair<JSTaggedValue, bool> JSForInIterator::NextInternal(JSThread *thread, const JSHandle<JSForInIterator> &it)
{
    while (true) {
        JSHandle<JSTaggedValue> object(thread, it->GetObject());
        if (object->IsNull() || object->IsUndefined()) {
            return std::make_pair(JSTaggedValue::Undefined(), true);
        }
        if (!it->GetWasVisited()) {
            GetAllEnumKeys(thread, it, object);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, std::make_pair(JSTaggedValue::Exception(), false));
        }

        JSHandle<TaggedQueue> remaining(thread, it->GetRemainingKeys());
        while (!remaining->Empty()) {
            ASSERT(object->IsHeapObject());
            JSTaggedValue r = remaining->Pop(thread);
            bool hasDelete = object->GetTaggedObject()->GetClass()->HasDeleteProperty();
            if (object->IsJSObject() && !hasDelete) {
                return std::make_pair(r, false);
            }
            JSHandle<JSTaggedValue> key(thread, r);
            PropertyDescriptor desc(thread);
            bool has = JSTaggedValue::GetOwnProperty(thread, object, key, desc);
            if (has) {
                if (desc.IsEnumerable()) {
                    return std::make_pair(key.GetTaggedValue(), false);
                }
            }
        }
        JSMutableHandle<TaggedQueue> visited(thread, it->GetVisitedObjs());
        TaggedQueue *newQueue = TaggedQueue::Push(thread, visited, object);
        visited.Update(JSTaggedValue(newQueue));
        it->SetVisitedObjs(thread, visited);
        JSTaggedValue proto = JSTaggedValue::GetPrototype(thread, object);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, std::make_pair(JSTaggedValue::Exception(), false));
        it->SetObject(thread, proto);
        it->SetWasVisited(false);
        it->SetHasVisitObjs(true);
    }
}

// 13.7.5.16.2.1 %ForInIteratorPrototype%.next ( )
JSTaggedValue JSForInIterator::Next(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg);
    JSThread *thread = msg->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSForInIterator> it(BuiltinsBase::GetThis(msg));
    ASSERT(it->IsForinIterator());
    std::pair<JSTaggedValue, bool> res = NextInternal(thread, it);
    return res.first;
}
}  // namespace panda::ecmascript
