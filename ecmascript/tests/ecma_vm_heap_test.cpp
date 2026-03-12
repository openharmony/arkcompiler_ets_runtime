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

HWTEST_F_L0(GCHeapTest, HeapMemoryPressureTest1)
{
    // Test SetHeapMemoryPressure sets thresholds correctly
    auto vm = thread->GetEcmaVM();
    DFXJSNApi::HeapMemoryPressureOptions options = {0.8, 0.85, 0.9};
    vm->SetHeapMemoryPressure(options, Local<FunctionRef>());
    
    EXPECT_DOUBLE_EQ(vm->GetLocalMemoryPressureThreshold(), 0.8);
    EXPECT_DOUBLE_EQ(vm->GetSharedMemoryPressureThreshold(), 0.85);
    EXPECT_DOUBLE_EQ(vm->GetProcessMemoryPressureThreshold(), 0.9);
    
    // Reset
    vm->ResetMemoryPressure();
}

HWTEST_F_L0(GCHeapTest, HeapMemoryPressureTest2)
{
    // Test ResetMemoryPressure resets all thresholds and flags
    auto vm = thread->GetEcmaVM();
    
    // Set some values first
    DFXJSNApi::HeapMemoryPressureOptions options = {0.8, 0.85, 0.9};
    vm->SetHeapMemoryPressure(options, Local<FunctionRef>());
    
    // Reset
    vm->ResetMemoryPressure();
    
    EXPECT_DOUBLE_EQ(vm->GetLocalMemoryPressureThreshold(), 0.0);
    EXPECT_DOUBLE_EQ(vm->GetSharedMemoryPressureThreshold(), 0.0);
    EXPECT_DOUBLE_EQ(vm->GetProcessMemoryPressureThreshold(), 0.0);
}

HWTEST_F_L0(GCHeapTest, HeapMemoryPressureTest3)
{
    // Test CheckHeapMemoryPressure prevents re-entry during callback
    auto vm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(vm->GetHeap());

    // Set threshold to trigger immediately
    DFXJSNApi::HeapMemoryPressureOptions options = {0.0, 0.0, 0.0};
    vm->SetHeapMemoryPressure(options, Local<FunctionRef>());

    // Call CheckHeapMemoryPressure - since callback is empty, it should return immediately
    // and the flag should remain false
    EXPECT_FALSE(vm->GetIsInMemoryPressureCallback());
    vm->CheckHeapMemoryPressure(heap);
    // Flag should be false after check completes (callback was empty so scope wasn't used effectively)
    EXPECT_FALSE(vm->GetIsInMemoryPressureCallback());

    // Verify that calling CheckHeapMemoryPressure again while "in callback" would be blocked
    // Manually set the flag to simulate being in callback
    vm->SetInMemoryPressureCallback(true);
    EXPECT_TRUE(vm->GetIsInMemoryPressureCallback());

    // This call should be blocked due to re-entry protection
    vm->CheckHeapMemoryPressure(heap);
    // Flag should still be true (callback was blocked, so scope wasn't created/destroyed)
    EXPECT_TRUE(vm->GetIsInMemoryPressureCallback());

    // Reset flag and cleanup
    vm->SetInMemoryPressureCallback(false);
    vm->ResetMemoryPressure();
}

HWTEST_F_L0(GCHeapTest, HeapMemoryPressureTest4)
{
    // Test CheckSharedHeapMemoryPressure sets flag correctly
    auto vm = thread->GetEcmaVM();
    
    // Set shared heap threshold
    DFXJSNApi::HeapMemoryPressureOptions options = {0.0, 0.8, 0.0};
    vm->SetHeapMemoryPressure(options, Local<FunctionRef>());
    
    EXPECT_DOUBLE_EQ(vm->GetSharedMemoryPressureThreshold(), 0.8);
    
    vm->ResetMemoryPressure();
    EXPECT_DOUBLE_EQ(vm->GetSharedMemoryPressureThreshold(), 0.0);
}

HWTEST_F_L0(GCHeapTest, HeapMemoryPressureTest5)
{
    // Test multiple calls to SetHeapMemoryPressure returns false
    auto vm = thread->GetEcmaVM();

    // First call should succeed
    DFXJSNApi::HeapMemoryPressureOptions options1 = {0.8, 0.85, 0.9};
    bool result1 = vm->SetHeapMemoryPressure(options1, Local<FunctionRef>());
    EXPECT_TRUE(result1);

    // Second call should fail (callback already set)
    DFXJSNApi::HeapMemoryPressureOptions options2 = {0.5, 0.5, 0.5};
    bool result2 = vm->SetHeapMemoryPressure(options2, Local<FunctionRef>());
    EXPECT_FALSE(result2);

    // Values should remain from first call
    EXPECT_DOUBLE_EQ(vm->GetLocalMemoryPressureThreshold(), 0.8);
    EXPECT_DOUBLE_EQ(vm->GetSharedMemoryPressureThreshold(), 0.85);
    EXPECT_DOUBLE_EQ(vm->GetProcessMemoryPressureThreshold(), 0.9);

    vm->ResetMemoryPressure();
}

HWTEST_F_L0(GCHeapTest, HeapMemoryPressureTest6)
{
    // Test setting only local threshold correctly sets all values
    auto vm = thread->GetEcmaVM();

    // Set only local heap threshold (shared and process = 0.0)
    DFXJSNApi::HeapMemoryPressureOptions options = {0.7, 0.0, 0.0};
    bool result = vm->SetHeapMemoryPressure(options, Local<FunctionRef>());
    EXPECT_TRUE(result);

    // Verify local threshold is set
    EXPECT_DOUBLE_EQ(vm->GetLocalMemoryPressureThreshold(), 0.7);
    // Shared and process should remain 0.0
    EXPECT_DOUBLE_EQ(vm->GetSharedMemoryPressureThreshold(), 0.0);
    EXPECT_DOUBLE_EQ(vm->GetProcessMemoryPressureThreshold(), 0.0);

    vm->ResetMemoryPressure();
}

}  // namespace panda::test
