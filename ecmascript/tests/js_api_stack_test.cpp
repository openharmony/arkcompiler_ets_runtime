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
#include "ecmascript/js_api/js_api_stack.h"
#include "ecmascript/js_api/js_api_stack_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;

using namespace panda::ecmascript;

using namespace panda::ecmascript::containers;

namespace panda::test {
class JSAPIStackTest : public BaseTestWithScope<false> {
protected:
    JSAPIStack *CreateStack()
    {
        return *EcmaContainerCommon::CreateJSApiStack(thread);
    }

    JSHandle<JSAPIStack> SearchAndEmptyCommon(JSMutableHandle<JSTaggedValue>& value, std::string& myValue,
        uint32_t len, bool search = false)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

        JSHandle<JSAPIStack> toor(thread, CreateStack());
        for (uint32_t i = 0; i < len; i++) {
            std::string ivalue = myValue + std::to_string(i);
            value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
            JSTaggedValue result = JSAPIStack::Push(thread, toor, value);
            EXPECT_EQ(result, value.GetTaggedValue());
            if (search) {
                EXPECT_EQ(toor->Search(value), static_cast<int>(i));
            } else {
                EXPECT_EQ(toor->Peek(), value.GetTaggedValue());
                EXPECT_EQ(toor->Empty(), false);
            }
        }
        return toor;
    }
};

HWTEST_F_L0(JSAPIStackTest, stackCreate)
{
    JSAPIStack *stack = CreateStack();
    EXPECT_TRUE(stack != nullptr);
}

HWTEST_F_L0(JSAPIStackTest, PushAndPeek)
{
    constexpr uint32_t NODE_NUMBERS = 9;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPIStack> toor(thread, CreateStack());

    // test Peek empty
    EXPECT_EQ(toor->Peek(), JSTaggedValue::Undefined());

    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ivalue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSTaggedValue result = JSAPIStack::Push(thread, toor, value);
        EXPECT_EQ(result, value.GetTaggedValue());
        EXPECT_EQ(toor->Peek(), value.GetTaggedValue());
    }
    EXPECT_EQ(static_cast<uint32_t>(toor->GetTop() + 1), NODE_NUMBERS);

    toor->Dump();
}

HWTEST_F_L0(JSAPIStackTest, Pop)
{
    constexpr uint32_t NODE_NUMBERS = 9;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPIStack> toor(thread, CreateStack());

    // test Pop empty
    EXPECT_EQ(toor->Pop(thread), JSTaggedValue::Undefined());

    std::string myValue("myvalue");
    for (uint32_t i = 1; i <= NODE_NUMBERS; i++) {
        std::string ivalue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSTaggedValue result = JSAPIStack::Push(thread, toor, value);
        EXPECT_EQ(result, value.GetTaggedValue());
        EXPECT_EQ(toor->Peek(), value.GetTaggedValue());
    }

    for (uint32_t i = NODE_NUMBERS; i >= 1; i--) {
        std::string ivalue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSTaggedValue gValue = toor->Pop(thread);
        EXPECT_EQ(gValue, value.GetTaggedValue());
    }

    toor->Dump();
}

HWTEST_F_L0(JSAPIStackTest, Empty)
{
    constexpr uint32_t NODE_NUMBERS = 9;
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myValue("myvalue");
    auto toor = SearchAndEmptyCommon(value, myValue, NODE_NUMBERS);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    int num = 8;
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ivalue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        toor->Pop(thread);
        if (num == -1) {
            EXPECT_EQ(toor->Empty(), true);
        }
    }

    toor->Dump();
}

HWTEST_F_L0(JSAPIStackTest, Search)
{
    constexpr uint32_t NODE_NUMBERS = 9;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myValue("myvalue");
    auto toor = SearchAndEmptyCommon(value, myValue, NODE_NUMBERS, true);
    value.Update(factory->NewFromStdString(myValue).GetTaggedValue());
    EXPECT_EQ(toor->Search(value), -1);

    toor->Dump();
}

HWTEST_F_L0(JSAPIStackTest, GetOwnProperty)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPIStack> toor(thread, CreateStack());

    std::string stackvalue("stackvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = stackvalue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIStack::Push(thread, toor, value);
    }
    // test GetOwnProperty
    int testInt = 1;
    JSHandle<JSTaggedValue> stackKey1(thread, JSTaggedValue(testInt));
    EXPECT_TRUE(JSAPIStack::GetOwnProperty(thread, toor, stackKey1));
    testInt = 20;
    JSHandle<JSTaggedValue> stackKey2(thread, JSTaggedValue(testInt));
    EXPECT_FALSE(JSAPIStack::GetOwnProperty(thread, toor, stackKey2));
    EXPECT_EXCEPTION();

    // test GetOwnProperty exception
    JSHandle<JSTaggedValue> undefined(thread, JSTaggedValue::Undefined());
    EXPECT_FALSE(JSAPIStack::GetOwnProperty(thread, toor, undefined));
    EXPECT_EXCEPTION();
}

/**
 * @tc.name: GetProperty
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIStackTest, GetProperty)
{
    JSHandle<JSAPIStack> toor(thread, CreateStack());
    uint32_t elementsNums = 8;
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIStack::Push(thread, toor, value);
    }
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        OperationResult getPropertyRes = JSAPIStack::GetProperty(thread, toor, key);
        EXPECT_EQ(getPropertyRes.GetValue().GetTaggedValue(), JSTaggedValue(i));
    }

    // test GetProperty Exception
    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(-1));
    OperationResult getPropertyRes = JSAPIStack::GetProperty(thread, toor, key);
    EXPECT_EQ(getPropertyRes.GetValue().GetTaggedValue(), JSTaggedValue::Exception());
    EXPECT_EXCEPTION();

    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(elementsNums));
    OperationResult getPropertyRes1 = JSAPIStack::GetProperty(thread, toor, key1);
    EXPECT_EQ(getPropertyRes1.GetValue().GetTaggedValue(), JSTaggedValue::Exception());
    EXPECT_EXCEPTION();
}

/**
 * @tc.name: SetProperty
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIStackTest, SetProperty)
{
    JSHandle<JSAPIStack> toor(thread, CreateStack());
    uint32_t elementsNums = 8;
    {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(-1));
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(-1));
        EXPECT_EQ(JSTaggedValue::Undefined(), // when length = -1, return Undefine() instead of assert error
            toor->Set(thread, key->GetInt(), value.GetTaggedValue()));
    }
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIStack::Push(thread, toor, value);
    }
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i * 2)); // 2 : It means double
        bool setPropertyRes = JSAPIStack::SetProperty(thread, toor, key, value);
        EXPECT_EQ(setPropertyRes, true);
    }
    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(-1));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(-1));
    EXPECT_FALSE(JSAPIStack::SetProperty(thread, toor, key, value));
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(elementsNums));
    EXPECT_FALSE(JSAPIStack::SetProperty(thread, toor, key1, value));
}

/**
 * @tc.name: Has
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIStackTest, Has)
{
    JSHandle<JSAPIStack> toor(thread, CreateStack());

    // test Has empty
    EXPECT_FALSE(toor->Has(JSTaggedValue(0)));

    uint32_t elementsNums = 8;
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIStack::Push(thread, toor, value);
    }
    for (uint32_t i = 0; i < elementsNums; i++) {
        EXPECT_TRUE(toor->Has(JSTaggedValue(i)));
    }
    EXPECT_FALSE(toor->Has(JSTaggedValue(elementsNums)));
}

/**
 * @tc.name: OwnKeys
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIStackTest, OwnKeys)
{
    JSHandle<JSAPIStack> toor(thread, CreateStack());
    uint32_t elementsNums = 8;
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIStack::Push(thread, toor, value);
    }
    JSHandle<TaggedArray> keyArray = JSAPIStack::OwnKeys(thread, toor);
    EXPECT_TRUE(keyArray->GetClass()->IsTaggedArray());
    EXPECT_TRUE(keyArray->GetLength() == elementsNums);
    for (uint32_t i = 0; i < elementsNums; i++) {
        ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(*(base::NumberHelper::NumberToString(thread, JSTaggedValue(i))),
            EcmaString::Cast(keyArray->Get(i).GetTaggedObject())));
    }
}
}  // namespace panda::test
