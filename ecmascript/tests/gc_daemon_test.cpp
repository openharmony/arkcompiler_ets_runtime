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

HWTEST_F_L0(GCTest, PostTaskWithDaemon)
{
    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);

    DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SUCCESS);
}

HWTEST_F_L0(GCTest, PostTaskTwiceWithDaemon)
{
    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);

    DaemonThread::PostTaskResult result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SUCCESS);

    result = DaemonThread::GetInstance()->CheckAndPostTask(MockForTestTask(thread));
    ASSERT_TRUE(result == DaemonThread::PostTaskResult::SAME_GROUP_TASK_ALREADY_POSTED);
}

HWTEST_F_L0(GCTest, PostTaskWithoutDaemon)
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
} // namespace panda::test