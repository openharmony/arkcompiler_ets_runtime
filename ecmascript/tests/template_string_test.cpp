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

#include "ecmascript/template_string.h"
#include "ecmascript/builtins/builtins_string.h"
#include "ecmascript/global_env.h"
#include "ecmascript/template_map.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
using BuiltinsString = ecmascript::builtins::BuiltinsString;
class TemplateStringTest : public BaseTestWithScope<false> {
};

/*
 * @tc.name: GetTemplateObject
 * @tc.desc: Call the function "arraycreate" to create a jsarray object, add key value pairs, and then call the
 *           "NewTagedArray" function to nest the jsarray object. Call the function createarrayfromlist to create
 *           a template literal, pass it to the "GetTemplateObject" function, check whether the templatemap object
 *           getted from env can find the jsarray object, then call the 'Raw' function to check whether the returned
 *           value is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(TemplateStringTest, GetTemplateObject)
{
    uint32_t arrayLength = 3;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> templateMapTag = env->GetTemplateMap();
    JSHandle<TemplateMap> templateMap(templateMapTag);
    JSHandle<JSTaggedValue> rawKey(factory->NewFromASCII("raw"));
    JSHandle<EcmaString> testString = factory->NewFromASCII("bar2bazJavaScriptbaz");
    JSHandle<EcmaString> javaScript = factory->NewFromASCII("JavaScript");
    JSHandle<TaggedArray> elements = factory->NewTaggedArray(arrayLength);
    // create jsArray object
    JSArray *arr = JSArray::ArrayCreate(thread, JSTaggedNumber(arrayLength)).GetObject<JSArray>();
    JSHandle<JSTaggedValue> jsArrayObj(thread, arr);
    // add key and value to the first index
    JSHandle<JSTaggedValue> arrayObjVal(factory->NewFromASCII("bar"));
    PropertyDescriptor propertyDesc(thread, arrayObjVal, true, true, true);
    JSHandle<JSTaggedValue> arrayObjKey(factory->NewFromASCII("0"));
    JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(jsArrayObj), arrayObjKey, propertyDesc);
    // add key and value to the second index
    JSHandle<JSTaggedValue> arrayObjVal1(factory->NewFromASCII("baz"));
    PropertyDescriptor propertyDesc1(thread, arrayObjVal1, true, true, true);
    JSHandle<JSTaggedValue> arrayObjKey1(factory->NewFromASCII("1"));
    JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(jsArrayObj), arrayObjKey1, propertyDesc1);
    // add key and value to the third index
    JSHandle<JSTaggedValue> arrayObjVal2(factory->NewFromASCII("foo"));
    PropertyDescriptor propertyDesc2(thread, arrayObjVal1, true, true, true);
    JSHandle<JSTaggedValue> arrayObjKey2(factory->NewFromASCII("2"));
    JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(jsArrayObj), arrayObjKey2, propertyDesc2);
    // create the list from jsArray
    elements->Set(thread, 0, jsArrayObj);
    elements->Set(thread, 1, jsArrayObj);
    JSHandle<JSTaggedValue> templateLiteral(JSArray::CreateArrayFromList(thread, elements));
    // call "GetTemplateObject" function
    JSHandle<JSTaggedValue> templateObject = TemplateString::GetTemplateObject(thread, templateLiteral);
    // find jsArray in templateMap
    int resultEntry = templateMap->FindEntry(thread, jsArrayObj.GetTaggedValue());
    EXPECT_NE(resultEntry, -1);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, templateObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(2)));
    ecmaRuntimeCallInfo->SetCallArg(2, javaScript.GetTaggedValue());
    // call "Raw" function
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsString::Raw(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsString());
    EXPECT_TRUE(
        EcmaStringAccessor::StringsAreEqual(thread, reinterpret_cast<EcmaString *>(result.GetRawData()), *testString));
}
}  // namespace panda::test
