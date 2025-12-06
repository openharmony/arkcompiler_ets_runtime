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

#include <chrono>
#include <thread>

#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"

#include "ecmascript/builtins/builtins_ark_tools.h"
#include "ecmascript/containers/containers_bitvector.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_api/js_api_bitvector.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/mem/full_gc.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/partial_gc.h"
#include "ecmascript/mem/sparse_space.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_marker.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_sweeper.h"
#include "ecmascript/mem/gc_key_stats.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/mem/allocation_inspector.h"
#include "ecmascript/dfx/hprof/heap_sampling.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;

using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;

namespace panda::test {
class GCTest : public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
        auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
        heap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::ENABLE);
        heap->GetSweeper()->EnableConcurrentSweep(EnableConcurrentSweepType::ENABLE);
    }
};

void MockDaemonTaskRunner()
{
    DaemonThread *dThread = DaemonThread::GetInstance();
    {
        SuspendAllScope scope(dThread);
        dThread->FinishRunningTask();
    }
}

class MockForTestTask : public DaemonTask {
public:
    explicit MockForTestTask(JSThread *thread)
        : DaemonTask(thread, DaemonTaskType::MOCK_FOR_TEST, DaemonTaskGroup::MOCK_FOR_TEST_GROUP,
                     &MockDaemonTaskRunner, nullptr) {}
    ~MockForTestTask() override = default;
};

void MockDaemonTaskRunner2()
{
    DaemonThread *dThread = DaemonThread::GetInstance();
    {
        SuspendAllScope scope(dThread);
    }
    {
        SuspendAllScope scope(dThread);
        dThread->FinishRunningTask();
    }
}

class MockForTestTask2 : public DaemonTask {
public:
    explicit MockForTestTask2(JSThread *thread)
        : DaemonTask(thread, DaemonTaskType::MOCK_FOR_TEST, DaemonTaskGroup::MOCK_FOR_TEST_GROUP,
                     &MockDaemonTaskRunner2, nullptr) {}
    ~MockForTestTask2() override = default;
};

HWTEST_F_L0(GCTest, PostTaskWithDaemon)
{
    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);

    DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SUCCESS);
}

HWTEST_F_L0(GCTest, PostTaskTwiceWithDaemon1)
{
    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);

    DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SUCCESS);

    result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SAME_GROUP_TASK_ALREADY_POSTED);
}

HWTEST_F_L0(GCTest, PostTaskTwiceWithDaemon2)
{
    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);

    DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask2(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SUCCESS);

    result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask2(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SAME_GROUP_TASK_ALREADY_POSTED);

    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
}

HWTEST_F_L0(GCTest, PostTaskTwiceWithDaemon3)
{
    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);

    DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SUCCESS);

    result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask2(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SAME_GROUP_TASK_ALREADY_POSTED);

    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
}

HWTEST_F_L0(GCTest, PostTaskTwiceWithDaemon4)
{
    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);

    DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask2(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SUCCESS);

    result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SAME_GROUP_TASK_ALREADY_POSTED);

    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
}

HWTEST_F_L0(GCTest, PostTaskTwiceWithDaemonMultiThreads1)
{
    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);

    Mutex mutex;
    ConditionVariable cv;
    size_t initialized = 0;
    size_t tryPost = 0;
    size_t success = 0;
    size_t duplicate = 0;
    constexpr size_t NUM_THREADS = 2;
    auto initializeGuard = [&mutex, &cv, &initialized]() {
        LockHolder lock(mutex);
        if (++initialized == NUM_THREADS) {
            cv.SignalAll();
        }
        while (initialized != NUM_THREADS) {
            cv.Wait(&mutex);
        }
    };
    auto checkResult = [&mutex, &cv, &tryPost, &success, &duplicate](DaemonThread::PostTaskResult result) {
        LockHolder lock(mutex);
        ++tryPost;
        if (result == DaemonThread::PostTaskResult::SUCCESS) {
            ++success;
        }
        if (result == DaemonThread::PostTaskResult::SAME_GROUP_TASK_ALREADY_POSTED) {
            ++duplicate;
        }
        if (tryPost == NUM_THREADS) {
            cv.SignalAll();
        }
    };

    std::thread t1([&initializeGuard, &checkResult]() {
        JSRuntimeOptions runtimeOptions;
        EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);
        initializeGuard();
    
        DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(
            MockForTestTask(vm->GetJSThread()));
        ASSERT_TRUE(result != DaemonThread::PostTaskResult::DAEMON_THREAD_NOT_RUNNING);
        checkResult(result);

        JSNApi::DestroyJSVM(vm);
    });
    std::thread t2([&initializeGuard, &checkResult]() {
        JSRuntimeOptions runtimeOptions;
        EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);
        initializeGuard();

        DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(
            MockForTestTask(vm->GetJSThread()));
        ASSERT_TRUE(result != DaemonThread::PostTaskResult::DAEMON_THREAD_NOT_RUNNING);
        checkResult(result);

        JSNApi::DestroyJSVM(vm);
    });

    {
        LockHolder lock(mutex);
        while (tryPost != NUM_THREADS) {
            cv.Wait(&mutex);
        }
        ASSERT(tryPost == NUM_THREADS);
        ASSERT(success == 1);
        ASSERT(duplicate == 1);
    }
    {
        ThreadNativeScope nativeScope(thread);
        t1.join();
        t2.join();
    }

    DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SUCCESS);
}

HWTEST_F_L0(GCTest, PostTaskTwiceWithDaemonMultiThreads2)
{
    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);

    Mutex mutex;
    ConditionVariable cv;
    size_t initialized = 0;
    bool interrupted = true;
    size_t tryPost = 0;
    constexpr size_t NUM_THREADS = 2;
    auto initializeGuard = [&mutex, &cv, &initialized, &interrupted]() {
        LockHolder lock(mutex);
        if (++initialized == NUM_THREADS) {
            cv.SignalAll();
        }
        while (interrupted) {
            cv.Wait(&mutex);
        }
    };
    auto postGuard = [&mutex, &cv, &tryPost]() {
        LockHolder lock(mutex);
        if (++tryPost == NUM_THREADS) {
            cv.SignalAll();
        }
    };

    std::thread t1([&initializeGuard, &postGuard]() {
        JSRuntimeOptions runtimeOptions;
        EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);
        initializeGuard();
    
        DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(
            MockForTestTask(vm->GetJSThread()));
        ASSERT_TRUE(result == DaemonThread::PostTaskResult::SAME_GROUP_TASK_ALREADY_POSTED);
        postGuard();

        JSNApi::DestroyJSVM(vm);
    });

    std::thread t2([&initializeGuard, &postGuard]() {
        JSRuntimeOptions runtimeOptions;
        EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);
        initializeGuard();

        DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(
            MockForTestTask(vm->GetJSThread()));
        ASSERT_TRUE(result == DaemonThread::PostTaskResult::SAME_GROUP_TASK_ALREADY_POSTED);
        postGuard();

        JSNApi::DestroyJSVM(vm);
    });

    {
        LockHolder lock(mutex);
        while (initialized != NUM_THREADS) {
            cv.Wait(&mutex);
        }
    }
    DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SUCCESS);

    result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SAME_GROUP_TASK_ALREADY_POSTED);

    {
        LockHolder lock(mutex);
        interrupted = false;
        cv.SignalAll();
        while (tryPost != NUM_THREADS) {
            cv.Wait(&mutex);
        }
    }
    {
        ThreadNativeScope nativeScope(thread);
        t1.join();
        t2.join();
    }

    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    std::thread t3([]() {
        JSRuntimeOptions runtimeOptions;
        EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);

        DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(
            MockForTestTask(vm->GetJSThread()));
        ASSERT_TRUE(result == DaemonThread::PostTaskResult::SUCCESS);

        JSNApi::DestroyJSVM(vm);
    });

    {
        ThreadNativeScope nativeScope(thread);
        t3.join();
    }

    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SUCCESS);
}

HWTEST_F_L0(GCTest, PostTaskWithoutDaemon1)
{
    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);

    JSNApi::PreFork(instance);
    DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::DAEMON_THREAD_NOT_RUNNING);

    {
        ThreadNativeScope nativeScope(thread);
        panda::RuntimeOption postOption;
        JSNApi::PostFork(instance, postOption);
    }
    ASSERT_TRUE(thread->IsInRunningState());
}

HWTEST_F_L0(GCTest, PostTaskWithoutDaemon2)
{
    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);

    JSNApi::PreFork(instance);
    DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(
        TriggerCollectGarbageTask<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::DAEMON_THREAD_NOT_RUNNING);

    {
        ThreadNativeScope nativeScope(thread);
        panda::RuntimeOption postOption;
        JSNApi::PostFork(instance, postOption);
    }
    ASSERT_TRUE(thread->IsInRunningState());
}

HWTEST_F_L0(GCTest, PostTaskWithoutDaemon3)
{
    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);

    JSNApi::PreFork(instance);
    DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(
        TriggerConcurrentMarkTask<TriggerGCType::SHARED_GC, MarkReason::OTHER>(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::DAEMON_THREAD_NOT_RUNNING);

    {
        ThreadNativeScope nativeScope(thread);
        panda::RuntimeOption postOption;
        JSNApi::PostFork(instance, postOption);
    }
    ASSERT_TRUE(thread->IsInRunningState());
}

HWTEST_F_L0(GCTest, PostTaskWithoutDaemonWithMultiThreads)
{
    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);

    JSNApi::PreFork(instance);

    DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::DAEMON_THREAD_NOT_RUNNING);

    std::thread t1([]() {
        JSRuntimeOptions runtimeOptions;
        EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);

        DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(
            MockForTestTask(vm->GetJSThread()));
        ASSERT_TRUE(result == DaemonThread::PostTaskResult::DAEMON_THREAD_NOT_RUNNING);

        JSNApi::DestroyJSVM(vm);
    });

    std::thread t2([]() {
        JSRuntimeOptions runtimeOptions;
        EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);

        DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(
            MockForTestTask(vm->GetJSThread()));
        ASSERT_TRUE(result == DaemonThread::PostTaskResult::DAEMON_THREAD_NOT_RUNNING);

        JSNApi::DestroyJSVM(vm);
    });

    {
        ThreadNativeScope nativeScope(thread);
        t1.join();
        t2.join();
    }

    {
        ThreadNativeScope nativeScope(thread);
        panda::RuntimeOption postOption;
        JSNApi::PostFork(instance, postOption);
    }
    ASSERT_TRUE(thread->IsInRunningState());

    result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SUCCESS);
    result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SAME_GROUP_TASK_ALREADY_POSTED);
}

HWTEST_F_L0(GCTest, CollectGarbageWithoutDaemonWithMultiThreads1)
{
    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);

    JSNApi::PreFork(instance);

    DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::DAEMON_THREAD_NOT_RUNNING);
    result = DaemonThread::GetInstance()->CheckAndPostTask(
        TriggerCollectGarbageTask<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::DAEMON_THREAD_NOT_RUNNING);

    std::thread t1([]() {
        JSRuntimeOptions runtimeOptions;
        EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);

        DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(
            TriggerCollectGarbageTask<TriggerGCType::SHARED_GC, GCReason::OTHER>(vm->GetJSThread()));
        ASSERT_TRUE(result == DaemonThread::PostTaskResult::DAEMON_THREAD_NOT_RUNNING);

        JSNApi::DestroyJSVM(vm);
    });

    std::thread t2([]() {
        JSRuntimeOptions runtimeOptions;
        EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);

        DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(
            TriggerCollectGarbageTask<TriggerGCType::SHARED_GC, GCReason::OTHER>(vm->GetJSThread()));
        ASSERT_TRUE(result == DaemonThread::PostTaskResult::DAEMON_THREAD_NOT_RUNNING);

        JSNApi::DestroyJSVM(vm);
    });

    {
        ThreadNativeScope nativeScope(thread);
        t1.join();
        t2.join();
    }

    {
        ThreadNativeScope nativeScope(thread);
        panda::RuntimeOption postOption;
        JSNApi::PostFork(instance, postOption);
    }
    ASSERT_TRUE(thread->IsInRunningState());

    result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SUCCESS);
    result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SAME_GROUP_TASK_ALREADY_POSTED);
}

HWTEST_F_L0(GCTest, CollectGarbageWithoutDaemonWithMultiThreads2)
{
    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);

    DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SUCCESS);
    result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SAME_GROUP_TASK_ALREADY_POSTED);

    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);

    std::thread t1([]() {
        JSRuntimeOptions runtimeOptions;
        EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);

        DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(
            MockForTestTask(vm->GetJSThread()));
        ASSERT_TRUE(result != DaemonThread::PostTaskResult::DAEMON_THREAD_NOT_RUNNING);

        JSNApi::DestroyJSVM(vm);
    });

    std::thread t2([]() {
        JSRuntimeOptions runtimeOptions;
        EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);

        DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(
            MockForTestTask(vm->GetJSThread()));
        ASSERT_TRUE(result != DaemonThread::PostTaskResult::DAEMON_THREAD_NOT_RUNNING);

        JSNApi::DestroyJSVM(vm);
    });

    {
        ThreadNativeScope nativeScope(thread);
        t1.join();
        t2.join();
    }

    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    JSNApi::PreFork(instance);

    {
        ThreadNativeScope nativeScope(thread);
        panda::RuntimeOption postOption;
        JSNApi::PostFork(instance, postOption);
    }
    ASSERT_TRUE(thread->IsInRunningState());

    result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SUCCESS);
    result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SAME_GROUP_TASK_ALREADY_POSTED);
}

HWTEST_F_L0(GCTest, CollectGarbageWithoutDaemonWithMultiThreads3)
{
    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);

    DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SUCCESS);
    result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask2(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SAME_GROUP_TASK_ALREADY_POSTED);

    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);

    std::thread t1([]() {
        JSRuntimeOptions runtimeOptions;
        EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);

        DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(
            MockForTestTask(vm->GetJSThread()));
        ASSERT_TRUE(result != DaemonThread::PostTaskResult::DAEMON_THREAD_NOT_RUNNING);

        JSNApi::DestroyJSVM(vm);
    });

    {
        ThreadNativeScope nativeScope(thread);
        t1.join();
    }

    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    JSNApi::PreFork(instance);

    std::thread t2([]() {
        JSRuntimeOptions runtimeOptions;
        EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);

        DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(
            TriggerCollectGarbageTask<TriggerGCType::SHARED_GC, GCReason::OTHER>(vm->GetJSThread()));
        ASSERT_TRUE(result == DaemonThread::PostTaskResult::DAEMON_THREAD_NOT_RUNNING);

        JSNApi::DestroyJSVM(vm);
    });

    {
        ThreadNativeScope nativeScope(thread);
        t2.join();
    }

    {
        ThreadNativeScope nativeScope(thread);
        panda::RuntimeOption postOption;
        JSNApi::PostFork(instance, postOption);
    }
    ASSERT_TRUE(thread->IsInRunningState());

    result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SUCCESS);
    result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SAME_GROUP_TASK_ALREADY_POSTED);
}
} // namespace panda::test