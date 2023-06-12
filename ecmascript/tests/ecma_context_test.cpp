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
    CVector<EcmaContext *> Cv1 = thread->GetEcmaContexts();
    EXPECT_EQ(Cv1.size(), 1);
    auto context2 = EcmaContext::Create(thread);
    Cv1 = thread->GetEcmaContexts();
    EXPECT_EQ(Cv1.size(), 1);
    thread->PushContext(context2);
    Cv1 = thread->GetEcmaContexts();
    EXPECT_EQ(Cv1.size(), 2);  // 2: size of contexts.
}

HWTEST_F_L0(EcmaContextTest, CreatePushContext)
{
    auto context = EcmaContext::Create(thread);
    auto context1 = EcmaContext::Create(thread);
    thread->PushContext(context1);
    CVector<EcmaContext *> context3 = thread->GetEcmaContexts();
    EXPECT_EQ(context3.size(), 2);  // 2: size of contexts.
    thread->PushContext(context);
    context3 = thread->GetEcmaContexts();
    EXPECT_EQ(context3.size(), 3);  // 3: size of contexts.
    thread->PopContext();
    context3 = thread->GetEcmaContexts();
    EXPECT_EQ(context3.size(), 2);  // 2: size of contexts.
}

HWTEST_F_L0(EcmaContextTest, GetRegExpCache)
{
    EcmaVM *vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    auto context = EcmaContext::Create(thread);
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
}  // namespace panda::test