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

#include "ecmascript/js_api/js_api_stack_iterator.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_stack.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

namespace panda::test {
class JSAPIStackIteratorTest : public BaseTestWithScope<false> {
protected:
    static JSHandle<JSAPIStack> CreateJSApiStack(JSThread *thread)
    {
        return EcmaContainerCommon::CreateJSApiStack(thread);
    }
};

/**
 * @tc.name: Next
 * @tc.desc: Create an iterator of JSAPIStack,and then loop through the elements of the iterator to check whether
 *           the elements are consistent through Next function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIStackIteratorTest, Next)
{
    constexpr uint32_t DEFAULT_LENGTH = 9;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIStack> jsStack = CreateJSApiStack(thread);
    EXPECT_TRUE(*jsStack != nullptr);
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> valueStr = thread->GlobalConstants()->GetHandledValueString();
    // insert value
    std::string stackValue("keyvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = stackValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIStack::Push(thread, jsStack, value);
    }
    JSHandle<JSAPIStackIterator> stackIterator = factory->NewJSAPIStackIterator(jsStack);
    for (uint32_t i = 0; i <= DEFAULT_LENGTH; i++) {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(stackIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPIStackIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSObject> resultObj(thread, result);
        if (i <= DEFAULT_LENGTH - 1U) {
            EXPECT_EQ(stackIterator->GetNextIndex(), (i + 1U));
            EXPECT_TRUE(JSObject::GetProperty(thread, resultObj, valueStr).GetValue()->IsString());
        }
        else {
            EXPECT_TRUE(stackIterator->GetIteratedStack().IsUndefined());
            EXPECT_TRUE(JSObject::GetProperty(thread, resultObj, valueStr).GetValue()->IsUndefined());
        }
    }
}

/**
 * @tc.name: Next
 * @tc.desc: test special return of Next, including throw exception and return undefined
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIStackIteratorTest, SpecialReturnOfNext)
{
    JSHandle<JSAPIStack> jsStack = CreateJSApiStack(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIStackIterator> stackIterator = factory->NewJSAPIStackIterator(jsStack);
    stackIterator->SetIteratedStack(thread, JSTaggedValue::Undefined());

    // test Next exception
    {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPIStackIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::Exception());
        EXPECT_EXCEPTION();
    }

    // test Next return undefined
    {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(stackIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPIStackIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, thread->GetEcmaVM()->GetGlobalEnv()->GetUndefinedIteratorResult().GetTaggedValue());
    }
}

/**
 * @tc.name: SetIteratedStack
 * @tc.desc: Call the "SetIteratedStack" function, check whether the result returned through "GetIteratedStack"
 *           function from the JSAPIStackIterator is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIStackIteratorTest, SetIteratedStack)
{
    constexpr uint32_t DEFAULT_LENGTH = 9;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIStack> jsStack1 = CreateJSApiStack(thread);
    JSHandle<JSAPIStack> jsStack2 = CreateJSApiStack(thread);
    EXPECT_TRUE(*jsStack1 != nullptr);
    EXPECT_TRUE(*jsStack2 != nullptr);
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    // insert value
    std::string stackValue("keyvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = stackValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIStack::Push(thread, jsStack1, value);
    }

    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = stackValue + std::to_string(i + 2U);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIStack::Push(thread, jsStack2, value);
    }
    JSHandle<JSAPIStackIterator> stackIterator = factory->NewJSAPIStackIterator(jsStack1);
    EXPECT_EQ(stackIterator->GetIteratedStack(), jsStack1.GetTaggedValue());

    stackIterator->SetIteratedStack(thread, jsStack2.GetTaggedValue());
    JSHandle<JSAPIStack> jsAPIStackTo(thread, JSAPIStack::Cast(stackIterator->GetIteratedStack().GetTaggedObject()));
    EXPECT_EQ(jsAPIStackTo->GetSize(), static_cast<int>(DEFAULT_LENGTH - 1U));

    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = stackValue + std::to_string(i + 2U);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        EXPECT_EQ(jsAPIStackTo->Search(value), static_cast<int>(i));
    }
}

/**
 * @tc.name: SetNextIndex
 * @tc.desc: Call the "SetNextIndex" function, check whether the result returned through "GetNextIndex"
 *           function from the JSAPIStackIterator is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIStackIteratorTest, SetNextIndex)
{
    constexpr uint32_t DEFAULT_LENGTH = 9;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIStack> jsStack = CreateJSApiStack(thread);
    EXPECT_TRUE(*jsStack != nullptr);
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    // insert value
    std::string stackValue("keyvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = stackValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIStack::Push(thread, jsStack, value);
    }
    JSHandle<JSAPIStackIterator> stackIterator = factory->NewJSAPIStackIterator(jsStack);
    EXPECT_EQ(stackIterator->GetNextIndex(), 0U);

    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        stackIterator->SetNextIndex(i);
        EXPECT_EQ(stackIterator->GetNextIndex(), i);
    }
}
}  // namespace panda::ecmascript
