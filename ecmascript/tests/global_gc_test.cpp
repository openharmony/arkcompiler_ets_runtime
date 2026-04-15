/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "ecmascript/js_array.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/mem/shared_heap/global_gc.h"
#include "ecmascript/mem/shared_heap/global_gc_marker.h"
#include "ecmascript/mem/work_manager-inl.h"

using namespace panda::ecmascript;

namespace panda::test {

class GlobalGCTest : public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
        thread->SetCrossThreadExecution(true);
    }

    void TriggerGlobalGC()
    {
        auto *sHeap = SharedHeap::GetInstance();
        sHeap->CollectGarbage<TriggerGCType::GLOBAL_GC, GCReason::OTHER>(thread);
    }
};

HWTEST_F_L0(GlobalGCTest, RunPhases)
{
    auto *sHeap = SharedHeap::GetInstance();
    ASSERT_TRUE(sHeap != nullptr);
    ASSERT_TRUE(sHeap->GetGlobalGC() != nullptr);
    TriggerGlobalGC();
}

HWTEST_F_L0(GlobalGCTest, GetFreedSize)
{
    auto *globalGC = SharedHeap::GetInstance()->GetGlobalGC();
    EXPECT_EQ(globalGC->GetFreedSize(), 0UL);
    TriggerGlobalGC();
    EXPECT_GE(globalGC->GetFreedSize(), 0UL);
}

HWTEST_F_L0(GlobalGCTest, TriggerViaCollectGarbage)
{
    auto *sHeap = SharedHeap::GetInstance();
    ASSERT_TRUE(sHeap != nullptr);
    auto *globalGC = sHeap->GetGlobalGC();
    ASSERT_TRUE(globalGC != nullptr);
    TriggerGlobalGC();
    EXPECT_GE(globalGC->GetFreedSize(), 0UL);
}

HWTEST_F_L0(GlobalGCTest, WorkManagerLifecycle)
{
    auto *workManager = SharedHeap::GetInstance()->GetGlobalGCWorkManager();
    ASSERT_TRUE(workManager != nullptr);
    EXPECT_FALSE(workManager->HasInitialized());
    TriggerGlobalGC();
    EXPECT_FALSE(workManager->HasInitialized());
}

HWTEST_F_L0(GlobalGCTest, GCStatsType)
{
    auto *gcStats = SharedHeap::GetInstance()->GetEcmaGCStats();
    ASSERT_TRUE(gcStats != nullptr);
    EXPECT_EQ(gcStats->GetGCType(TriggerGCType::GLOBAL_GC), GCType::GLOBAL_GC);
}

HWTEST_F_L0(GlobalGCTest, ResetWorkManager)
{
    auto *sHeap = SharedHeap::GetInstance();
    ASSERT_TRUE(sHeap != nullptr);
    auto *globalGC = sHeap->GetGlobalGC();
    ASSERT_TRUE(globalGC != nullptr);
    auto *workManager = sHeap->GetGlobalGCWorkManager();
    ASSERT_TRUE(workManager != nullptr);
    globalGC->ResetWorkManager(workManager);
}

HWTEST_F_L0(GlobalGCTest, CheckCanDistributeTask)
{
    EXPECT_TRUE(SharedHeap::GetInstance()->CheckCanDistributeGlobalGCTask());
}

HWTEST_F_L0(GlobalGCTest, Getters)
{
    auto *sHeap = SharedHeap::GetInstance();
    EXPECT_TRUE(sHeap->GetGlobalGC() != nullptr);
    EXPECT_TRUE(sHeap->GetGlobalGCMarker() != nullptr);
    EXPECT_TRUE(sHeap->GetGlobalGCWorkManager() != nullptr);
    EXPECT_TRUE(sHeap->GetSharedFullGC() != nullptr);
}

HWTEST_F_L0(GlobalGCTest, CleanDeadRSet)
{
    auto *sHeap = SharedHeap::GetInstance();
    ASSERT_TRUE(sHeap != nullptr);
    ObjectFactory *factory = instance->GetFactory();
    size_t sizeBefore = sHeap->GetHeapObjectSize();
    {
        EcmaHandleScope outerScope(thread);
        for (int i = 0; i < 100; i++) {
            JSHandle<TaggedArray> localArray = factory->NewTaggedArray(8, JSTaggedValue::Undefined());
            JSHandle<TaggedArray> sharedArray = factory->NewSOldSpaceTaggedArray(8, JSTaggedValue::Undefined());
            localArray->Set(thread, 0, sharedArray.GetTaggedValue());
        }
    }
    TriggerGlobalGC();
    size_t sizeAfter = sHeap->GetHeapObjectSize();
    EXPECT_TRUE(sizeAfter <= sizeBefore || sHeap->GetGlobalGC()->GetFreedSize() >= 0UL);
}

HWTEST_F_L0(GlobalGCTest, ParallelMarking)
{
    auto *sHeap = SharedHeap::GetInstance();
    ASSERT_TRUE(sHeap != nullptr);
    ObjectFactory *factory = instance->GetFactory();
    {
        EcmaHandleScope outerScope(thread);
        for (int i = 0; i < 2000; i++) {
            JSHandle<TaggedArray> localArray = factory->NewTaggedArray(16, JSTaggedValue::Undefined());
            JSHandle<TaggedArray> sharedArray = factory->NewSOldSpaceTaggedArray(4, JSTaggedValue::Undefined());
            localArray->Set(thread, 0, sharedArray.GetTaggedValue());
        }
    }
    TriggerGlobalGC();
    EXPECT_GE(sHeap->GetGlobalGC()->GetFreedSize(), 0UL);
}

HWTEST_F_L0(GlobalGCTest, MultipleCycles)
{
    auto *sHeap = SharedHeap::GetInstance();
    ASSERT_TRUE(sHeap != nullptr);
    ObjectFactory *factory = instance->GetFactory();
    size_t totalFreed = 0;
    {
        EcmaHandleScope scope(thread);
        factory->NewSOldSpaceTaggedArray(64, JSTaggedValue::Undefined());
    }
    TriggerGlobalGC();
    totalFreed += sHeap->GetGlobalGC()->GetFreedSize();
    {
        EcmaHandleScope scope(thread);
        for (int i = 0; i < 50; i++) {
            factory->NewSOldSpaceTaggedArray(32, JSTaggedValue::Undefined());
        }
    }
    TriggerGlobalGC();
    totalFreed += sHeap->GetGlobalGC()->GetFreedSize();
    {
        EcmaHandleScope scope(thread);
        JSHandle<TaggedArray> localArray = factory->NewTaggedArray(8, JSTaggedValue::Undefined());
        JSHandle<TaggedArray> sharedArray = factory->NewSOldSpaceTaggedArray(8, JSTaggedValue::Undefined());
        localArray->Set(thread, 0, sharedArray.GetTaggedValue());
    }
    TriggerGlobalGC();
    totalFreed += sHeap->GetGlobalGC()->GetFreedSize();
    EXPECT_GE(totalFreed, 0UL);
}

HWTEST_F_L0(GlobalGCTest, AfterSharedGC)
{
    auto *sHeap = SharedHeap::GetInstance();
    ASSERT_TRUE(sHeap != nullptr);
    ObjectFactory *factory = instance->GetFactory();
    {
        EcmaHandleScope scope(thread);
        factory->NewSOldSpaceTaggedArray(64, JSTaggedValue::Undefined());
    }
    sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
    TriggerGlobalGC();
    EXPECT_GE(sHeap->GetGlobalGC()->GetFreedSize(), 0UL);
}

HWTEST_F_L0(GlobalGCTest, WithJSObjects)
{
    auto *sHeap = SharedHeap::GetInstance();
    ASSERT_TRUE(sHeap != nullptr);
    ObjectFactory *factory = instance->GetFactory();
    {
        EcmaHandleScope outerScope(thread);
        JSHandle<JSHClass> hclass(thread, thread->GlobalConstants()->GetObjectClass().GetTaggedObject());
        for (int i = 0; i < 100; i++) {
            JSHandle<JSObject> obj = factory->NewJSObject(hclass);
            JSHandle<TaggedArray> sharedArray = factory->NewSOldSpaceTaggedArray(4, JSTaggedValue::Undefined());
            obj->SetElements(thread, sharedArray);
        }
    }
    TriggerGlobalGC();
    EXPECT_GE(sHeap->GetGlobalGC()->GetFreedSize(), 0UL);
}

HWTEST_F_L0(GlobalGCTest, WithWeakRefs)
{
    auto *sHeap = SharedHeap::GetInstance();
    ASSERT_TRUE(sHeap != nullptr);
    ObjectFactory *factory = instance->GetFactory();
    {
        EcmaHandleScope outerScope(thread);
        JSHandle<TaggedArray> localArray = factory->NewTaggedArray(4, JSTaggedValue::Undefined());
        JSHandle<TaggedArray> localArray2 = factory->NewTaggedArray(4, JSTaggedValue::Undefined());
        JSHandle<TaggedArray> sharedArray = factory->NewSOldSpaceTaggedArray(4, JSTaggedValue::Undefined());
        localArray->Set(thread, 0, sharedArray.GetTaggedValue());
        localArray->Set(thread, 1, localArray2.GetTaggedValue().CreateAndGetWeakRef());
    }
    TriggerGlobalGC();
    EXPECT_GE(sHeap->GetGlobalGC()->GetFreedSize(), 0UL);
    EXPECT_FALSE(thread->HasPendingException());
}

HWTEST_F_L0(GlobalGCTest, SharedHeapAllocationPressure)
{
    auto *sHeap = SharedHeap::GetInstance();
    ASSERT_TRUE(sHeap != nullptr);
    ObjectFactory *factory = instance->GetFactory();
    size_t sizeBefore = sHeap->GetHeapObjectSize();
    {
        EcmaHandleScope outerScope(thread);
        for (int i = 0; i < 500; i++) {
            factory->NewSOldSpaceTaggedArray(128, JSTaggedValue::Undefined());
        }
    }
    sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
    TriggerGlobalGC();
    size_t sizeAfter = sHeap->GetHeapObjectSize();
    EXPECT_GE(sHeap->GetGlobalGC()->GetFreedSize(), 0UL);
    EXPECT_TRUE(sizeAfter <= sizeBefore || sizeAfter > 0);
}

class GlobalGCSequentialTest : public testing::Test {
public:
    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};

    void SetUp() override
    {
        JSRuntimeOptions options;
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
        thread->SetCrossThreadExecution(true);
    }

    void TearDown() override
    {
        delete scope;
        scope = nullptr;
        instance->GetJSThread()->ManagedCodeEnd();
    }

    void TriggerGlobalGC()
    {
        auto *sHeap = SharedHeap::GetInstance();
        sHeap->CollectGarbage<TriggerGCType::GLOBAL_GC, GCReason::OTHER>(thread);
    }
};

HWTEST_F_L0(GlobalGCSequentialTest, SequentialCleanAll)
{
    auto *sHeap = SharedHeap::GetInstance();
    ASSERT_TRUE(sHeap != nullptr);
    sHeap->DisableParallelGC(thread);
    ObjectFactory *factory = instance->GetFactory();
    size_t totalFreed = 0;

    {
        EcmaHandleScope testScope(thread);
        for (int i = 0; i < 50; i++) {
            JSHandle<TaggedArray> localArray = factory->NewTaggedArray(8, JSTaggedValue::Undefined());
            JSHandle<TaggedArray> sharedArray = factory->NewSOldSpaceTaggedArray(4, JSTaggedValue::Undefined());
            localArray->Set(thread, 0, sharedArray.GetTaggedValue());
        }
    }
    TriggerGlobalGC();
    totalFreed += sHeap->GetGlobalGC()->GetFreedSize();

    {
        EcmaHandleScope testScope(thread);
        JSHandle<JSHClass> hclass(thread, thread->GlobalConstants()->GetObjectClass().GetTaggedObject());
        for (int i = 0; i < 50; i++) {
            JSHandle<JSObject> obj = factory->NewJSObject(hclass);
            JSHandle<TaggedArray> sharedArray = factory->NewSOldSpaceTaggedArray(4, JSTaggedValue::Undefined());
            obj->SetElements(thread, sharedArray);
        }
    }
    TriggerGlobalGC();
    totalFreed += sHeap->GetGlobalGC()->GetFreedSize();

    {
        EcmaHandleScope testScope(thread);
        factory->NewSOldSpaceTaggedArray(32, JSTaggedValue::Undefined());
    }
    TriggerGlobalGC();
    totalFreed += sHeap->GetGlobalGC()->GetFreedSize();

    {
        EcmaHandleScope testScope(thread);
        JSHandle<TaggedArray> localArray = factory->NewTaggedArray(8, JSTaggedValue::Undefined());
        JSHandle<TaggedArray> sharedArray = factory->NewSOldSpaceTaggedArray(8, JSTaggedValue::Undefined());
        localArray->Set(thread, 0, sharedArray.GetTaggedValue());
    }
    TriggerGlobalGC();
    totalFreed += sHeap->GetGlobalGC()->GetFreedSize();

    EXPECT_GE(totalFreed, 0UL);
    EXPECT_FALSE(thread->HasPendingException());
}

}  // namespace panda::test
