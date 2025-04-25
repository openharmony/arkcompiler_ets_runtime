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

#include "ecmascript/js_api/js_api_hashset_iterator.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_hasharray_iterator.h"
#include "ecmascript/js_api/js_api_hashset.h"
namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
using ContainerError = containers::ContainerError;
using ErrorFlag = containers::ErrorFlag;
JSTaggedValue JSAPIHashSetIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));
    if (!input->IsJSAPIHashSetIterator()) {
        JSTaggedValue error =
            ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                          "The Symbol.iterator method cannot be bound");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSAPIHashSetIterator> iter = JSHandle<JSAPIHashSetIterator>::Cast(input);
    JSHandle<JSTaggedValue> iteratedHashSet(thread, iter->GetIteratedHashSet());
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    if (iteratedHashSet->IsUndefined()) {
        return env->GetUndefinedIteratorResult().GetTaggedValue();
    }
    JSHandle<JSAPIHashSet> hashSet = JSHandle<JSAPIHashSet>::Cast(iteratedHashSet);
    JSHandle<TaggedHashArray> tableArr(thread, hashSet->GetTable());
    uint32_t tableLength = tableArr->GetLength();
    uint32_t index = iter->GetNextIndex();
    JSMutableHandle<TaggedQueue> queue(thread, iter->GetTaggedQueue());
    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<TaggedNode> currentNode(thread, JSTaggedValue::Undefined());
    IterationKind itemKind = iter->GetIterationKind();
    while (index < tableLength) {
        currentNode.Update(JSAPIHashArrayIterator::GetCurrentNode<JSAPIHashSetIterator>(thread, iter, queue, tableArr));
        if (currentNode.GetTaggedValue().IsHole()) {
            iter->SetNextIndex(++index);
            continue;
        }
        valueHandle.Update(currentNode->GetKey());
        if (itemKind == IterationKind::VALUE) {
            return JSIterator::CreateIterResultObject(thread, valueHandle, false).GetTaggedValue();
        }
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<TaggedArray> array = factory->NewTaggedArray(2); // 2 means the length of array
        array->Set(thread, 0, valueHandle);
        array->Set(thread, 1, valueHandle);
        JSHandle<JSTaggedValue> keyAndValue(JSArray::CreateArrayFromList(thread, array));
        return JSIterator::CreateIterResultObject(thread, keyAndValue, false).GetTaggedValue();
    }
    // Set O.[[IteratedMap]] to undefined.
    iter->SetIteratedHashSet(thread, JSTaggedValue::Undefined());
    return env->GetUndefinedIteratorResult().GetTaggedValue();
}

JSHandle<JSTaggedValue> JSAPIHashSetIterator::CreateHashSetIterator(JSThread *thread,
                                                                    const JSHandle<JSTaggedValue> &obj,
                                                                    IterationKind kind)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!obj->IsJSAPIHashSet()) {
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::BIND_ERROR,
                                                            "The Symbol.iterator method cannot be bound");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
    }
    JSHandle<JSTaggedValue> iter(factory->NewJSAPIHashSetIterator(JSHandle<JSAPIHashSet>(obj), kind));
    return iter;
}
}  // namespace panda::ecmascript