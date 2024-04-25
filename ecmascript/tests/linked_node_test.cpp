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
#include "ecmascript/tagged_node.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
namespace panda::test {
class LinkedNodeTest : public BaseTestWithScope<false> {
public:
    JSHandle<GlobalEnv> GetGlobalEnv()
    {
        EcmaVM *ecma = thread->GetEcmaVM();
        return ecma->GetGlobalEnv();
    }
    uint32_t NODE_NUMBERS = 8;

protected:
    JSHandle<LinkedNode> CreateLinkedList()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
        JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
        std::string myKey("mykey");
        std::string myValue("myvalue");
        JSHandle<LinkedNode> head(thread, JSTaggedValue::Hole());
        for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
            std::string iKey = myKey + std::to_string(i);
            std::string iValue = myValue + std::to_string(i);
            key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
            value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
            int hash = TaggedNode::Hash(thread, key.GetTaggedValue());
            head = factory->NewLinkedNode(hash, key, value, head);
        }
        return head;
    }
};

HWTEST_F_L0(LinkedNodeTest, LinkedNodeCreate)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::string k("testKey");
    std::string v("testValue");
    JSHandle<JSTaggedValue> key(thread, factory->NewFromStdString(k).GetTaggedValue());
    JSHandle<JSTaggedValue> value(thread, factory->NewFromStdString(v).GetTaggedValue());
    int hash = TaggedNode::Hash(thread, factory->NewFromStdString(k).GetTaggedValue());
    JSHandle<LinkedNode> hole(thread, JSTaggedValue::Hole());
    JSHandle<LinkedNode> newNode = factory->NewLinkedNode(hash, key, value, hole);
    EXPECT_TRUE(!newNode.GetTaggedValue().IsHole());
    EXPECT_TRUE(newNode.GetTaggedValue().IsLinkedNode());
}

HWTEST_F_L0(LinkedNodeTest, Treeify)
{
    JSHandle<LinkedNode> head = CreateLinkedList();
    JSHandle<RBTreeNode> root = LinkedNode::Treeing(thread, head);
    EXPECT_EQ(root->GetCount(), NODE_NUMBERS);
}
}