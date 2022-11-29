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
}  // namespace panda::test
