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

#include "ecmascript/builtins/builtins_errors.h"
#include "ecmascript/js_api/js_api_hashmap.h"
#include "ecmascript/js_array.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_node.h"
#include "ecmascript/tagged_queue.h"

namespace panda::ecmascript {
using BuiltinsBase = base::BuiltinsBase;
JSTaggedValue JSAPIHashMapIterator::Next(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> input(BuiltinsBase::GetThis(argv));

    if (!input->IsJSAPIHashMapIterator()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not a hashmap iterator", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIHashMapIterator> iter = JSHandle<JSAPIHashMapIterator>::Cast(input);
    JSHandle<JSTaggedValue> iteratedHashMap(thread, iter->GetIteratedHashMap());
    // If m is undefined, return CreateIterResultObject(undefined, true).
    JSHandle<JSTaggedValue> undefinedHandle = thread->GlobalConstants()->GetHandledUndefined();
    if (iteratedHashMap->IsUndefined()) {
        return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
    }
    JSHandle<TaggedHashArray> tableArr(thread, JSHandle<JSAPIHashMap>::Cast(iteratedHashMap)->GetTable());
    uint32_t tableLength = tableArr->GetLength();
    uint32_t index = iter->GetNextIndex();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<TaggedQueue> queue(thread, iter->GetTaggedQueue());
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<TaggedNode> currentNode(thread, JSTaggedValue::Undefined());
    JSMutableHandle<TaggedArray> array(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> keyAndValue(thread, JSTaggedValue::Undefined());
    IterationKind itemKind = iter->GetIterationKind();
    while (index < tableLength) {
        currentNode.Update(JSAPIHashMapIterator::GetCurrentNode(thread, iter, queue, tableArr));
        if (!currentNode.GetTaggedValue().IsHole()) {
            JSTaggedValue key = currentNode->GetKey();
            keyHandle.Update(key);
            if (itemKind == IterationKind::KEY) {
                return JSIterator::CreateIterResultObject(thread, keyHandle, false).GetTaggedValue();
            }
            valueHandle.Update(currentNode->GetValue());
            if (itemKind == IterationKind::VALUE) {
                return JSIterator::CreateIterResultObject(thread, valueHandle, false).GetTaggedValue();
            }

            array.Update(factory->NewTaggedArray(2));  // 2 means the length of array
            array->Set(thread, 0, keyHandle);
            array->Set(thread, 1, valueHandle);
            keyAndValue.Update(JSArray::CreateArrayFromList(thread, array));
            return JSIterator::CreateIterResultObject(thread, keyAndValue, false).GetTaggedValue();
        }
        index++;
    }
    // Set [[IteratedMap]] to undefined.
    iter->SetIteratedHashMap(thread, JSTaggedValue::Undefined());
    return JSIterator::CreateIterResultObject(thread, undefinedHandle, true).GetTaggedValue();
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
    } else { // linkedNode
        JSHandle<LinkedNode> root = JSHandle<LinkedNode>::Cast(rootValue);
        if (!root->GetNext().IsHole()) {
            JSHandle<JSTaggedValue> next(thread, root->GetNext());
            queue.Update(JSTaggedValue(TaggedQueue::Push(thread, queue, next)));
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
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIHashMap",
                                    thread->GlobalConstants()->GetHandledUndefined());
    }
    JSHandle<JSTaggedValue> iter(factory->NewJSAPIHashMapIterator(JSHandle<JSAPIHashMap>(obj), kind));
    return iter;
}
}  // namespace panda::ecmascript