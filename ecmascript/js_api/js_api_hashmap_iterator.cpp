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
    JSHandle<JSTaggedValue> iteratedHashMap(thread, iter->GetIteratedHashMap());
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    // If m is undefined, return undefinedIteratorResult.
    if (iteratedHashMap->IsUndefined()) {
        return globalConst->GetUndefinedIterResult();
    }
    JSHandle<TaggedHashArray> tableArr(thread, JSHandle<JSAPIHashMap>::Cast(iteratedHashMap)->GetTable());
    uint32_t tableLength = tableArr->GetLength();
    uint32_t index = iter->GetNextIndex();

    JSMutableHandle<TaggedQueue> queue(thread, iter->GetTaggedQueue());
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<TaggedNode> currentNode(thread, JSTaggedValue::Undefined());

    IterationKind itemKind = iter->GetIterationKind();
    while (index < tableLength) {
        currentNode.Update(JSAPIHashMapIterator::FastGetCurrentNode(thread, iter, queue, tableArr));
        if (!currentNode.GetTaggedValue().IsHole() && !currentNode.GetTaggedValue().IsUndefined()) {
            JSTaggedValue key = currentNode->GetKey();
            keyHandle.Update(key);
            if (itemKind == IterationKind::KEY) {
                return JSIterator::CreateIterResultObject(thread, keyHandle, false).GetTaggedValue();
            }
            valueHandle.Update(currentNode->GetValue());
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
        index++;
        if (!currentNode.GetTaggedValue().IsRBTreeNode()) {
            iter->SetNextIndex(index);
        }
    }
    // Set [[IteratedMap]] to undefined.
    iter->SetIteratedHashMap(thread, JSTaggedValue::Undefined());
    return globalConst->GetUndefinedIterResult();
}

JSHandle<JSTaggedValue> JSAPIHashMapIterator::FastGetCurrentNode(JSThread *thread,
                                                                 JSHandle<JSAPIHashMapIterator> &iter,
                                                                 JSMutableHandle<TaggedQueue> &queue,
                                                                 JSHandle<TaggedHashArray> &tableArr)
{
    JSHandle<JSTaggedValue> rootValue(thread, JSTaggedValue::Undefined());
    uint32_t index = iter->GetNextIndex();
    JSHandle<JSTaggedValue> prevNodeValue(thread, iter->GetCurrentNodeResult());
    if (prevNodeValue->IsRBTreeNode()) {
        return GetCurrentNode(thread, iter, queue, tableArr);
    }
    if (prevNodeValue->IsUndefined() || prevNodeValue->IsHole()) {
        rootValue = JSHandle<JSTaggedValue>(thread, tableArr->Get(index));
        iter->SetCurrentNodeResult(thread, rootValue);
        return rootValue;
    }
    JSHandle<LinkedNode> prevNode = JSHandle<LinkedNode>::Cast(prevNodeValue);
    if (!prevNode->GetNext().IsHole()) {
        JSHandle<JSTaggedValue> next(thread, prevNode->GetNext());
        iter->SetCurrentNodeResult(thread, next);
        return next;
    }
    iter->SetCurrentNodeResult(thread, JSTaggedValue::Undefined());
    return rootValue;
}

// level traversal
JSHandle<JSTaggedValue> JSAPIHashMapIterator::GetCurrentNode(JSThread *thread,
                                                             JSHandle<JSAPIHashMapIterator> &iter,
                                                             JSMutableHandle<TaggedQueue> &queue,
                                                             JSHandle<TaggedHashArray> &tableArr)
{
    JSHandle<JSTaggedValue> rootValue(thread, JSTaggedValue::Undefined());
    uint32_t index = iter->GetNextIndex();
    if (queue->Empty()) {
        rootValue = JSHandle<JSTaggedValue>(thread, tableArr->Get(index));
        if (rootValue->IsHole()) {
            iter->SetNextIndex(++index);
            return rootValue;
        }
    } else {
        rootValue = JSHandle<JSTaggedValue>(thread, queue->Pop(thread));
    }
    if (rootValue->IsRBTreeNode()) {
        JSHandle<RBTreeNode> root = JSHandle<RBTreeNode>::Cast(rootValue);
        if (!root->GetLeft().IsHole()) {
            JSHandle<JSTaggedValue> left(thread, root->GetLeft());
            queue.Update(JSTaggedValue(TaggedQueue::Push(thread, queue, left)));
        }
        if (!root->GetRight().IsHole()) {
            JSHandle<JSTaggedValue> right(thread, root->GetRight());
            queue.Update(JSTaggedValue(TaggedQueue::Push(thread, queue, right)));
        }
    }
    iter->SetTaggedQueue(thread, queue.GetTaggedValue());
    if (queue->Empty()) {
        iter->SetNextIndex(++index);
    }
    return rootValue;
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