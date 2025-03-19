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
        key.Update(next->GetKey());
        value.Update(next->GetValue());
        rootNode.Update(RBTreeNode::Set(thread, rootNode, next->GetHash().GetInt(), key, value));
        rootNode->SetIsRed(thread, JSTaggedValue(false));
        next.Update(next->GetNext());
    }
    return rootNode;
}

void RBTreeNode::InitRBTreeNode(JSThread *thread, int hash, JSHandle<JSTaggedValue> key,
                                JSHandle<JSTaggedValue> value, int count)
{
    InitTaggedNode(thread, hash, key, value);
    SetLeft(thread, JSTaggedValue::Hole());
    SetRight(thread, JSTaggedValue::Hole());
    SetIsRed(thread, JSTaggedValue(true));
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
        JSHandle<RBTreeNode> leftChild = JSHandle<RBTreeNode>(thread, treeNode->GetLeft());
        InOrderTraverse(thread, leftChild, head, tail);
        JSHandle<LinkedNode> linkedNode = TaggedHashArray::CreateLinkedNodeFrom(thread, treeNode);
        if (tail.GetTaggedValue().IsHole()) {
            head = linkedNode;
        } else {
            tail->SetNext(thread, linkedNode.GetTaggedValue());
        }
        tail = linkedNode;
        JSHandle<RBTreeNode> rightChild(thread, treeNode->GetRight());
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
        JSHandle<RBTreeNode> leftChild(thread, treeNode->GetLeft());
        InOrderTraverse(thread, leftChild, bit, nodeStruct);
        
        JSHandle<LinkedNode> linkedNode = TaggedHashArray::CreateLinkedNodeFrom(thread, treeNode);
        // the elements from each bin must either stay at same index,
        // or move with a power of two offset in the new table
        if ((static_cast<uint32_t>(linkedNode->GetHash().GetInt()) & static_cast<uint32_t>(bit)) == 0) {
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

        JSHandle<RBTreeNode> rightChild(thread, treeNode->GetRight());
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
        lowerHead.Update(lowerHead->GetNext());
    }
    JSMutableHandle<LinkedNode> higherHead(thread, nodeStruct.higherHead);
    while (!higherHead.GetTaggedValue().IsHole()) {
        loCount++;
        higherHead.Update(higherHead->GetNext());
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

int RBTreeNode::Compare(int hash1, JSTaggedValue key1, int hash2, JSTaggedValue key2)
{
    ASSERT(!key1.IsHole() && !key2.IsHole());
    if (JSTaggedValue::SameValue(key1, key2)) {
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
    return treeNode->GetIsRed().ToBoolean();
}

// insert the key-value pair in the subtree rooted at treeNode
JSHandle<RBTreeNode> RBTreeNode::Set(JSThread *thread, JSHandle<RBTreeNode> treeNode, int hash,
                                     JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> value)
{
    if (treeNode.GetTaggedValue().IsHole()) {
        treeNode = TaggedHashArray::NewTreeNode(thread, hash, key, value);
        return treeNode;
    }
    JSHandle<JSTaggedValue> treeNodeKey(thread, treeNode->GetKey());
    int cmp = Compare(hash, key.GetTaggedValue(), treeNode->GetHash().GetInt(), treeNodeKey.GetTaggedValue());
    JSHandle<RBTreeNode> leftChild(thread, treeNode->GetLeft());
    JSHandle<RBTreeNode> rightChild(thread, treeNode->GetRight());
    if (cmp < 0) {
        JSHandle<RBTreeNode> left = Set(thread, leftChild, hash, key, value);
        treeNode->SetLeft(thread, left);
    } else if (cmp > 0) {
        JSHandle<RBTreeNode> right = Set(thread, rightChild, hash, key, value);
        treeNode->SetRight(thread, right);
    } else {
        treeNode->SetValue(thread, value);
    }

    if (IsRed(treeNode->GetRight()) && !IsRed(treeNode->GetLeft())) {
        treeNode = JSHandle<RBTreeNode>(thread, treeNode->RotateLeft(thread));
    }
    JSTaggedValue leftChildVa = treeNode->GetLeft();
    if (!leftChildVa.IsHole()) {
        leftChild = JSHandle<RBTreeNode>(thread, leftChildVa);
        if (IsRed(treeNode->GetLeft()) && IsRed(leftChild->GetLeft())) {
            treeNode = JSHandle<RBTreeNode>(thread, treeNode->RotateRight(thread));
        }
    }
    if (IsRed(treeNode->GetLeft()) && IsRed(treeNode->GetRight())) {
        treeNode->FlipColors(thread);
    }

    // 1 : root count
    uint32_t count = Count(treeNode->GetLeft()) + Count(treeNode->GetRight()) + 1;
    treeNode->SetCount(count);

    return treeNode;
}

// make a right-leaning link lean to the left
RBTreeNode *RBTreeNode::RotateLeft(JSThread *thread)
{
    ASSERT(!JSTaggedValue(this).IsHole() && IsRed(GetRight()));
    RBTreeNode *temp = RBTreeNode::Cast(GetRight().GetTaggedObject());
    SetRight(thread, temp->GetLeft());
    temp->SetLeft(thread, JSTaggedValue(this));
    RBTreeNode *tempLeft = RBTreeNode::Cast(temp->GetLeft().GetTaggedObject());
    temp->SetIsRed(thread, tempLeft->GetIsRed());
    tempLeft->SetIsRed(thread, JSTaggedValue(true));

    temp->SetCount(GetCount());
    // 1 : root count
    uint32_t count = Count(GetLeft()) + Count(GetRight()) + 1;
    SetCount(count);

    return temp;
}

// make a left-leaning link lean to the right
RBTreeNode *RBTreeNode::RotateRight(JSThread *thread)
{
    ASSERT(!JSTaggedValue(this).IsHole() && IsRed(GetLeft()));
    RBTreeNode *temp = RBTreeNode::Cast(GetLeft().GetTaggedObject());
    SetLeft(thread, temp->GetRight());
    temp->SetRight(thread, JSTaggedValue(this));
    RBTreeNode *tempRight = RBTreeNode::Cast(temp->GetRight().GetTaggedObject());
    temp->SetIsRed(thread, tempRight->GetIsRed());
    tempRight->SetIsRed(thread, JSTaggedValue(true));

    temp->SetCount(GetCount());
    // 1 : root count
    uint32_t count = Count(GetLeft()) + Count(GetRight()) + 1;
    SetCount(count);

    return temp;
}

// flip the colors of a node and its two children
void RBTreeNode::FlipColors(JSThread *thread)
{
    SetIsRed(thread, JSTaggedValue(!GetIsRed().ToBoolean()));
    RBTreeNode *leftChild = RBTreeNode::Cast(GetLeft().GetTaggedObject());
    leftChild->SetIsRed(thread, JSTaggedValue(!leftChild->GetIsRed().ToBoolean()));
    RBTreeNode *rightChild = RBTreeNode::Cast(GetRight().GetTaggedObject());
    rightChild->SetIsRed(thread, JSTaggedValue(!rightChild->GetIsRed().ToBoolean()));
}

// restore red-black tree invariant
JSTaggedValue RBTreeNode::Balance(JSThread *thread, RBTreeNode *treeNode)
{
    if (IsRed(treeNode->GetRight()) && !IsRed(treeNode->GetLeft())) {
        treeNode = treeNode->RotateLeft(thread);
    }
    JSTaggedValue leftValue = treeNode->GetLeft();
    if (!leftValue.IsHole()) {
        RBTreeNode *leftChild = RBTreeNode::Cast(leftValue.GetTaggedObject());
        if (IsRed(treeNode->GetLeft()) && IsRed(leftChild->GetLeft())) {
            treeNode = treeNode->RotateRight(thread);
        }
    }
    if (IsRed(treeNode->GetLeft()) && IsRed(treeNode->GetRight())) {
        treeNode->FlipColors(thread);
    }
    // 1 : root count
    uint32_t count = Count(treeNode->GetLeft()) + Count(treeNode->GetRight()) + 1;
    treeNode->SetCount(count);

    return JSTaggedValue(treeNode);
}

RBTreeNode *RBTreeNode::MoveRedLeft(JSThread *thread)
{
    RBTreeNode *treeNode = this;
    treeNode->FlipColors(thread);
    RBTreeNode *rightChild = RBTreeNode::Cast(treeNode->GetRight().GetTaggedObject());
    if (IsRed(rightChild->GetLeft())) {
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
    RBTreeNode *leftChild = RBTreeNode::Cast(treeNode->GetLeft().GetTaggedObject());
    if (IsRed(leftChild->GetLeft())) {
        treeNode = treeNode->RotateRight(thread);
        treeNode->FlipColors(thread);
    }

    return treeNode;
}

// delete the key-value pair with the minimum key rooted at treeNode
JSTaggedValue RBTreeNode::DeleteMin(JSThread *thread, RBTreeNode *treeNode)
{
    if (treeNode->GetLeft().IsHole()) {
        return JSTaggedValue::Hole();
    }
    RBTreeNode *leftChild = RBTreeNode::Cast(treeNode->GetLeft().GetTaggedObject());
    if (!IsRed(treeNode->GetLeft()) && !IsRed(leftChild->GetLeft())) {
        treeNode = treeNode->MoveRedLeft(thread);
        leftChild = RBTreeNode::Cast(treeNode->GetLeft().GetTaggedObject());
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
    JSTaggedValue leftChildVa = treeNode->GetLeft();
    JSTaggedValue treeNodeKey = treeNode->GetKey();
    int cmp = Compare(hash, key, treeNode->GetHash().GetInt(), treeNodeKey);
    if (cmp < 0) {
        if (!leftChildVa.IsHole() &&
            !IsRed(treeNode->GetLeft()) && !IsRed(RBTreeNode::Cast(leftChildVa.GetTaggedObject())->GetLeft())) {
            treeNode = treeNode->MoveRedLeft(thread);
        }
        leftChildVa = treeNode->GetLeft();
        JSTaggedValue leftValue = Delete(thread, leftChildVa, hash, key, oldValue);
        treeNode->SetLeft(thread, leftValue);
    } else {
        if (IsRed(treeNode->GetLeft())) {
            treeNode = treeNode->RotateRight(thread);
            treeNodeKey = treeNode->GetKey();
        }
        cmp = Compare(hash, key, treeNode->GetHash().GetInt(), treeNodeKey);
        if (cmp == 0 && treeNode->GetRight().IsHole()) {
            oldValue = treeNode->GetValue();
            return JSTaggedValue::Hole();
        }
        JSTaggedValue rightChildVa = treeNode->GetRight();
        if (!rightChildVa.IsHole() &&
            !IsRed(rightChildVa) && !IsRed(RBTreeNode::Cast(rightChildVa.GetTaggedObject())->GetLeft())) {
            treeNode = treeNode->MoveRedRight(thread);
            treeNodeKey = treeNode->GetKey();
        }
        
        cmp = Compare(hash, key, treeNode->GetHash().GetInt(), treeNodeKey);
        rightChildVa = treeNode->GetRight();
        if (rightChildVa.IsHole()) {
            return Balance(thread, treeNode);
        }
        RBTreeNode *rightChild = RBTreeNode::Cast(rightChildVa.GetTaggedObject());
        if (cmp == 0) {
            oldValue = treeNode->GetValue();
            RBTreeNode *minNode = rightChild->Min();
            treeNode->SetKey(thread, minNode->GetKey());
            treeNode->SetValue(thread, minNode->GetValue());
            treeNode->SetHash(thread, minNode->GetHash());
            treeNode->SetRight(thread, DeleteMin(thread, rightChild));
        } else {
            JSTaggedValue tmpValue = Delete(thread, rightChildVa, hash, key, oldValue);
            treeNode->SetRight(thread, tmpValue);
        }
    }
    return Balance(thread, treeNode);
}

// the smallest key in subtree rooted at treeNode; hole if no such key
RBTreeNode *RBTreeNode::Min()
{
    if (GetLeft().IsHole()) {
        return this;
    } else {
        return RBTreeNode::Cast(GetLeft().GetTaggedObject())->Min();
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
    while (!queue->Empty()) {
        root.Update(queue->Pop(thread));
        currentKey.Update(root->GetKey());
        if (root->GetHash().GetInt() == hash && (!currentKey->IsHole() && JSTaggedValue::SameValue(key, currentKey))) {
            return root.GetTaggedValue();
        }
        if (!root->GetLeft().IsHole()) {
            left.Update(root->GetLeft());
            queue.Update(JSTaggedValue(TaggedQueue::Push(thread, queue, left)));
        }
        if (!root->GetRight().IsHole()) {
            right.Update(root->GetRight());
            queue.Update(JSTaggedValue(TaggedQueue::Push(thread, queue, right)));
        }
    }
    return JSTaggedValue::Hole();
}
} // namespace panda::ecmascript
