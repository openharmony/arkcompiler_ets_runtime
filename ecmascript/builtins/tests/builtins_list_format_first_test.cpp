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

#include "builtin_test_util.h"
#include "ecmascript/builtins/builtins_list_format.h"

#include "ecmascript/global_env.h"
#include "ecmascript/js_list_format.h"
#include "ecmascript/js_array.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class BuiltinsListFormatTest : public BaseTestWithScope<true> {
};

// new Intl.ListFormat(locales)
HWTEST_F_L0(BuiltinsListFormatTest, ListFormatConstructor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetListFormatFunction());

    JSHandle<JSTaggedValue> localesString(factory->NewFromASCII("en-GB"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, localesString.GetTaggedValue());
    // option tag is default value
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsListFormat::ListFormatConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsJSListFormat());
}

JSTaggedValue FormatCommon(JSThread* thread, JSHandle<JSListFormat>& jsFormat, JSTaggedValue value)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value);

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsListFormat::Format(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

// Format("Motorcycle" ,type(conjunction))
HWTEST_F_L0(BuiltinsListFormatTest, Format_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-GB"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("conjunction")); // the default value
    JSHandle<JSListFormat> jSListFormat =
        JSHandle<JSListFormat>(thread, BuiltTestUtil::JSListFormatCreateWithOptionTest(thread, locale, typeValue));

    JSHandle<JSTaggedValue> listValue(factory->NewFromASCII("Motorcycle"));
    auto result = FormatCommon(thread, jSListFormat, listValue.GetTaggedValue());

    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("M, o, t, o, r, c, y, c, l and e", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

void DefineOwnPropertyCommonTest(JSThread* thread, JSHandle<JSObject>& obj, std::vector<JSHandle<JSTaggedValue>>& vals)
{
    for (size_t i = 0; i < vals.size(); i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(static_cast<int>(i)));
        PropertyDescriptor desc(thread, vals[i], true, true, true);
        JSArray::DefineOwnProperty(thread, obj, key, desc);
    }
}

JSHandle<EcmaString> CommonTest(JSThread* thread, JSHandle<JSTaggedValue>& locale, JSHandle<JSTaggedValue>& typeValue,
    std::vector<std::string>& strValues)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSListFormat> jSListFormat =
        JSHandle<JSListFormat>(thread, BuiltTestUtil::JSListFormatCreateWithOptionTest(thread, locale, typeValue));
    JSArray *arr = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSArray>();
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> value(thread, arr);
    std::vector<JSHandle<JSTaggedValue>> vals;
    for (size_t i = 0; i < strValues.size(); i++) {
        JSHandle<JSTaggedValue> listValue(factory->NewFromStdString(strValues[i]));
        vals.push_back(listValue);
    }
    
    DefineOwnPropertyCommonTest(thread, value, vals);

    auto result = FormatCommon(thread, jSListFormat, value.GetTaggedValue());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    return handleEcmaStr;
}

// Format(["Motorcycle", "Bus", "Car" ], type(conjunction))
HWTEST_F_L0(BuiltinsListFormatTest, Format_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-GB"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("conjunction")); // the default value
    std::vector<std::string> listVal{"Motorcycle", "Bus", "Car"};
    JSHandle<EcmaString> handleEcmaStr = CommonTest(thread, locale, typeValue, listVal);
    EXPECT_STREQ("Motorcycle, Bus and Car", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

// Format(["Motorcycle", "Bus", "Car" ], type(disjunction))
HWTEST_F_L0(BuiltinsListFormatTest, Format_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-GB"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("disjunction")); // the default value
    std::vector<std::string> listVal{"Motorcycle", "Bus", "Car"};
    JSHandle<EcmaString> handleEcmaStr = CommonTest(thread, locale, typeValue, listVal);
    EXPECT_STREQ("Motorcycle, Bus or Car", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

// Format(["中文英文" ], type(disjunction))
HWTEST_F_L0(BuiltinsListFormatTest, Format_004)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("zh-cn"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("disjunction")); // the default value

    std::vector<std::string> listVal{"中文英文"};
    JSHandle<EcmaString> handleEcmaStr = CommonTest(thread, locale, typeValue, listVal);
    EXPECT_STREQ("中文英文", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

// Format(["中文", "英文", "韩文" ], type(conjunction))
HWTEST_F_L0(BuiltinsListFormatTest, Format_005)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("zh-cn"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("conjunction")); // the default value

    std::vector<std::string> listVal{"中文", "英文", "韩文" };
    JSHandle<EcmaString> handleEcmaStr = CommonTest(thread, locale, typeValue, listVal);
    EXPECT_STREQ("中文、英文和韩文", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}
}  // namespace panda::test
