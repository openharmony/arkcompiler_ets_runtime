/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/builtins/builtins_string_iterator.h"

#include "ecmascript/js_iterator.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class BuiltinsStringIteratorTest : public BaseTestWithScope<false> {
};

static JSTaggedValue CreateBuiltinsJSStringIterator(JSThread *thread, const CString keyCStr)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<EcmaString> string = factory->NewFromUtf8(&keyCStr[0]);
    JSHandle<JSStringIterator> stringIterator = JSStringIterator::CreateStringIterator(thread, string);
    EXPECT_TRUE(*stringIterator != nullptr);

    return stringIterator.GetTaggedValue();
}

JSTaggedValue NextCommon(JSThread* thread, JSHandle<JSStringIterator>& strIter)
{
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(strIter.GetTaggedValue());

    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    auto result = BuiltinsStringIterator::Next(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

// Single char16_t Basic Multilingual plane character
HWTEST_F_L0(BuiltinsStringIteratorTest, Next_001)
{
    auto globalConst = thread->GlobalConstants();
    CString string = "没有";
    JSHandle<JSStringIterator> stringIterator =
        JSHandle<JSStringIterator>(thread, CreateBuiltinsJSStringIterator(thread, string));
    JSHandle<JSTaggedValue> valueStr = globalConst->GetHandledValueString();

    NextCommon(thread, stringIterator);
    EXPECT_EQ(stringIterator->GetStringIteratorNextIndex(), 1U);

    NextCommon(thread, stringIterator);
    EXPECT_EQ(stringIterator->GetStringIteratorNextIndex(), 2U);

    JSTaggedValue result = NextCommon(thread, stringIterator);
    JSHandle<JSTaggedValue> resultObj(thread, result);
    EXPECT_TRUE(JSObject::GetProperty(thread, JSHandle<JSObject>(thread, result), valueStr).GetValue()->IsUndefined());
}

// character with lead surrogate and trail surrogate character
HWTEST_F_L0(BuiltinsStringIteratorTest, Next_002)
{
    auto globalConst = thread->GlobalConstants();
    CString string = "没𠕇";
    JSHandle<JSStringIterator> stringIterator =
        JSHandle<JSStringIterator>(thread, CreateBuiltinsJSStringIterator(thread, string));
    JSHandle<JSTaggedValue> valueStr = globalConst->GetHandledValueString();

    NextCommon(thread, stringIterator);
    EXPECT_EQ(stringIterator->GetStringIteratorNextIndex(), 1U);

    NextCommon(thread, stringIterator);
    EXPECT_EQ(stringIterator->GetStringIteratorNextIndex(), 3U);

    auto result = NextCommon(thread, stringIterator);
    JSHandle<JSTaggedValue> resultObj(thread, result);
    EXPECT_TRUE(JSObject::GetProperty(thread, JSHandle<JSObject>(thread, result), valueStr).GetValue()->IsUndefined());
}
}  // namespace panda::test
