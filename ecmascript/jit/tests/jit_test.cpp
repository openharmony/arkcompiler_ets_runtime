/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/global_env.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/jit/jit_task.h"

using namespace panda::ecmascript;

namespace panda::test {
class JitTest : public testing::Test {
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
        TestHelper::CreateEcmaVMWithScope(instance_, thread_, scope_, false, false, true, true);
        thread_ = instance_->GetJSThread();
        ThreadNativeScope scope(thread_);
        JitTaskpool::GetCurrentTaskpool()->WaitForJitTaskPoolReady();
        compilerVm_ = JitTaskpool::GetCurrentTaskpool()->GetCompilerVm();
        jit_ = Jit::GetInstance();
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance_, scope_);
    }

    EcmaVM *instance_ {nullptr};
    EcmaHandleScope *scope_ {nullptr};
    JSThread *thread_ {nullptr};
    EcmaVM *compilerVm_ {nullptr};
    Jit *jit_ {nullptr};
};

/**
 * @tc.name: IsEnableFastJit
 * @tc.desc: check jit is enable fast.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JitTest, IsEnableFastJit)
{
    EXPECT_TRUE(jit_->IsEnableFastJit());
}

/**
 * @tc.name: NewJitTask
 * @tc.desc: check new jit task.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JitTest, NewJitTask)
{
    JSHandle<JSFunction> emptyHandle;
    CString methodName("methodName");
    std::shared_ptr<JitTask> jitTask = std::make_shared<JitTask>(thread_, compilerVm_->GetJSThreadNoCheck(),
        jit_, emptyHandle, CompilerTier(CompilerTier::Tier::FAST), methodName, 0,
        JitCompileMode(JitCompileMode::Mode::SYNC));
    EXPECT_TRUE(jitTask->GetHostThread() == thread_);
}
}  // namespace panda::test
