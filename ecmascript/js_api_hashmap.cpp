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

#include "js_api_hashmap.h"
#include "ecmascript/tagged_hash_array.h"
#include "ecmascript/tagged_queue.h"
#include "js_handle.h"
#include "object_factory.h"
#include "tagged_node.h"

namespace panda::ecmascript {
JSTaggedValue JSAPIHashMap::IsEmpty()
{
    return JSTaggedValue(GetSize() == 0);
}

JSTaggedValue JSAPIHashMap::HasKey(JSThread *thread, JSTaggedValue key)
{
    TaggedHashArray *hashArray = TaggedHashArray::Cast(GetTable().GetTaggedObject());
    int hash = TaggedNode::Hash(key);
    return JSTaggedValue(!(hashArray->GetNode(thread, hash, key).IsHole()));
}

JSTaggedValue JSAPIHashMap::HasValue(JSThread *thread, JSHandle<JSAPIHashMap> hashMap,
                                     JSHandle<JSTaggedValue> value)
{
    JSHandle<TaggedHashArray> hashArray(thread, hashMap->GetTable());
    uint32_t tabLength = hashArray->GetLength();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<TaggedQueue> queue(factory->NewTaggedQueue(0));
    JSMutableHandle<TaggedNode> node(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> currentValue(thread, JSTaggedValue::Undefined());
    uint32_t index = 0;
    while (index < tabLength) {
        node.Update(TaggedHashArray::GetCurrentNode(thread, queue, hashArray, index));
        if (!node.GetTaggedValue().IsHole()) {
            currentValue.Update(node->GetValue());
            if (JSTaggedValue::SameValue(value, currentValue)) {
                return JSTaggedValue::True();
            }
        }
    }
    return JSTaggedValue::False();
}

JSTaggedValue JSAPIHashMap::Replace(JSThread *thread, JSTaggedValue key, JSTaggedValue newValue)
{
    TaggedHashArray *hashArray = TaggedHashArray::Cast(GetTable().GetTaggedObject());
    int hash = TaggedNode::Hash(key);
    JSTaggedValue nodeVa = hashArray->GetNode(thread, hash, key);
    if (nodeVa.IsHole()) {
        return JSTaggedValue::False();
    }
    if (nodeVa.IsLinkedNode()) {
        LinkedNode::Cast(nodeVa.GetTaggedObject())->SetValue(thread, newValue);
    } else {
        RBTreeNode::Cast(nodeVa.GetTaggedObject())->SetValue(thread, newValue);
    }
    return JSTaggedValue::True();
}

void JSAPIHashMap::Set(JSThread *thread, JSHandle<JSAPIHashMap> hashMap,
                       JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> value)
{
    if (!TaggedHashArray::IsKey(key.GetTaggedValue())) {
        THROW_TYPE_ERROR(thread, "the value must be Key of JS");
    }
    JSHandle<TaggedHashArray> hashArray(thread, hashMap->GetTable());
    int hash = TaggedNode::Hash(key.GetTaggedValue());
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
    int hash = TaggedNode::Hash(key);
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
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<TaggedQueue> queue(factory->NewTaggedQueue(0));
    JSHandle<TaggedHashArray> thisHashArray(thread, dst->GetTable());
    uint32_t nodeNum = dst->GetSize();
    uint32_t index = 0;
    JSMutableHandle<TaggedNode> node(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> currentKey(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> currentValue(thread, JSTaggedValue::Undefined());
    while (index < srcTabLength) {
        node.Update(TaggedHashArray::GetCurrentNode(thread, queue, hashArray, index));
        if (!node.GetTaggedValue().IsHole()) {
            currentKey.Update(node->GetKey());
            currentValue.Update(node->GetValue());
            JSTaggedValue setValue =
                TaggedHashArray::SetVal(thread, thisHashArray, node->GetHash().GetInt(), currentKey, currentValue);
            if (!setValue.IsUndefined()) {
                dst->SetSize(++nodeNum);
            }
            uint32_t tableLength = (thisHashArray->GetLength()) * TaggedHashArray::DEFAULT_LOAD_FACTOR;
            if (nodeNum > tableLength) {
                thisHashArray = TaggedHashArray::Resize(thread, thisHashArray, thisHashArray->GetLength());
            }
        }
    }
    dst->SetTable(thread, thisHashArray);
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
    int hash = TaggedNode::Hash(key);
    JSHandle<JSTaggedValue> removeValue(thread, hashArray->RemoveNode(thread, hash, key));
    if (removeValue->IsHole()) {
        return JSTaggedValue::Undefined();
    }
    hashMap->SetSize(--nodeNum);
    uint32_t length = hashArray->GetLength();
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
