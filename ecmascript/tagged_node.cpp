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

#include "ecmascript/tagged_node.h"

#include "ecmascript/tagged_hash_array.h"

namespace panda::ecmascript {
JSHandle<RBTreeNode> LinkedNode::Treeing(JSThread *thread, const JSHandle<LinkedNode> &head)
{
    JSMutableHandle<RBTreeNode> rootNode(thread, JSTaggedValue::Hole());
    JSMutableHandle<LinkedNode> next(thread, head);
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    while (!next.GetTaggedValue().IsHole()) {
        key.Update(next->GetKey(thread));
        value.Update(next->GetValue(thread));
        rootNode.Update(RBTreeNode::Set(thread, rootNode, next->GetHash(thread).GetInt(), key, value));
        rootNode->SetIsRed(false);
        next.Update(next->GetNext(thread));
    }
    return rootNode;
}

void RBTreeNode::InitRBTreeNode(JSThread *thread, int hash, JSHandle<JSTaggedValue> key,
                                JSHandle<JSTaggedValue> value, int count)
{
    InitTaggedNode(thread, hash, key, value);
    SetLeft(thread, JSTaggedValue::Hole());
    SetRight(thread, JSTaggedValue::Hole());
    SetIsRed(true);
    SetCount(count);
}

// number of node in subtree rooted at treeNode; 0 if treeNode is Hole
uint32_t RBTreeNode::Count(JSTaggedValue nodeValue)
{
    if (nodeValue.IsHole()) {
        return 0;
    }
    return RBTreeNode::Cast(nodeValue.GetTaggedObject())->GetCount();
}

void RBTreeNode::InOrderTraverse(JSThread *thread, const JSHandle<RBTreeNode> &treeNode,
                                 JSHandle<LinkedNode> &head, JSHandle<LinkedNode> &tail)
{
    if (!treeNode.GetTaggedValue().IsHole()) {
        JSHandle<RBTreeNode> leftChild = JSHandle<RBTreeNode>(thread, treeNode->GetLeft(thread));
        InOrderTraverse(thread, leftChild, head, tail);
        JSHandle<LinkedNode> linkedNode = TaggedHashArray::CreateLinkedNodeFrom(thread, treeNode);
        if (tail.GetTaggedValue().IsHole()) {
            head = linkedNode;
        } else {
            tail->SetNext(thread, linkedNode.GetTaggedValue());
        }
        tail = linkedNode;
        JSHandle<RBTreeNode> rightChild(thread, treeNode->GetRight(thread));
        InOrderTraverse(thread, rightChild, head, tail);
    }
}

JSHandle<LinkedNode> RBTreeNode::Detreeing(JSThread *thread, const JSHandle<RBTreeNode> &root)
{
    JSHandle<LinkedNode> head(thread, JSTaggedValue::Hole());
    JSHandle<LinkedNode> tail(thread, JSTaggedValue::Hole());

    InOrderTraverse(thread, root, head, tail);

    return head;
}

void RBTreeNode::InOrderTraverse(JSThread *thread, const JSHandle<RBTreeNode> &treeNode, int bit,
                                 LinkedNodeStruct &nodeStruct)
{
    if (!treeNode.GetTaggedValue().IsHole()) {
        JSHandle<RBTreeNode> leftChild(thread, treeNode->GetLeft(thread));
        InOrderTraverse(thread, leftChild, bit, nodeStruct);
        
        JSHandle<LinkedNode> linkedNode = TaggedHashArray::CreateLinkedNodeFrom(thread, treeNode);
        // the elements from each bin must either stay at same index,
        // or move with a power of two offset in the new table
        if ((static_cast<uint32_t>(linkedNode->GetHash(thread).GetInt()) & static_cast<uint32_t>(bit)) == 0) {
            if (nodeStruct.lowerTail.GetTaggedValue().IsHole()) {
                nodeStruct.lowerHead = linkedNode;
            } else {
                nodeStruct.lowerTail->SetNext(thread, linkedNode.GetTaggedValue());
            }
            nodeStruct.lowerTail = linkedNode;
        } else {
            if (nodeStruct.higherTail.GetTaggedValue().IsHole()) {
                nodeStruct.higherHead = linkedNode;
            } else {
                nodeStruct.higherTail->SetNext(thread, linkedNode.GetTaggedValue());
            }
            nodeStruct.higherTail = linkedNode;
        }

        JSHandle<RBTreeNode> rightChild(thread, treeNode->GetRight(thread));
        InOrderTraverse(thread, rightChild, bit, nodeStruct);
    }
}

void RBTreeNode::Divide(JSThread *thread, JSHandle<TaggedHashArray> table,
                        JSHandle<JSTaggedValue> nodeVa, int index, int bit)
{
    JSHandle<RBTreeNode> self = JSHandle<RBTreeNode>::Cast(nodeVa);
    LinkedNodeStruct nodeStruct {JSHandle<LinkedNode>(thread, JSTaggedValue::Hole()),
                                 JSHandle<LinkedNode>(thread, JSTaggedValue::Hole()),
                                 JSHandle<LinkedNode>(thread, JSTaggedValue::Hole()),
                                 JSHandle<LinkedNode>(thread, JSTaggedValue::Hole())};

    InOrderTraverse(thread, self, bit, nodeStruct);

    uint32_t loCount = 0;
    uint32_t hiCount = 0;
    JSMutableHandle<LinkedNode> lowerHead(thread, nodeStruct.lowerHead);
    while (!lowerHead.GetTaggedValue().IsHole()) {
        loCount++;
        lowerHead.Update(lowerHead->GetNext(thread));
    }
    JSMutableHandle<LinkedNode> higherHead(thread, nodeStruct.higherHead);
    while (!higherHead.GetTaggedValue().IsHole()) {
        loCount++;
        higherHead.Update(higherHead->GetNext(thread));
    }

    if (!nodeStruct.lowerHead.GetTaggedValue().IsHole()) {
        if (loCount >= TaggedHashArray::TREEIFY_THRESHOLD) {
            JSHandle<RBTreeNode> loRoot = LinkedNode::Treeing(thread, nodeStruct.lowerHead);
            table->Set(thread, index, loRoot);
        } else {
            table->Set(thread, index, nodeStruct.lowerHead);
        }
    }
    if (!nodeStruct.higherHead.GetTaggedValue().IsHole()) {
        if (hiCount >= TaggedHashArray::TREEIFY_THRESHOLD) {
            JSHandle<RBTreeNode> hiRoot = LinkedNode::Treeing(thread, nodeStruct.higherHead);
            table->Set(thread, index + bit, hiRoot);
        } else {
            table->Set(thread, index + bit, nodeStruct.higherHead);
        }
    }
}

int RBTreeNode::Compare(const JSThread *thread, int hash1, JSTaggedValue key1, int hash2, JSTaggedValue key2)
{
    ASSERT(!key1.IsHole() && !key2.IsHole());
    if (JSTaggedValue::SameValue(thread, key1, key2)) {
        return 0;
    }
    if (hash1 < hash2) {
        return -1;
    } else {
        return 1;
    }
}

bool RBTreeNode::IsRed(JSTaggedValue treeNodeValue)
{
    if (treeNodeValue.IsHole()) {
        return false;
    }
    RBTreeNode *treeNode = RBTreeNode::Cast(treeNodeValue.GetTaggedObject());
    return treeNode->GetIsRed();
}

// insert the key-value pair in the subtree rooted at treeNode
JSHandle<RBTreeNode> RBTreeNode::Set(JSThread *thread, JSHandle<RBTreeNode> treeNode, int hash,
                                     JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> value)
{
    if (treeNode.GetTaggedValue().IsHole()) {
        treeNode = TaggedHashArray::NewTreeNode(thread, hash, key, value);
        return treeNode;
    }
    JSHandle<JSTaggedValue> treeNodeKey(thread, treeNode->GetKey(thread));
    int cmp =
        Compare(thread, hash, key.GetTaggedValue(), treeNode->GetHash(thread).GetInt(), treeNodeKey.GetTaggedValue());
    JSHandle<RBTreeNode> leftChild(thread, treeNode->GetLeft(thread));
    JSHandle<RBTreeNode> rightChild(thread, treeNode->GetRight(thread));
    if (cmp < 0) {
        JSHandle<RBTreeNode> left = Set(thread, leftChild, hash, key, value);
        treeNode->SetLeft(thread, left);
    } else if (cmp > 0) {
        JSHandle<RBTreeNode> right = Set(thread, rightChild, hash, key, value);
        treeNode->SetRight(thread, right);
    } else {
        treeNode->SetValue(thread, value);
    }

    if (IsRed(treeNode->GetRight(thread)) && !IsRed(treeNode->GetLeft(thread))) {
        treeNode = JSHandle<RBTreeNode>(thread, treeNode->RotateLeft(thread));
    }
    JSTaggedValue leftChildVa = treeNode->GetLeft(thread);
    if (!leftChildVa.IsHole()) {
        leftChild = JSHandle<RBTreeNode>(thread, leftChildVa);
        if (IsRed(treeNode->GetLeft(thread)) && IsRed(leftChild->GetLeft(thread))) {
            treeNode = JSHandle<RBTreeNode>(thread, treeNode->RotateRight(thread));
        }
    }
    if (IsRed(treeNode->GetLeft(thread)) && IsRed(treeNode->GetRight(thread))) {
        treeNode->FlipColors(thread);
    }

    // 1 : root count
    uint32_t count = Count(treeNode->GetLeft(thread)) + Count(treeNode->GetRight(thread)) + 1;
    treeNode->SetCount(count);

    return treeNode;
}

// make a right-leaning link lean to the left
RBTreeNode *RBTreeNode::RotateLeft(JSThread *thread)
{
    ASSERT(!JSTaggedValue(this).IsHole() && IsRed(GetRight(thread)));
    RBTreeNode *temp = RBTreeNode::Cast(GetRight(thread).GetTaggedObject());
    SetRight(thread, temp->GetLeft(thread));
    temp->SetLeft(thread, JSTaggedValue(this));
    RBTreeNode *tempLeft = RBTreeNode::Cast(temp->GetLeft(thread).GetTaggedObject());
    temp->SetIsRed(tempLeft->GetIsRed());
    tempLeft->SetIsRed(true);

    temp->SetCount(GetCount());
    // 1 : root count
    uint32_t count = Count(GetLeft(thread)) + Count(GetRight(thread)) + 1;
    SetCount(count);

    return temp;
}

// make a left-leaning link lean to the right
RBTreeNode *RBTreeNode::RotateRight(JSThread *thread)
{
    ASSERT(!JSTaggedValue(this).IsHole() && IsRed(GetLeft(thread)));
    RBTreeNode *temp = RBTreeNode::Cast(GetLeft(thread).GetTaggedObject());
    SetLeft(thread, temp->GetRight(thread));
    temp->SetRight(thread, JSTaggedValue(this));
    RBTreeNode *tempRight = RBTreeNode::Cast(temp->GetRight(thread).GetTaggedObject());
    temp->SetIsRed(tempRight->GetIsRed());
    tempRight->SetIsRed(true);

    temp->SetCount(GetCount());
    // 1 : root count
    uint32_t count = Count(GetLeft(thread)) + Count(GetRight(thread)) + 1;
    SetCount(count);

    return temp;
}

// flip the colors of a node and its two children
void RBTreeNode::FlipColors(JSThread *thread)
{
    SetIsRed(!GetIsRed());
    RBTreeNode *leftChild = RBTreeNode::Cast(GetLeft(thread).GetTaggedObject());
    leftChild->SetIsRed(!leftChild->GetIsRed());
    RBTreeNode *rightChild = RBTreeNode::Cast(GetRight(thread).GetTaggedObject());
    rightChild->SetIsRed(!rightChild->GetIsRed());
}

// restore red-black tree invariant
JSTaggedValue RBTreeNode::Balance(JSThread *thread, RBTreeNode *treeNode)
{
    if (IsRed(treeNode->GetRight(thread)) && !IsRed(treeNode->GetLeft(thread))) {
        treeNode = treeNode->RotateLeft(thread);
    }
    JSTaggedValue leftValue = treeNode->GetLeft(thread);
    if (!leftValue.IsHole()) {
        RBTreeNode *leftChild = RBTreeNode::Cast(leftValue.GetTaggedObject());
        if (IsRed(treeNode->GetLeft(thread)) && IsRed(leftChild->GetLeft(thread))) {
            treeNode = treeNode->RotateRight(thread);
        }
    }
    if (IsRed(treeNode->GetLeft(thread)) && IsRed(treeNode->GetRight(thread))) {
        treeNode->FlipColors(thread);
    }
    // 1 : root count
    uint32_t count = Count(treeNode->GetLeft(thread)) + Count(treeNode->GetRight(thread)) + 1;
    treeNode->SetCount(count);

    return JSTaggedValue(treeNode);
}

RBTreeNode *RBTreeNode::MoveRedLeft(JSThread *thread)
{
    RBTreeNode *treeNode = this;
    treeNode->FlipColors(thread);
    RBTreeNode *rightChild = RBTreeNode::Cast(treeNode->GetRight(thread).GetTaggedObject());
    if (IsRed(rightChild->GetLeft(thread))) {
        rightChild = rightChild->RotateRight(thread);
        treeNode->SetRight(thread, JSTaggedValue(rightChild));
        treeNode = treeNode->RotateLeft(thread);
        treeNode->FlipColors(thread);
    }

    return treeNode;
}

RBTreeNode *RBTreeNode::MoveRedRight(JSThread *thread)
{
    RBTreeNode *treeNode = this;
    treeNode->FlipColors(thread);
    RBTreeNode *leftChild = RBTreeNode::Cast(treeNode->GetLeft(thread).GetTaggedObject());
    if (IsRed(leftChild->GetLeft(thread))) {
        treeNode = treeNode->RotateRight(thread);
        treeNode->FlipColors(thread);
    }

    return treeNode;
}

// delete the key-value pair with the minimum key rooted at treeNode
JSTaggedValue RBTreeNode::DeleteMin(JSThread *thread, RBTreeNode *treeNode)
{
    if (treeNode->GetLeft(thread).IsHole()) {
        return JSTaggedValue::Hole();
    }
    RBTreeNode *leftChild = RBTreeNode::Cast(treeNode->GetLeft(thread).GetTaggedObject());
    if (!IsRed(treeNode->GetLeft(thread)) && !IsRed(leftChild->GetLeft(thread))) {
        treeNode = treeNode->MoveRedLeft(thread);
        leftChild = RBTreeNode::Cast(treeNode->GetLeft(thread).GetTaggedObject());
    }

    treeNode->SetLeft(thread, DeleteMin(thread, leftChild));
    return Balance(thread, treeNode);
}

// delete the key-value pair with the given key rooted at treeNode
JSTaggedValue RBTreeNode::Delete(JSThread *thread, const JSTaggedValue &treeNodeVa, int hash,
                                 const JSTaggedValue &key, JSTaggedValue &oldValue)
{
    if (treeNodeVa.IsHole()) {
        return JSTaggedValue::Hole();
    }
    RBTreeNode *treeNode = RBTreeNode::Cast(treeNodeVa.GetTaggedObject());
    JSTaggedValue leftChildVa = treeNode->GetLeft(thread);
    JSTaggedValue treeNodeKey = treeNode->GetKey(thread);
    int cmp = Compare(thread, hash, key, treeNode->GetHash(thread).GetInt(), treeNodeKey);
    if (cmp < 0) {
        if (!leftChildVa.IsHole() &&
            !IsRed(treeNode->GetLeft(thread)) &&
            !IsRed(RBTreeNode::Cast(leftChildVa.GetTaggedObject())->GetLeft(thread))) {
            treeNode = treeNode->MoveRedLeft(thread);
        }
        leftChildVa = treeNode->GetLeft(thread);
        JSTaggedValue leftValue = Delete(thread, leftChildVa, hash, key, oldValue);
        treeNode->SetLeft(thread, leftValue);
    } else {
        if (IsRed(treeNode->GetLeft(thread))) {
            treeNode = treeNode->RotateRight(thread);
            treeNodeKey = treeNode->GetKey(thread);
        }
        cmp = Compare(thread, hash, key, treeNode->GetHash(thread).GetInt(), treeNodeKey);
        if (cmp == 0 && treeNode->GetRight(thread).IsHole()) {
            oldValue = treeNode->GetValue(thread);
            return JSTaggedValue::Hole();
        }
        JSTaggedValue rightChildVa = treeNode->GetRight(thread);
        if (!rightChildVa.IsHole() &&
            !IsRed(rightChildVa) && !IsRed(RBTreeNode::Cast(rightChildVa.GetTaggedObject())->GetLeft(thread))) {
            treeNode = treeNode->MoveRedRight(thread);
            treeNodeKey = treeNode->GetKey(thread);
        }
        
        cmp = Compare(thread, hash, key, treeNode->GetHash(thread).GetInt(), treeNodeKey);
        rightChildVa = treeNode->GetRight(thread);
        if (rightChildVa.IsHole()) {
            return Balance(thread, treeNode);
        }
        RBTreeNode *rightChild = RBTreeNode::Cast(rightChildVa.GetTaggedObject());
        if (cmp == 0) {
            oldValue = treeNode->GetValue(thread);
            RBTreeNode *minNode = rightChild->Min(thread);
            treeNode->SetKey(thread, minNode->GetKey(thread));
            treeNode->SetValue(thread, minNode->GetValue(thread));
            treeNode->SetHash(thread, minNode->GetHash(thread));
            treeNode->SetRight(thread, DeleteMin(thread, rightChild));
        } else {
            JSTaggedValue tmpValue = Delete(thread, rightChildVa, hash, key, oldValue);
            treeNode->SetRight(thread, tmpValue);
        }
    }
    return Balance(thread, treeNode);
}

// the smallest key in subtree rooted at treeNode; hole if no such key
RBTreeNode *RBTreeNode::Min(JSThread *thread)
{
    if (GetLeft(thread).IsHole()) {
        return this;
    } else {
        return RBTreeNode::Cast(GetLeft(thread).GetTaggedObject())->Min(thread);
    }
}

// node associated with the given key in subtree rooted at treeNode; null if no such key
JSTaggedValue RBTreeNode::GetTreeNode(JSThread *thread, JSHandle<JSTaggedValue> treeNodeVa,
                                      int hash, JSHandle<JSTaggedValue> key)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<TaggedQueue> queue(thread, factory->NewTaggedQueue(0));
    queue.Update(JSTaggedValue(TaggedQueue::Push(thread, queue, treeNodeVa)));
    JSMutableHandle<RBTreeNode> root(thread, JSTaggedValue::Hole());
    JSMutableHandle<JSTaggedValue> currentKey(thread, JSTaggedValue::Hole());
    JSMutableHandle<JSTaggedValue> left(thread, JSTaggedValue::Hole());
    JSMutableHandle<JSTaggedValue> right(thread, JSTaggedValue::Hole());
    while (!queue->Empty(thread)) {
        root.Update(queue->Pop(thread));
        currentKey.Update(root->GetKey(thread));
        if (root->GetHash(thread).GetInt() == hash &&
            (!currentKey->IsHole() && JSTaggedValue::SameValue(thread, key, currentKey))) {
            return root.GetTaggedValue();
        }
        if (!root->GetLeft(thread).IsHole()) {
            left.Update(root->GetLeft(thread));
            queue.Update(JSTaggedValue(TaggedQueue::Push(thread, queue, left)));
        }
        if (!root->GetRight(thread).IsHole()) {
            right.Update(root->GetRight(thread));
            queue.Update(JSTaggedValue(TaggedQueue::Push(thread, queue, right)));
        }
    }
    return JSTaggedValue::Hole();
}
} // namespace panda::ecmascript
