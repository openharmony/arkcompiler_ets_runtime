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
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/tests/ecma_test_common.h"

#if defined(PANDA_TARGET_OHOS) && !defined(STANDALONE_MODE)
#include "parameters.h"
#endif

using namespace panda::ecmascript;

namespace panda::test {

class HeapTest :  public BaseTestWithScope<false> {
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

class HeapTestHelper {
public:
    explicit HeapTestHelper(Heap *heap) : heap_(heap) {}

    ~HeapTestHelper() = default;

    Heap *GetHeap()
    {
        return heap_;
    }

private:
    Heap *heap_{nullptr};
};

#ifdef USE_CMC_GC
HWTEST_F_L0(HeapTest, TryTriggerConcurrentMarking1)
{
    bool temp = g_isEnableCMCGC;
    g_isEnableCMCGC = true;
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    ASSERT_NE(heap, nullptr);
    EXPECT_EQ(heap->TryTriggerConcurrentMarking(MarkReason::ALLOCATION_LIMIT), false);
    g_isEnableCMCGC = temp;
}

HWTEST_F_L0(HeapTest, TryTriggerConcurrentMarking2)
{
    bool temp = g_isEnableCMCGC;
    g_isEnableCMCGC = false;
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    ASSERT_NE(heap, nullptr);

    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    EXPECT_EQ(heap->CheckCanTriggerConcurrentMarking(), false);
    EXPECT_EQ(heap->TryTriggerConcurrentMarking(MarkReason::ALLOCATION_LIMIT), false);
    g_isEnableCMCGC = temp;
}

HWTEST_F_L0(HeapTest, TryTriggerConcurrentMarking3)
{
    if constexpr (G_USE_CMS_GC) {
        return;
    }
    bool temp = g_isEnableCMCGC;
    g_isEnableCMCGC = false;
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    ASSERT_NE(heap, nullptr);

    heap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    EXPECT_EQ(heap->CheckCanTriggerConcurrentMarking(), true);

    uint32_t length = 1000;
    heap->GetOldSpace()->SetInitialCapacity(static_cast<size_t>(length));
    heap->SetFullMarkRequestedState(true);
    EXPECT_EQ(heap->TryTriggerConcurrentMarking(MarkReason::ALLOCATION_LIMIT), true);
    EXPECT_EQ(heap->GetMarkType(), MarkType::MARK_FULL);
    g_isEnableCMCGC = temp;
}

HWTEST_F_L0(HeapTest, TryTriggerConcurrentMarking4)
{
    bool temp = g_isEnableCMCGC;
    g_isEnableCMCGC = false;
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    ASSERT_NE(heap, nullptr);

    heap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    heap->SetFullMarkRequestedState(false);
    EXPECT_EQ(heap->CheckCanTriggerConcurrentMarking(), true);
    EXPECT_EQ(heap->IsFullMarkRequested(), false);

    heap->SetSensitiveStatus(AppSensitiveStatus::ENTER_HIGH_SENSITIVE);
    EXPECT_EQ(heap->InSensitiveStatus(), true);
    heap->SetRecordHeapObjectSizeBeforeSensitive(1 * 1024 * 1024);
    EXPECT_EQ(heap->ObjectExceedHighSensitiveThresholdForCM(), false);
    EXPECT_EQ(heap->TryTriggerConcurrentMarking(MarkReason::ALLOCATION_LIMIT), false);
    g_isEnableCMCGC = temp;
}

HWTEST_F_L0(HeapTest, TryTriggerConcurrentMarking5)
{
    bool temp = g_isEnableCMCGC;
    g_isEnableCMCGC = false;
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    ASSERT_NE(heap, nullptr);

    heap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    heap->SetFullMarkRequestedState(false);
    EXPECT_EQ(heap->CheckCanTriggerConcurrentMarking(), true);
    EXPECT_EQ(heap->IsFullMarkRequested(), false);

    heap->NotifyPostFork();
    EXPECT_EQ(heap->FinishStartupEvent(), true);
    EXPECT_EQ(heap->IsJustFinishStartup(), true);
    EXPECT_EQ(heap->TryTriggerConcurrentMarking(MarkReason::ALLOCATION_LIMIT), false);
    g_isEnableCMCGC = temp;
}

HWTEST_F_L0(HeapTest, TryTriggerConcurrentMarking6)
{
    if constexpr (G_USE_CMS_GC) {
        return;
    }
    bool temp = g_isEnableCMCGC;
    g_isEnableCMCGC = false;
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    ASSERT_NE(heap, nullptr);

    heap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    heap->SetFullMarkRequestedState(false);
    EXPECT_EQ(heap->CheckCanTriggerConcurrentMarking(), true);
    EXPECT_EQ(heap->IsFullMarkRequested(), false);

    heap->GetOldSpace()->SetInitialCapacity(1000);
    EXPECT_EQ(heap->TryTriggerConcurrentMarking(MarkReason::ALLOCATION_LIMIT), true);
    EXPECT_EQ(heap->GetMarkType(), MarkType::MARK_FULL);
    g_isEnableCMCGC = temp;
}

HWTEST_F_L0(HeapTest, TryTriggerConcurrentMarking7)
{
    if constexpr (G_USE_CMS_GC) {
        return;
    }
    bool temp = g_isEnableCMCGC;
    g_isEnableCMCGC = false;
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    ASSERT_NE(heap, nullptr);

    heap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    heap->SetFullMarkRequestedState(false);
    EXPECT_EQ(heap->CheckCanTriggerConcurrentMarking(), true);
    EXPECT_EQ(heap->IsFullMarkRequested(), false);

    heap->GetNewSpace()->IncreaseCommitted(10000000);
    EXPECT_EQ(heap->TryTriggerConcurrentMarking(MarkReason::ALLOCATION_LIMIT), true);
    EXPECT_EQ(heap->GetMarkType(), MarkType::MARK_YOUNG);
    EXPECT_EQ(heap->GetEcmaGCStats()->GetMarkReason(), MarkReason::ALLOCATION_LIMIT);
    g_isEnableCMCGC = temp;
}

#if defined(PANDA_TARGET_OHOS) && !defined(STANDALONE_MODE)
HWTEST_F_L0(HeapTest, TryTriggerConcurrentMarking8)
{
    OHOS::system::SetParameter("persist.ark.sensitive.threshold", "3");

    JSRuntimeOptions options;
    auto vm = JSNApi::CreateEcmaVM(options);
    ASSERT_TRUE(vm != nullptr) << "Cannot create EcmaVM";
    auto thread = vm->GetJSThread();
    thread->ManagedCodeBegin();

    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    EXPECT_TRUE(heap != nullptr);

    EXPECT_TRUE(heap->GetEcmaParamConfiguration().GetIncObjSizeThresholdInSensitive() == 40 * 1024 * 1024 ||
           heap->GetEcmaParamConfiguration().GetIncObjSizeThresholdInSensitive() == 80 * 1024 * 1024)
    << "Expected 40MB or 80MB, but got " << heap->GetEcmaParamConfiguration().GetIncObjSizeThresholdInSensitive();

    bool temp = g_isEnableCMCGC;
    g_isEnableCMCGC = false;

    heap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    heap->SetFullMarkRequestedState(false);
    EXPECT_EQ(heap->CheckCanTriggerConcurrentMarking(), true);
    EXPECT_EQ(heap->IsFullMarkRequested(), false);

    heap->SetSensitiveStatus(AppSensitiveStatus::ENTER_HIGH_SENSITIVE);
    EXPECT_EQ(heap->InSensitiveStatus(), true);
    heap->SetRecordHeapObjectSizeBeforeSensitive(1 * 1024 * 1024);
    EXPECT_EQ(heap->ObjectExceedHighSensitiveThresholdForCM(), false);
    EXPECT_EQ(heap->TryTriggerConcurrentMarking(MarkReason::ALLOCATION_LIMIT), false);

    g_isEnableCMCGC = temp;
    JSNApi::DestroyJSVM(vm);
}

HWTEST_F_L0(HeapTest, TryTriggerConcurrentMarking9)
{
    OHOS::system::SetParameter("persist.ark.sensitive.threshold", "350");

    JSRuntimeOptions options;
    auto vm = JSNApi::CreateEcmaVM(options);
    ASSERT_TRUE(vm != nullptr) << "Cannot create EcmaVM";
    auto thread = vm->GetJSThread();
    thread->ManagedCodeBegin();

    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    EXPECT_TRUE(heap != nullptr);

    EXPECT_TRUE(heap->GetEcmaParamConfiguration().GetIncObjSizeThresholdInSensitive() == 40 * 1024 * 1024 ||
           heap->GetEcmaParamConfiguration().GetIncObjSizeThresholdInSensitive() == 80 * 1024 * 1024)
    << "Expected 40MB or 80MB, but got " << heap->GetEcmaParamConfiguration().GetIncObjSizeThresholdInSensitive();

    bool temp = g_isEnableCMCGC;
    g_isEnableCMCGC = false;

    heap->GetConcurrentMarker()->ConfigConcurrentMark(true);
    heap->SetFullMarkRequestedState(false);
    EXPECT_EQ(heap->CheckCanTriggerConcurrentMarking(), true);
    EXPECT_EQ(heap->IsFullMarkRequested(), false);

    heap->SetSensitiveStatus(AppSensitiveStatus::ENTER_HIGH_SENSITIVE);
    EXPECT_EQ(heap->InSensitiveStatus(), true);
    heap->SetRecordHeapObjectSizeBeforeSensitive(1 * 1024 * 1024);
    EXPECT_EQ(heap->ObjectExceedHighSensitiveThresholdForCM(), false);
    EXPECT_EQ(heap->TryTriggerConcurrentMarking(MarkReason::ALLOCATION_LIMIT), false);

    g_isEnableCMCGC = temp;
    JSNApi::DestroyJSVM(vm);
}
#endif

HWTEST_F_L0(HeapTest, ObjectExceedHighSensitiveThresholdForCM1)
{
    OHOS::system::SetParameter("persist.ark.sensitive.threshold", "5");

    JSRuntimeOptions options;
    auto vm = JSNApi::CreateEcmaVM(options);
    ASSERT_TRUE(vm != nullptr) << "Cannot create EcmaVM";
    auto thread = vm->GetJSThread();
    thread->ManagedCodeBegin();

    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    EXPECT_TRUE(heap != nullptr);

    EXPECT_EQ(heap->GetEcmaParamConfiguration().GetIncObjSizeThresholdInSensitive(), 5 * 1024 * 1024);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 1024; i++) {
            factory->NewTaggedArray(500, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        }
        for (int i = 0; i < 1024; i++) {
            factory->NewTaggedArray(500, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        }
    }

    heap->SetRecordHeapObjectSizeBeforeSensitive(1 * 1024 * 1024);
    EXPECT_EQ(heap->ObjectExceedHighSensitiveThresholdForCM(), true)
    << "heap->GetHeapObjectSize() is " << heap->GetHeapObjectSize()
    << ", heap->GetRecordHeapObjectSizeBeforeSensitive() is " << heap->GetRecordHeapObjectSizeBeforeSensitive()
    << ", config_.GetIncObjSizeThresholdInSensitive() is "
    << heap->GetEcmaParamConfiguration().GetIncObjSizeThresholdInSensitive();
    JSNApi::DestroyJSVM(vm);
}

HWTEST_F_L0(HeapTest, ObjectExceedHighSensitiveThresholdForCM2)
{
    OHOS::system::SetParameter("persist.ark.sensitive.threshold", "50");

    JSRuntimeOptions options;
    auto vm = JSNApi::CreateEcmaVM(options);
    ASSERT_TRUE(vm != nullptr) << "Cannot create EcmaVM";
    auto thread = vm->GetJSThread();
    thread->ManagedCodeBegin();

    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    EXPECT_TRUE(heap != nullptr);

    EXPECT_EQ(heap->GetEcmaParamConfiguration().GetIncObjSizeThresholdInSensitive(), 50 * 1024 * 1024);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 1024; i++) {
            factory->NewTaggedArray(5000, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        }
        for (int i = 0; i < 1024; i++) {
            factory->NewTaggedArray(5000, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        }
    }

    heap->SetRecordHeapObjectSizeBeforeSensitive(1 * 1024 * 1024);
    EXPECT_EQ(heap->ObjectExceedHighSensitiveThresholdForCM(), true)
    << "heap->GetHeapObjectSize() is " << heap->GetHeapObjectSize()
    << ", heap->GetRecordHeapObjectSizeBeforeSensitive() is " << heap->GetRecordHeapObjectSizeBeforeSensitive()
    << ", config_.GetIncObjSizeThresholdInSensitive() is "
    << heap->GetEcmaParamConfiguration().GetIncObjSizeThresholdInSensitive();
    JSNApi::DestroyJSVM(vm);
}

HWTEST_F_L0(HeapTest, ObjectExceedHighSensitiveThresholdForCM3)
{
    OHOS::system::SetParameter("persist.ark.sensitive.threshold", "320");

    JSRuntimeOptions options;
    auto vm = JSNApi::CreateEcmaVM(options);
    ASSERT_TRUE(vm != nullptr) << "Cannot create EcmaVM";
    auto thread = vm->GetJSThread();
    thread->ManagedCodeBegin();

    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    EXPECT_TRUE(heap != nullptr);

    EXPECT_EQ(heap->GetEcmaParamConfiguration().GetIncObjSizeThresholdInSensitive(), 320 * 1024 * 1024);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 1024; i++) {
            factory->NewTaggedArray(500, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        }
        for (int i = 0; i < 1024; i++) {
            factory->NewTaggedArray(500, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        }
    }

    heap->SetRecordHeapObjectSizeBeforeSensitive(1 * 1024 * 1024);
    EXPECT_EQ(heap->ObjectExceedHighSensitiveThresholdForCM(), false)
    << "heap->GetHeapObjectSize() is " << heap->GetHeapObjectSize()
    << ", heap->GetRecordHeapObjectSizeBeforeSensitive() is " << heap->GetRecordHeapObjectSizeBeforeSensitive()
    << ", config_.GetIncObjSizeThresholdInSensitive() is "
    << heap->GetEcmaParamConfiguration().GetIncObjSizeThresholdInSensitive();
    JSNApi::DestroyJSVM(vm);
}

HWTEST_F_L0(HeapTest, GlobalNativeSizeLargerToTriggerGC1)
{
    OHOS::system::SetParameter("persist.ark.native.stepsize", "64");

    JSRuntimeOptions options;
    auto vm = JSNApi::CreateEcmaVM(options);
    ASSERT_TRUE(vm != nullptr) << "Cannot create EcmaVM";
    auto thread = vm->GetJSThread();
    thread->ManagedCodeBegin();

    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    EXPECT_TRUE(heap != nullptr);

    EXPECT_EQ(heap->GetEcmaParamConfiguration().GetStepNativeSizeInc(), 64 * 1024 * 1024);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    {
        for (int i = 0; i < 1024; i++) {
            [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
                auto newData = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(2048);
            [[maybe_unused]] JSHandle<JSNativePointer> obj = factory->NewJSNativePointer(
                newData, NativeAreaAllocator::FreeBufferFunc, nullptr, true, 1024 * 1024 * 1024);
        }
    }

    heap->IncreaseNativeBindingSize(1 * 1024 * 1024);
    heap->ResetNativeSizeAfterLastGC();
    heap->IncreaseNativeBindingSize(11 * 1024 * 1024);
    heap->IncNativeSizeAfterLastGC(60 * 1024 * 1024);
    EXPECT_EQ(heap->GlobalNativeSizeLargerToTriggerGC(), true)
    << "heap->GetGlobalNativeSize() is " << heap->GetGlobalNativeSize()
    << ", nativeSizeTriggerGCThreshold_ is " << heap->GetEcmaParamConfiguration().GetMaxNativeSizeInc()
    << ", nativeBindingSize_ is " << heap->GetNativeBindingSize()
    << ", nativeBindingSizeAfterLastGC_ is " << heap->GetNativeBindingSizeAfterLastGC();
    JSNApi::DestroyJSVM(vm);
}

HWTEST_F_L0(HeapTest, GlobalNativeSizeLargerToTriggerGC2)
{
    OHOS::system::SetParameter("persist.ark.native.stepsize", "1024");

    JSRuntimeOptions options;
    auto vm = JSNApi::CreateEcmaVM(options);
    ASSERT_TRUE(vm != nullptr) << "Cannot create EcmaVM";
    auto thread = vm->GetJSThread();
    thread->ManagedCodeBegin();

    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    EXPECT_TRUE(heap != nullptr);

    EXPECT_EQ(heap->GetEcmaParamConfiguration().GetStepNativeSizeInc(), 1024 * 1024 * 1024);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    {
        for (int i = 0; i < 1024; i++) {
            [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
                auto newData = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(2048);
            [[maybe_unused]] JSHandle<JSNativePointer> obj = factory->NewJSNativePointer(
                newData, NativeAreaAllocator::FreeBufferFunc, nullptr, true, 1024 * 1024 * 1024);
        }
    }

    heap->IncreaseNativeBindingSize(1 * 1024 * 1024);
    heap->ResetNativeSizeAfterLastGC();
    heap->IncreaseNativeBindingSize(11 * 1024 * 1024);
    heap->IncNativeSizeAfterLastGC(300 * 1024 * 1024);
    EXPECT_EQ(heap->GlobalNativeSizeLargerToTriggerGC(), false)
    << "heap->GetGlobalNativeSize() is " << heap->GetGlobalNativeSize()
    << ", nativeSizeTriggerGCThreshold_ is " << heap->GetEcmaParamConfiguration().GetMaxNativeSizeInc()
    << ", nativeBindingSize_ is " << heap->GetNativeBindingSize()
    << ", nativeBindingSizeAfterLastGC_ is " << heap->GetNativeBindingSizeAfterLastGC();
    JSNApi::DestroyJSVM(vm);
}

HWTEST_F_L0(HeapTest, GlobalNativeSizeLargerToTriggerGC3)
{
    OHOS::system::SetParameter("persist.ark.native.stepsize", "63");

    JSRuntimeOptions options;
    auto vm = JSNApi::CreateEcmaVM(options);
    ASSERT_TRUE(vm != nullptr) << "Cannot create EcmaVM";
    auto thread = vm->GetJSThread();
    thread->ManagedCodeBegin();

    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    EXPECT_TRUE(heap != nullptr);

    EXPECT_TRUE(heap->GetEcmaParamConfiguration().GetStepNativeSizeInc() == 256 * 1024 * 1024 ||
           heap->GetEcmaParamConfiguration().GetStepNativeSizeInc() == 300 * 1024 * 1024)
    << "Expected 256MB or 300MB, but got " << heap->GetEcmaParamConfiguration().GetStepNativeSizeInc();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    {
        for (int i = 0; i < 1024; i++) {
            [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
                auto newData = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(2048);
            [[maybe_unused]] JSHandle<JSNativePointer> obj = factory->NewJSNativePointer(
                newData, NativeAreaAllocator::FreeBufferFunc, nullptr, true, 1024 * 1024 * 1024);
        }
    }

    heap->IncreaseNativeBindingSize(1 * 1024 * 1024);
    heap->ResetNativeSizeAfterLastGC();
    heap->IncreaseNativeBindingSize(11 * 1024 * 1024);
    heap->IncNativeSizeAfterLastGC(60 * 1024 * 1024);
    EXPECT_EQ(heap->GlobalNativeSizeLargerToTriggerGC(), false)
    << "heap->GetGlobalNativeSize() is " << heap->GetGlobalNativeSize()
    << ", nativeSizeTriggerGCThreshold_ is " << heap->GetEcmaParamConfiguration().GetMaxNativeSizeInc()
    << ", nativeBindingSize_ is " << heap->GetNativeBindingSize()
    << ", nativeBindingSizeAfterLastGC_ is " << heap->GetNativeBindingSizeAfterLastGC();
    JSNApi::DestroyJSVM(vm);
}

HWTEST_F_L0(HeapTest, GlobalNativeSizeLargerToTriggerGC4)
{
    OHOS::system::SetParameter("persist.ark.native.stepsize", "1025");

    JSRuntimeOptions options;
    auto vm = JSNApi::CreateEcmaVM(options);
    ASSERT_TRUE(vm != nullptr) << "Cannot create EcmaVM";
    auto thread = vm->GetJSThread();
    thread->ManagedCodeBegin();

    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    EXPECT_TRUE(heap != nullptr);

    EXPECT_TRUE(heap->GetEcmaParamConfiguration().GetStepNativeSizeInc() == 256 * 1024 * 1024 ||
           heap->GetEcmaParamConfiguration().GetStepNativeSizeInc() == 300 * 1024 * 1024)
    << "Expected 256MB or 300MB, but got " << heap->GetEcmaParamConfiguration().GetStepNativeSizeInc();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    {
        for (int i = 0; i < 1024; i++) {
            [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
                auto newData = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(2048);
            [[maybe_unused]] JSHandle<JSNativePointer> obj = factory->NewJSNativePointer(
                newData, NativeAreaAllocator::FreeBufferFunc, nullptr, true, 1024 * 1024 * 1024);
        }
    }

    heap->IncreaseNativeBindingSize(1 * 1024 * 1024);
    heap->ResetNativeSizeAfterLastGC();
    heap->IncreaseNativeBindingSize(11 * 1024 * 1024);
    heap->IncNativeSizeAfterLastGC(60 * 1024 * 1024);
    EXPECT_EQ(heap->GlobalNativeSizeLargerToTriggerGC(), false)
    << "heap->GetGlobalNativeSize() is " << heap->GetGlobalNativeSize()
    << ", nativeSizeTriggerGCThreshold_ is " << heap->GetEcmaParamConfiguration().GetMaxNativeSizeInc()
    << ", nativeBindingSize_ is " << heap->GetNativeBindingSize()
    << ", nativeBindingSizeAfterLastGC_ is " << heap->GetNativeBindingSizeAfterLastGC();
    JSNApi::DestroyJSVM(vm);
}

HWTEST_F_L0(HeapTest, GlobalNativeSizeLargerToTriggerGC5)
{
    OHOS::system::SetParameter("persist.ark.native.stepsize", "100");

    JSRuntimeOptions options;
    auto vm = JSNApi::CreateEcmaVM(options);
    ASSERT_TRUE(vm != nullptr) << "Cannot create EcmaVM";
    auto thread = vm->GetJSThread();
    thread->ManagedCodeBegin();

    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    EXPECT_TRUE(heap != nullptr);

    EXPECT_EQ(heap->GetEcmaParamConfiguration().GetStepNativeSizeInc(), 100 * 1024 * 1024);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    {
        for (int i = 0; i < 1024; i++) {
            [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
                auto newData = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(2048);
            [[maybe_unused]] JSHandle<JSNativePointer> obj = factory->NewJSNativePointer(
                newData, NativeAreaAllocator::FreeBufferFunc, nullptr, true, 1024 * 1024 * 1024);
        }
    }

    heap->IncreaseNativeBindingSize(1 * 1024 * 1024);
    heap->ResetNativeSizeAfterLastGC();
    heap->IncreaseNativeBindingSize(11 * 1024 * 1024);
    heap->IncNativeSizeAfterLastGC(100 * 1024 * 1024);
    EXPECT_EQ(heap->GlobalNativeSizeLargerToTriggerGC(), true)
    << "heap->GetGlobalNativeSize() is " << heap->GetGlobalNativeSize()
    << ", nativeSizeTriggerGCThreshold_ is " << heap->GetEcmaParamConfiguration().GetMaxNativeSizeInc()
    << ", nativeBindingSize_ is " << heap->GetNativeBindingSize()
    << ", nativeBindingSizeAfterLastGC_ is " << heap->GetNativeBindingSizeAfterLastGC();
    JSNApi::DestroyJSVM(vm);
}
#endif
}