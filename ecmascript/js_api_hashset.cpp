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

#include "js_api_hashset.h"
#include "ecmascript/tagged_queue.h"
#include "tagged_hash_array.h"
#include "tagged_node.h"

namespace panda::ecmascript {
JSTaggedValue JSAPIHashSet::IsEmpty()
{
    return JSTaggedValue(GetSize() == 0);
}

JSTaggedValue JSAPIHashSet::Has(JSThread *thread, JSTaggedValue value)
{
    TaggedHashArray *hashArray = TaggedHashArray::Cast(GetTable().GetTaggedObject());
    int hash = TaggedNode::Hash(value);
    return JSTaggedValue(!(hashArray->GetNode(thread, hash, value).IsHole()));
}

void JSAPIHashSet::Add(JSThread *thread, JSHandle<JSAPIHashSet> hashSet, JSHandle<JSTaggedValue> value)
{
    if (!TaggedHashArray::IsKey(value.GetTaggedValue())) {
        THROW_TYPE_ERROR(thread, "the value must be Key of JS");
    }
    JSHandle<TaggedHashArray> hashArray(thread, hashSet->GetTable());
    int hash = TaggedNode::Hash(value.GetTaggedValue());
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());
    JSTaggedValue setValue = TaggedHashArray::SetVal(thread, hashArray, hash, value, nullHandle);
    uint32_t nodeNum = hashSet->GetSize();
    if (!setValue.IsUndefined()) {
        hashSet->SetSize(++nodeNum);
    }
    uint32_t tableLength = hashArray->GetLength() * TaggedHashArray::DEFAULT_LOAD_FACTOR;
    if (nodeNum > tableLength) {
        hashArray = TaggedHashArray::Resize(thread, hashArray, hashArray->GetLength());
    }
    hashSet->SetTable(thread, hashArray);
}

void JSAPIHashSet::Clear(JSThread *thread)
{
    TaggedHashArray *hashArray = TaggedHashArray::Cast(GetTable().GetTaggedObject());
    uint32_t nodeLength = GetSize();
    if (nodeLength > 0) {
        hashArray->Clear(thread);
        SetSize(0);
    }
}

JSTaggedValue JSAPIHashSet::Remove(JSThread *thread, JSHandle<JSAPIHashSet> hashSet, JSTaggedValue key)
{
    if (!TaggedHashArray::IsKey(key)) {
        return JSTaggedValue::False();
    }

    JSHandle<TaggedHashArray> hashArray(thread, hashSet->GetTable());
    uint32_t nodeNum = hashSet->GetSize();
    if (nodeNum == 0) {
        return JSTaggedValue::False();
    }
    int hash = TaggedNode::Hash(key);
    JSTaggedValue removeValue = hashArray->RemoveNode(thread, hash, key);
    if (removeValue.IsHole()) {
        return JSTaggedValue::False();
    }
    hashSet->SetSize(--nodeNum);
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
    return JSTaggedValue::True();
}
}
