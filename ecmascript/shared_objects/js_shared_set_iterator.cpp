/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/shared_objects/js_shared_set_iterator.h"

#include "ecmascript/js_array.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/shared_objects/concurrent_api_scope.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
JSTaggedValue JSSharedSetIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisObj(BuiltinsBase::GetThis(argv));
    return NextInternal(thread, thisObj);
}

JSTaggedValue JSSharedSetIterator::NextInternal(JSThread *thread, JSHandle<JSTaggedValue> thisObj)
{
    if (!thisObj->IsJSSharedSetIterator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not a shared-set iterator", JSTaggedValue::Exception());
    }
    JSHandle<JSSharedSetIterator> iter(thisObj);
    JSHandle<JSTaggedValue> undefinedHandle(thread, JSTaggedValue::Undefined());
    if (iter->GetIteratedSet().IsUndefined()) {
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    JSHandle<JSSharedSet> iteratedSet(thread, iter->GetIteratedSet());
    [[maybe_unused]] ConcurrentApiScope<JSSharedSet> scope(thread, JSHandle<JSTaggedValue>::Cast(iteratedSet));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
    JSHandle<LinkedHashSet> set(thread, iteratedSet->GetLinkedSet());

    int index = static_cast<int>(iter->GetNextIndex());
    IterationKind itemKind = iter->GetIterationKind();
    int totalElements = set->NumberOfElements() + set->NumberOfDeletedElements();

    while (index < totalElements) {
        if (!set->GetKey(index).IsHole()) {
            iter->SetNextIndex(index + 1);
            JSHandle<JSTaggedValue> key(thread, set->GetKey(index));
            if (itemKind == IterationKind::VALUE || itemKind == IterationKind::KEY) {
                return JSIterator::CreateIterResultObject(thread, key, false).GetTaggedValue();
            }
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            JSHandle<TaggedArray> array(factory->NewTaggedArray(2));  // 2: key and value pair
            array->Set(thread, 0, key);
            array->Set(thread, 1, key);
            JSHandle<JSTaggedValue> keyAndValue(JSArray::CreateArrayFromList(thread, array));
            return JSIterator::CreateIterResultObject(thread, keyAndValue, false).GetTaggedValue();
        }
        index++;
    }
    iter->SetIteratedSet(thread, JSTaggedValue::Undefined());
    return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
}

JSHandle<JSTaggedValue> JSSharedSetIterator::CreateSetIterator(JSThread *thread,
    const JSHandle<JSTaggedValue> &obj, IterationKind kind)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!obj->IsJSSharedSet()) {
        JSHandle<JSTaggedValue> undefinedHandle(thread, JSTaggedValue::Undefined());
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not SharedSet", undefinedHandle);
    }
    JSHandle<JSTaggedValue> iter(factory->NewJSSetIterator(JSHandle<JSSharedSet>(obj), kind));
    return iter;
}
}  // namespace panda::ecmascript
