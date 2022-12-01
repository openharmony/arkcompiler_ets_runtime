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

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
namespace panda::test {
class SlowRuntimeStubTest : public testing::Test {
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
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

static JSFunction *JSObjectTestCreate(JSThread *thread)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    return globalEnv->GetObjectFunction().GetObject<JSFunction>();
}

HWTEST_F_L0(SlowRuntimeStubTest, StPatchVar)
{
    JSTaggedValue result = SlowRuntimeStub::StPatchVar(thread, 0, JSTaggedValue::True());
    ASSERT_TRUE(result.IsTrue());
}

HWTEST_F_L0(SlowRuntimeStubTest, LdPatchVar)
{
    JSTaggedValue result = SlowRuntimeStub::StPatchVar(thread, 0, JSTaggedValue::False());
    ASSERT_TRUE(result.IsTrue());
    JSTaggedValue res = SlowRuntimeStub::LdPatchVar(thread, 0);
    ASSERT_TRUE(res.IsFalse());
}

HWTEST_F_L0(SlowRuntimeStubTest, StObjByIndex)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. handler has no "GetPrototypeOf"
    JSHandle<JSTaggedValue> hclass(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> proto(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(hclass), hclass));
    JSTaggedValue result = SlowRuntimeStub::StObjByIndex(thread, proto.GetTaggedValue(), 0, JSTaggedValue::False());
    ASSERT_TRUE(result.IsTrue());
}

HWTEST_F_L0(SlowRuntimeStubTest, LdObjByIndex)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. handler has no "GetPrototypeOf"
    JSHandle<JSTaggedValue> hclass(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> proto(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(hclass), hclass));
    JSTaggedValue result = SlowRuntimeStub::StObjByIndex(thread, proto.GetTaggedValue(), 0, JSTaggedValue::False());
    ASSERT_TRUE(result.IsTrue());
    JSTaggedValue res = SlowRuntimeStub::LdObjByIndex(thread, proto.GetTaggedValue(), 0, false, JSTaggedValue::False());
    ASSERT_TRUE(res.IsFalse());
}

HWTEST_F_L0(SlowRuntimeStubTest, ThrowTypeError)
{
    JSTaggedValue result = SlowRuntimeStub::ThrowTypeError(thread, "Typeerror");
    ASSERT_TRUE(result.IsException());
}
}  // namespace panda::test
