/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_TAGGED_NODE_H
#define ECMASCRIPT_TAGGED_NODE_H

#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/tagged_object.h"

namespace panda::ecmascript {
class JSThread;
class TaggedNode : public TaggedObject {
public:
    void InitTaggedNode(JSThread *thread, int hash, JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> value)
    {
        SetHash(thread, JSTaggedValue(hash));
        SetKey(thread, key.GetTaggedValue());
        SetValue(thread, value.GetTaggedValue());
    }

    static TaggedNode *Cast(TaggedObject *object)
    {
        return static_cast<TaggedNode *>(object);
    }

    static constexpr size_t HASH_OFFSET = TaggedObject::TaggedObjectSize();
    ACCESSORS(Hash, HASH_OFFSET, KEY_OFFSET);
    ACCESSORS(Key, KEY_OFFSET, VALUE_OFFSET);
    ACCESSORS(Value, VALUE_OFFSET, DATA_OFFSET);

    static constexpr size_t SIZE = DATA_OFFSET;
    DECL_VISIT_OBJECT(HASH_OFFSET, SIZE);

    static int Hash(const JSThread *thread, JSTaggedValue key)
    {
        if (key.IsInt()) {
            return key.GetInt();
        }
        if (key.IsDouble()) {
            double keyDoubleVal = key.GetDouble();
            int32_t tryIntKey = static_cast<int32_t>(keyDoubleVal);
            if (tryIntKey == keyDoubleVal) {
                return tryIntKey;
            }
            uint64_t keyValue = key.GetRawData();
            return GetHash32(reinterpret_cast<uint8_t *>(&keyValue), sizeof(keyValue) / sizeof(uint8_t));
        }
        if (key.IsSymbol()) {
            auto symbolString = JSSymbol::Cast(key.GetTaggedObject());
            return static_cast<JSTaggedNumber>(symbolString->GetHashField()).GetInt();
        }
        if (key.IsString()) {
            auto keyString = reinterpret_cast<EcmaString *>(key.GetTaggedObject());
            return EcmaStringAccessor(keyString).GetHashcode(thread);
        }
        if (key.IsECMAObject()) {
            int32_t hash = ECMAObject::Cast(key.GetTaggedObject())->GetHash(thread);
            if (hash == 0) {
                hash = base::RandomGenerator::GenerateIdentityHash();
                JSHandle<ECMAObject> ecmaObj(thread, key);
                ECMAObject::Cast(key.GetTaggedObject())->SetHash(thread, hash, ecmaObj);
            }
            return hash;
        }
        if (key.IsBigInt()) {
            uint32_t keyValue = BigInt::Cast(key.GetTaggedObject())->GetDigit(0);
            return GetHash32(reinterpret_cast<uint8_t *>(&keyValue), sizeof(keyValue) / sizeof(uint8_t));
        }
        // Special and HeapObject(except symbol and string)
        uint64_t keyValue = key.GetRawData();
        return GetHash32(reinterpret_cast<uint8_t *>(&keyValue), sizeof(keyValue) / sizeof(uint8_t));
    }
};

class LinkedNode : public TaggedNode {
public:
    void InitLinkedNode(JSThread *thread, int hash, JSHandle<JSTaggedValue> key,
                        JSHandle<JSTaggedValue> value, JSHandle<LinkedNode> next)
    {
        SetNext(thread, next);
        InitTaggedNode(thread, hash, key, value);
    }

    static LinkedNode *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsLinkedNode());
        return static_cast<LinkedNode *>(object);
    }

    static constexpr size_t NEXT_OFFSET = TaggedNode::SIZE;

    ACCESSORS(Next, NEXT_OFFSET, LAST_OFFSET);
 
    static constexpr size_t SIZE = LAST_OFFSET;
    DECL_VISIT_OBJECT(TaggedObject::SIZE, SIZE)
    DECL_DUMP()

    static JSHandle<RBTreeNode> Treeing(JSThread *thread, const JSHandle<LinkedNode> &head);
};

class RBTreeNode : public TaggedNode {
public:
    struct LinkedNodeStruct {
        JSHandle<LinkedNode> lowerHead;
        JSHandle<LinkedNode> lowerTail;
        JSHandle<LinkedNode> higherHead;
        JSHandle<LinkedNode> higherTail;
    };

    static RBTreeNode *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsRBTreeNode());
        return static_cast<RBTreeNode *>(object);
    }
    static constexpr size_t LEFT_OFFSET = TaggedNode::SIZE;
    ACCESSORS(Left, LEFT_OFFSET, RIGHT_OFFSET);
    ACCESSORS(Right, RIGHT_OFFSET, ISRED_OFFSET);
    ACCESSORS_FIXED_SIZE_FIELD(IsRed, bool, uint64_t, ISRED_OFFSET, COUNT_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(Count, uint32_t, COUNT_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);
    DECL_DUMP()
    DECL_VISIT_OBJECT(TaggedObject::SIZE, ISRED_OFFSET)

    void InitRBTreeNode(JSThread *thread, int hash, JSHandle<JSTaggedValue> key,
                        JSHandle<JSTaggedValue> value, int count);
    static void Divide(JSThread *thread, JSHandle<TaggedHashArray> table,
                       JSHandle<JSTaggedValue> nodeVa, int index, int bit);
    static JSHandle<RBTreeNode> Set(JSThread *thread, JSHandle<RBTreeNode> treeNode, int hash,
                                    JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> value);
    static JSTaggedValue Delete(JSThread *thread, const JSTaggedValue &treeNodeVa,
                                int hash, const JSTaggedValue &key, JSTaggedValue &oldValue);
    static JSTaggedValue GetTreeNode(JSThread *thread, JSHandle<JSTaggedValue> treeNodeVa, int hash,
                                     JSHandle<JSTaggedValue> key);
    static void InOrderTraverse(JSThread *thread, const JSHandle<RBTreeNode> &treeNode,
                                JSHandle<LinkedNode> &head, JSHandle<LinkedNode> &tail);
    static JSHandle<LinkedNode> Detreeing(JSThread *thread, const JSHandle<RBTreeNode> &root);
    static uint32_t Count(JSTaggedValue nodeValue);
    static int Compare(JSThread *thread, int hash1, JSTaggedValue key1, int hash2, JSTaggedValue key2);
    static bool IsRed(JSTaggedValue treeNodeValue);
private:
    static void InOrderTraverse(JSThread *thread, const JSHandle<RBTreeNode> &treeNode,
                                int bit, LinkedNodeStruct &nodeStruct);
    static JSTaggedValue Balance(JSThread *thread, RBTreeNode *treeNode);
    static JSTaggedValue DeleteMin(JSThread *thread, RBTreeNode *treeNode);
    RBTreeNode *RotateLeft(JSThread *thread);
    RBTreeNode *RotateRight(JSThread *thread);
    void FlipColors(JSThread *thread);
    RBTreeNode *MoveRedLeft(JSThread *thread);
    RBTreeNode *MoveRedRight(JSThread *thread);
    RBTreeNode *Min(JSThread *thread);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TAGGED_NODE_H
