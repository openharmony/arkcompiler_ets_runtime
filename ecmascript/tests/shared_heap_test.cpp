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

#include "ecmascript/tests/test_helper.h"

#include "ecmascript/daemon/daemon_thread.h"
#include "ecmascript/daemon/daemon_task.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_marker.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_sweeper.h"
#include "ecmascript/mem/verification.h"

using namespace panda::ecmascript;

namespace panda::test {
class SharedHeapTest : public BaseTestWithScope<false> {
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
    }
};

static SharedMarkStatus g_sharedMarkStatus = SharedMarkStatus::READY_TO_CONCURRENT_MARK;

void SetSharedMarkStatusRunner()
{
    DaemonThread *dThread = DaemonThread::GetInstance();
    {
        SuspendAllScope scope(dThread);
        dThread->SetSharedMarkStatus(g_sharedMarkStatus);
    }
}

class SetSharedMarkStatusTask : public DaemonTask {
public:
    explicit SetSharedMarkStatusTask(JSThread *thread)
        : DaemonTask(thread, DaemonTaskType::MOCK_FOR_TEST, DaemonTaskGroup::NONE,
                     &SetSharedMarkStatusRunner, nullptr) {}
    ~SetSharedMarkStatusTask() override = default;
};

HWTEST_F_L0(SharedHeapTest, SharedConcurrentMarker3)
{
    auto sHeap = SharedHeap::GetInstance();
    sHeap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    sHeap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::DISABLE);
    auto dThread = DaemonThread::GetInstance();
    
    // 使用自定义任务设置共享标记状态，而不是直接调用SetSharedMarkStatus
    g_sharedMarkStatus = SharedMarkStatus::CONCURRENT_MARKING_OR_FINISHED;
    dThread->CheckAndPostTask(SetSharedMarkStatusTask(thread));
    
    sHeap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::ENABLE);
    EXPECT_TRUE(sHeap->GetConcurrentMarker()->IsEnabled());
}

HWTEST_F_L0(SharedHeapTest, SharedConcurrentMarker4)
{
    auto sHeap = SharedHeap::GetInstance();
    sHeap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    sHeap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::DISABLE);
    auto dThread = DaemonThread::GetInstance();
    
    // 使用自定义任务设置共享标记状态，而不是直接调用SetSharedMarkStatus
    g_sharedMarkStatus = SharedMarkStatus::READY_TO_CONCURRENT_MARK;
    dThread->CheckAndPostTask(SetSharedMarkStatusTask(thread));
    
    sHeap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::DISABLE);
    EXPECT_TRUE(sHeap->GetConcurrentMarker()->IsDisabled());
}

HWTEST_F_L0(SharedHeapTest, SharedConcurrentMarker5)
{
    auto sHeap = SharedHeap::GetInstance();
    auto dThread = DaemonThread::GetInstance();
    sHeap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    
    g_sharedMarkStatus = SharedMarkStatus::CONCURRENT_MARKING_OR_FINISHED;
    dThread->CheckAndPostTask(SetSharedMarkStatusTask(thread));
    
    sHeap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::ENABLE);
    EXPECT_TRUE(sHeap->GetConcurrentMarker()->IsEnabled());
}

HWTEST_F_L0(SharedHeapTest, SharedConcurrentMarker6)
{
    auto sHeap = SharedHeap::GetInstance();
    auto dThread = DaemonThread::GetInstance();
    sHeap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    
    g_sharedMarkStatus = SharedMarkStatus::READY_TO_CONCURRENT_MARK;
    dThread->CheckAndPostTask(SetSharedMarkStatusTask(thread));
    
    sHeap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::DISABLE);
    EXPECT_TRUE(sHeap->GetConcurrentMarker()->IsDisabled());
}

HWTEST_F_L0(SharedHeapTest, SharedConcurrentMarker7)
{
    auto sHeap = SharedHeap::GetInstance();
    sHeap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    sHeap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::DISABLE);
    auto dThread = DaemonThread::GetInstance();
    
    g_sharedMarkStatus = SharedMarkStatus::CONCURRENT_MARKING_OR_FINISHED;
    dThread->CheckAndPostTask(SetSharedMarkStatusTask(thread));
    
    sHeap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::DISABLE);
    EXPECT_TRUE(sHeap->GetConcurrentMarker()->IsDisabled());
}

HWTEST_F_L0(SharedHeapTest, SharedConcurrentSweeper1)
{
    auto sHeap = SharedHeap::GetInstance();
    sHeap->GetSweeper()->ConfigConcurrentSweep(false);
    sHeap->GetSweeper()->EnableConcurrentSweep(EnableConcurrentSweepType::ENABLE);
    EXPECT_TRUE(sHeap->GetSweeper()->IsConfigDisabled());
}

HWTEST_F_L0(SharedHeapTest, SharedConcurrentSweeper2)
{
    auto sHeap = SharedHeap::GetInstance();
    sHeap->GetSweeper()->ConfigConcurrentSweep(true);
    sHeap->GetSweeper()->EnableConcurrentSweep(EnableConcurrentSweepType::ENABLE);
    EXPECT_TRUE(sHeap->GetSweeper()->ConcurrentSweepEnabled());
}

HWTEST_F_L0(SharedHeapTest, SharedConcurrentSweeper3)
{
    auto sHeap = SharedHeap::GetInstance();
    sHeap->GetSweeper()->ConfigConcurrentSweep(true);
    sHeap->GetSweeper()->EnableConcurrentSweep(EnableConcurrentSweepType::DISABLE);
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    sHeap->GetSweeper()->EnableConcurrentSweep(EnableConcurrentSweepType::ENABLE);
    EXPECT_TRUE(sHeap->GetSweeper()->ConcurrentSweepEnabled());
}

HWTEST_F_L0(SharedHeapTest, SharedConcurrentSweeper4)
{
    auto sHeap = SharedHeap::GetInstance();
    sHeap->GetSweeper()->ConfigConcurrentSweep(true);
    sHeap->GetSweeper()->EnableConcurrentSweep(EnableConcurrentSweepType::DISABLE);
    sHeap->GetSweeper()->EnsureAllTaskFinished();
    sHeap->GetSweeper()->EnableConcurrentSweep(EnableConcurrentSweepType::DISABLE);
    EXPECT_TRUE(sHeap->GetSweeper()->IsDisabled());
}

HWTEST_F_L0(SharedHeapTest, SharedConcurrentSweeper5)
{
    auto sHeap = SharedHeap::GetInstance();
    sHeap->GetSweeper()->ConfigConcurrentSweep(true);
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    sHeap->GetSweeper()->EnableConcurrentSweep(EnableConcurrentSweepType::ENABLE);
    EXPECT_TRUE(sHeap->GetSweeper()->ConcurrentSweepEnabled());
}

HWTEST_F_L0(SharedHeapTest, SharedConcurrentSweeper6)
{
    auto sHeap = SharedHeap::GetInstance();
    sHeap->GetSweeper()->ConfigConcurrentSweep(true);
    sHeap->GetSweeper()->EnsureAllTaskFinished();
    sHeap->GetSweeper()->EnableConcurrentSweep(EnableConcurrentSweepType::DISABLE);
    EXPECT_TRUE(sHeap->GetSweeper()->IsDisabled());
}

HWTEST_F_L0(SharedHeapTest, SharedConcurrentSweeper7)
{
    auto sHeap = SharedHeap::GetInstance();
    sHeap->GetSweeper()->ConfigConcurrentSweep(true);
    sHeap->GetSweeper()->EnableConcurrentSweep(EnableConcurrentSweepType::DISABLE);
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    sHeap->GetSweeper()->EnableConcurrentSweep(EnableConcurrentSweepType::DISABLE);
    EXPECT_TRUE(sHeap->GetSweeper()->IsDisabled());
}

HWTEST_F_L0(SharedHeapTest, SharedConcurrentSweeper8)
{
    auto sHeap = SharedHeap::GetInstance();
    sHeap->GetSweeper()->ConfigConcurrentSweep(true);
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    sHeap->GetSweeper()->EnableConcurrentSweep(EnableConcurrentSweepType::DISABLE);
    EXPECT_TRUE(sHeap->GetSweeper()->IsRequestDisabled());
}

HWTEST_F_L0(SharedHeapTest, CheckIfNeedStopCollectionByStartup1)
{
    auto sHeap = SharedHeap::GetInstance();
    sHeap->NotifyPostFork();
    EXPECT_TRUE(sHeap->CheckIfNeedStopCollectionByStartup());
}

HWTEST_F_L0(SharedHeapTest, CheckIfNeedStopCollectionByStartup2)
{
    auto sHeap = SharedHeap::GetInstance();
    sHeap->NotifyPostFork();

    SharedHugeObjectSpace *sharedHugeObjectSpace = sHeap->GetHugeObjectSpace();
    sharedHugeObjectSpace->IncreaseCommitted(500 * 1024 *1024);
    EXPECT_TRUE(sharedHugeObjectSpace->CommittedSizeExceed());
    EXPECT_FALSE(sHeap->CheckIfNeedStopCollectionByStartup());
}

HWTEST_F_L0(SharedHeapTest, CheckIfNeedStopCollectionByStartup3)
{
    auto sHeap = SharedHeap::GetInstance();
    sHeap->NotifyPostFork();
    sHeap->FinishStartupEvent();

    EXPECT_TRUE(sHeap->CheckIfNeedStopCollectionByStartup());
}

HWTEST_F_L0(SharedHeapTest, CheckIfNeedStopCollectionByStartup4)
{
    auto sHeap = SharedHeap::GetInstance();
    sHeap->NotifyPostFork();
    sHeap->FinishStartupEvent();

    SharedHugeObjectSpace *sharedHugeObjectSpace = sHeap->GetHugeObjectSpace();
    sharedHugeObjectSpace->IncreaseCommitted(500 * 1024 *1024);
    EXPECT_TRUE(sharedHugeObjectSpace->CommittedSizeExceed());
    EXPECT_FALSE(sHeap->CheckIfNeedStopCollectionByStartup());
}
}  // namespace panda::test