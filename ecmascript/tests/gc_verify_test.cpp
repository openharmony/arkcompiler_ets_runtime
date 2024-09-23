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
#include "ecmascript/ecma_vm.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/tests/ecma_test_common.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/mem/partial_gc.h"
#include "ecmascript/mem/full_gc.h"

using namespace panda;

using namespace panda::ecmascript;

//In this test case objects cannot be created frequently, because of the HEAP_VERIFY switch is turned on,
//which is easy to cause the test cases to time out.
namespace panda::test {
class GCTest : public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        options.SetEnableEdenGC(true);
        options.SetArkProperties(options.GetArkProperties() | ArkProperties::ENABLE_HEAP_VERIFY);
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

HWTEST_F_L0(GCTest, VerificationTest1)
{
    Heap *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetMarkType(MarkType::MARK_EDEN);
    auto partialGc = heap->GetPartialGC();
    partialGc->RunPhases();
}

HWTEST_F_L0(GCTest, VerificationTest2)
{
    Heap *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetMarkType(MarkType::MARK_YOUNG);
    auto partialGc = heap->GetPartialGC();
    partialGc->RunPhases();
}

HWTEST_F_L0(GCTest, VerificationTest3)
{
    Heap *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetMarkType(MarkType::MARK_FULL);
    auto partialGc = heap->GetPartialGC();
    partialGc->RunPhases();
}

HWTEST_F_L0(GCTest, VerificationTest4)
{
    Heap *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto fullGc = heap->GetFullGC();
    fullGc->RunPhases();
}

HWTEST_F_L0(GCTest, SharedHeapVerificationTest)
{
    SharedHeap *sHeap = SharedHeap::GetInstance();
    JSRuntimeOptions options;
    options.SetEnableEdenGC(true);
    options.SetArkProperties(options.GetArkProperties() | ArkProperties::ENABLE_HEAP_VERIFY);
    auto nativeAreaAllocator_ = std::make_unique<NativeAreaAllocator>();
    auto heapRegionAllocator_ = std::make_unique<HeapRegionAllocator>();
    sHeap->Initialize(nativeAreaAllocator_.get(), heapRegionAllocator_.get(), options, DaemonThread::GetInstance());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
    auto oldSizebase = sHeap->GetOldSpace()->GetHeapObjectSize();
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        factory->NewSOldSpaceTaggedArray(1024, JSTaggedValue::Undefined());
    }
    size_t oldSizeBefore = sHeap->GetOldSpace()->GetHeapObjectSize();
    EXPECT_TRUE(oldSizeBefore > oldSizebase);
    sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
    auto oldSizeAfter = sHeap->GetOldSpace()->GetHeapObjectSize();
    EXPECT_TRUE(oldSizeBefore > oldSizeAfter);
}
} // namespace panda::test
