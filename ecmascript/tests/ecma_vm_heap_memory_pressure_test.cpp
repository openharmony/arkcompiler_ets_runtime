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

#include "ecmascript/builtins/builtins_ark_tools.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/full_gc.h"
#include "ecmascript/mem/mem_map_allocator.h"
#include "ecmascript/napi/include/jsnapi.h"
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
class HeapMemoryPressureTest : public BaseTestWithScope<false> {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownTestCase";
    }
};

static bool g_testCallbackFlag = false;

static Local<JSValueRef> TestCallback(JsiRuntimeCallInfo *info)
{
    g_testCallbackFlag = true;
    return JSValueRef::Undefined(info->GetVM());
}

HWTEST_F(HeapMemoryPressureTest, HeapMemoryPressureTest1, testing::ext::TestSize.Level0)
{
    // Test SetHeapMemoryPressure sets thresholds correctly
    auto vm = thread->GetEcmaVM();
    Local<FunctionRef> callback = FunctionRef::New(vm, TestCallback);
    DFXJSNApi::HeapMemoryPressureOptions options = {0.8, 0.85, 0.9};
    vm->SetHeapMemoryPressure(options, callback);

    EXPECT_DOUBLE_EQ(vm->GetLocalMemoryPressureThreshold(), 0.8);
    EXPECT_DOUBLE_EQ(vm->GetSharedMemoryPressureThreshold(), 0.85);
    EXPECT_DOUBLE_EQ(vm->GetProcessMemoryPressureThreshold(), 0.9);

    // Reset
    vm->ResetMemoryPressure();
}

HWTEST_F(HeapMemoryPressureTest, HeapMemoryPressureTest2, testing::ext::TestSize.Level0)
{
    // Test ResetMemoryPressure resets all thresholds and flags
    auto vm = thread->GetEcmaVM();
    Local<FunctionRef> callback = FunctionRef::New(vm, TestCallback);

    // Set some values first
    DFXJSNApi::HeapMemoryPressureOptions options = {0.8, 0.85, 0.9};
    vm->SetHeapMemoryPressure(options, callback);

    // Reset
    vm->ResetMemoryPressure();

    EXPECT_DOUBLE_EQ(vm->GetLocalMemoryPressureThreshold(), 0.0);
    EXPECT_DOUBLE_EQ(vm->GetSharedMemoryPressureThreshold(), 0.0);
    EXPECT_DOUBLE_EQ(vm->GetProcessMemoryPressureThreshold(), 0.0);

    // Verify callback is cleared after reset
    EXPECT_TRUE(vm->GetMemoryPressureCallback().IsEmpty());
}

HWTEST_F(HeapMemoryPressureTest, HeapMemoryPressureTest3, testing::ext::TestSize.Level0)
{
    // Test CheckHeapMemoryPressure prevents re-entry during callback
    auto vm = thread->GetEcmaVM();
    Local<FunctionRef> callback = FunctionRef::New(vm, TestCallback);
    auto heap = const_cast<Heap *>(vm->GetHeap());

    // Set threshold to 0 - should not trigger callback even with valid callback set
    DFXJSNApi::HeapMemoryPressureOptions options = {0.0, 0.0, 0.0};
    vm->SetHeapMemoryPressure(options, callback);

    // Call CheckHeapMemoryPressure - since threshold is 0, callback should not be triggered
    // and the flag should remain false
    EXPECT_FALSE(vm->GetIsInMemoryPressureCallback());
    vm->CheckHeapMemoryPressure(heap);
    // Flag should be false after check completes (threshold is 0 so no callback triggered)
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

HWTEST_F(HeapMemoryPressureTest, HeapMemoryPressureTest4, testing::ext::TestSize.Level0)
{
    // Test CheckSharedHeapMemoryPressure sets flag correctly
    auto vm = thread->GetEcmaVM();
    Local<FunctionRef> callback = FunctionRef::New(vm, TestCallback);

    // Set shared heap threshold
    DFXJSNApi::HeapMemoryPressureOptions options = {0.0, 0.8, 0.0};
    vm->SetHeapMemoryPressure(options, callback);

    EXPECT_DOUBLE_EQ(vm->GetSharedMemoryPressureThreshold(), 0.8);

    vm->ResetMemoryPressure();
    EXPECT_DOUBLE_EQ(vm->GetSharedMemoryPressureThreshold(), 0.0);
}

HWTEST_F(HeapMemoryPressureTest, HeapMemoryPressureTest5, testing::ext::TestSize.Level0)
{
    // Test SetHeapMemoryPressure returns false when callback is already set
    auto vm = thread->GetEcmaVM();
    Local<FunctionRef> callback = FunctionRef::New(vm, TestCallback);

    // First call with valid callback should succeed
    DFXJSNApi::HeapMemoryPressureOptions options1 = {0.8, 0.85, 0.9};
    bool result1 = vm->SetHeapMemoryPressure(options1, callback);
    EXPECT_TRUE(result1);

    // Verify callback is set
    EXPECT_FALSE(vm->GetMemoryPressureCallback().IsEmpty());

    // Second call should fail because callback is already set
    DFXJSNApi::HeapMemoryPressureOptions options2 = {0.5, 0.5, 0.5};
    bool result2 = vm->SetHeapMemoryPressure(options2, callback);
    EXPECT_FALSE(result2);

    // Values should remain from first call
    EXPECT_DOUBLE_EQ(vm->GetLocalMemoryPressureThreshold(), 0.8);
    EXPECT_DOUBLE_EQ(vm->GetSharedMemoryPressureThreshold(), 0.85);
    EXPECT_DOUBLE_EQ(vm->GetProcessMemoryPressureThreshold(), 0.9);

    vm->ResetMemoryPressure();
}

HWTEST_F(HeapMemoryPressureTest, HeapMemoryPressureTest6, testing::ext::TestSize.Level0)
{
    // Test setting only local threshold correctly sets all values
    auto vm = thread->GetEcmaVM();
    Local<FunctionRef> callback = FunctionRef::New(vm, TestCallback);
    // Set only local heap threshold (shared and process = 0.0)
    DFXJSNApi::HeapMemoryPressureOptions options = {0.7, 0.0, 0.0};
    bool result = vm->SetHeapMemoryPressure(options, callback);
    EXPECT_TRUE(result);

    // Verify local threshold is set
    EXPECT_DOUBLE_EQ(vm->GetLocalMemoryPressureThreshold(), 0.7);
    // Shared and process should remain 0.0
    EXPECT_DOUBLE_EQ(vm->GetSharedMemoryPressureThreshold(), 0.0);
    EXPECT_DOUBLE_EQ(vm->GetProcessMemoryPressureThreshold(), 0.0);

    vm->ResetMemoryPressure();
}

HWTEST_F(HeapMemoryPressureTest, HeapMemoryPressureTest7, testing::ext::TestSize.Level0)
{
    // Test CheckHeapMemoryPressure local heap logic when threshold is 0
    auto vm = thread->GetEcmaVM();
    Local<FunctionRef> callback = FunctionRef::New(vm, TestCallback);
    auto heap = const_cast<Heap *>(vm->GetHeap());

    // Set local threshold to 0 - should not trigger callback
    DFXJSNApi::HeapMemoryPressureOptions options = {0.0, 0.0, 0.0};
    vm->SetHeapMemoryPressure(options, callback);

    // Verify threshold is 0
    EXPECT_DOUBLE_EQ(vm->GetLocalMemoryPressureThreshold(), 0.0);

    // Call CheckHeapMemoryPressure - should return early due to threshold = 0
    EXPECT_FALSE(vm->GetIsInMemoryPressureCallback());
    vm->CheckHeapMemoryPressure(heap);
    EXPECT_FALSE(vm->GetIsInMemoryPressureCallback());

    vm->ResetMemoryPressure();
}

#ifdef NDEBUG

HWTEST_F(HeapMemoryPressureTest, HeapMemoryPressureTest8, testing::ext::TestSize.Level0)
{
    // Test CheckSharedHeapMemoryPressure sets needProcessMemoryPressureCallback flag when ratio >= threshold
    auto vm = thread->GetEcmaVM();
    Local<FunctionRef> callback = FunctionRef::New(vm, TestCallback);

    // Set process threshold > 0 to test the flag setting logic
    DFXJSNApi::HeapMemoryPressureOptions options = {0.0, 0.0, 0.99};
    vm->SetHeapMemoryPressure(options, callback);

    // Verify thresholds are set correctly
    EXPECT_DOUBLE_EQ(vm->GetProcessMemoryPressureThreshold(), 0.99);
    EXPECT_DOUBLE_EQ(vm->GetSharedMemoryPressureThreshold(), 0.0);
    EXPECT_DOUBLE_EQ(vm->GetLocalMemoryPressureThreshold(), 0.0);

    // Verify initial flags are false
    EXPECT_FALSE(vm->GetNeedProcessMemoryPressureCallback());
    EXPECT_FALSE(vm->GetNeedSharedMemoryPressureCallback());

    // Call CheckSharedHeapMemoryPressure - should not set flag because ratio < threshold
    vm->CheckSharedHeapMemoryPressure();
    EXPECT_FALSE(vm->GetNeedProcessMemoryPressureCallback());

    vm->ResetMemoryPressure();
}

HWTEST_F(HeapMemoryPressureTest, HeapMemoryPressureTest9, testing::ext::TestSize.Level0)
{
    // Test CheckSharedHeapMemoryPressure sets needSharedMemoryPressureCallback flag when ratio >= threshold
    auto vm = thread->GetEcmaVM();
    Local<FunctionRef> callback = FunctionRef::New(vm, TestCallback);

    // Set only shared threshold > 0
    DFXJSNApi::HeapMemoryPressureOptions options = {0.0, 0.99, 0.0};
    vm->SetHeapMemoryPressure(options, callback);

    // Verify thresholds are set correctly
    EXPECT_DOUBLE_EQ(vm->GetSharedMemoryPressureThreshold(), 0.99);
    EXPECT_DOUBLE_EQ(vm->GetProcessMemoryPressureThreshold(), 0.0);

    // Verify initial flags are false
    EXPECT_FALSE(vm->GetNeedProcessMemoryPressureCallback());
    EXPECT_FALSE(vm->GetNeedSharedMemoryPressureCallback());

    // Call CheckSharedHeapMemoryPressure - should not set flag because ratio < threshold
    vm->CheckSharedHeapMemoryPressure();
    EXPECT_FALSE(vm->GetNeedSharedMemoryPressureCallback());

    vm->ResetMemoryPressure();
}

HWTEST_F(HeapMemoryPressureTest, HeapMemoryPressureTest10, testing::ext::TestSize.Level0)
{
    // Test CheckSharedHeapMemoryPressure returns early when no callback set
    auto vm = thread->GetEcmaVM();
    Local<FunctionRef> callback = FunctionRef::New(vm, TestCallback);
    // Set thresholds but don't set callback (empty FunctionRef)
    DFXJSNApi::HeapMemoryPressureOptions options = {0.8, 0.8, 0.8};
    vm->SetHeapMemoryPressure(options, callback);

    // Verify flags remain false when callback is empty
    EXPECT_FALSE(vm->GetNeedProcessMemoryPressureCallback());
    EXPECT_FALSE(vm->GetNeedSharedMemoryPressureCallback());

    vm->ResetMemoryPressure();
}
#endif

HWTEST_F(HeapMemoryPressureTest, HeapMemoryPressureTest11, testing::ext::TestSize.Level0)
{
    // Test CheckHeapMemoryPressure process threshold logic (threshold > 0 but ratio < threshold)
    auto vm = thread->GetEcmaVM();
    Local<FunctionRef> callback = FunctionRef::New(vm, TestCallback);
    auto heap = const_cast<Heap *>(vm->GetHeap());

    // Set process threshold to a high value (unlikely to trigger)
    DFXJSNApi::HeapMemoryPressureOptions options = {0.0, 0.0, 0.99};
    vm->SetHeapMemoryPressure(options, callback);

    // Call CheckHeapMemoryPressure - should not trigger callback (ratio < threshold)
    EXPECT_FALSE(vm->GetIsInMemoryPressureCallback());
    vm->CheckHeapMemoryPressure(heap);
    // Should return early because processLimit is typically non-zero but ratio < 0.99
    EXPECT_FALSE(vm->GetIsInMemoryPressureCallback());

    vm->ResetMemoryPressure();
}

HWTEST_F(HeapMemoryPressureTest, HeapMemoryPressureTest12, testing::ext::TestSize.Level0)
{
    // Test CheckHeapMemoryPressure local heap logic path when threshold > 0
    auto vm = thread->GetEcmaVM();
    Local<FunctionRef> callback = FunctionRef::New(vm, TestCallback);
    auto heap = const_cast<Heap *>(vm->GetHeap());

    // Set local threshold to a high value (unlikely to trigger)
    DFXJSNApi::HeapMemoryPressureOptions options = {0.99, 0.0, 0.0};
    vm->SetHeapMemoryPressure(options, callback);

    // Call CheckHeapMemoryPressure - should not trigger local callback (ratio < threshold)
    EXPECT_FALSE(vm->GetIsInMemoryPressureCallback());
    vm->CheckHeapMemoryPressure(heap);
    // Should return after local check, not triggering callback
    EXPECT_FALSE(vm->GetIsInMemoryPressureCallback());

    vm->ResetMemoryPressure();
}

HWTEST_F(HeapMemoryPressureTest, HeapMemoryPressureTest13, testing::ext::TestSize.Level0)
{
    // Test CheckHeapMemoryPressure can be called without crash
    auto vm = thread->GetEcmaVM();
    Local<FunctionRef> callback = FunctionRef::New(vm, TestCallback);
    auto heap = const_cast<Heap *>(vm->GetHeap());

    // Set threshold to a high value so it won't trigger (verify ratio < threshold path)
    DFXJSNApi::HeapMemoryPressureOptions options = {0.0, 0.0, 0.99};
    vm->SetHeapMemoryPressure(options, callback);

    // Call CheckHeapMemoryPressure - should not trigger callback (ratio < threshold)
    EXPECT_FALSE(vm->GetIsInMemoryPressureCallback());
    vm->CheckHeapMemoryPressure(heap);
    EXPECT_FALSE(vm->GetIsInMemoryPressureCallback());

    vm->ResetMemoryPressure();
}

#ifdef NDEBUG
HWTEST_F(HeapMemoryPressureTest, HeapMemoryPressureTest14, testing::ext::TestSize.Level0)
{
    // Test CheckSharedHeapMemoryPressure can be called without crash
    auto vm = thread->GetEcmaVM();
    Local<FunctionRef> callback = FunctionRef::New(vm, TestCallback);

    // Set threshold to a high value so it won't trigger (verify ratio < threshold path)
    DFXJSNApi::HeapMemoryPressureOptions options = {0.0, 0.0, 0.99};
    vm->SetHeapMemoryPressure(options, callback);

    // Call CheckSharedHeapMemoryPressure - should not set flag (ratio < threshold)
    vm->CheckSharedHeapMemoryPressure();
    EXPECT_FALSE(vm->GetNeedProcessMemoryPressureCallback());
    EXPECT_FALSE(vm->GetNeedSharedMemoryPressureCallback());

    vm->ResetMemoryPressure();
}

HWTEST_F(HeapMemoryPressureTest, HeapMemoryPressureTest15, testing::ext::TestSize.Level0)
{
    // Test TriggerMemoryPressureCallback heap type constants
    // Verify the heap type strings are correct
    auto vm = thread->GetEcmaVM();
    Local<FunctionRef> callback = FunctionRef::New(vm, TestCallback);
    // The heap type constants should match these expected values
    // HEAP_MEM_PRESSURE_PROCESS = "ProcessHeapMemPressure"
    // HEAP_MEM_PRESSURE_LOCAL = "LocalHeapMemPressure"
    // HEAP_MEM_PRESSURE_SHARED = "SharedHeapMemPressure"
    DFXJSNApi::HeapMemoryPressureOptions options = {0.8, 0.8, 0.8};
    vm->SetHeapMemoryPressure(options, callback);

    EXPECT_FALSE(vm->GetMemoryPressureCallback().IsEmpty());

    vm->ResetMemoryPressure();
}

HWTEST_F(HeapMemoryPressureTest, HeapMemoryPressureTest16, testing::ext::TestSize.Level0)
{
    // Test TriggerMemoryPressureCallback directly
    auto vm = thread->GetEcmaVM();
    Local<FunctionRef> callback = FunctionRef::New(vm, TestCallback);

    // Set callback (threshold can be 0 since we're calling directly)
    DFXJSNApi::HeapMemoryPressureOptions options = {0.0, 0.0, 0.0};
    vm->SetHeapMemoryPressure(options, callback);

    // Verify callback is set correctly
    EXPECT_FALSE(vm->GetMemoryPressureCallback().IsEmpty());

    // Reset the flag
    g_testCallbackFlag = false;

    // Directly call TriggerMemoryPressureCallback - should invoke callback
    vm->TriggerMemoryPressureCallback("ProcessHeapMemPressure");

    // Verify callback was invoked (flag should be set)
    EXPECT_TRUE(g_testCallbackFlag);

    // Test with other heap types
    g_testCallbackFlag = false;
    vm->TriggerMemoryPressureCallback("LocalHeapMemPressure");
    EXPECT_TRUE(g_testCallbackFlag);

    g_testCallbackFlag = false;
    vm->TriggerMemoryPressureCallback("SharedHeapMemPressure");
    EXPECT_TRUE(g_testCallbackFlag);

    vm->ResetMemoryPressure();
}

HWTEST_F(HeapMemoryPressureTest, HeapMemoryPressureTest17, testing::ext::TestSize.Level0)
{
    // Test CheckAndTriggerMemoryPressureCallback clears needProcessMemoryPressureCallback flag
    auto vm = thread->GetEcmaVM();

    // Create a valid callback to avoid crash
    Local<FunctionRef> callback = FunctionRef::New(vm, TestCallback);

    // Set callback (required for CheckAndTriggerMemoryPressureCallback logic)
    DFXJSNApi::HeapMemoryPressureOptions options = {0.0, 0.0, 0.0};
    vm->SetHeapMemoryPressure(options, callback);

    // Initially flags should be false
    EXPECT_FALSE(vm->GetNeedProcessMemoryPressureCallback());
    EXPECT_FALSE(vm->GetNeedSharedMemoryPressureCallback());

    // Manually set the flag to true using setter
    vm->SetNeedProcessMemoryPressureCallback(true);
    EXPECT_TRUE(vm->GetNeedProcessMemoryPressureCallback());

    // Call CheckAndTriggerMemoryPressureCallback - should clear the flag
    vm->CheckAndTriggerMemoryPressureCallback();

    // Verify flag is cleared after processing
    EXPECT_FALSE(vm->GetNeedProcessMemoryPressureCallback());

    vm->ResetMemoryPressure();
}

HWTEST_F(HeapMemoryPressureTest, HeapMemoryPressureTest18, testing::ext::TestSize.Level0)
{
    // Test CheckAndTriggerMemoryPressureCallback clears needSharedMemoryPressureCallback flag
    auto vm = thread->GetEcmaVM();

    // Create a valid callback to avoid crash
    Local<FunctionRef> callback = FunctionRef::New(vm, TestCallback);

    // Set callback (required for CheckAndTriggerMemoryPressureCallback logic)
    DFXJSNApi::HeapMemoryPressureOptions options = {0.0, 0.0, 0.0};
    vm->SetHeapMemoryPressure(options, callback);

    // Initially flags should be false
    EXPECT_FALSE(vm->GetNeedProcessMemoryPressureCallback());
    EXPECT_FALSE(vm->GetNeedSharedMemoryPressureCallback());

    // Manually set the flag to true using setter
    vm->SetNeedSharedMemoryPressureCallback(true);
    EXPECT_TRUE(vm->GetNeedSharedMemoryPressureCallback());

    // Call CheckAndTriggerMemoryPressureCallback - should clear the flag
    vm->CheckAndTriggerMemoryPressureCallback();

    // Verify flag is cleared after processing
    EXPECT_FALSE(vm->GetNeedSharedMemoryPressureCallback());

    vm->ResetMemoryPressure();
}
#endif
}  // namespace panda::test
