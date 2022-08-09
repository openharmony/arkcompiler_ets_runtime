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
class TaggedHashArrayTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

static bool CheckHole(JSHandle<TaggedHashArray> &hashArray)
{
    uint32_t arrayLength = hashArray->GetLength();
    for (uint32_t i = 0; i < arrayLength; i++) {
        JSTaggedValue indexValue = hashArray->Get(i);
        if (indexValue.IsHole()) {
            return false;
        }
    }
    return true;
}

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
    int keyHash = TaggedNode::Hash(hashKey.GetTaggedValue());
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
    int keyHash = TaggedNode::Hash(hashKey.GetTaggedValue());
    JSHandle<RBTreeNode> treeNode =
        TaggedHashArray::NewTreeNode(thread, keyHash, hashKey, hashKeyValue);
    EXPECT_TRUE(*treeNode != nullptr);
    EXPECT_TRUE(treeNode->GetIsRed().ToBoolean());
    EXPECT_TRUE(treeNode->GetLeft().IsHole());
    EXPECT_TRUE(treeNode->GetRight().IsHole());
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
    int keyHash = 0;
    // set key and value
    for (uint32_t i = 0; i < static_cast<uint32_t>(numOfElement); i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        JSHandle<JSTaggedValue> listKey(thread, factory->NewFromStdString(iKey).GetTaggedValue());
        JSHandle<JSTaggedValue> listValue(thread, factory->NewFromStdString(iValue).GetTaggedValue());
        keyHash = TaggedNode::Hash(listKey.GetTaggedValue());
        TaggedHashArray::SetVal(thread, taggedHashArray, keyHash, listKey, listValue);
    }
    bool result = CheckHole(taggedHashArray);
    EXPECT_TRUE(result);
    keyHash = TaggedNode::Hash(myKey4.GetTaggedValue());
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
    JSHandle<RBTreeNode> rootTreeNode(thread, JSTaggedValue::Hole());
    JSHandle<JSTaggedValue> myKey5(factory->NewFromStdString("mykey5"));
    JSHandle<JSTaggedValue> myKey5Value(factory->NewFromStdString("myvalue50"));
    JSHandle<JSTaggedValue> myKey8(factory->NewFromStdString("mykey8"));
    JSHandle<JSTaggedValue> myKey8Value(factory->NewFromStdString("myvalue8"));
    std::string myKey("mykey");
    std::string myValue("myvalue");
    int keyHash = 0;
    // set key and value
    for (uint32_t i = 0; i < static_cast<uint32_t>(numOfElement); i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        JSHandle<JSTaggedValue> treeKey(thread, factory->NewFromStdString(iKey).GetTaggedValue());
        JSHandle<JSTaggedValue> treeValue(thread, factory->NewFromStdString(iValue).GetTaggedValue());
        keyHash = TaggedNode::Hash(treeKey.GetTaggedValue());
        JSHandle<RBTreeNode> rootTreeWithValueNode =
            RBTreeNode::Set(thread, rootTreeNode, keyHash, treeKey, treeValue);
        uint32_t hashArrayIndex = static_cast<uint32_t>(numOfElement - 1) & keyHash;
        taggedHashArray->Set(thread, hashArrayIndex, rootTreeWithValueNode.GetTaggedValue());
    }
    bool result = CheckHole(taggedHashArray);
    EXPECT_TRUE(result);
    keyHash = TaggedNode::Hash(myKey5.GetTaggedValue());
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
    int keyHash = 0;
    // set key and value
    for (uint32_t i = 0; i < static_cast<uint32_t>(numOfElement); i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        JSHandle<JSTaggedValue> listKey(thread, factory->NewFromStdString(iKey).GetTaggedValue());
        JSHandle<JSTaggedValue> listValue(thread, factory->NewFromStdString(iValue).GetTaggedValue());
        keyHash = TaggedNode::Hash(listKey.GetTaggedValue());
        TaggedHashArray::SetVal(thread, taggedHashArray, keyHash, listKey, listValue);
    }
    keyHash = TaggedNode::Hash(myKey5.GetTaggedValue());
    TaggedHashArray::SetVal(thread, taggedHashArray, keyHash, myKey8, myKey8Value);

    // test Remove()
    keyHash = TaggedNode::Hash(myKey4.GetTaggedValue());
    JSTaggedValue currentNodeVal =taggedHashArray->RemoveNode(thread, keyHash, myKey4.GetTaggedValue());
    EXPECT_EQ(currentNodeVal, myKey4Value.GetTaggedValue());

    // test Remove() with linkNode has next value
    keyHash = TaggedNode::Hash(myKey5.GetTaggedValue());
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
    JSHandle<RBTreeNode> rootTreeNode(thread, JSTaggedValue::Hole());
    JSHandle<JSTaggedValue> myKey5(factory->NewFromStdString("mykey5"));
    JSHandle<JSTaggedValue> myKey8(factory->NewFromStdString("mykey8"));
    JSHandle<JSTaggedValue> myKey8Value(factory->NewFromStdString("myvalue8"));
    std::string myKey("mykey");
    std::string myValue("myvalue");
    int keyHash = 0;
    // set key and value
    for (uint32_t i = 0; i < static_cast<uint32_t>(numOfElement); i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        JSHandle<JSTaggedValue> treeKey(thread, factory->NewFromStdString(iKey).GetTaggedValue());
        JSHandle<JSTaggedValue> treeValue(thread, factory->NewFromStdString(iValue).GetTaggedValue());
        keyHash = TaggedNode::Hash(treeKey.GetTaggedValue());
        JSHandle<RBTreeNode> rootTreeWithValueNode =
            RBTreeNode::Set(thread, rootTreeNode, keyHash, treeKey, treeValue);
        uint32_t hashArrayIndex = static_cast<uint32_t>(numOfElement - 1) & keyHash;
        taggedHashArray->Set(thread, hashArrayIndex, rootTreeWithValueNode.GetTaggedValue());
    }
    keyHash = TaggedNode::Hash(myKey5.GetTaggedValue());
    TaggedHashArray::SetVal(thread, taggedHashArray, keyHash, myKey8, myKey8Value);
    uint32_t keyHashIndex = static_cast<uint32_t>(numOfElement - 1) & keyHash;
    JSHandle<RBTreeNode> hashTreeNode(thread, taggedHashArray->Get(keyHashIndex));
    EXPECT_EQ(hashTreeNode->GetCount(), 2U);
    // test Remove()
    taggedHashArray->RemoveNode(thread, keyHash, myKey5.GetTaggedValue());
    EXPECT_EQ(hashTreeNode->GetCount(), 1U);
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
    int keyHash = 0;
    // set key and value
    for (uint32_t i = 0; i < static_cast<uint32_t>(numOfElement); i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        JSHandle<JSTaggedValue> listKey(thread, factory->NewFromStdString(iKey).GetTaggedValue());
        JSHandle<JSTaggedValue> listValue(thread, factory->NewFromStdString(iValue).GetTaggedValue());
        keyHash = TaggedNode::Hash(listKey.GetTaggedValue());
        TaggedHashArray::SetVal(thread, taggedHashArray, keyHash, listKey, listValue);
    }
    keyHash = TaggedNode::Hash(myKey5.GetTaggedValue());
    TaggedHashArray::SetVal(thread, taggedHashArray, keyHash, myKey8, myKey8Value);
    // remove node
    taggedHashArray->RemoveNode(thread, keyHash, myKey5.GetTaggedValue());
    keyHash = TaggedNode::Hash(myKey4.GetTaggedValue());
    taggedHashArray->RemoveNode(thread, keyHash, myKey4.GetTaggedValue());

    // test Resize()
    taggedHashArray = TaggedHashArray::Resize(thread, taggedHashArray, numOfElement);
    EXPECT_EQ(taggedHashArray->GetLength(), 16U);  // 16: reseted length

    keyHash = TaggedNode::Hash(myKey4.GetTaggedValue());
    uint32_t hashArrayIndex = static_cast<uint32_t>(numOfElement - 1) & keyHash;
    EXPECT_TRUE(taggedHashArray->Get(hashArrayIndex).IsHole());

    keyHash = TaggedNode::Hash(myKey5.GetTaggedValue());
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
    int keyHash = 0;
    // set key and value
    for (uint32_t i = 0; i < static_cast<uint32_t>(numOfElement); i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        JSHandle<JSTaggedValue> treeKey(thread, factory->NewFromStdString(iKey).GetTaggedValue());
        JSHandle<JSTaggedValue> treeValue(thread, factory->NewFromStdString(iValue).GetTaggedValue());
        keyHash = TaggedNode::Hash(treeKey.GetTaggedValue());
        JSHandle<RBTreeNode> rootTreeWithValueNode =
            RBTreeNode::Set(thread, rootTreeNode, keyHash, treeKey, treeValue);
        uint32_t hashArrayIndex = static_cast<uint32_t>(numOfElement - 1) & keyHash;
        taggedHashArray->Set(thread, hashArrayIndex, rootTreeWithValueNode.GetTaggedValue());
    }
    keyHash = TaggedNode::Hash(myKey5.GetTaggedValue());
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