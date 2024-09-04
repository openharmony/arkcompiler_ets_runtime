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
#include "ecmascript/tagged_queue.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class TaggedHashArrayTest : public BaseTestWithScope<false> {
};

/**
 * @tc.name: CreateTaggedHashArray
 * @tc.desc: Call "TaggedHashArray::Create" function Create TaggedHashArray object, check whether the object
 *           is created successfully.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TaggedHashArrayTest, CreateTaggedHashArray)
{
    int numOfElement = 64;
    JSHandle<TaggedHashArray> taggedHashArray(thread, TaggedHashArray::Create(thread, numOfElement));
    EXPECT_TRUE(*taggedHashArray != nullptr);
    JSHandle<TaggedArray> taggedArray(taggedHashArray);
    EXPECT_EQ(taggedHashArray->GetLength(), static_cast<uint32_t>(numOfElement));
    for (int i = 0; i < numOfElement; i++) {
        EXPECT_TRUE(taggedArray->Get(i).IsHole());
    }
}

/**
 * @tc.name: NewLinkedNode
 * @tc.desc: Call "NewLinkedNode" function Create LinkedNode object, check whether the object is created successfully.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TaggedHashArrayTest, NewLinkedNode)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> hashKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> hashKeyValue(thread, JSTaggedValue(11)); // 11: key value
    int keyHash = TaggedNode::Hash(thread, hashKey.GetTaggedValue());
    JSHandle<LinkedNode> linkedNode =
        TaggedHashArray::NewLinkedNode(thread, keyHash, hashKey, hashKeyValue);
    EXPECT_TRUE(*linkedNode != nullptr);
    EXPECT_TRUE(linkedNode->GetNext().IsHole());
    EXPECT_EQ(linkedNode->GetKey(), hashKey.GetTaggedValue());
    EXPECT_EQ(linkedNode->GetValue(), hashKeyValue.GetTaggedValue());
    EXPECT_EQ(linkedNode->GetHash().GetInt(), keyHash);
}

/**
 * @tc.name: NewTreeNode
 * @tc.desc: Call "NewTreeNode" function Create LinkedNode object, check whether the object is created successfully.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TaggedHashArrayTest, NewTreeNode)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> hashKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> hashKeyValue(thread, JSTaggedValue(12));  // 12: key value
    int keyHash = TaggedNode::Hash(thread, hashKey.GetTaggedValue());
    JSHandle<RBTreeNode> treeNode =
        TaggedHashArray::NewTreeNode(thread, keyHash, hashKey, hashKeyValue);
    EXPECT_TRUE(*treeNode != nullptr);
    EXPECT_TRUE(treeNode->GetIsRed().ToBoolean());
    EXPECT_TRUE(treeNode->GetLeft().IsHole());
    EXPECT_TRUE(treeNode->GetRight().IsHole());
}

void HashCommon1(JSThread *thread, JSHandle<TaggedHashArray>& taggedHashArray, std::string& myKey,
    std::string& myValue, uint32_t nums)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<RBTreeNode> rootTreeNode(thread, JSTaggedValue::Hole());
    for (uint32_t i = 0; i < nums; i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        JSHandle<JSTaggedValue> treeKey(thread, factory->NewFromStdString(iKey).GetTaggedValue());
        JSHandle<JSTaggedValue> treeValue(thread, factory->NewFromStdString(iValue).GetTaggedValue());
        auto keyHash = TaggedNode::Hash(thread, treeKey.GetTaggedValue());
        JSHandle<RBTreeNode> rootTreeWithValueNode =
            RBTreeNode::Set(thread, rootTreeNode, keyHash, treeKey, treeValue);
        uint32_t hashArrayIndex = static_cast<uint32_t>(nums - 1) & keyHash;
        taggedHashArray->Set(thread, hashArrayIndex, rootTreeWithValueNode.GetTaggedValue());
    }
}

void HashCommon2(JSThread *thread, JSHandle<TaggedHashArray>& taggedHashArray, std::string& myKey,
    std::string& myValue, uint32_t nums)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    for (uint32_t i = 0; i < nums; i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        JSHandle<JSTaggedValue> listKey(thread, factory->NewFromStdString(iKey).GetTaggedValue());
        JSHandle<JSTaggedValue> listValue(thread, factory->NewFromStdString(iValue).GetTaggedValue());
        auto keyHash = TaggedNode::Hash(thread, listKey.GetTaggedValue());
        TaggedHashArray::SetVal(thread, taggedHashArray, keyHash, listKey, listValue);
    }
}

/**
 * @tc.name: SetValAndGetLinkNode
 * @tc.desc: Call "Create" function Create TaggedHashArray object and "SetVal" function to add a key value pair to
 *           the taggedharray object,The value set is the LinkedNode object and check whether there is a null value in
 *           the object.If there is a null value,it means that the addition fails. Call the "SetVal" function to add
 *           and change the key value pair, and then call the "GetNode" function to get one of the key value pair,check
 *           whether the value meets the expectation.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TaggedHashArrayTest, SetValAndGetLinkNode)
{
    int numOfElement = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedHashArray> taggedHashArray(thread, TaggedHashArray::Create(thread, numOfElement));
    JSHandle<JSTaggedValue> myKey4(factory->NewFromStdString("mykey4"));
    JSHandle<JSTaggedValue> myKey4Value(factory->NewFromStdString("myvalue40"));
    JSHandle<JSTaggedValue> myKey8(factory->NewFromStdString("mykey8"));
    JSHandle<JSTaggedValue> myKey8Value(factory->NewFromStdString("myvalue8"));
    std::string myKey("mykey");
    std::string myValue("myvalue");
    // set key and value
    HashCommon2(thread, taggedHashArray, myKey, myValue, static_cast<uint32_t>(numOfElement));
    auto keyHash = TaggedNode::Hash(thread, myKey4.GetTaggedValue());
    // change value and add new key
    TaggedHashArray::SetVal(thread, taggedHashArray, keyHash, myKey4, myKey4Value);
    TaggedHashArray::SetVal(thread, taggedHashArray, keyHash, myKey8, myKey8Value);
    // test GetNode()
    JSTaggedValue hashNodeVal = taggedHashArray->GetNode(thread, keyHash, myKey4.GetTaggedValue());
    EXPECT_TRUE(hashNodeVal.IsLinkedNode());
    JSHandle<LinkedNode> hashNode(thread, hashNodeVal);
    EXPECT_EQ(hashNode->GetValue(), myKey4Value.GetTaggedValue());
    EXPECT_TRUE(hashNode->GetNext().IsLinkedNode());
}

/**
 * @tc.name: SetValAndGetTreeNode
 * @tc.desc: Call "Create" function Create TaggedHashArray object and "SetVal" function to add a key value pair to
 *           the taggedharray object,The value set is the RBTreeNode object and check whether there is a null value in
 *           the object.If there is a null value,it means that the addition fails. Call the "SetVal" function to add
 *           and change the key value pair, and then call the "GetNode" function to get one of the key value pair,check
 *           whether the value meets the expectation.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TaggedHashArrayTest, SetValAndGetTreeNode)
{
    int numOfElement = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedHashArray> taggedHashArray(thread, TaggedHashArray::Create(thread, numOfElement));
    JSHandle<JSTaggedValue> myKey5(factory->NewFromStdString("mykey5"));
    JSHandle<JSTaggedValue> myKey5Value(factory->NewFromStdString("myvalue50"));
    JSHandle<JSTaggedValue> myKey8(factory->NewFromStdString("mykey8"));
    JSHandle<JSTaggedValue> myKey8Value(factory->NewFromStdString("myvalue8"));
    std::string myKey("mykey");
    std::string myValue("myvalue");
    // set key and value
    HashCommon1(thread, taggedHashArray, myKey, myValue, static_cast<uint32_t>(numOfElement));
    auto keyHash = TaggedNode::Hash(thread, myKey5.GetTaggedValue());
    // change value and add new key
    TaggedHashArray::SetVal(thread, taggedHashArray, keyHash, myKey5, myKey5Value);
    TaggedHashArray::SetVal(thread, taggedHashArray, keyHash, myKey8, myKey8Value);
    // test GetNode()
    JSTaggedValue hashNodeVal = taggedHashArray->GetNode(thread, keyHash, myKey5.GetTaggedValue());
    EXPECT_TRUE(hashNodeVal.IsRBTreeNode());
    JSHandle<RBTreeNode> hashNode(thread, hashNodeVal);
    EXPECT_EQ(hashNode->GetValue(), myKey5Value.GetTaggedValue());
    EXPECT_TRUE(hashNode->GetLeft().IsHole());
    EXPECT_TRUE(hashNode->GetRight().IsHole());
}

/**
 * @tc.name: RemoveLinkNode
 * @tc.desc: Call "Create" function Create TaggedHashArray object and "SetVal" function to add a key value pair to
 *           the taggedharray object,The value set is the LinkedNode object,call "RemoveNode" function to delete a
 *           node, and check whether the return value of the "RemoveNode" function is the value of the deleted node.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TaggedHashArrayTest, RemoveLinkNode)
{
    int numOfElement = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedHashArray> taggedHashArray(thread, TaggedHashArray::Create(thread, numOfElement));
    JSHandle<JSTaggedValue> myKey4(factory->NewFromStdString("mykey4"));
    JSHandle<JSTaggedValue> myKey5(factory->NewFromStdString("mykey5"));
    JSHandle<JSTaggedValue> myKey4Value(factory->NewFromStdString("myvalue4"));
    JSHandle<JSTaggedValue> myKey8(factory->NewFromStdString("mykey8"));
    JSHandle<JSTaggedValue> myKey8Value(factory->NewFromStdString("myvalue8"));
    std::string myKey("mykey");
    std::string myValue("myvalue");
    // set key and value
    HashCommon2(thread, taggedHashArray, myKey, myValue, static_cast<uint32_t>(numOfElement));
    auto keyHash = TaggedNode::Hash(thread, myKey5.GetTaggedValue());
    TaggedHashArray::SetVal(thread, taggedHashArray, keyHash, myKey8, myKey8Value);

    // test Remove()
    keyHash = TaggedNode::Hash(thread, myKey4.GetTaggedValue());
    JSTaggedValue currentNodeVal =taggedHashArray->RemoveNode(thread, keyHash, myKey4.GetTaggedValue());
    EXPECT_EQ(currentNodeVal, myKey4Value.GetTaggedValue());

    // test Remove() with linkNode has next value
    keyHash = TaggedNode::Hash(thread, myKey5.GetTaggedValue());
    currentNodeVal = taggedHashArray->RemoveNode(thread, keyHash, myKey8.GetTaggedValue());
    EXPECT_EQ(currentNodeVal, myKey8Value.GetTaggedValue());
}

/**
 * @tc.name: RemoveTreeNode
 * @tc.desc: Call "Create" function Create TaggedHashArray object and "SetVal" function to add a key value pair to
 *           the taggedharray object,The value set is the RBTreeNode object,call "RemoveNode" function to delete a
 *           node, check whether the count attribute of RBTreeNode at the deleted node is expected.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TaggedHashArrayTest, RemoveTreeNode)
{
    int numOfElement = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedHashArray> taggedHashArray(thread, TaggedHashArray::Create(thread, numOfElement));
    JSHandle<JSTaggedValue> myKey5(factory->NewFromStdString("mykey5"));
    JSHandle<JSTaggedValue> myKey8(factory->NewFromStdString("mykey8"));
    JSHandle<JSTaggedValue> myKey8Value(factory->NewFromStdString("myvalue8"));
    std::string myKey("mykey");
    std::string myValue("myvalue");
    // set key and value
    HashCommon1(thread, taggedHashArray, myKey, myValue, static_cast<uint32_t>(numOfElement));
    auto keyHash = TaggedNode::Hash(thread, myKey5.GetTaggedValue());
    TaggedHashArray::SetVal(thread, taggedHashArray, keyHash, myKey8, myKey8Value);
    uint32_t keyHashIndex = static_cast<uint32_t>(numOfElement - 1) & keyHash;
    JSHandle<RBTreeNode> hashTreeNode(thread, taggedHashArray->Get(keyHashIndex));
    EXPECT_EQ(hashTreeNode->GetCount(), 2U);
    // test Remove()
    taggedHashArray->RemoveNode(thread, keyHash, myKey5.GetTaggedValue());
    EXPECT_EQ(hashTreeNode->GetCount(), 1U);
}

bool HasNode(JSTaggedValue hashTreeNodeVa, JSTaggedValue key)
{
    if (hashTreeNodeVa.IsHole()) {
        return false;
    }

    RBTreeNode *hashTreeNode = RBTreeNode::Cast(hashTreeNodeVa.GetTaggedObject());
    JSTaggedValue rootKey = hashTreeNode->GetKey();
    if (JSTaggedValue::SameValue(rootKey, key)) {
        return true;
    }
    return HasNode(hashTreeNode->GetLeft(), key) || HasNode(hashTreeNode->GetRight(), key);
}

int CheckRBTreeNodeNums(JSTaggedValue hashTreeNodeVa, int count)
{
    if (hashTreeNodeVa.IsHole()) {
        return count;
    }

    count++;
    int temp = count;
    RBTreeNode *hashTreeNode = RBTreeNode::Cast(hashTreeNodeVa.GetTaggedObject());
    return CheckRBTreeNodeNums(hashTreeNode->GetLeft(), count) +
           CheckRBTreeNodeNums(hashTreeNode->GetRight(), count) - temp;
}

template <typename T>
bool CheckRBTreeOfAllPaths(JSHandle<T> &tree, int numsOfBlack, JSTaggedValue hashTreeNodeVa, int count)
{
    if (hashTreeNodeVa.IsHole()) {
        return count == numsOfBlack;
    }
    if (!RBTreeNode::IsRed(hashTreeNodeVa)) {
        count++;
    }
    RBTreeNode *hashTreeNode = RBTreeNode::Cast(hashTreeNodeVa.GetTaggedObject());
    if (CheckRBTreeOfAllPaths(tree, numsOfBlack, hashTreeNode->GetLeft(), count) &&
        CheckRBTreeOfAllPaths(tree, numsOfBlack, hashTreeNode->GetRight(), count)) {
        return true;
    }
    return false;
}

bool CheckRedNode(JSTaggedValue hashTreeNodeva)
{
    if (hashTreeNodeva.IsHole()) {
        return true;
    }
    RBTreeNode *hashTreeNode = RBTreeNode::Cast(hashTreeNodeva.GetTaggedObject());
    JSTaggedValue leftChildVa = hashTreeNode->GetLeft();
    JSTaggedValue rightChildVa = hashTreeNode->GetRight();
    if (hashTreeNode->GetIsRed().ToBoolean()) {
        if (!leftChildVa.IsHole() && RBTreeNode::IsRed(leftChildVa)) {
            return false;
        }
        if (!rightChildVa.IsHole() && RBTreeNode::IsRed(rightChildVa)) {
            return false;
        }
    }
    return CheckRedNode(leftChildVa) && CheckRedNode(rightChildVa);
}

template <typename T>
bool CheckBlackNodeNumbers(JSHandle<T> &tree, JSHandle<RBTreeNode> hashTreeNode)
{
    int numsOfBlack = 0;
    JSTaggedValue ChildVa = hashTreeNode.GetTaggedValue();
    while (!ChildVa.IsHole()) {
        if (!RBTreeNode::IsRed(ChildVa)) {
            numsOfBlack++;
        }
        ChildVa = RBTreeNode::Cast(ChildVa.GetTaggedObject())->GetLeft();
        if (ChildVa.IsHole()) {
            break;
        }
    }

    return CheckRBTreeOfAllPaths(tree, numsOfBlack, hashTreeNode.GetTaggedValue(), 0);
}

template <typename T>
bool IsVaildRBTree(JSThread *thread, JSHandle<T> &tree, JSHandle<RBTreeNode> hashTreeNode)
{
    if (hashTreeNode.GetTaggedValue().IsHole()) {
        return true;
    }

    if (hashTreeNode->GetIsRed().ToBoolean()) {
        return false;
    }

    JSTaggedValue leftChildVa = hashTreeNode->GetLeft();
    JSTaggedValue rightChildVa = hashTreeNode->GetRight();
    JSTaggedValue treeNodeKey = hashTreeNode->GetKey();
    int hash = hashTreeNode->GetHash().GetInt();
    if (!leftChildVa.IsHole()) {
        RBTreeNode *leftTreeNode = RBTreeNode::Cast(leftChildVa.GetTaggedObject());
        int cmp = RBTreeNode::Compare(leftTreeNode->GetHash().GetInt(), leftTreeNode->GetKey(), hash, treeNodeKey);
        if (cmp > 0) {
            return false;
        }
    }
    if (!rightChildVa.IsHole()) {
        RBTreeNode *rightTreeNode = RBTreeNode::Cast(rightChildVa.GetTaggedObject());
        int cmp = RBTreeNode::Compare(rightTreeNode->GetHash().GetInt(), rightTreeNode->GetKey(), hash, treeNodeKey);
        if (cmp < 0) {
            return false;
        }
    }

    // check red node
    if (!CheckRedNode(leftChildVa) || !CheckRedNode(rightChildVa)) {
        return false;
    }
    // check black node
    if (!CheckBlackNodeNumbers(tree, hashTreeNode)) {
        return false;
    }

    return true;
}

/**
 * @tc.name: RemoveRBTreeNode
 * @tc.desc: Call "Create" function Create TaggedHashArray object and "SetVal" function to add a key value pair to
 *           the taggedharray object,The value set is the RBTreeNode object,call "RemoveNode" function to delete a
 *           node, check if it can be hole then return.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TaggedHashArrayTest, RemoveRBTreeNode)
{
    int numOfElement = 1;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedHashArray> taggedHashArray(thread, TaggedHashArray::Create(thread, numOfElement));
    std::string myKey("mykey");
    std::string myValue("myvalue");
    HashCommon1(thread, taggedHashArray, myKey, myValue, static_cast<uint32_t>(numOfElement));
    for (int i = numOfElement; i < numOfElement + 100; i++) {
        JSHandle<JSTaggedValue> myKey1(factory->NewFromStdString(myKey + std::to_string(i)));
        JSHandle<JSTaggedValue> myKey1Value(factory->NewFromStdString(myValue + std::to_string(i)));
        auto keyHash = TaggedNode::Hash(thread, myKey1.GetTaggedValue());
        TaggedHashArray::SetVal(thread, taggedHashArray, keyHash, myKey1, myKey1Value);
        uint32_t keyHashIndex = static_cast<uint32_t>(numOfElement - 1) & keyHash;
        JSHandle<RBTreeNode> hashTreeNode(thread, taggedHashArray->Get(keyHashIndex));
        EXPECT_TRUE(IsVaildRBTree(thread, taggedHashArray, hashTreeNode));
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(2, 30);
    for (int i = numOfElement; i < numOfElement + 20; i++) {
        int index = dis(gen);
        JSHandle<JSTaggedValue> myKey1(factory->NewFromStdString(myKey + std::to_string(index)));
        auto keyHash = TaggedNode::Hash(thread, myKey1.GetTaggedValue());
        uint32_t keyHashIndex = static_cast<uint32_t>(numOfElement - 1) & keyHash;
        JSHandle<RBTreeNode> hashTreeNode(thread, taggedHashArray->Get(keyHashIndex));
        EXPECT_EQ(taggedHashArray->GetLength(), numOfElement);
        int beforeNum = CheckRBTreeNodeNums(taggedHashArray->Get(keyHashIndex), 0);
        taggedHashArray->RemoveNode(thread, keyHash, myKey1.GetTaggedValue());
        if (HasNode(taggedHashArray->Get(keyHashIndex), myKey1.GetTaggedValue())) {
            EXPECT_EQ(beforeNum - 1, CheckRBTreeNodeNums(taggedHashArray->Get(keyHashIndex), 0));
        }
        EXPECT_TRUE(!HasNode(taggedHashArray->Get(keyHashIndex), myKey1.GetTaggedValue()));
        JSHandle<RBTreeNode> hashTreeNode1(thread, taggedHashArray->Get(keyHashIndex));
        EXPECT_TRUE(IsVaildRBTree(thread, taggedHashArray, hashTreeNode1));
        JSHandle<JSTaggedValue> myKey2(factory->NewFromStdString(myKey + std::to_string(index + 100)));
        JSHandle<JSTaggedValue> myKey2Value(factory->NewFromStdString(myValue + std::to_string(index + 100)));
        auto keyHash1 = TaggedNode::Hash(thread, myKey2.GetTaggedValue());
        uint32_t keyHashIndex1 = static_cast<uint32_t>(numOfElement - 1) & keyHash1;
        beforeNum = CheckRBTreeNodeNums(taggedHashArray->Get(keyHashIndex1), 0);
        TaggedHashArray::SetVal(thread, taggedHashArray, keyHash1, myKey2, myKey2Value);
        if (!HasNode(taggedHashArray->Get(keyHashIndex1), myKey2.GetTaggedValue())) {
            EXPECT_EQ(beforeNum + 1, CheckRBTreeNodeNums(taggedHashArray->Get(keyHashIndex1), 0));
        }
        EXPECT_TRUE(HasNode(taggedHashArray->Get(keyHashIndex1), myKey2.GetTaggedValue()));
        JSHandle<RBTreeNode> hashTreeNode2(thread, taggedHashArray->Get(keyHashIndex1));
        EXPECT_TRUE(IsVaildRBTree(thread, taggedHashArray, hashTreeNode2));
    }
}

/**
 * @tc.name: ResetLinkNodeSize
 * @tc.desc: Call "Create" function Create TaggedHashArray object and "SetVal" function to add a key value pair to
 *           the taggedharray object,The value set is the LinkedNode object,call "RemoveNode" function to delete a
 *           node,call the "Resize" function to reset the taggedharray object and return. check whether the length
 *           of the object after resetting is twice as long as that before resetting and the deleted node before
 *           resetting still exists.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TaggedHashArrayTest, ResetLinkNodeSize)
{
    int numOfElement = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedHashArray> taggedHashArray(thread, TaggedHashArray::Create(thread, numOfElement));
    JSHandle<JSTaggedValue> myKey4(factory->NewFromStdString("mykey4"));
    JSHandle<JSTaggedValue> myKey5(factory->NewFromStdString("mykey5"));
    JSHandle<JSTaggedValue> myKey8(factory->NewFromStdString("mykey8"));
    JSHandle<JSTaggedValue> myKey8Value(factory->NewFromStdString("myvalue8"));
    JSHandle<JSTaggedValue> myKey9(factory->NewFromStdString("mykey9"));
    JSHandle<JSTaggedValue> myKey9Value(factory->NewFromStdString("myvalue9"));
    std::string myKey("mykey");
    std::string myValue("myvalue");
    // set key and value
    HashCommon2(thread, taggedHashArray, myKey, myValue, static_cast<uint32_t>(numOfElement));
    auto keyHash = TaggedNode::Hash(thread, myKey5.GetTaggedValue());
    TaggedHashArray::SetVal(thread, taggedHashArray, keyHash, myKey8, myKey8Value);
    // remove node
    taggedHashArray->RemoveNode(thread, keyHash, myKey5.GetTaggedValue());
    keyHash = TaggedNode::Hash(thread, myKey4.GetTaggedValue());
    taggedHashArray->RemoveNode(thread, keyHash, myKey4.GetTaggedValue());

    // test Resize()
    taggedHashArray = TaggedHashArray::Resize(thread, taggedHashArray, numOfElement);
    EXPECT_EQ(taggedHashArray->GetLength(), 16U);  // 16: reseted length

    keyHash = TaggedNode::Hash(thread, myKey4.GetTaggedValue());
    uint32_t hashArrayIndex = static_cast<uint32_t>(numOfElement - 1) & keyHash;
    EXPECT_TRUE(taggedHashArray->Get(hashArrayIndex).IsHole());

    keyHash = TaggedNode::Hash(thread, myKey5.GetTaggedValue());
    hashArrayIndex = static_cast<uint32_t>(numOfElement - 1) & keyHash;
    JSHandle<LinkedNode> hashNode(thread, taggedHashArray->Get(hashArrayIndex));
    EXPECT_EQ(hashNode->GetValue(), myKey8Value.GetTaggedValue());
}

/**
 * @tc.name: ResetLinkNodeSize
 * @tc.desc: Call "Create" function Create TaggedHashArray object and "SetVal" function to add a key value pair to
 *           the taggedharray object,The value set is the RBTreeNode object. call the "GetCurrentNode" function to
 *           obtain the current node and store it in a queue,then check whether the value is in the current node.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TaggedHashArrayTest, GetCurrentNode)
{
    int numOfElement = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedHashArray> taggedHashArray(thread, TaggedHashArray::Create(thread, numOfElement));
    JSMutableHandle<TaggedQueue> taggedQueue(thread, factory->NewTaggedQueue(0));
    JSHandle<RBTreeNode> rootTreeNode(thread, JSTaggedValue::Hole());
    JSHandle<JSTaggedValue> myKey5(factory->NewFromStdString("mykey5"));
    JSHandle<JSTaggedValue> myKey5Value(factory->NewFromStdString("myvalue5"));
    JSHandle<JSTaggedValue> myKey8(factory->NewFromStdString("mykey8"));
    JSHandle<JSTaggedValue> myKey8Value(factory->NewFromStdString("myvalue8"));
    std::string myKey("mykey");
    std::string myValue("myvalue");

    HashCommon1(thread, taggedHashArray, myKey, myValue, static_cast<uint32_t>(numOfElement));
    auto keyHash = TaggedNode::Hash(thread, myKey5.GetTaggedValue());
    TaggedHashArray::SetVal(thread, taggedHashArray, keyHash, myKey8, myKey8Value);
    // test GetCurrentNode()
    uint32_t nodeIndex = static_cast<uint32_t>(numOfElement - 1) & keyHash;
    JSHandle<JSTaggedValue> currentNode =
        TaggedHashArray::GetCurrentNode(thread, taggedQueue, taggedHashArray, nodeIndex);
    EXPECT_TRUE(currentNode->IsRBTreeNode());
    // Pop queue
    JSHandle<RBTreeNode> storeStartTreeNode(thread, taggedQueue->Pop(thread));
    EXPECT_EQ(storeStartTreeNode->GetValue(), myKey5Value.GetTaggedValue());
    EXPECT_TRUE(taggedQueue->Pop(thread).IsHole());
}
}  // namespace panda::test