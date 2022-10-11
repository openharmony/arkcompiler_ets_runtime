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

#include "ecmascript/builtins/builtins_errors.h"
#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/js_api/js_api_hashset.h"
#include "ecmascript/js_array.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_node.h"
#include "ecmascript/tagged_queue.h"

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
    JSHandle<JSTaggedValue> undefinedHandle = thread->GlobalConstants()->GetHandledUndefined();
    if (iteratedHashSet->IsUndefined()) {
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    JSHandle<JSAPIHashSet> hashSet = JSHandle<JSAPIHashSet>::Cast(iteratedHashSet);
    JSHandle<TaggedHashArray> tableArr(thread, hashSet->GetTable());
    uint32_t tableLength = tableArr->GetLength();
    uint32_t tableIndex = iter->GetTableIndex();
    uint32_t index = iter->GetNextIndex();
    uint32_t size = hashSet->GetSize();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<TaggedQueue> queue(thread, iter->GetTaggedQueue());
    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<TaggedNode> currentNode(thread, JSTaggedValue::Undefined());
    JSMutableHandle<TaggedArray> array(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> keyAndValue(thread, JSTaggedValue::Undefined());
    IterationKind itemKind = iter->GetIterationKind();
    while (tableIndex < tableLength && index < size) {
        currentNode.Update(GetCurrentNode(thread, iter, queue, tableArr));
        if (!currentNode.GetTaggedValue().IsHole()) {
            iter->SetNextIndex(++index);
            valueHandle.Update(currentNode->GetKey());
            if (itemKind == IterationKind::VALUE) {
                return JSIterator::CreateIterResultObject(thread, valueHandle, false).GetTaggedValue();
            }

            array.Update(factory->NewTaggedArray(2));  // 2 means the length of array
            array->Set(thread, 0, JSTaggedValue(--index));
            array->Set(thread, 1, valueHandle);
            keyAndValue.Update(JSArray::CreateArrayFromList(thread, array));
            return JSIterator::CreateIterResultObject(thread, keyAndValue, false).GetTaggedValue();
        }
        tableIndex++;
    }
    // Set O.[[IteratedMap]] to undefined.
    iter->SetIteratedHashSet(thread, JSTaggedValue::Undefined());
    return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
}

// level traversal
JSHandle<JSTaggedValue> JSAPIHashSetIterator::GetCurrentNode(JSThread *thread, JSHandle<JSAPIHashSetIterator> &iter,
                                                             JSMutableHandle<TaggedQueue> &queue,
                                                             JSHandle<TaggedHashArray> &tableArr)
{
    JSHandle<JSTaggedValue> rootValue(thread, JSTaggedValue::Undefined());
    uint32_t index = iter->GetTableIndex();
    if (queue->Empty()) {
        rootValue = JSHandle<JSTaggedValue>(thread, tableArr->Get(index));
        if (rootValue->IsHole()) {
            iter->SetTableIndex(++index);
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
    } else { // linkedNode
        JSHandle<LinkedNode> root = JSHandle<LinkedNode>::Cast(rootValue);
        if (!root->GetNext().IsHole()) {
            JSHandle<JSTaggedValue> next(thread, root->GetNext());
            queue.Update(JSTaggedValue(TaggedQueue::Push(thread, queue, next)));
        }
    }
    iter->SetTaggedQueue(thread, queue.GetTaggedValue());
    if (queue->Empty()) {
        iter->SetTableIndex(++index);
    }
    return rootValue;
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