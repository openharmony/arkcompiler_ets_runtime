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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/log.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/checkpoint/thread_state_transition.h"

#include <csetjmp>
#include <csignal>
using namespace panda::ecmascript;

namespace panda::test {

class StateTransitioningTest : public testing::Test {
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
        // InitializeLogger();
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void InitializeLogger()
    {
        panda::ecmascript::JSRuntimeOptions runtimeOptions;
        runtimeOptions.SetLogLevel("error");
        ecmascript::Log::Initialize(runtimeOptions);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    static void NewVMThreadEntry(EcmaVM *newVm, bool nativeState, std::atomic<bool> *isTestEnded, std::atomic<size_t> *activeThreadCount)
    {
        panda::RuntimeOption postOption;
        JSNApi::PostFork(newVm, postOption);
        activeThreadCount->fetch_add(1);
        if (nativeState) {
            ThreadNativeScope nativeScope(JSThread::GetCurrent());
            while (!isTestEnded->load()) {}
        } else {
            while (!isTestEnded->load()) {
                JSThread::GetCurrent()->CheckSafepoint();
            }
        }
        JSNApi::DestroyJSVM(newVm);
        activeThreadCount->fetch_sub(1);
    }

    void CreateNewVMInSeparateThread(bool nativeState)
    {
        RuntimeOption options;
        EcmaVM *newVm = JSNApi::CreateJSVM(options);
        vms.push_back(newVm);
        JSNApi::PreFork(newVm);
        size_t oldCount = activeThreadCount;
        std::thread *worker_thread = new std::thread(StateTransitioningTest::NewVMThreadEntry, newVm, nativeState, &isTestEnded, &activeThreadCount);
        threads.push_back(worker_thread);
        while (activeThreadCount == oldCount) {}
    }

    void DestroyAllVMs()
    {
        isTestEnded = true;
        while (activeThreadCount != 0) {}
        for (auto i: threads) {
            i->join();
            delete(i);
        }
    }

    std::list<EcmaVM *> vms;
    std::list<std::thread *> threads;
    std::atomic<size_t> activeThreadCount {0};
    std::atomic<bool> isTestEnded {false};
    JSThread *thread {nullptr};
    EcmaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
};

HWTEST_F_L0(StateTransitioningTest, ChangeStateTest)
{
    {
        ThreadNativeScope nativeScope(thread);
        ASSERT(!Runtime::GetInstance()->GetMutatorLock()->HasLock());
    }
    {
        ThreadManagedScope managedScope(thread);
        ASSERT(Runtime::GetInstance()->GetMutatorLock()->HasLock());
    }
    {
        ThreadNativeScope nativeScope(thread);
        ASSERT(!Runtime::GetInstance()->GetMutatorLock()->HasLock());
        {
            ThreadManagedScope managedScope(thread);
            ASSERT(Runtime::GetInstance()->GetMutatorLock()->HasLock());
        }
    }
}

HWTEST_F_L0(StateTransitioningTest, SuspendAllManagedTest)
{
    CreateNewVMInSeparateThread(false);
    {
        SuspendAllScope suspendScope(JSThread::GetCurrent());
    }
    DestroyAllVMs();
}

HWTEST_F_L0(StateTransitioningTest, SuspendAllNativeTest)
{
    CreateNewVMInSeparateThread(true);
    {
        SuspendAllScope suspendScope(JSThread::GetCurrent());
    }
    DestroyAllVMs();
}
}  // namespace panda::test