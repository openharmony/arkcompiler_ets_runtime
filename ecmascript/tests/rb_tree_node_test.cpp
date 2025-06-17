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

#include <string>
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_hash_array.h"
#include "ecmascript/tagged_node.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
namespace panda::test {
class RBTreeNodeTest : public BaseTestWithScope<false> {
public:
    JSHandle<GlobalEnv> GetGlobalEnv()
    {
        EcmaVM *ecma = thread->GetEcmaVM();
        return ecma->GetGlobalEnv();
    }
    uint32_t NODE_NUMBERS = 8;
    uint32_t TREE_NODE_NUMBERS = 32;

    void RBTreeNodeInit(JSHandle<RBTreeNode>& rootNode, std::string& myKey, std::string& myValue,
    uint32_t nums)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        for (uint32_t i = 0; i < nums; i++) {
            std::string iKey = myKey + std::to_string(i);
            std::string iValue = myValue + std::to_string(i);
            JSHandle<JSTaggedValue> key(thread, factory->NewFromStdString(iKey).GetTaggedValue());
            JSHandle<JSTaggedValue> value(thread, factory->NewFromStdString(iValue).GetTaggedValue());
            int hash = TaggedNode::Hash(thread, factory->NewFromStdString(iKey).GetTaggedValue());
            rootNode = RBTreeNode::Set(thread, rootNode, hash, key, value);
            rootNode->SetIsRed(false);
        }
    }

    void RBTreeValueCheck(JSHandle<RBTreeNode>& rootNode, std::string& myKey, std::string& myValue,
        uint32_t startNum, uint32_t nums)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        for (uint32_t i = startNum; i < nums; i++) {
            std::string iKey = myKey + std::to_string(i);
            std::string iValue = myValue + std::to_string(i);
            JSHandle<JSTaggedValue> key(thread, factory->NewFromStdString(iKey).GetTaggedValue());
            JSHandle<JSTaggedValue> value(thread, factory->NewFromStdString(iValue).GetTaggedValue());
            int hash = TaggedNode::Hash(thread, key.GetTaggedValue());
            JSHandle<JSTaggedValue> rootNodeVa(thread, rootNode.GetTaggedValue());
            JSTaggedValue gValue = RBTreeNode::GetTreeNode(thread, rootNodeVa, hash, key);
            JSTaggedValue resValue = RBTreeNode::Cast(gValue.GetTaggedObject())->GetValue(thread);
            EXPECT_EQ(resValue, value.GetTaggedValue());
        }
    }
};

HWTEST_F_L0(RBTreeNodeTest, RBTreeNodeCreate)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::string k("testKey");
    std::string v("testValue");
    JSHandle<JSTaggedValue> key(thread, factory->NewFromStdString(k).GetTaggedValue());
    JSHandle<JSTaggedValue> value(thread, factory->NewFromStdString(v).GetTaggedValue());
    int hash = TaggedNode::Hash(thread, factory->NewFromStdString(k).GetTaggedValue());
    JSHandle<RBTreeNode> newNode = factory->NewTreeNode(hash, key, value);

    EXPECT_TRUE(!newNode.GetTaggedValue().IsHole());
    EXPECT_TRUE(newNode.GetTaggedValue().IsRBTreeNode());
}

HWTEST_F_L0(RBTreeNodeTest, RBTreeNodeSetAndGet)
{
    // test RBTreeNode
    JSHandle<RBTreeNode> rootNode(thread, JSTaggedValue::Hole());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    RBTreeNodeInit(rootNode, myKey, myValue, NODE_NUMBERS);
    EXPECT_EQ(rootNode->GetCount(), NODE_NUMBERS);

    RBTreeValueCheck(rootNode, myKey, myValue, 0, NODE_NUMBERS);
}

HWTEST_F_L0(RBTreeNodeTest, RBTreeNodeDelete)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // test RBTreeNode
    JSHandle<RBTreeNode> rootNode(thread, JSTaggedValue::Hole());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    RBTreeNodeInit(rootNode, myKey, myValue, NODE_NUMBERS);
    EXPECT_EQ(rootNode->GetCount(), NODE_NUMBERS);

    for (uint32_t i = 0; i < NODE_NUMBERS / 2; i++) {
        std::string iKey = myKey + std::to_string(i);
        JSHandle<JSTaggedValue> key(thread, factory->NewFromStdString(iKey).GetTaggedValue());
        int hash = TaggedNode::Hash(thread, key.GetTaggedValue());
        JSTaggedValue holeValue = JSTaggedValue::Hole();
        JSTaggedValue dValue =
            RBTreeNode::Delete(thread, rootNode.GetTaggedValue(), hash, key.GetTaggedValue(), holeValue);
        rootNode = JSHandle<RBTreeNode>(thread, dValue);
    }
    EXPECT_EQ(rootNode->GetCount(), (NODE_NUMBERS / 2));

    for (uint32_t i = 0; i < NODE_NUMBERS / 2; i++) {
        std::string iKey = myKey + std::to_string(i);
        JSHandle<JSTaggedValue> key(thread, factory->NewFromStdString(iKey).GetTaggedValue());
        int hash = TaggedNode::Hash(thread, key.GetTaggedValue());
        JSHandle<JSTaggedValue> rootNodeVa(thread, rootNode.GetTaggedValue());
        JSTaggedValue gValue = RBTreeNode::GetTreeNode(thread, rootNodeVa, hash, key);
        EXPECT_EQ(gValue, JSTaggedValue::Hole());
    }

    RBTreeValueCheck(rootNode, myKey, myValue, NODE_NUMBERS / 2, NODE_NUMBERS);
}

HWTEST_F_L0(RBTreeNodeTest, RBTreeNodeDivide)
{
    JSHandle<RBTreeNode> rootNode(thread, JSTaggedValue::Hole());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    RBTreeNodeInit(rootNode, myKey, myValue, TREE_NODE_NUMBERS);
    EXPECT_EQ(rootNode->GetCount(), TREE_NODE_NUMBERS);

    uint32_t count = rootNode->GetCount();
    JSHandle<TaggedHashArray> newTab =
        JSHandle<TaggedHashArray>(thread,
                                  TaggedHashArray::Create(thread, TaggedHashArray::DEFAULT_INITIAL_CAPACITY * 2));
    JSHandle<JSTaggedValue> rootNodeVa = JSHandle<JSTaggedValue>::Cast(rootNode);
    RBTreeNode::Divide(thread, newTab, rootNodeVa, 0, TaggedHashArray::DEFAULT_INITIAL_CAPACITY);
    JSTaggedValue loNode = newTab->Get(thread, 0);
    uint32_t loCount = 0;
    uint32_t hiCount = 0;
    if (loNode.IsLinkedNode()) {
        for (JSHandle<LinkedNode> node = JSHandle<LinkedNode>(thread, loNode);
            !node.GetTaggedValue().IsHole();
            node = JSHandle<LinkedNode>(thread, node->GetNext(thread))) {
            loCount++;
        }
    } else {
        JSHandle<RBTreeNode> node = JSHandle<RBTreeNode>(thread, loNode);
        loCount = node->GetCount();
    }
    JSTaggedValue hiNode = newTab->Get(thread, TaggedHashArray::DEFAULT_INITIAL_CAPACITY);
    if (hiNode.IsLinkedNode()) {
        for (JSHandle<LinkedNode> node = JSHandle<LinkedNode>(thread, hiNode);
            !node.GetTaggedValue().IsHole();
            node = JSHandle<LinkedNode>(thread, node->GetNext(thread))) {
            hiCount++;
        }
    } else {
        JSHandle<RBTreeNode> node = JSHandle<RBTreeNode>(thread, hiNode);
        hiCount = node->GetCount();
    }

    EXPECT_TRUE(count == loCount + hiCount);
}

HWTEST_F_L0(RBTreeNodeTest, RBTreeNodeUntreeify)
{
    JSHandle<RBTreeNode> rootNode(thread, JSTaggedValue::Hole());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    RBTreeNodeInit(rootNode, myKey, myValue, NODE_NUMBERS);
    EXPECT_EQ(rootNode->GetCount(), NODE_NUMBERS);

    JSHandle<LinkedNode> head = RBTreeNode::Detreeing(thread, rootNode);

    uint32_t count = 0;
    for (; !head.GetTaggedValue().IsHole(); head = JSHandle<LinkedNode>(thread, head->GetNext(thread))) {
        count++;
    }

    EXPECT_EQ(count, rootNode->GetCount());
}

HWTEST_F_L0(RBTreeNodeTest, RBTreeNodeCompare)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::string value1("Compare1");
    std::string value2("Compare2");
    std::string value3("Compare1");
    std::string value4("name");
    JSHandle<JSTaggedValue> a(thread, factory->NewFromStdString(value1).GetTaggedValue());
    JSHandle<JSTaggedValue> b(thread, factory->NewFromStdString(value2).GetTaggedValue());
    JSHandle<JSTaggedValue> c(thread, factory->NewFromStdString(value3).GetTaggedValue());
    JSHandle<JSTaggedValue> d(thread, factory->NewFromStdString(value4).GetTaggedValue());
    int rvalue = RBTreeNode::Compare(thread, 12345, a.GetTaggedValue(), 12345, b.GetTaggedValue());
    EXPECT_TRUE(rvalue != 0);
    rvalue = RBTreeNode::Compare(thread, 54321, a.GetTaggedValue(), 54321, c.GetTaggedValue());
    EXPECT_EQ(rvalue, 0);
    rvalue = RBTreeNode::Compare(thread, 3373707, d.GetTaggedValue(), 3373707, JSTaggedValue(38754584));
    EXPECT_EQ(rvalue, 1);
}
}
