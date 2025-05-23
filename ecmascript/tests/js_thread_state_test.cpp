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
#include "ecmascript/js_thread.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/checkpoint/thread_state_transition.h"

#include <csetjmp>
#include <csignal>
using namespace panda::ecmascript;

namespace panda::test {

class StateTransitioningTest : public BaseTestWithScope<false> {
public:
    void InitializeLogger()
    {
        panda::ecmascript::JSRuntimeOptions runtimeOptions;
        runtimeOptions.SetLogLevel("error");
        Log::Initialize(runtimeOptions.GetLogOptions());
    }

    static void NewVMThreadEntry(EcmaVM *newVm,
                                 bool nativeState,
                                 std::atomic<bool> *changeToRunning,
                                 std::atomic<bool> *isTestEnded,
                                 std::atomic<size_t> *activeThreadCount)
    {
        panda::RuntimeOption postOption;
        JSNApi::PostFork(newVm, postOption);
        {
            JSThread *thread = JSThread::GetCurrent();
            ThreadManagedScope managedScope(thread);
            if (nativeState) {
                ThreadNativeScope nativeScope(thread);
                activeThreadCount->fetch_add(1);
                while (!isTestEnded->load() && !changeToRunning->load()) {}
                ThreadManagedScope secondManagedScope(thread);
                while (!isTestEnded->load()) {}
            } else {
                activeThreadCount->fetch_add(1);
                while (!isTestEnded->load()) {
                    JSThread::GetCurrent()->CheckSafepoint();
                }
            }
        }
        JSNApi::DestroyJSVM(newVm);
        activeThreadCount->fetch_sub(1);
    }

    void CreateNewVMInSeparateThread(bool nativeState)
    {
        std::thread t1([&]() {
            RuntimeOption options;
            EcmaVM *newVm = JSNApi::CreateJSVM(options);
            vms.push_back(newVm);
            {
                ThreadManagedScope managedScope(newVm->GetJSThread());
                JSNApi::PreFork(newVm);
            }
            size_t oldCount = activeThreadCount;
            // This case isn't a really fork which causes JSThread::GetCurrentThread() equals nullptr in worker_thread.
            // So reset the threadState as CREATED to skip the check.
            newVm->GetAssociatedJSThread()->UpdateState(ecmascript::ThreadState::CREATED);
            std::thread *worker_thread = new std::thread(StateTransitioningTest::NewVMThreadEntry, newVm, nativeState,
                                                         &changeToRunning, &isTestEnded, &activeThreadCount);
            threads.push_back(worker_thread);
            while (activeThreadCount == oldCount) {
            }
        });
        {
            ThreadSuspensionScope suspensionScope(thread);
            t1.join();
        }
    }

    void ChangeAllThreadsToRunning()
    {
        changeToRunning.store(true);
    }

    bool CheckAllThreadsSuspended()
    {
        bool result = true;
        for (auto i: vms) {
            result &= i->GetAssociatedJSThread()->HasSuspendRequest();
        }
        return result;
    }

    bool CheckAllThreadsState(ecmascript::ThreadState expectedState)
    {
        bool result = true;
        for (auto i: vms) {
            result &= (i->GetAssociatedJSThread()->GetState() == expectedState);
        }
        return result;
    }

    void SuspendOrResumeAllThreads(bool toSuspend)
    {
        for (auto i: vms) {
            if (toSuspend) {
                i->GetAssociatedJSThread()->SuspendThread(false);
            } else {
                i->GetAssociatedJSThread()->ResumeThread(false);
            }
        }
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
    std::atomic<bool> changeToRunning {false};
};

HWTEST_F_L0(StateTransitioningTest, ThreadStateTransitionScopeTest)
{
    ecmascript::ThreadState mainState = thread->GetState();
    {
        ThreadStateTransitionScope<JSThread, ecmascript::ThreadState::CREATED> scope(thread);
        EXPECT_TRUE(thread->GetState() == ecmascript::ThreadState::CREATED);
    }
    EXPECT_TRUE(thread->GetState() == mainState);
    {
        ThreadStateTransitionScope<JSThread, ecmascript::ThreadState::RUNNING> scope(thread);
        EXPECT_TRUE(thread->GetState() == ecmascript::ThreadState::RUNNING);
    }
    EXPECT_TRUE(thread->GetState() == mainState);
    {
        ThreadStateTransitionScope<JSThread, ecmascript::ThreadState::NATIVE> scope(thread);
        EXPECT_TRUE(thread->GetState() == ecmascript::ThreadState::NATIVE);
    }
    EXPECT_TRUE(thread->GetState() == mainState);
    {
        ThreadStateTransitionScope<JSThread, ecmascript::ThreadState::WAIT> scope(thread);
        EXPECT_TRUE(thread->GetState() == ecmascript::ThreadState::WAIT);
    }
    EXPECT_TRUE(thread->GetState() == mainState);
    {
        ThreadStateTransitionScope<JSThread, ecmascript::ThreadState::IS_SUSPENDED> scope(thread);
        EXPECT_TRUE(thread->GetState() == ecmascript::ThreadState::IS_SUSPENDED);
    }
    EXPECT_TRUE(thread->GetState() == mainState);
    {
        ThreadStateTransitionScope<JSThread, ecmascript::ThreadState::TERMINATED> scope(thread);
        EXPECT_TRUE(thread->GetState() == ecmascript::ThreadState::TERMINATED);
    }
    EXPECT_TRUE(thread->GetState() == mainState);
}

HWTEST_F_L0(StateTransitioningTest, ThreadManagedScopeTest)
{
    ecmascript::ThreadState mainState = thread->GetState();
    {
        ThreadManagedScope scope(thread);
        EXPECT_TRUE(thread->GetState() == ecmascript::ThreadState::RUNNING);
    }
    if (mainState == ecmascript::ThreadState::RUNNING) {
        ThreadStateTransitionScope<JSThread, ecmascript::ThreadState::WAIT> tempScope(thread);
        {
            ThreadManagedScope scope(thread);
            EXPECT_TRUE(thread->GetState() == ecmascript::ThreadState::RUNNING);
        }
        EXPECT_TRUE(thread->GetState() == ecmascript::ThreadState::WAIT);
    }
    EXPECT_TRUE(thread->GetState() == mainState);
}

HWTEST_F_L0(StateTransitioningTest, ThreadNativeScopeTest)
{
    ecmascript::ThreadState mainState = thread->GetState();
    {
        ThreadNativeScope scope(thread);
        EXPECT_TRUE(thread->GetState() == ecmascript::ThreadState::NATIVE);
    }
    if (mainState == ecmascript::ThreadState::NATIVE) {
        ThreadStateTransitionScope<JSThread, ecmascript::ThreadState::WAIT> tempScope(thread);
        {
            ThreadNativeScope scope(thread);
            EXPECT_TRUE(thread->GetState() == ecmascript::ThreadState::NATIVE);
        }
        EXPECT_TRUE(thread->GetState() == ecmascript::ThreadState::WAIT);
    }
    EXPECT_TRUE(thread->GetState() == mainState);
}

HWTEST_F_L0(StateTransitioningTest, ThreadSuspensionScopeTest)
{
    ecmascript::ThreadState mainState = thread->GetState();
    {
        ThreadSuspensionScope scope(thread);
        EXPECT_TRUE(thread->GetState() == ecmascript::ThreadState::IS_SUSPENDED);
    }
    if (mainState == ecmascript::ThreadState::IS_SUSPENDED) {
        ThreadStateTransitionScope<JSThread, ecmascript::ThreadState::WAIT> tempScope(thread);
        {
            ThreadSuspensionScope scope(thread);
            EXPECT_TRUE(thread->GetState() == ecmascript::ThreadState::IS_SUSPENDED);
        }
        EXPECT_TRUE(thread->GetState() == ecmascript::ThreadState::WAIT);
    }
    EXPECT_TRUE(thread->GetState() == mainState);
}

HWTEST_F_L0(StateTransitioningTest, IsInRunningStateTest)
{
    {
        ThreadNativeScope scope(thread);
        EXPECT_TRUE(!thread->IsInRunningState());
    }
    {
        ThreadManagedScope scope(thread);
        EXPECT_TRUE(thread->IsInRunningState());
    }
}

HWTEST_F_L0(StateTransitioningTest, ChangeStateTest)
{
    {
        ThreadNativeScope nativeScope(thread);
    }
    {
        ThreadNativeScope nativeScope(thread);
        {
            ThreadManagedScope managedScope(thread);
        }
    }
}

HWTEST_F_L0(StateTransitioningTest, SuspendResumeRunningThreadVMTest)
{
    CreateNewVMInSeparateThread(false);
    EXPECT_FALSE(CheckAllThreadsSuspended());
    {
        SuspendOrResumeAllThreads(true);
        while (!CheckAllThreadsState(ecmascript::ThreadState::IS_SUSPENDED)) {}
        EXPECT_TRUE(CheckAllThreadsSuspended());
        EXPECT_TRUE(CheckAllThreadsState(ecmascript::ThreadState::IS_SUSPENDED));
    }
    SuspendOrResumeAllThreads(false);
    while (CheckAllThreadsState(ecmascript::ThreadState::IS_SUSPENDED)) {}
    EXPECT_FALSE(CheckAllThreadsSuspended());
    EXPECT_FALSE(CheckAllThreadsState(ecmascript::ThreadState::IS_SUSPENDED));
    DestroyAllVMs();
}

HWTEST_F_L0(StateTransitioningTest, SuspendAllManagedTest)
{
    CreateNewVMInSeparateThread(false);
    EXPECT_TRUE(CheckAllThreadsState(ecmascript::ThreadState::RUNNING));
    {
        SuspendAllScope suspendScope(JSThread::GetCurrent());
        EXPECT_TRUE(CheckAllThreadsSuspended());
    }
    while (CheckAllThreadsState(ecmascript::ThreadState::IS_SUSPENDED)) {}
    EXPECT_TRUE(CheckAllThreadsState(ecmascript::ThreadState::RUNNING));
    DestroyAllVMs();
}

HWTEST_F_L0(StateTransitioningTest, SuspendAllNativeTest)
{
    CreateNewVMInSeparateThread(true);
    EXPECT_TRUE(CheckAllThreadsState(ecmascript::ThreadState::NATIVE));
    {
        SuspendAllScope suspendScope(JSThread::GetCurrent());
        EXPECT_TRUE(CheckAllThreadsState(ecmascript::ThreadState::NATIVE));
    }
    EXPECT_TRUE(CheckAllThreadsState(ecmascript::ThreadState::NATIVE));
    DestroyAllVMs();
}

HWTEST_F_L0(StateTransitioningTest, SuspendAllNativeTransferToRunningTest)
{
    CreateNewVMInSeparateThread(true);
    EXPECT_TRUE(CheckAllThreadsState(ecmascript::ThreadState::NATIVE));
    {
        SuspendAllScope suspendScope(JSThread::GetCurrent());
        EXPECT_TRUE(CheckAllThreadsState(ecmascript::ThreadState::NATIVE));
        ChangeAllThreadsToRunning();
        while (!CheckAllThreadsState(ecmascript::ThreadState::NATIVE)) {}
        EXPECT_TRUE(CheckAllThreadsState(ecmascript::ThreadState::NATIVE));
    }
    while (CheckAllThreadsState(ecmascript::ThreadState::IS_SUSPENDED)) {}
    while (CheckAllThreadsState(ecmascript::ThreadState::NATIVE)) {}
    EXPECT_TRUE(CheckAllThreadsState(ecmascript::ThreadState::RUNNING));
    DestroyAllVMs();
}
}  // namespace panda::test
