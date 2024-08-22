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

#ifndef ECMASCRIPT_TAGGED_HASH_ARRAY_H
#define ECMASCRIPT_TAGGED_HASH_ARRAY_H

#include "ecmascript/js_api/js_api_hashmap.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/tagged_node.h"

namespace panda::ecmascript {
class TaggedHashArray : public TaggedArray {
public:
    static constexpr uint32_t MAXIMUM_CAPACITY = 1 << 30;
    static constexpr uint32_t UNTREEIFY_THRESHOLD = 6;
    static constexpr uint32_t DEFAULT_INITIAL_CAPACITY = 1 << 4;
    static constexpr float DEFAULT_LOAD_FACTOR = 0.75f;
    static constexpr uint32_t TREEIFY_THRESHOLD = 8;

    static TaggedHashArray *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsTaggedArray());
        return static_cast<TaggedHashArray *>(object);
    }
    static JSTaggedValue Create(const JSThread *thread, uint32_t numberOfElements = DEFAULT_INITIAL_CAPACITY);
    static JSTaggedValue SetVal(JSThread *thread, JSHandle<TaggedHashArray> table, int hash,
                                JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> value);
    static JSHandle<TaggedHashArray> Resize(JSThread *thread, const JSHandle<TaggedHashArray> &oldTab,
                                            uint32_t Capacity);
    static JSHandle<LinkedNode> NewLinkedNode(JSThread *thread, int hash, JSHandle<JSTaggedValue> key,
                                              JSHandle<JSTaggedValue> value);
    static JSHandle<LinkedNode> CreateLinkedNodeFrom(JSThread *thread, JSHandle<RBTreeNode> treeNode);
    static JSHandle<RBTreeNode> NewTreeNode(JSThread *thread, int hash, JSHandle<JSTaggedValue> key,
                                            JSHandle<JSTaggedValue> value);
    static JSHandle<RBTreeNode> CreateTreeNodeFrom(JSThread *thread, JSHandle<LinkedNode> linkedNode);
    static JSHandle<JSTaggedValue> GetCurrentNode(JSThread *thread, JSMutableHandle<TaggedQueue> &queue,
                                                  const JSHandle<TaggedHashArray> &tableArr, uint32_t &index);
    JSTaggedValue GetNode(JSThread *thread, int hash, JSTaggedValue key);
    JSTaggedValue RemoveNode(JSThread *thread, int hash, JSTaggedValue key);
    void Clear(JSThread *thread);
    inline static bool IsKey(JSTaggedValue key)
    {
        return !key.IsHole();
    }

    DECL_DUMP()
private:
    static void TreeingBin(JSThread *thread, const JSHandle<TaggedHashArray> &tab, int hash);
    static void NodeDisperse(JSThread *thread, const JSHandle<TaggedHashArray> &newTab,
                             JSTaggedValue nodeVa, int index, int oldCapacity);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TAGGED_HASH_ARRAY_H
