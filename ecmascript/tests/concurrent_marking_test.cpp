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

#include "ecmascript/tests/test_helper.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/verification.h"

using namespace panda::ecmascript;

namespace panda::test {
class ConcurrentMarkingTest : public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
        instance->SetEnableForceGC(false);
        auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
        heap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::ENABLE);
    }

    JSHandle<TaggedArray> CreateTaggedArray(uint32_t length, JSTaggedValue initVal, MemSpaceType spaceType)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        return factory->NewTaggedArray(length, initVal, spaceType);
    }
};

HWTEST_F_L0(ConcurrentMarkingTest, PerformanceWithConcurrentMarking)
{
    uint32_t length = 1_KB;
    JSHandle<TaggedArray> rootArray =
        CreateTaggedArray(length, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    for (uint32_t i = 0; i < length; i++) {
        auto array = CreateTaggedArray(length, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        rootArray->Set(thread, i, array);
    }
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->TriggerConcurrentMarking();  // concurrent mark
    for (uint32_t i = 0; i < length; i++) {
        auto array = CreateTaggedArray(length, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        rootArray->Set(thread, i, array);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
}

HWTEST_F_L0(ConcurrentMarkingTest, PerformanceWithoutConcurrentMarking)
{
    uint32_t length = 1_KB;
    JSHandle<TaggedArray> rootArray =
        CreateTaggedArray(length, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    for (uint32_t i = 0; i < length; i++) {
        auto array = CreateTaggedArray(length, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        rootArray->Set(thread, i, array);
    }
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    for (uint32_t i = 0; i < length; i++) {
        auto array = CreateTaggedArray(length, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        rootArray->Set(thread, i, array);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
}

HWTEST_F_L0(ConcurrentMarkingTest, ConcurrentMarkingWithOldSpace)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    if (heap->GetConcurrentMarker()->IsEnabled()) {
        heap->SetFullMarkRequestedState(false);
        {
            [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
            uint32_t length = 1_KB;
            for (uint32_t i = 0; i < length * 2; i++) {
                [[maybe_unused]] auto array =
                    CreateTaggedArray(length, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
            }

            heap->GetOldSpace()->SetInitialCapacity(static_cast<size_t>(length));
            EXPECT_FALSE(heap->IsConcurrentFullMark());
            heap->TryTriggerConcurrentMarking();
            EXPECT_TRUE(heap->IsConcurrentFullMark());
        }
    }
}

HWTEST_F_L0(ConcurrentMarkingTest, ConcurrentMarkingWithNewSpace)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->SetFullMarkRequestedState(false);
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        uint32_t length = 1_KB;
        for (uint32_t i = 0; i < length * 2; i++) {
            [[maybe_unused]] auto array =
                CreateTaggedArray(length, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        }

        heap->GetNewSpace()->SetInitialCapacity(static_cast<size_t>(length));
        EXPECT_FALSE(heap->IsConcurrentFullMark());
        heap->TryTriggerConcurrentMarking();
        EXPECT_TRUE(!heap->IsConcurrentFullMark());
    }
}

HWTEST_F_L0(ConcurrentMarkingTest, ConcurrentMarkingWithFreshRegion)
{
    Heap *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    if (heap->GetConcurrentMarker()->IsEnabled()) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

        heap->CollectGarbage(TriggerGCType::FULL_GC);
        JSHandle<TaggedArray> arr = factory->NewTaggedArray(1);
        EXPECT_TRUE(!thread->IsConcurrentMarkingOrFinished());

        SemiSpace *space = heap->GetNewSpace();
        EXPECT_TRUE(space->Expand(false));
        Region *region = space->GetCurrentRegion();
        EXPECT_TRUE(!region->IsFreshRegion());

        JSHandle<JSHClass> hclass(thread, thread->GlobalConstants()->GetObjectClass().GetTaggedObject());
        uint32_t numInlinedProps = hclass->GetInlinedProperties();
        EXPECT_TRUE(numInlinedProps == JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS);
        uint32_t size = hclass->GetObjectSize();
        EXPECT_TRUE(size == JSObject::SIZE + numInlinedProps * sizeof(JSTaggedValue));
        uintptr_t addr = space->Allocate(size);
        EXPECT_TRUE(addr != 0);
        JSObject *obj = reinterpret_cast<JSObject*>(addr);

        JSHandle<TaggedArray> emptyArray = factory->EmptyArray();
        factory->InitializeExtraProperties(hclass, obj, numInlinedProps);
        obj->InitializeHash();
        obj->SetElements(thread, emptyArray, SKIP_BARRIER);
        obj->SetProperties(thread, emptyArray, SKIP_BARRIER);
        obj->SynchronizedSetClass(thread, *hclass);

        arr->Set(thread, 0, JSTaggedValue(obj));

        heap->SetMarkType(MarkType::MARK_YOUNG);
        heap->TriggerConcurrentMarking();
        EXPECT_TRUE(thread->IsConcurrentMarkingOrFinished());
        EXPECT_TRUE(region->IsHalfFreshRegion());
        region->SetRegionTypeFlag(RegionTypeFlag::FRESH);
        EXPECT_TRUE(region->IsFreshRegion());

        heap->WaitConcurrentMarkingFinished();
        JSHandle<JSObject> objHandle(thread, obj);
        heap->GetConcurrentMarker()->HandleMarkingFinished();
    }
}

HWTEST_F_L0(ConcurrentMarkingTest, ConcurrentMarkingRequestBySharedSize)
{
    SharedHeap::GetInstance()->TryTriggerLocalConcurrentMarking();
}
}  // namespace panda::test
