/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/runtime.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda::ecmascript;
namespace panda::test {

class RuntimeTest : public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
    }
};

HWTEST_F_L0(RuntimeTest, RegisterTest)
{
    RuntimeOption option;
    std::thread t1([&]() {
        auto vm1 = JSNApi::CreateJSVM(option);
        ASSERT_TRUE(vm1 != nullptr) << "Cannot create EcmaVM1";
        auto thread1 = vm1->GetJSThread();
        ASSERT_TRUE(thread1 != nullptr);
        EXPECT_EQ(thread1->GetState(), ecmascript::ThreadState::NATIVE);
        JSNApi::DestroyJSVM(vm1);
    });
    
    std::thread t2([&]() {
        auto vm2 = JSNApi::CreateJSVM(option);
        ASSERT_TRUE(vm2 != nullptr) << "Cannot create EcmaVM2";
        auto thread2 = vm2->GetJSThread();
        ASSERT_TRUE(thread2 != nullptr);
        EXPECT_EQ(thread2->GetState(), ecmascript::ThreadState::NATIVE);
        JSNApi::DestroyJSVM(vm2);
    });

    {
        ecmascript::ThreadSuspensionScope scope(thread);
        t1.join();
        t2.join();
    }
}
}
