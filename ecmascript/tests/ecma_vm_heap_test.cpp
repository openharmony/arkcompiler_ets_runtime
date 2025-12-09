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

#include "ecmascript/builtins/builtins_ark_tools.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/full_gc.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/partial_gc.h"
#include "ecmascript/serializer/serialize_chunk.h"
#include "ecmascript/tests/ecma_test_common.h"
#include "ecmascript/string/external_string.h"
#if defined(PANDA_TARGET_OHOS) && !defined(STANDALONE_MODE)
#include "parameters.h"
#endif

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class GCHeapTest : public BaseTestWithScope<false> {
public:
    static constexpr uint32_t TYPE_INFO_SIZE = 4;
    static constexpr uint32_t UTF8_CACHED_DATA_SIZE = 4;
    static constexpr uint32_t UTF16_CACHED_DATA_SIZE = 8;
    static constexpr uint32_t UTF16_STRING_LENGTH = UTF16_CACHED_DATA_SIZE / sizeof(uint16_t);

    static inline void CallBackFn([[maybe_unused]] void *data, void *hint)
    {
        free(hint);
    }

    void SetUp() override
    {
#if defined(PANDA_TARGET_OHOS) && !defined(STANDALONE_MODE)
        OHOS::system::SetParameter("persist.ark.sheap.growfactor", "8");
        OHOS::system::SetParameter("persist.ark.sheap.growstep", "320");
        OHOS::system::SetParameter("persist.ark.sensitive.threshold", "320");
        OHOS::system::SetParameter("persist.ark.native.stepsize", "1024");
        OHOS::system::SetParameter("persist.ark.global.alloclimit", "128");
#endif
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

HWTEST_F_L0(GCHeapTest, HeapParamTest1)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto allocLimit1 = heap->GetGlobalSpaceAllocLimit();
    heap->AdjustBySurvivalRate(1024 * 1024);
    auto allocLimit2 = heap->GetGlobalSpaceAllocLimit();
    EXPECT_TRUE(allocLimit2 != allocLimit1);
}

HWTEST_F_L0(GCHeapTest, HeapParamTest2)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto allocLimit1 = heap->GetGlobalSpaceAllocLimit();
    heap->ObjectExceedHighSensitiveThresholdForCM();
    auto allocLimit2 = heap->GetGlobalSpaceAllocLimit();
    EXPECT_TRUE(allocLimit2 == allocLimit1);
}

HWTEST_F_L0(GCHeapTest, HeapParamTest3)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto allocLimit1 = heap->GetGlobalSpaceAllocLimit();
    heap->CheckIfNeedStopCollectionByHighSensitive();
    auto allocLimit2 = heap->GetGlobalSpaceAllocLimit();
    EXPECT_TRUE(allocLimit2 == allocLimit1);
}

HWTEST_F_L0(GCHeapTest, SHeapParamTest1)
{
    constexpr size_t ALLOCATE_COUNT = 100;
    constexpr size_t ALLOCATE_SIZE = 512;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto sHeap = SharedHeap::GetInstance();
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    auto oldSizebase = sHeap->GetOldSpace()->GetHeapObjectSize();
    auto allocLimit1 = sHeap->GetGlobalSpaceAllocLimit();
    EXPECT_TRUE(oldSizebase > 0);
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < ALLOCATE_COUNT; i++) {
            factory->NewSOldSpaceTaggedArray(ALLOCATE_SIZE, JSTaggedValue::Undefined());
        }
    }
    size_t oldSizeBefore = sHeap->GetOldSpace()->GetHeapObjectSize();
    EXPECT_TRUE(oldSizeBefore > oldSizebase);
    EXPECT_TRUE(oldSizeBefore > TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), ALLOCATE_SIZE));
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    auto oldSizeAfter = sHeap->GetOldSpace()->GetHeapObjectSize();
    EXPECT_TRUE(oldSizeBefore > oldSizeAfter);
    auto allocLimit2 = sHeap->GetGlobalSpaceAllocLimit();
    EXPECT_TRUE(allocLimit1 == allocLimit2);
}

HWTEST_F_L0(GCHeapTest, SHeapParamTest2)
{
    constexpr size_t ALLOCATE_COUNT = 100;
    constexpr size_t ALLOCATE_SIZE = 512;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto sHeap = SharedHeap::GetInstance();
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    auto oldSizebase = sHeap->GetOldSpace()->GetHeapObjectSize();
    auto allocLimit1 = sHeap->GetGlobalSpaceAllocLimit();
    EXPECT_TRUE(oldSizebase > 0);
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < ALLOCATE_COUNT; i++) {
            factory->NewSOldSpaceTaggedArray(ALLOCATE_SIZE, JSTaggedValue::Undefined());
        }
    }
    sHeap->AdjustGlobalSpaceAllocLimit();
    thread->SetSharedMarkStatus(SharedMarkStatus::CONCURRENT_MARKING_OR_FINISHED);
    sHeap->CheckAndTriggerSharedGC(thread);
    auto allocLimit2 = sHeap->GetGlobalSpaceAllocLimit();
    EXPECT_TRUE(allocLimit1 == allocLimit2);
}
}  // namespace panda::test
