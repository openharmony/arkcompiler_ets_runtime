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

#include "ecmascript/builtins/builtins_global.h"

#include "ecmascript/js_hclass.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
using BuiltinsBase = panda::ecmascript::base::BuiltinsBase;

namespace panda::test {
class BuiltinsGlobalTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};

    static JSHandle<JSObject> NewJSObject(JSThread *thread, ObjectFactory *factory, JSHandle<GlobalEnv> globalEnv)
    {
    JSFunction *jsFunc = globalEnv->GetObjectFunction().GetObject<JSFunction>();
    JSHandle<JSTaggedValue> jsFunc1(thread, jsFunc);
    JSHandle<JSObject> jsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(jsFunc1), jsFunc1);
    return jsObj;
    }
};

HWTEST_F_L0(BuiltinsGlobalTest, DecodeURIComponent)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(123));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsGlobal::DecodeURIComponent(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> ecmaStrHandle(thread, result);
    EXPECT_STREQ("123", EcmaStringAccessor(ecmaStrHandle).ToCString().c_str());
}

HWTEST_F_L0(BuiltinsGlobalTest, EncodeURIComponent)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(123));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsGlobal::EncodeURIComponent(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> ecmaStrHandle(thread, result);
    EXPECT_STREQ("123", EcmaStringAccessor(ecmaStrHandle).ToCString().c_str());
}

HWTEST_F_L0(BuiltinsGlobalTest, PrintEntrypointAndPrintString)
{
    // msg == nullptr
    JSTaggedValue result = BuiltinsGlobal::PrintEntrypoint(nullptr);
    EXPECT_EQ(result, JSTaggedValue::Undefined());
}

HWTEST_F_L0(BuiltinsGlobalTest, CallJsProxy)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> emptyObj(thread, NewJSObject(thread, factory, env).GetTaggedValue());
    JSHandle<JSProxy> proxy = factory->NewJSProxy(emptyObj, emptyObj);
    proxy->SetCallable(false);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(proxy.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsGlobal::CallJsProxy(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(thread->HasPendingException());
    EXPECT_EQ(result, JSTaggedValue::Undefined());
    thread->ClearException();
}

HWTEST_F_L0(BuiltinsGlobalTest, Escape)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> str1 = factory->NewFromASCII("?!=()#%&");
    auto ecmaRuntimeCallInfo1 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // NOLINT, 6 means 3 paras
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, str1.GetTaggedValue());

    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result1 = BuiltinsGlobal::Escape(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev1);
    EXPECT_TRUE(result1.IsString());
    JSHandle<EcmaString> ecmaStrHandle1(thread, result1);
    EXPECT_STREQ("%3F%21%3D%28%29%23%25%26", EcmaStringAccessor(ecmaStrHandle1).ToCString().c_str()); // NOLINT

    JSHandle<EcmaString> str2 = factory->NewFromASCII("%u%u0%u9%ua%uF%u00%u09%u0f%u0F%u000%u00a%u00F");
    auto ecmaRuntimeCallInfo2 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // NOLINT
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetCallArg(0, str2.GetTaggedValue()); // NOLINT

    [[maybe_unused]] auto prev2 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    JSTaggedValue result2 = BuiltinsGlobal::Escape(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev2);
    EXPECT_TRUE(result2.IsString());
    JSHandle<EcmaString> ecmaStrHandle2(thread, result2);
    EXPECT_STREQ("%25u%25u0%25u9%25ua%25uF%25u00%25u09%25u0f%25u0F%25u000%25u00a%25u00F", // NOLINT special value
        EcmaStringAccessor(ecmaStrHandle2).ToCString().c_str());

    JSHandle<EcmaString> str3 = factory->NewFromASCII("Hello World!");
    auto ecmaRuntimeCallInfo3 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // NOLINT
    ecmaRuntimeCallInfo3->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetCallArg(0, str3.GetTaggedValue());

    [[maybe_unused]] auto prev3 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3);
    JSTaggedValue result3 = BuiltinsGlobal::Escape(ecmaRuntimeCallInfo3);
    TestHelper::TearDownFrame(thread, prev3);
    EXPECT_TRUE(result3.IsString());
    JSHandle<EcmaString> ecmaStrHandle3(thread, result3);
    EXPECT_STREQ("Hello%20World%21", EcmaStringAccessor(ecmaStrHandle3).ToCString().c_str());
}

HWTEST_F_L0(BuiltinsGlobalTest, Unescape)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> str1 = factory->NewFromASCII("");
    auto ecmaRuntimeCallInfo1 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // NOLINT
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, str1.GetTaggedValue());

    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result1 = BuiltinsGlobal::Unescape(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev1);
    EXPECT_TRUE(result1.IsString());
    JSHandle<EcmaString> ecmaStrHandle1(thread, result1);
    EXPECT_STREQ("", EcmaStringAccessor(ecmaStrHandle1).ToCString().c_str());

    JSHandle<EcmaString> str2 = factory->NewFromASCII("%u%u0%u9%ua%uF%u00%u09%u0f%u0F%u000%u00a%u00F");
    auto ecmaRuntimeCallInfo2 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // NOLINT
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetCallArg(0, str2.GetTaggedValue()); // NOLINT

    [[maybe_unused]] auto prev2 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    JSTaggedValue result2 = BuiltinsGlobal::Unescape(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev2);
    EXPECT_TRUE(result2.IsString());
    JSHandle<EcmaString> ecmaStrHandle2(thread, result2);
    EXPECT_STREQ("%u%u0%u9%ua%uF%u00%u09%u0f%u0F%u000%u00a%u00F",
        EcmaStringAccessor(ecmaStrHandle2).ToCString().c_str());

    JSHandle<EcmaString> str3 = factory->NewFromASCII("Hello%20World%21");
    auto ecmaRuntimeCallInfo3 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // NOLINT 6 means 3 paras
    ecmaRuntimeCallInfo3->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetCallArg(0, str3.GetTaggedValue());

    [[maybe_unused]] auto prev3 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3);
    JSTaggedValue result3 = BuiltinsGlobal::Escape(ecmaRuntimeCallInfo3);
    TestHelper::TearDownFrame(thread, prev3);
    EXPECT_TRUE(result3.IsString());
    JSHandle<EcmaString> ecmaStrHandle3(thread, result3);
    EXPECT_STREQ("Hello%2520World%2521", EcmaStringAccessor(ecmaStrHandle3).ToCString().c_str());
}
}  // namespace panda::test
