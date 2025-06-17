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
#include "ecmascript/js_api/js_api_list.h"
#include "ecmascript/js_api/js_api_list_iterator.h"
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
class JSAPIListTest : public BaseTestWithScope<false> {
protected:
    JSAPIList *CreateList()
    {
        return EcmaContainerCommon::CreateList(thread);
    }
};

HWTEST_F_L0(JSAPIListTest, listCreate)
{
    JSAPIList *list = CreateList();
    EXPECT_TRUE(list != nullptr);
}

HWTEST_F_L0(JSAPIListTest, AddHasAndIsEmpty)
{
    constexpr int NODE_NUMBERS = 9;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPIList> toor(thread, CreateList());

    EXPECT_TRUE(toor->IsEmpty(thread));

    std::string myValue("myvalue");
    for (int i = 0; i < NODE_NUMBERS; i++) {
        std::string ivalue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIList::Add(thread, toor, value);
    }
    EXPECT_EQ(toor->Length(thread), NODE_NUMBERS);
    EXPECT_FALSE(toor->IsEmpty(thread));

    EcmaTestCommon::ListAddHasCommon(thread, toor, value, myValue, NODE_NUMBERS);
    toor->Dump(thread);
}

HWTEST_F_L0(JSAPIListTest, InsertAndGetLastAndGetFirst)
{
    JSHandle<JSAPIList> toor(thread, CreateList());
    EcmaTestCommon::InsertAndGetLastCommon<JSAPIList>(thread, toor);
}

HWTEST_F_L0(JSAPIListTest, GetIndexOfAndGetLastIndexOf)
{
    JSHandle<JSAPIList> toor(thread, CreateList());
    EcmaTestCommon::GetIndexOfAndGetLastIndexOfCommon<JSAPIList>(thread, toor);
}

HWTEST_F_L0(JSAPIListTest, Remove)
{
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPIList> toor(thread, CreateList());
    EcmaTestCommon::ListRemoveCommon<JSAPIList>(thread, toor, value);

    value.Update(JSTaggedValue(4));
    EXPECT_EQ(JSAPIList::RemoveByIndex(thread, toor, 4), value.GetTaggedValue());
    EXPECT_EQ(toor->Has(thread, value.GetTaggedValue()), false);
    EXPECT_EQ(toor->Length(thread), 19);

    value.Update(JSTaggedValue(8));
    EXPECT_EQ(toor->Remove(thread, value.GetTaggedValue()), JSTaggedValue::True());
    EXPECT_EQ(toor->Has(thread, value.GetTaggedValue()), false);
    EXPECT_EQ(toor->Length(thread), 18);

    toor->Dump(thread);
}

HWTEST_F_L0(JSAPIListTest, Clear)
{
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSHandle<JSAPIList> list(thread, CreateList());
    JSAPIList::Add(thread, list, value);

    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(2));
    JSAPIList::Insert(thread, list, value1, 0);

    list->Clear(thread);

    EXPECT_EQ(list->Length(thread), 0);
    EXPECT_TRUE(list->GetFirst(thread).IsUndefined());
}

HWTEST_F_L0(JSAPIListTest, Set)
{
    constexpr uint32_t NODE_NUMBERS = 20;
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPIList> toor(thread, CreateList());
    EcmaTestCommon::ListRemoveCommon<JSAPIList>(thread, toor, value);

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i + 1));
        JSAPIList::Set(thread, toor, i, value);
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i + 1));
        JSTaggedValue gValue = toor->Get(thread, i);
        EXPECT_EQ(gValue, value.GetTaggedValue());
    }
}

HWTEST_F_L0(JSAPIListTest, GetOwnProperty)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPIList> toor(thread, CreateList());

    std::string listvalue("listvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = listvalue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIList::Add(thread, toor, value);
    }
    // test GetOwnProperty
    int testInt = 1;
    JSHandle<JSTaggedValue> listKey1(thread, JSTaggedValue(testInt));
    EXPECT_TRUE(JSAPIList::GetOwnProperty(thread, toor, listKey1));
    testInt = 20;
    JSHandle<JSTaggedValue> listKey2(thread, JSTaggedValue(testInt));
    EXPECT_FALSE(JSAPIList::GetOwnProperty(thread, toor, listKey2));
    EXPECT_EXCEPTION();

    // test GetOwnProperty exception
    JSHandle<JSTaggedValue> undefined(thread, JSTaggedValue::Undefined());
    EXPECT_FALSE(JSAPIList::GetOwnProperty(thread, toor, undefined));
    EXPECT_EXCEPTION();
}

/**
 * @tc.name: GetProperty
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIListTest, GetProperty)
{
    JSHandle<JSAPIList> toor(thread, CreateList());
    uint32_t elementsNums = 8;
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIList::Add(thread, toor, value);
    }
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        OperationResult getPropertyRes = JSAPIList::GetProperty(thread, toor, key);
        EXPECT_EQ(getPropertyRes.GetValue().GetTaggedValue(), JSTaggedValue(i));
    }
}

/**
 * @tc.name: SetProperty
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIListTest, SetProperty)
{
    JSHandle<JSAPIList> toor(thread, CreateList());
    uint32_t elementsNums = 8;
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIList::Add(thread, toor, value);
    }
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i * 2)); // 2 : It means double
        bool setPropertyRes = JSAPIList::SetProperty(thread, toor, key, value);
        EXPECT_EQ(setPropertyRes, true);
    }
    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(-1));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(-1));
    EXPECT_FALSE(JSAPIList::SetProperty(thread, toor, key, value));
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(elementsNums));
    EXPECT_FALSE(JSAPIList::SetProperty(thread, toor, key1, value));
}

/**
 * @tc.name: GetSubList
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIListTest, GetSubList)
{
    JSHandle<JSAPIList> List(thread, CreateList());
    uint32_t addElementNums = 256;
    for (uint32_t i = 0; i < addElementNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIList::Add(thread, List, value);
    }

    // throw error test
    uint32_t smallIndex = -1;
    uint32_t bigIndex = List->Length(thread) + 10;
    uint32_t zeroIndex = 0;

    // fromIndex < 0
    JSAPIList::GetSubList(thread, List, smallIndex, zeroIndex);
    EXPECT_EXCEPTION();

    // fromIndex >= size
    JSAPIList::GetSubList(thread, List, bigIndex, zeroIndex);
    EXPECT_EXCEPTION();

    // toIndex <= fromIndex
    JSAPIList::GetSubList(thread, List, zeroIndex, zeroIndex);
    EXPECT_EXCEPTION();

    // toIndex < 0
    JSAPIList::GetSubList(thread, List, zeroIndex, smallIndex);
    EXPECT_EXCEPTION();

    // toIndex > length
    JSAPIList::GetSubList(thread, List, zeroIndex, bigIndex);
    EXPECT_EXCEPTION();
}

HWTEST_F_L0(JSAPIListTest, OwnKeys)
{
    uint32_t elementsNums = 8;
    JSHandle<JSAPIList> list(thread, CreateList());
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIList::Add(thread, list, value);
    }
    JSHandle<TaggedArray> keyArray = JSAPIList::OwnKeys(thread, list);
    EXPECT_TRUE(keyArray->GetClass()->IsTaggedArray());
    EXPECT_TRUE(keyArray->GetLength() == elementsNums);
    for (uint32_t i = 0; i < elementsNums; i++) {
        ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(thread,
                                                        *(base::NumberHelper::NumberToString(thread, JSTaggedValue(i))),
                                                        EcmaString::Cast(keyArray->Get(thread, i).GetTaggedObject())));
    }
}
}  // namespace panda::test
