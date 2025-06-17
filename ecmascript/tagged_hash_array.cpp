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

#include "ecmascript/tagged_hash_array.h"

namespace panda::ecmascript {
JSTaggedValue TaggedHashArray::Create(const JSThread *thread, uint32_t numberOfElements)
{
    ASSERT_PRINT(numberOfElements > 0, "size must be a non-negative integer");
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedHashArray> tableArray = factory->NewTaggedHashArray(numberOfElements);
    return tableArray.GetTaggedValue();
}

void TaggedHashArray::Clear(JSThread *thread)
{
    for (uint32_t i = 0; i < GetLength(); ++i) {
        Set(thread, i, JSTaggedValue::Hole());
    }
}

JSTaggedValue TaggedHashArray::GetNode(JSThread *thread, int hash, JSTaggedValue key)
{
    uint32_t nodeLength = GetLength();
    ASSERT(nodeLength > 0);
    JSTaggedValue nodeValue = Get(thread, ((nodeLength - 1) & hash));
    JSTaggedValue hashValue = JSTaggedValue(hash);
    if (nodeValue.IsHole()) {
        return JSTaggedValue::Hole();
    }
    if (nodeValue.IsRBTreeNode()) {
        JSHandle<JSTaggedValue> node(thread, nodeValue);
        JSHandle<JSTaggedValue> handleKey(thread, key);
        return RBTreeNode::GetTreeNode(thread, node, hash, handleKey);
    } else if (!key.IsHole()) {
        JSTaggedValue nextNodeVa = nodeValue;
        while (!nextNodeVa.IsHole()) {
            LinkedNode *nextNode = LinkedNode::Cast(nextNodeVa.GetTaggedObject());
            if (nextNode->GetHash(thread) == hashValue &&
                JSTaggedValue::SameValue(thread, key, nextNode->GetKey(thread))) {
                return nextNodeVa;
            }
            nextNodeVa = nextNode->GetNext(thread);
        }
    }
    return JSTaggedValue::Hole();
}

JSHandle<LinkedNode> TaggedHashArray::NewLinkedNode(JSThread *thread, int hash, JSHandle<JSTaggedValue> key,
                                                    JSHandle<JSTaggedValue> value)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<LinkedNode> hole(thread, JSTaggedValue::Hole());
    return factory->NewLinkedNode(hash, key, value, hole);
}

JSHandle<LinkedNode> TaggedHashArray::CreateLinkedNodeFrom(JSThread *thread, JSHandle<RBTreeNode> treeNode)
{
    JSHandle<JSTaggedValue> key(thread, treeNode->GetKey(thread));
    JSHandle<JSTaggedValue> value(thread, treeNode->GetValue(thread));
    return NewLinkedNode(thread, treeNode->GetHash(thread).GetInt(), key, value);
}

JSHandle<RBTreeNode> TaggedHashArray::NewTreeNode(JSThread *thread, int hash, JSHandle<JSTaggedValue> key,
                                                  JSHandle<JSTaggedValue> value)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewTreeNode(hash, key, value);
}

JSHandle<RBTreeNode> TaggedHashArray::CreateTreeNodeFrom(JSThread *thread, JSHandle<LinkedNode> linkedNode)
{
    JSHandle<JSTaggedValue> key(thread, linkedNode->GetKey(thread));
    JSHandle<JSTaggedValue> value(thread, linkedNode->GetValue(thread));
    return NewTreeNode(thread, linkedNode->GetHash(thread).GetInt(), key, value);
}

void TaggedHashArray::TreeingBin(JSThread *thread, const JSHandle<TaggedHashArray> &tab, int hash)
{
    uint32_t length = tab->GetLength();
    ASSERT(length > 0);
    uint32_t index = (length - 1) & hash;
    JSTaggedValue nodeVa = tab->Get(thread, index);
    if (!nodeVa.IsHole()) {
        JSHandle<LinkedNode> node(thread, nodeVa);
        JSHandle<RBTreeNode> root = LinkedNode::Treeing(thread, node);
        tab->Set(thread, index, root);
    }
}

JSHandle<TaggedHashArray> TaggedHashArray::Resize(JSThread *thread, const JSHandle<TaggedHashArray> &oldTab,
                                                  uint32_t Capacity)
{
    uint32_t oldCapacity = oldTab->GetLength();
    if (oldCapacity >= MAXIMUM_CAPACITY) {
        return oldTab;
    }
    uint32_t newCapacity = Capacity << 1;
    JSTaggedValue newTabValue = Create(thread, newCapacity);
    JSHandle<TaggedHashArray> newTab(thread, newTabValue);
    JSMutableHandle<JSTaggedValue> oldValue(thread, JSTaggedValue::Undefined());
    for (uint32_t j = 0; j < oldCapacity; ++j) {
        oldValue.Update(oldTab->Get(thread, j));
        if (oldValue->IsHole()) {
            continue;
        } else if (oldValue->IsRBTreeNode()) {
            RBTreeNode::Divide(thread, newTab, oldValue, j, oldCapacity);
        } else if (oldValue->IsLinkedNode()) {
            LinkedNode *node = LinkedNode::Cast(oldValue.GetTaggedValue().GetTaggedObject());
            if (node->GetNext(thread).IsHole()) {
                int newhash = (node->GetHash(thread).GetInt()) & (newCapacity - 1);
                newTab->Set(thread, newhash, JSTaggedValue(node));
            } else { // preserve order
                NodeDisperse(thread, newTab, oldValue.GetTaggedValue(), j, oldCapacity);
            }
        }
    }
    return newTab;
}

void TaggedHashArray::NodeDisperse(JSThread *thread, const JSHandle<TaggedHashArray> &newTab,
                                   JSTaggedValue nodeVa, int index, int oldCapacity)
{
    JSTaggedValue loHead = JSTaggedValue::Hole();
    JSTaggedValue loTail = JSTaggedValue::Hole();
    JSTaggedValue hiHead = JSTaggedValue::Hole();
    JSTaggedValue hiTail = JSTaggedValue::Hole();
    JSTaggedValue next = JSTaggedValue::Hole();
    do {
        next = LinkedNode::Cast(nodeVa.GetTaggedObject())->GetNext(thread);
        if (((LinkedNode::Cast(nodeVa.GetTaggedObject())->GetHash(thread).GetInt()) & oldCapacity) == 0) {
            if (loTail.IsHole()) {
                loHead = nodeVa;
            } else {
                LinkedNode::Cast(loTail.GetTaggedObject())->SetNext(thread, nodeVa);
            }
            loTail = nodeVa;
        } else {
            if (hiTail.IsHole()) {
                hiHead = nodeVa;
            } else {
                LinkedNode::Cast(hiTail.GetTaggedObject())->SetNext(thread, nodeVa);
            }
            hiTail = nodeVa;
        }
        nodeVa = next;
    } while (!(next.IsHole()));
    if (!loTail.IsHole()) {
        LinkedNode::Cast(loTail.GetTaggedObject())->SetNext(thread, JSTaggedValue::Hole());
        newTab->Set(thread, index, loHead);
    }
    if (!hiTail.IsHole()) {
        LinkedNode::Cast(hiTail.GetTaggedObject())->SetNext(thread, JSTaggedValue::Hole());
        newTab->Set(thread, index + oldCapacity, hiHead);
    }
}

JSTaggedValue TaggedHashArray::SetVal(JSThread *thread, JSHandle<TaggedHashArray> table, int hash,
                                      JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> value)
{
    uint32_t length = table->GetLength();
    ASSERT(length > 0);
    uint32_t index = (length - 1) & hash;
    JSHandle<JSTaggedValue> node(thread, table->Get(thread, index));
    if (node->IsHole()) {
        JSHandle<LinkedNode> newNode = TaggedHashArray::NewLinkedNode(thread, hash, key, value);
        table->Set(thread, index, newNode.GetTaggedValue());
        return JSTaggedValue::True();
    } else if (node->IsLinkedNode()) {
        JSMutableHandle<LinkedNode> root(thread, JSTaggedValue::Undefined());
        uint32_t count = 0;
        JSMutableHandle<JSTaggedValue> currentKey(thread, JSTaggedValue::Undefined());
        JSMutableHandle<JSTaggedValue> nextVal(thread, node.GetTaggedValue());
        do {
            root.Update(nextVal);
            currentKey.Update(root->GetKey(thread));
            if (root->GetHash(thread).GetInt() == hash && (!key->IsHole() &&
                JSTaggedValue::SameValue(thread, key.GetTaggedValue(), currentKey.GetTaggedValue()))) {
                root->SetValue(thread, value.GetTaggedValue());
                return JSTaggedValue::Undefined();
            }
            nextVal.Update(root->GetNext(thread));
            count++;
        } while (!nextVal->IsHole());
        JSHandle<LinkedNode> newNode = TaggedHashArray::NewLinkedNode(thread, hash, key, value);
        root->SetNext(thread, newNode);
        if (count >= TREEIFY_THRESHOLD - 1) {
            TaggedHashArray::TreeingBin(thread, table, hash);
        }
        return JSTaggedValue::True();
    } else if (node->IsRBTreeNode()) {
        JSHandle<RBTreeNode> root = JSHandle<RBTreeNode>::Cast(node);
        uint32_t curCount = root->GetCount();
        JSHandle<RBTreeNode> changeNode = RBTreeNode::Set(thread, root, hash, key, value);
        changeNode->SetIsRed(false);
        table->Set(thread, index, changeNode);
        uint32_t updateCount = changeNode->GetCount();
        if (curCount == updateCount) {
            return JSTaggedValue::Undefined();
        }
        return JSTaggedValue::True();
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue TaggedHashArray::RemoveNode(JSThread *thread, int hash, JSTaggedValue key)
{
    uint32_t length = GetLength();
    ASSERT(length > 0);
    uint32_t index = (length - 1) & hash;
    JSTaggedValue node = Get(thread, index);
    if (node.IsHole()) {
        return JSTaggedValue::Hole();
    } else if (node.IsLinkedNode()) {
        LinkedNode *head = LinkedNode::Cast(node.GetTaggedObject());
        JSTaggedValue newKey = head->GetKey(thread);
        if (head->GetHash(thread).GetInt() == hash && (!key.IsHole() &&
            JSTaggedValue::SameValue(thread, key, newKey))) {
            Set(thread, index, head->GetNext(thread));
            return head->GetValue(thread);
        }
        JSTaggedValue nodeNextVa = head->GetNext(thread);
        LinkedNode *previousNode = head;
        while (!nodeNextVa.IsHole()) {
            LinkedNode *nodeNext = LinkedNode::Cast(nodeNextVa.GetTaggedObject());
            newKey = nodeNext->GetKey(thread);
            if (nodeNext->GetHash(thread).GetInt() == hash && (!key.IsHole() &&
                JSTaggedValue::SameValue(thread, key, newKey))) {
                previousNode->SetNext(thread, nodeNext->GetNext(thread));
                Set(thread, index, node);
                return nodeNext->GetValue(thread);
            }
            previousNode = LinkedNode::Cast(nodeNextVa.GetTaggedObject());
            nodeNextVa = nodeNext->GetNext(thread);
        }
    } else if (node.IsRBTreeNode()) {
        JSTaggedValue oldValue = JSTaggedValue::Hole();
        JSTaggedValue rootTreeNodeVa = RBTreeNode::Delete(thread, node, hash, key, oldValue);
        if (rootTreeNodeVa.IsHole()) {
            Set(thread, index, JSTaggedValue::Hole());
            return oldValue;
        }
        //set root node as black
        RBTreeNode *root = RBTreeNode::Cast(rootTreeNodeVa.GetTaggedObject());
        if (root->GetIsRed()) {
            root->SetIsRed(false);
            rootTreeNodeVa = JSTaggedValue(root);
        }
        if (!rootTreeNodeVa.IsHole()) {
            Set(thread, index, rootTreeNodeVa);
            return oldValue;
        }
    }
    return JSTaggedValue::Hole();
}

JSHandle<JSTaggedValue> TaggedHashArray::GetCurrentNode(JSThread *thread, JSMutableHandle<TaggedQueue> &queue,
                                                        const JSHandle<TaggedHashArray> &tableArr, uint32_t &index)
{
    JSMutableHandle<JSTaggedValue> rootValue(thread, JSTaggedValue::Undefined());
    if (queue->Empty(thread)) {
        rootValue.Update(tableArr->Get(thread, index));
        if (rootValue->IsHole()) {
            ++index;
            return rootValue;
        }
    } else {
        rootValue.Update(queue->Pop(thread));
    }
    if (rootValue->IsRBTreeNode()) {
        JSHandle<RBTreeNode> root = JSHandle<RBTreeNode>::Cast(rootValue);
        if (!root->GetLeft(thread).IsHole()) {
            JSHandle<JSTaggedValue> left(thread, root->GetLeft(thread));
            queue.Update(JSTaggedValue(TaggedQueue::Push(thread, queue, left)));
        }
        if (!root->GetRight(thread).IsHole()) {
            JSHandle<JSTaggedValue> right(thread, root->GetRight(thread));
            queue.Update(JSTaggedValue(TaggedQueue::Push(thread, queue, right)));
        }
    } else {
        JSHandle<LinkedNode> root = JSHandle<LinkedNode>::Cast(rootValue);
        if (!root->GetNext(thread).IsHole()) {
            JSHandle<JSTaggedValue> next(thread, root->GetNext(thread));
            queue.Update(JSTaggedValue(TaggedQueue::Push(thread, queue, next)));
        }
    }
    if (queue->Empty(thread)) {
        ++index;
    }
    return rootValue;
}
}  // namespace panda::ecmascript
