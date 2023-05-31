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

// #include "ecmascript/builtins/builtins.h"
#include "ecmascript/ecma_context.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/global_env.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;
namespace panda::test {
class EcmaContextTest : public testing::Test {
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
        TestHelper::CreateEcmaVMWithScope(ecmaVMPtr, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(ecmaVMPtr, scope);
    }

    EcmaVM *ecmaVMPtr {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(EcmaContextTest, Create)
{
    [[maybe_unused]]auto context = EcmaContext::Create(thread);
    CVector<EcmaContext *> context3 = thread->GetEcmaContexts();
    EXPECT_EQ(context3.size(), 1);
    [[maybe_unused]]auto context2 = EcmaContext::MountContext(thread);
    EXPECT_EQ(context3.size(), 2);

}

HWTEST_F_L0(EcmaContextTest, GetEcmaStringTable)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    auto context = EcmaContext::Create(thread);
    EcmaStringTable *table = context->GetEcmaStringTable();
    JSHandle<EcmaString> ecmaStrCreateHandle = factory->NewFromASCII("hello world");
    EcmaString *ecmaStrGetPtr = table->GetOrInternString(*ecmaStrCreateHandle);
    EXPECT_STREQ(EcmaStringAccessor(ecmaStrGetPtr).ToCString().c_str(), "hello world");
}

HWTEST_F_L0(EcmaContextTest, CreatePushContext)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    auto context = EcmaContext::Create(thread);
    auto context1 = EcmaContext::Create(thread);
    EcmaStringTable *table1 = context->GetEcmaStringTable();
    JSHandle<EcmaString> ecmaStrCreateHandle1 = factory->NewFromASCII("hello world");
    EcmaString *ecmaStrGetPtr1 = table1->GetOrInternString(*ecmaStrCreateHandle1);
    EXPECT_STREQ(EcmaStringAccessor(ecmaStrGetPtr1).ToCString().c_str(), "hello world");
    EcmaStringTable *table2 = context1->GetEcmaStringTable();
    JSHandle<EcmaString> ecmaStrCreateHandle2 = factory->NewFromASCII("hello world2");
    EcmaString *ecmaStrGetPtr2 = table2->GetOrInternString(*ecmaStrCreateHandle2);
    EXPECT_STREQ(EcmaStringAccessor(ecmaStrGetPtr2).ToCString().c_str(), "hello world2");
    CVector<EcmaContext *> context3 = thread->GetEcmaContexts();
    EXPECT_EQ(context3.size(), 2);
    thread->PushContext(context);
    EXPECT_EQ(context3.size(), 3);
    thread->PopContext();
    EXPECT_EQ(context3.size(), 2);   
}

HWTEST_F_L0(EcmaContextTest, GetRegExpCache)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    auto context = EcmaContext::Create(thread);
    JSHandle<JSTaggedValue> value = context->GetRegExpCache();
    JSTaggedValue res = value.GetTaggedValue();
    EXPECT_EQ(res, JSTaggedValue::Null());
    JSHandle<EcmaString> regexp = factory->NewFromASCII("\\g");
    JSHandle<JSTaggedValue> value2(regexp);
    context->SetRegExpCache(value2.GetTaggedValue());
    JSHandle<JSTaggedValue> res2 = context->GetRegExpCache();
    EXPECT_EQ(res2.GetTaggedValue(), value2.GetTaggedValue());
}

HWTEST_F_L0(EcmaContextTest, AllowAtomicWait)
{
    auto context = EcmaContext::Create(thread);
    bool value = context->GetAllowAtomicWait();
    EXPECT_TRUE(value);
    context->SetAllowAtomicWait(false);
    bool value2 = context->GetAllowAtomicWait();
    EXPECT_FALSE(value2);
}

HWTEST_F_L0(EcmaContextTest, SwitchCurrentContext)
{
    auto context1 = EcmaContext::Create(thread);
    context1->SetAllowAtomicWait(false);
    bool value1 = thread->GetCurrentEcmaContext()->GetAllowAtomicWait();
    EXPECT_FALSE(value1);
    auto context2 = EcmaContext::Create(thread);
    bool wait = context2->GetAllowAtomicWait();
    EXPECT_TRUE(wait);
    thread->SwitchCurrentContext(context2);
    bool value2 = thread->GetCurrentEcmaContext()->GetAllowAtomicWait();
    EXPECT_TRUE(value2);
    thread->SwitchCurrentContext(context1);
    bool value3 = thread->GetCurrentEcmaContext()->GetAllowAtomicWait();
    EXPECT_FALSE(value3);
}


}  // namespace panda::test