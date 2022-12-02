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

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
namespace panda::test {
class RuntimeStubsTest : public testing::Test {
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
        TestHelper::CreateEcmaVMWithScope(ecmaVm, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(ecmaVm, scope);
    }

    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
    EcmaVM *ecmaVm = nullptr;
};

static JSFunction *JSObjectTestCreate(JSThread *thread)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    return globalEnv->GetObjectFunction().GetObject<JSFunction>();
}

HWTEST_F_L0(RuntimeStubsTest, GetSymbolFunction)
{
    auto glue = thread->GetGlueAddr();
    JSTaggedValue result =
        JSTaggedValue(RuntimeStubs::GetSymbolFunction(glue, 0, reinterpret_cast<uintptr_t>(nullptr)));
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    JSTaggedValue symbol = globalEnv->GetSymbolFunction().GetTaggedValue();
    ASSERT_TRUE(result == symbol);
}

HWTEST_F_L0(RuntimeStubsTest, CallGetPrototype)
{
    auto glue = thread->GetGlueAddr();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> hclass(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> proto(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(hclass), hclass));
    JSHandle<JSTaggedValue> targetHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(hclass), hclass));
    EXPECT_TRUE(targetHandle->IsECMAObject());
    JSObject::SetPrototype(thread, JSHandle<JSObject>(targetHandle), proto);

    JSHandle<JSTaggedValue> handlerHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(hclass), hclass));
    EXPECT_TRUE(handlerHandle->IsECMAObject());
    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);

    JSProxy *proxy = *proxyHandle;
    JSTaggedValue result =
        JSTaggedValue(RuntimeStubs::CallGetPrototype(glue, 1, reinterpret_cast<uintptr_t>(&proxy)));

    ASSERT_TRUE(JSTaggedValue::SameValue(result, proto.GetTaggedValue()));
}

HWTEST_F_L0(RuntimeStubsTest, GetTaggedArrayPtrTest)
{
    auto glue = thread->GetGlueAddr();
    JSTaggedType arr = JSTaggedValue::VALUE_UNDEFINED;
    JSTaggedValue result =
        JSTaggedValue(RuntimeStubs::GetTaggedArrayPtrTest(glue, 1, reinterpret_cast<uintptr_t>(&arr)));
    ASSERT_TRUE(result.IsTaggedArray());
}

HWTEST_F_L0(RuntimeStubsTest, ThrowDeleteSuperProperty)
{
    auto glue = thread->GetGlueAddr();
    JSTaggedValue result = 
        JSTaggedValue(RuntimeStubs::ThrowDeleteSuperProperty(glue, 0, reinterpret_cast<uintptr_t>(nullptr)));
    ASSERT_TRUE(result.IsHole());
}

HWTEST_F_L0(RuntimeStubsTest, ThrowNonConstructorException)
{
    auto glue = thread->GetGlueAddr();
    JSTaggedValue result =
        JSTaggedValue(RuntimeStubs::ThrowNonConstructorException(glue, 0, reinterpret_cast<uintptr_t>(nullptr)));
    ASSERT_TRUE(result.IsException());
}
}  // namespace panda::test
