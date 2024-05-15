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

#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_linked_list.h"
#include "ecmascript/js_api/js_api_linked_list_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_list.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;

using namespace panda::ecmascript;

using namespace panda::ecmascript::containers;

namespace panda::test {
class JSAPILinkedListTest : public BaseTestWithScope<false> {
protected:
    JSAPILinkedList *CreateLinkedList()
    {
        return EcmaContainerCommon::CreateLinkedList(thread);
    }
};

HWTEST_F_L0(JSAPILinkedListTest, LinkedListCreate)
{
    JSAPILinkedList *linkedlist = CreateLinkedList();
    EXPECT_TRUE(linkedlist != nullptr);
}

HWTEST_F_L0(JSAPILinkedListTest, AddAndHas)
{
    constexpr int NODE_NUMBERS = 9;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPILinkedList> toor(thread, CreateLinkedList());

    std::string myValue("myvalue");
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ivalue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPILinkedList::Add(thread, toor, value);
    }
    EXPECT_EQ(toor->Length(), NODE_NUMBERS);

    EcmaTestCommon::ListAddHasCommon(thread, toor, value, myValue, NODE_NUMBERS) ;
}

HWTEST_F_L0(JSAPILinkedListTest, AddFirstAndGetFirst)
{
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSHandle<JSAPILinkedList> list(thread, CreateLinkedList());
    list->Add(thread, list, value);
    EXPECT_EQ(list->Length(), 1);
    EXPECT_EQ(list->Get(0).GetInt(), 1);

    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(2));
    list->AddFirst(thread, list, value1);
    EXPECT_EQ(list->Length(), 2);
    EXPECT_EQ(list->GetFirst().GetInt(), 2);
}

HWTEST_F_L0(JSAPILinkedListTest, InsertAndGetLast)
{
    JSHandle<JSAPILinkedList> toor(thread, CreateLinkedList());
    EcmaTestCommon::InsertAndGetLastCommon<JSAPILinkedList>(thread, toor);
}

HWTEST_F_L0(JSAPILinkedListTest, GetIndexOfAndGetLastIndexOf)
{
    JSHandle<JSAPILinkedList> toor(thread, CreateLinkedList());
    EcmaTestCommon::GetIndexOfAndGetLastIndexOfCommon<JSAPILinkedList>(thread, toor);
}

HWTEST_F_L0(JSAPILinkedListTest, Remove)
{
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPILinkedList> toor(thread, CreateLinkedList());

    EcmaTestCommon::ListRemoveCommon<JSAPILinkedList>(thread, toor, value);

    EXPECT_EQ(JSAPILinkedList::RemoveFirst(thread, toor), JSTaggedValue(0));
    value.Update(JSTaggedValue(0));
    EXPECT_EQ(toor->Has(value.GetTaggedValue()), false);
    EXPECT_EQ(toor->Length(), 19);

    EXPECT_EQ(JSAPILinkedList::RemoveLast(thread, toor), JSTaggedValue(19));
    value.Update(JSTaggedValue(19));
    EXPECT_EQ(toor->Has(value.GetTaggedValue()), false);
    EXPECT_EQ(toor->Length(), 18);

    value.Update(JSTaggedValue(5));
    EXPECT_EQ(JSAPILinkedList::RemoveByIndex(thread, toor, 4), value.GetTaggedValue());
    EXPECT_EQ(toor->Has(value.GetTaggedValue()), false);
    EXPECT_EQ(toor->Length(), 17);

    value.Update(JSTaggedValue(8));
    EXPECT_EQ(toor->Remove(thread, value.GetTaggedValue()), JSTaggedValue::True());
    EXPECT_EQ(toor->Has(value.GetTaggedValue()), false);
    EXPECT_EQ(toor->Length(), 16);

    value.Update(JSTaggedValue(11));
    EXPECT_EQ(JSAPILinkedList::RemoveFirstFound(thread, toor, value.GetTaggedValue()), JSTaggedValue::True());
    EXPECT_EQ(toor->Has(value.GetTaggedValue()), false);
    EXPECT_EQ(toor->Length(), 15);

    value.Update(JSTaggedValue(14));
    EXPECT_EQ(JSAPILinkedList::RemoveLastFound(thread, toor, value.GetTaggedValue()), JSTaggedValue::True());
    EXPECT_EQ(toor->Has(value.GetTaggedValue()), false);
    EXPECT_EQ(toor->Length(), 14);

    toor->Dump();
}

HWTEST_F_L0(JSAPILinkedListTest, SpecialReturnOfRemove)
{
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPILinkedList> linkedList(thread, CreateLinkedList());
    JSAPILinkedList::RemoveFirst(thread, linkedList);
    EXPECT_EXCEPTION();

    JSAPILinkedList::RemoveLast(thread, linkedList);
    EXPECT_EXCEPTION();

    JSAPILinkedList::RemoveFirstFound(thread, linkedList, value.GetTaggedValue());
    EXPECT_EXCEPTION();

    // test Remove and RemoveLastFound linkedlist whose nodeLength less than 0
    JSHandle<TaggedDoubleList> doubleList(thread, linkedList->GetDoubleList());
    doubleList->SetNumberOfNodes(thread, -1);
    EXPECT_EQ(linkedList->Remove(thread, value.GetTaggedValue()), JSTaggedValue::False());

    JSAPILinkedList::RemoveFirstFound(thread, linkedList, value.GetTaggedValue());
    EXPECT_EXCEPTION();
}

HWTEST_F_L0(JSAPILinkedListTest, Clear)
{
    JSAPILinkedList *linkedlist = CreateLinkedList();

    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSHandle<JSAPILinkedList> list(thread, linkedlist);
    JSAPILinkedList::Add(thread, list, value);

    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(2));
    JSAPILinkedList::Insert(thread, list, value1, 0);

    list->Clear(thread);

    EXPECT_EQ(list->Length(), 0);
    EXPECT_TRUE(list->GetFirst().IsUndefined());
}

HWTEST_F_L0(JSAPILinkedListTest, Set)
{
    constexpr uint32_t NODE_NUMBERS = 20;
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPILinkedList> toor(thread, CreateLinkedList());

    EcmaTestCommon::ListRemoveCommon<JSAPILinkedList>(thread, toor, value);

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i + 1));
        JSAPILinkedList::Set(thread, toor, i, value);
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i + 1));
        JSTaggedValue gValue = toor->Get(i);
        EXPECT_EQ(gValue, value.GetTaggedValue());
    }
}

HWTEST_F_L0(JSAPILinkedListTest, GetOwnProperty)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPILinkedList> toor(thread, CreateLinkedList());

    std::string linkedListvalue("linkedListvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = linkedListvalue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPILinkedList::Add(thread, toor, value);
    }
    // test GetOwnProperty
    int testInt = 1;
    JSHandle<JSTaggedValue> linkedListKey1(thread, JSTaggedValue(testInt));
    EXPECT_TRUE(JSAPILinkedList::GetOwnProperty(thread, toor, linkedListKey1));
    testInt = 20;
    JSHandle<JSTaggedValue> linkedListKey2(thread, JSTaggedValue(testInt));
    EXPECT_FALSE(JSAPILinkedList::GetOwnProperty(thread, toor, linkedListKey2));
    EXPECT_EXCEPTION();

    // test GetOwnProperty exception
    JSHandle<JSTaggedValue> undefined(thread, JSTaggedValue::Undefined());
    EXPECT_FALSE(JSAPILinkedList::GetOwnProperty(thread, toor, undefined));
    EXPECT_EXCEPTION();
}

/**
 * @tc.name: GetProperty
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPILinkedListTest, GetProperty)
{
    JSHandle<JSAPILinkedList> toor(thread, CreateLinkedList());
    uint32_t elementsNums = 8;
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPILinkedList::Add(thread, toor, value);
    }
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        OperationResult getPropertyRes = JSAPILinkedList::GetProperty(thread, toor, key);
        EXPECT_EQ(getPropertyRes.GetValue().GetTaggedValue(), JSTaggedValue(i));
    }
}

/**
 * @tc.name: SetProperty
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPILinkedListTest, SetProperty)
{
    JSHandle<JSAPILinkedList> toor(thread, CreateLinkedList());
    uint32_t elementsNums = 8;
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPILinkedList::Add(thread, toor, value);
    }
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i * 2)); // 2 : It means double
        bool setPropertyRes = JSAPILinkedList::SetProperty(thread, toor, key, value);
        EXPECT_EQ(setPropertyRes, true);
    }
    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(-1));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(-1));
    EXPECT_FALSE(JSAPILinkedList::SetProperty(thread, toor, key, value));
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(elementsNums));
    EXPECT_FALSE(JSAPILinkedList::SetProperty(thread, toor, key1, value));
}

HWTEST_F_L0(JSAPILinkedListTest, Clone)
{
    uint32_t elementsNums = 8;
    JSAPILinkedList *linkedlist = CreateLinkedList();
    JSHandle<JSAPILinkedList> list(thread, linkedlist);
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPILinkedList::Add(thread, list, value);
    }
    JSHandle<JSAPILinkedList> cloneList = JSAPILinkedList::Clone(thread, list);

    for (uint32_t i = 0; i < elementsNums; i++) {
        EXPECT_EQ(cloneList->Get(i), list->Get(i));
    }
    EXPECT_EQ(list->Length(), cloneList->Length());
}

HWTEST_F_L0(JSAPILinkedListTest, OwnKeys)
{
    uint32_t elementsNums = 8;
    JSHandle<JSAPILinkedList> linkedList(thread, CreateLinkedList());
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPILinkedList::Add(thread, linkedList, value);
    }
    JSHandle<TaggedArray> keyArray = JSAPILinkedList::OwnKeys(thread, linkedList);
    EXPECT_TRUE(keyArray->GetClass()->IsTaggedArray());
    EXPECT_TRUE(keyArray->GetLength() == elementsNums);
    for (uint32_t i = 0; i < elementsNums; i++) {
        ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(*(base::NumberHelper::NumberToString(thread, JSTaggedValue(i))),
            EcmaString::Cast(keyArray->Get(i).GetTaggedObject())));
    }
}
}  // namespace panda::test
