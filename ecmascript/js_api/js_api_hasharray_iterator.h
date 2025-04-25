/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JS_API_JS_API_HASHARRAY_ITERATOR_H
#define ECMASCRIPT_JS_API_JS_API_HASHARRAY_ITERATOR_H

#include "ecmascript/js_api/js_api_hashmap_iterator.h"
#include "ecmascript/js_api/js_api_hashset_iterator.h"

#include "ecmascript/js_handle.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/tagged_hash_array.h"

namespace panda::ecmascript {
class JSAPIHashArrayIterator {
public:
    template <typename T>
    static JSHandle<JSTaggedValue> GetRBTreeCurrentNode(JSThread *thread, JSHandle<T> iter,
                                                        JSMutableHandle<TaggedQueue> &queue,
                                                        JSHandle<TaggedHashArray> &tableArr)
    {
        JSTaggedValue rootValue;
        uint32_t index = iter->GetNextIndex();
        if (queue->Empty()) {
            rootValue = tableArr->Get(index);
            ASSERT(rootValue.IsRBTreeNode());
        } else {
            rootValue = queue->Pop(thread);
        }
        JSHandle<RBTreeNode> root = JSHandle<RBTreeNode>(thread, rootValue);
        if (!root->GetLeft().IsHole()) {
            JSHandle<JSTaggedValue> left(thread, root->GetLeft());
            queue.Update(JSTaggedValue(TaggedQueue::Push(thread, queue, left)));
        }
        if (!root->GetRight().IsHole()) {
            JSHandle<JSTaggedValue> right(thread, root->GetRight());
            queue.Update(JSTaggedValue(TaggedQueue::Push(thread, queue, right)));
        }
        // iter == RBTree.end(), move index to next position
        if (queue->Empty()) {
            iter->SetNextIndex(++index);
        }
        iter->SetTaggedQueue(thread, queue.GetTaggedValue());
        return JSHandle<JSTaggedValue>::Cast(root);
    }

    template <typename T>
    static JSHandle<JSTaggedValue> GetCurrentNode(JSThread *thread, JSHandle<T> iter,
                                                  JSMutableHandle<TaggedQueue> &queue,
                                                  JSHandle<TaggedHashArray> &tableArr)
    {
        ASSERT((std::is_same_v<T, JSAPIHashMapIterator>) || (std::is_same_v<T, JSAPIHashSetIterator>));
        uint32_t index = iter->GetNextIndex();
        // judge type of tableArr[index](Hole, LinkList, RBTree)
        JSHandle<JSTaggedValue> root(thread, tableArr->Get(index));
        if (root->IsHole()) {
            JSHandle<JSTaggedValue> rootValue = JSHandle<JSTaggedValue>(thread, tableArr->Get(index));
            return rootValue;
        }
        // RBTree
        if (root->IsRBTreeNode()) {
            return GetRBTreeCurrentNode<T>(thread, iter, queue, tableArr);
        }
        // LinkList
        JSHandle<JSTaggedValue> currentNodeValue(thread, iter->GetCurrentNodeResult());
        if (!currentNodeValue->IsLinkedNode()) {
            currentNodeValue = root;
        }
        JSHandle<LinkedNode> currentNode = JSHandle<LinkedNode>::Cast(currentNodeValue);
        JSTaggedValue next = currentNode->GetNext();
        // iter == linklist.end(), move index to next position
        if (next.IsHole()) {
            iter->SetNextIndex(++index);
        }
        iter->SetCurrentNodeResult(thread, next);
        return currentNodeValue;
    }
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_API_JS_API_HASHARRAY_ITERATOR_H