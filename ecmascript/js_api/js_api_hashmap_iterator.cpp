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

#include "ecmascript/js_api/js_api_hashmap_iterator.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_hasharray_iterator.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
using ContainerError = containers::ContainerError;
using ErrorFlag = containers::ErrorFlag;
JSTaggedValue JSAPIHashMapIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));

    if (!input->IsJSAPIHashMapIterator()) {
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                            "The Symbol.iterator method cannot be bound");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSAPIHashMapIterator> iter = JSHandle<JSAPIHashMapIterator>::Cast(input);
    JSHandle<JSTaggedValue> iteratedHashMap(thread, iter->GetIteratedHashMap(thread));
    // If m is undefined, return undefinedIteratorResult.
    if (iteratedHashMap->IsUndefined()) {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        return env->GetUndefinedIteratorResult().GetTaggedValue();
    }
    JSHandle<TaggedHashArray> tableArr(thread, JSHandle<JSAPIHashMap>::Cast(iteratedHashMap)->GetTable(thread));
    uint32_t tableLength = tableArr->GetLength();
    uint32_t index = iter->GetNextIndex();

    JSMutableHandle<TaggedQueue> queue(thread, iter->GetTaggedQueue(thread));
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<TaggedNode> currentNode(thread, JSTaggedValue::Undefined());

    IterationKind itemKind = iter->GetIterationKind();
    while (index < tableLength) {
        currentNode.Update(JSAPIHashArrayIterator::GetCurrentNode<JSAPIHashMapIterator>(thread, iter, queue, tableArr));
        if (currentNode.GetTaggedValue().IsHole()) {
            iter->SetNextIndex(++index);
            continue;
        }
        JSTaggedValue key = currentNode->GetKey(thread);
        keyHandle.Update(key);
        if (itemKind == IterationKind::KEY) {
            return JSIterator::CreateIterResultObject(thread, keyHandle, false).GetTaggedValue();
        }
        valueHandle.Update(currentNode->GetValue(thread));
        if (itemKind == IterationKind::VALUE) {
            return JSIterator::CreateIterResultObject(thread, valueHandle, false).GetTaggedValue();
        }
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<TaggedArray> array = factory->NewTaggedArray(2); // 2 means the length of array
        array->Set(thread, 0, keyHandle);
        array->Set(thread, 1, valueHandle);
        JSHandle<JSTaggedValue> keyAndValue(JSArray::CreateArrayFromList(thread, array));
        return JSIterator::CreateIterResultObject(thread, keyAndValue, false).GetTaggedValue();
    }
    // Set [[IteratedMap]] to undefined.
    iter->SetIteratedHashMap(thread, JSTaggedValue::Undefined());
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    return env->GetUndefinedIteratorResult().GetTaggedValue();
}

JSHandle<JSTaggedValue> JSAPIHashMapIterator::CreateHashMapIterator(JSThread *thread,
                                                                    const JSHandle<JSTaggedValue> &obj,
                                                                    IterationKind kind)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!obj->IsJSAPIHashMap()) {
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                            "The Symbol.iterator method cannot be bound");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
    }
    JSHandle<JSTaggedValue> iter(factory->NewJSAPIHashMapIterator(JSHandle<JSAPIHashMap>(obj), kind));
    return iter;
}
}  // namespace panda::ecmascript