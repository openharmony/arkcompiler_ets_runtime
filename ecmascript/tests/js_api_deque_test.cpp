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
#include "ecmascript/js_api/js_api_deque.h"
#include "ecmascript/js_api/js_api_deque_iterator.h"
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
class JSAPIDequeTest : public BaseTestWithScope<false> {
protected:
    JSAPIDeque *CreateDeque()
    {
        return EcmaContainerCommon::CreateJSApiDeque(thread);
    }
};

HWTEST_F_L0(JSAPIDequeTest, dequeCreate)
{
    JSAPIDeque *deque = CreateDeque();
    EXPECT_TRUE(deque != nullptr);
}

HWTEST_F_L0(JSAPIDequeTest, InsertFrontAndGetFront)
{
    constexpr uint32_t NODE_NUMBERS = 9;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPIDeque> toor(thread, CreateDeque());

    // test GetFront of empty deque
    EXPECT_EQ(toor->GetFront(), JSTaggedValue::Undefined());

    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ivalue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIDeque::InsertFront(thread, toor, value);
        EXPECT_EQ(toor->GetFront(), value.GetTaggedValue());
    }

    toor->Dump();
}

HWTEST_F_L0(JSAPIDequeTest, InsertEndAndGetTail)
{
    constexpr uint32_t NODE_NUMBERS = 9;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPIDeque> toor(thread, CreateDeque());

    // test GetTail of empty deque
    EXPECT_EQ(toor->GetTail(), JSTaggedValue::Undefined());

    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ivalue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIDeque::InsertEnd(thread, toor, value);
        EXPECT_EQ(toor->GetTail(), value.GetTaggedValue());
    }

    toor->Dump();
}

HWTEST_F_L0(JSAPIDequeTest, PopFirst)
{
    constexpr uint32_t NODE_NUMBERS = 9;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPIDeque> toor(thread, CreateDeque());

    // test PopFirst of empty deque
    EXPECT_EQ(toor->PopFirst(), JSTaggedValue::Undefined());

    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ivalue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIDeque::InsertFront(thread, toor, value);
        EXPECT_EQ(toor->PopFirst(), value.GetTaggedValue());
    }

    toor->Dump();
}

HWTEST_F_L0(JSAPIDequeTest, PopLast)
{
    constexpr uint32_t NODE_NUMBERS = 9;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPIDeque> toor(thread, CreateDeque());

    // test PopLast of empty deque
    EXPECT_EQ(toor->PopLast(), JSTaggedValue::Undefined());

    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ivalue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIDeque::InsertEnd(thread, toor, value);
        EXPECT_EQ(toor->PopLast(), value.GetTaggedValue());
    }

    toor->Dump();
}

HWTEST_F_L0(JSAPIDequeTest, GetOwnProperty)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPIDeque> toor(thread, CreateDeque());

    std::string dequeValue("dequevalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = dequeValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIDeque::InsertFront(thread, toor, value);
        JSAPIDeque::InsertEnd(thread, toor, value);
    }
    // test GetOwnProperty
    int testInt = 1;
    JSHandle<JSTaggedValue> dequeKey1(thread, JSTaggedValue(testInt));
    EXPECT_TRUE(JSAPIDeque::GetOwnProperty(thread, toor, dequeKey1));
    testInt = 20;
    JSHandle<JSTaggedValue> dequeKey2(thread, JSTaggedValue(testInt));
    EXPECT_FALSE(JSAPIDeque::GetOwnProperty(thread, toor, dequeKey2));
    EXPECT_EXCEPTION();

    // test GetOwnProperty exception
    JSHandle<JSTaggedValue> undefined(thread, JSTaggedValue::Undefined());
    EXPECT_FALSE(JSAPIDeque::GetOwnProperty(thread, toor, undefined));
    EXPECT_EXCEPTION();
}

/**
 * @tc.name: GetProperty
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIDequeTest, GetProperty)
{
    JSHandle<JSAPIDeque> toor(thread, CreateDeque());
    uint32_t elementsNums = 8;
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIDeque::InsertEnd(thread, toor, value);
    }
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        OperationResult getPropertyRes = JSAPIDeque::GetProperty(thread, toor, key);
        EXPECT_EQ(getPropertyRes.GetValue().GetTaggedValue(), JSTaggedValue(i));
    }
}

/**
 * @tc.name: SetProperty
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIDequeTest, SetProperty)
{
    JSHandle<JSAPIDeque> toor(thread, CreateDeque());
    uint32_t elementsNums = 8;
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIDeque::InsertFront(thread, toor, value);
        JSAPIDeque::InsertEnd(thread, toor, value);
    }
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i * 2)); // 2 : It means double
        bool setPropertyRes = JSAPIDeque::SetProperty(thread, toor, key, value);
        EXPECT_EQ(setPropertyRes, true);
    }
    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(-1));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(-1));
    EXPECT_FALSE(JSAPIDeque::SetProperty(thread, toor, key, value));
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(elementsNums * 2));
    EXPECT_FALSE(JSAPIDeque::SetProperty(thread, toor, key1, value));
}

/**
 * @tc.name: OwnKeys
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIDequeTest, OwnKeys)
{
    JSHandle<JSAPIDeque> toor(thread, CreateDeque());
    uint32_t elementsNums = 8;
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIDeque::InsertEnd(thread, toor, value);
    }

    // test OwnKeys
    JSHandle<TaggedArray> keyArray = JSAPIDeque::OwnKeys(thread, toor);
    EXPECT_TRUE(keyArray->GetClass()->IsTaggedArray());
    EXPECT_TRUE(keyArray->GetLength() == elementsNums);
    for (uint32_t i = 0; i < elementsNums; i++) {
        ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(*(base::NumberHelper::NumberToString(thread, JSTaggedValue(i))),
            EcmaString::Cast(keyArray->Get(i).GetTaggedObject())));
    }
}
}  // namespace panda::test
