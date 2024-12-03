/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/ecma_context.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/js_tagged_value-inl.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;
namespace panda::test {
class EcmaContextTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(EcmaContextTest, Create)
{
    auto context = EcmaContext::CreateAndInitialize(thread);
    thread->SwitchCurrentContext(context);
    CVector<EcmaContext *> Cv1 = thread->GetEcmaContexts();
    EXPECT_EQ(Cv1.size(), 2);  // 2: size of contexts.
    auto context2 = EcmaContext::CreateAndInitialize(thread);
    thread->SwitchCurrentContext(context2);
    Cv1 = thread->GetEcmaContexts();
    EXPECT_EQ(Cv1.size(), 3);  // 3: size of contexts.
    EcmaContext::CheckAndDestroy(thread, context);
    Cv1 = thread->GetEcmaContexts();
    EXPECT_EQ(Cv1.size(), 2);  // 2: size of contexts.
    EcmaContext::CheckAndDestroy(thread, context2);
}

HWTEST_F_L0(EcmaContextTest, GetRegExpCache)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    auto context = EcmaContext::CreateAndInitialize(thread);
    thread->SwitchCurrentContext(context);
    JSHandle<EcmaString> regexp = factory->NewFromASCII("\\g");
    JSHandle<JSTaggedValue> value2(regexp);
    context->SetRegExpCache(value2.GetTaggedValue());
    JSHandle<JSTaggedValue> res2 = context->GetRegExpCache();
    EXPECT_EQ(res2.GetTaggedValue(), value2.GetTaggedValue());
    EcmaContext::CheckAndDestroy(thread, context);
}

HWTEST_F_L0(EcmaContextTest, AllowAtomicWait)
{
    auto context = EcmaContext::CreateAndInitialize(thread);
    thread->SwitchCurrentContext(context);
    bool value = context->GetAllowAtomicWait();
    EXPECT_TRUE(value);
    context->SetAllowAtomicWait(false);
    bool value2 = context->GetAllowAtomicWait();
    EXPECT_FALSE(value2);
    EcmaContext::CheckAndDestroy(thread, context);
}

HWTEST_F_L0(EcmaContextTest, SwitchCurrentContext)
{
    auto context = EcmaContext::CreateAndInitialize(thread);
    auto context1 = EcmaContext::CreateAndInitialize(thread);

    CVector<EcmaContext *> contextVector = thread->GetEcmaContexts();
    EXPECT_EQ(contextVector.size(), 3);  // 3: size of contexts.

    thread->SwitchCurrentContext(context);

    EcmaContext::CheckAndDestroy(thread, context);
    contextVector = thread->GetEcmaContexts();
    EXPECT_EQ(contextVector.size(), 2);  // 3: size of contexts.
    EcmaContext::CheckAndDestroy(thread, context1);
}

HWTEST_F_L0(EcmaContextTest, CStringTest001)
{
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<EcmaString> testString = factory->NewFromASCII("bar2bazJavaScriptbaz");
        CString str = ConvertToString(EcmaString::Cast(testString.GetObject<EcmaString>()),
                                                        StringConvertedUsage::LOGICOPERATION, false);
        EXPECT_EQ(str, CString("bar2bazJavaScriptbaz"));
}

HWTEST_F_L0(EcmaContextTest, CStringTest002)
{
        CString str = ConvertToString(nullptr, StringConvertedUsage::LOGICOPERATION, false);
        EXPECT_EQ(str, CString(""));
}

HWTEST_F_L0(EcmaContextTest, CStringTest003)
{
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<EcmaString> testString = factory->NewFromASCII("test");
        EcmaString *ecmaString = EcmaString::Cast(testString.GetObject<EcmaString>());
        JSTaggedValue key = JSTaggedValue(ecmaString);
        CString str = ConvertToString(key);
        EXPECT_EQ(str, CString("test"));
}

HWTEST_F_L0(EcmaContextTest, CStringTest004)
{
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSSymbol> symbol = factory->NewPublicSymbolWithChar("bbb");
        JSHandle<JSTaggedValue> value5(symbol);
        JSTaggedValue key =
            JSTaggedValue(JSHandle<JSPrimitiveRef>::Cast(JSTaggedValue::ToObject(thread, value5))->GetValue());
        CString str = ConvertToString(key);
        EXPECT_EQ(str, CString("bbb"));
}

HWTEST_F_L0(EcmaContextTest, CStringTest005)
{
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSSymbol> symbol = factory->NewPublicSymbolWithChar("");
        symbol->SetDescription(thread, JSTaggedValue::Undefined());
        symbol->SetFlags(0);
        symbol->SetHashField(0);
        JSHandle<JSTaggedValue> value5(symbol);
        JSTaggedValue key =
            JSTaggedValue(JSHandle<JSPrimitiveRef>::Cast(JSTaggedValue::ToObject(thread, value5))->GetValue());
        CString str = ConvertToString(key);
        EXPECT_EQ(str, CString("Symbol()"));
}

}  // namespace panda::test