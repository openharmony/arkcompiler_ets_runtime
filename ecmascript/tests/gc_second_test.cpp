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
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/full_gc.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/stw_young_gc.h"
#include "ecmascript/mem/partial_gc.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class GCTest : public testing::Test {
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

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(GCTest, NativeGCTestConcurrentMarkDisabled)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    // Disable concurrent mark.
    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    size_t oldNativeSize = heap->GetNativeBindingSize();
    size_t newNativeSize = heap->GetNativeBindingSize();
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        auto newData = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1 * 1024 * 1024);
        [[maybe_unused]] JSHandle<JSNativePointer> obj = factory->NewJSNativePointer(newData,
            NativeAreaAllocator::FreeBufferFunc, nullptr, false, 1 * 1024 * 1024);
        newNativeSize = heap->GetNativeBindingSize();
        EXPECT_EQ(newNativeSize - oldNativeSize, 1UL * 1024 * 1024);

        auto newData1 = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1 * 1024 * 1024);
        [[maybe_unused]] JSHandle<JSNativePointer> obj2 = factory->NewJSNativePointer(newData1,
            NativeAreaAllocator::FreeBufferFunc, nullptr, false, 1 * 1024 * 1024);

        EXPECT_TRUE(newNativeSize - oldNativeSize > 0);
        EXPECT_TRUE(newNativeSize - oldNativeSize <= 2 * 1024 *1024);
        for (int i = 0; i < 2048; i++) {
            [[maybe_unused]] ecmascript::EcmaHandleScope baseScopeForeach(thread);
            auto newData2 = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1 * 1024);
            [[maybe_unused]] JSHandle<JSNativePointer> obj3 = factory->NewJSNativePointer(newData2,
                NativeAreaAllocator::FreeBufferFunc, nullptr, false, 1 * 1024 * 1024);
        }
    }
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::OLD_GC);
    newNativeSize = heap->GetNativeBindingSize();
    EXPECT_EQ(newNativeSize - oldNativeSize, 0UL);
}

HWTEST_F_L0(GCTest, NonNewSpaceNativeGCTestConcurrentMarkDisabled)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    // Disable concurrent mark.
    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    size_t oldNativeSize = heap->GetNativeBindingSize();
    size_t newNativeSize = heap->GetNativeBindingSize();
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        auto newData = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1 * 1024 * 1024);
        [[maybe_unused]] JSHandle<JSNativePointer> obj = factory->NewJSNativePointer(newData,
            NativeAreaAllocator::FreeBufferFunc, nullptr, true, 1 * 1024 * 1024);
        newNativeSize = heap->GetNativeBindingSize();
        EXPECT_EQ(newNativeSize - oldNativeSize, 1UL * 1024 * 1024);

        auto newData1 = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1 * 1024 * 1024);
        [[maybe_unused]] JSHandle<JSNativePointer> obj2 = factory->NewJSNativePointer(newData1,
            NativeAreaAllocator::FreeBufferFunc, nullptr, false, 1 * 1024 * 1024);

        EXPECT_TRUE(newNativeSize - oldNativeSize > 0);
        EXPECT_TRUE(newNativeSize - oldNativeSize <= 2 * 1024 *1024);
        for (int i = 0; i < 2048; i++) {
            [[maybe_unused]] ecmascript::EcmaHandleScope baseScopeForeach(thread);
            auto newData2 = thread->GetEcmaVM()->GetNativeAreaAllocator()->AllocateBuffer(1 * 1024);
            // malloc size is smaller to avoid test fail in the small devices.
            [[maybe_unused]] JSHandle<JSNativePointer> obj3 = factory->NewJSNativePointer(newData2,
                NativeAreaAllocator::FreeBufferFunc, nullptr, true, 1 * 1024 * 1024);
        }
        newNativeSize = heap->GetNativeBindingSize();
        // Old GC should be trigger here, so the size should be reduced.
        EXPECT_TRUE(newNativeSize - oldNativeSize < 2048 * 1024 *1024);
    }
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::OLD_GC);
    newNativeSize = heap->GetNativeBindingSize();
    EXPECT_EQ(newNativeSize - oldNativeSize, 0UL);
}

HWTEST_F_L0(GCTest, ArkToolsForceFullGC)
{
    const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);
    size_t originalHeapSize = thread->GetEcmaVM()->GetHeap()->GetCommittedSize();
    size_t newSize = originalHeapSize;
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);

        for (int i = 0; i < 10; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> obj = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(1024 * 1024);
        }
        newSize = thread->GetEcmaVM()->GetHeap()->GetCommittedSize();
    }
    EXPECT_TRUE(newSize > originalHeapSize);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 0);

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    [[maybe_unused]] JSTaggedValue result1 = builtins::BuiltinsArkTools::ForceFullGC(ecmaRuntimeCallInfo);

    ASSERT_TRUE(thread->GetEcmaVM()->GetHeap()->GetCommittedSize() < newSize);
}

HWTEST_F_L0(GCTest, ColdStartForceExpand)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    size_t originalHeapSize = heap->GetCommittedSize();
    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    heap->NotifyPostFork();
    heap->NotifyFinishColdStartSoon();
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 500; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                10 * 1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        }
    }
    size_t expandHeapSize = thread->GetEcmaVM()->GetHeap()->GetCommittedSize();
    usleep(2500000);
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 100; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                10 * 1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        }
    }
    size_t newSize = thread->GetEcmaVM()->GetHeap()->GetCommittedSize();
    EXPECT_TRUE(originalHeapSize < expandHeapSize);
    EXPECT_TRUE(expandHeapSize > newSize);
}
} // namespace panda::test
