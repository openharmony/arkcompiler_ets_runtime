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

#include "ecmascript/js_api/js_api_hashmap.h"

#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/tagged_hash_array.h"

namespace panda::ecmascript {
using ContainerError = containers::ContainerError;
using ErrorFlag = containers::ErrorFlag;
JSTaggedValue JSAPIHashMap::IsEmpty()
{
    return JSTaggedValue(GetSize() == 0);
}

JSTaggedValue JSAPIHashMap::HasKey(JSThread *thread, JSTaggedValue key)
{
    TaggedHashArray *hashArray = TaggedHashArray::Cast(GetTable().GetTaggedObject());
    int hash = TaggedNode::Hash(thread, key);
    return JSTaggedValue(!(hashArray->GetNode(thread, hash, key).IsHole()));
}

JSTaggedValue JSAPIHashMap::HasValue(JSThread *thread, JSHandle<JSAPIHashMap> hashMap,
                                     JSHandle<JSTaggedValue> value)
{
    JSHandle<TaggedHashArray> hashArray(thread, hashMap->GetTable());
    uint32_t tabLength = hashArray->GetLength();
    JSTaggedValue taggedValue = value.GetTaggedValue();
    for (uint32_t index = 0; index < tabLength; index++) {
        JSTaggedValue node = hashArray->Get(index);
        if (node.IsHole()) {
            continue;
        }
        if (node.IsLinkedNode()) {
            if (HasValueLinkedNode(node, taggedValue)) {
                return JSTaggedValue::True();
            }
        } else {
            if (HasValueRBTreeNode(node, taggedValue)) {
                return JSTaggedValue::True();
            }
        }
    }
    return JSTaggedValue::False();
}

bool JSAPIHashMap::HasValueLinkedNode(JSTaggedValue node, JSTaggedValue value)
{
    ASSERT(node.IsLinkedNode());
    while (!node.IsHole()) {
        LinkedNode *p = LinkedNode::Cast(node.GetTaggedObject());
        if (JSTaggedValue::SameValue(p->GetValue(), value)) {
            return true;
        }
        node = p->GetNext();
    }
    return false;
}

bool JSAPIHashMap::HasValueRBTreeNode(JSTaggedValue node, JSTaggedValue value)
{
    ASSERT(node.IsRBTreeNode());
    RBTreeNode *p = RBTreeNode::Cast(node.GetTaggedObject());
    if (JSTaggedValue::SameValue(p->GetValue(), value)) {
        return true;
    }
    JSTaggedValue left = p->GetLeft();
    if (!left.IsHole() && HasValueRBTreeNode(left, value)) {
        return true;
    }
    JSTaggedValue right = p->GetRight();
    if (!right.IsHole() && HasValueRBTreeNode(right, value)) {
        return true;
    }
    return false;
}

bool JSAPIHashMap::Replace(JSThread *thread, JSTaggedValue key, JSTaggedValue newValue)
{
    TaggedHashArray *hashArray = TaggedHashArray::Cast(GetTable().GetTaggedObject());
    int hash = TaggedNode::Hash(thread, key);
    JSTaggedValue nodeVa = hashArray->GetNode(thread, hash, key);
    if (nodeVa.IsHole()) {
        return false;
    }
    if (nodeVa.IsLinkedNode()) {
        LinkedNode::Cast(nodeVa.GetTaggedObject())->SetValue(thread, newValue);
    } else {
        RBTreeNode::Cast(nodeVa.GetTaggedObject())->SetValue(thread, newValue);
    }
    return true;
}

void JSAPIHashMap::Set(JSThread *thread, JSHandle<JSAPIHashMap> hashMap,
                       JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> value)
{
    if (key.GetTaggedValue().IsUndefined()) {
        JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, key.GetTaggedValue());
        CString errorMsg =
            "The type of \"key\" must be Key of JS. Received value is: " + ConvertToString(*result);
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, errorMsg.c_str());
        THROW_NEW_ERROR_AND_RETURN(thread, error);
    }
    JSHandle<TaggedHashArray> hashArray(thread, hashMap->GetTable());
    int hash = TaggedNode::Hash(thread, key.GetTaggedValue());
    JSTaggedValue setValue = TaggedHashArray::SetVal(thread, hashArray, hash, key, value);
    uint32_t nodeNum = hashMap->GetSize();
    if (!setValue.IsUndefined()) {
        hashMap->SetSize(++nodeNum);
    }
    uint32_t tableLength = (hashArray->GetLength()) * TaggedHashArray::DEFAULT_LOAD_FACTOR;
    if (nodeNum > tableLength) {
        hashArray = TaggedHashArray::Resize(thread, hashArray, hashArray->GetLength());
        hashMap->SetTable(thread, hashArray);
    }
}

JSTaggedValue JSAPIHashMap::Get(JSThread *thread, JSTaggedValue key)
{
    TaggedHashArray *hashArray = TaggedHashArray::Cast(GetTable().GetTaggedObject());
    int hash = TaggedNode::Hash(thread, key);
    JSTaggedValue node = hashArray->GetNode(thread, hash, key);
    if (node.IsHole()) {
        return JSTaggedValue::Undefined();
    } else if (node.IsRBTreeNode()) {
        return RBTreeNode::Cast(node.GetTaggedObject())->GetValue();
    } else {
        return LinkedNode::Cast(node.GetTaggedObject())->GetValue();
    }
}

void JSAPIHashMap::SetAll(JSThread *thread, JSHandle<JSAPIHashMap> dst, JSHandle<JSAPIHashMap> src)
{
    JSHandle<TaggedHashArray> hashArray(thread, src->GetTable());
    uint32_t srcTabLength = hashArray->GetLength();
    JSMutableHandle<JSTaggedValue> node(thread, JSTaggedValue::Hole());
    for (uint32_t index = 0; index < srcTabLength; index++) {
        node.Update(hashArray->Get(index));
        if (node->IsHole()) {
            continue;
        }
        if (node->IsLinkedNode()) {
            SetAllLinkedNode(thread, dst, JSMutableHandle<LinkedNode>::Cast(node));
        } else {
            SetAllRBTreeNode(thread, dst, JSHandle<RBTreeNode>(node));
        }
    }
}

void JSAPIHashMap::SetAllLinkedNode(JSThread *thread, JSHandle<JSAPIHashMap> hashMap, JSMutableHandle<LinkedNode> node)
{
    ASSERT(node.GetTaggedValue().IsLinkedNode());
    while (!node.GetTaggedValue().IsHole()) {
        if (!hashMap->Replace(thread, node->GetKey(), node->GetValue())) {
            JSHandle<JSTaggedValue> key(thread, node->GetKey());
            JSHandle<JSTaggedValue> value(thread, node->GetValue());
            Set(thread, hashMap, key, value);
        }
        node.Update(node->GetNext());
    }
}

void JSAPIHashMap::SetAllRBTreeNode(JSThread *thread, JSHandle<JSAPIHashMap> hashMap, JSHandle<RBTreeNode> node)
{
    ASSERT(node.GetTaggedValue().IsRBTreeNode());
    JSMutableHandle<JSTaggedValue> key(thread, node->GetKey());
    JSMutableHandle<JSTaggedValue> value(thread, node->GetValue());
    if (!hashMap->Replace(thread, key.GetTaggedValue(), value.GetTaggedValue())) {
        Set(thread, hashMap, key, value);
    }
    JSMutableHandle<RBTreeNode> left(thread, node->GetLeft());
    if (!left.GetTaggedValue().IsHole()) {
        SetAllRBTreeNode(thread, hashMap, left);
    }
    JSMutableHandle<RBTreeNode> right(thread, node->GetRight());
    if (!right.GetTaggedValue().IsHole()) {
        SetAllRBTreeNode(thread, hashMap, right);
    }
}

void JSAPIHashMap::Clear(JSThread *thread)
{
    TaggedHashArray *hashArray = TaggedHashArray::Cast(GetTable().GetTaggedObject());
    uint32_t nodeLength = GetSize();
    if (nodeLength > 0) {
        hashArray->Clear(thread);
        SetSize(0);
    }
}

JSTaggedValue JSAPIHashMap::Remove(JSThread *thread, JSHandle<JSAPIHashMap> hashMap, JSTaggedValue key)
{
    if (!TaggedHashArray::IsKey(key)) {
        return JSTaggedValue::Undefined();
    }

    JSHandle<TaggedHashArray> hashArray(thread, hashMap->GetTable());
    uint32_t nodeNum = hashMap->GetSize();
    if (nodeNum == 0) {
        return JSTaggedValue::Undefined();
    }
    uint32_t hash = static_cast<uint32_t>(TaggedNode::Hash(thread, key));
    JSHandle<JSTaggedValue> removeValue(thread, hashArray->RemoveNode(thread, hash, key));
    if (removeValue->IsHole()) {
        return JSTaggedValue::Undefined();
    }
    hashMap->SetSize(--nodeNum);
    uint32_t length = hashArray->GetLength();
    ASSERT_PRINT(length >= TaggedHashArray::DEFAULT_INITIAL_CAPACITY,
                 "TaggedHashArray length must greater than or equal to the default minimum value");

    uint32_t index = (length - 1) & hash;
    JSTaggedValue rootVa = hashArray->Get(index);
    if (rootVa.IsRBTreeNode()) {
        uint32_t numTreeNode = RBTreeNode::Count(rootVa);
        if (numTreeNode < TaggedHashArray::UNTREEIFY_THRESHOLD) {
            JSHandle<RBTreeNode> root(thread, rootVa);
            JSHandle<LinkedNode> head = RBTreeNode::Detreeing(thread, root);
            hashArray->Set(thread, index, head);
        }
    }
    return removeValue.GetTaggedValue();
}
}
