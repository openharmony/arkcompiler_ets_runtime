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

#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class SharedTestSpace;
class SharedPartialGCTest : public BaseTestWithScope<false> {
public:
    JSHandle<TaggedObject> CreateSharedObjectsInOneRegion(std::shared_ptr<SharedTestSpace> space, double aliveRate);
    void InitTaggedArray(TaggedObject *obj, size_t arrayLen);
    void CreateTaggedArray();
};

class SharedTestSpace : public Space {
public:
    static constexpr size_t CAP = 10 * 1024 * 1024;
    explicit SharedTestSpace(SharedHeap *heap)
        : Space(heap, heap->GetHeapRegionAllocator(), MemSpaceType::SHARED_OLD_SPACE, CAP, CAP), sHeap_(heap) {}
    ~SharedTestSpace() override = default;
    NO_COPY_SEMANTIC(SharedTestSpace);
    NO_MOVE_SEMANTIC(SharedTestSpace);

    void Expand(JSThread *thread)
    {
        Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE, thread, sHeap_);
        FillBumpPointer();
        allocator_.Reset(region->GetBegin(), region->GetEnd());
    }

    uintptr_t Allocate(size_t size)
    {
        return allocator_.Allocate(size);
    }

    uintptr_t GetTop()
    {
        return allocator_.GetTop();
    }

    void FillBumpPointer()
    {
        auto begin = allocator_.GetTop();
        auto size = allocator_.Available();
        FreeObject::FillFreeObject(sHeap_, begin, size);
    }

    uintptr_t GetEnd()
    {
        return allocator_.GetEnd();
    }
private:
    SharedHeap *sHeap_ {nullptr};
    BumpPointerAllocator allocator_;
};

void SharedPartialGCTest::InitTaggedArray(TaggedObject *obj, size_t arrayLen)
{
    JSHClass *arrayClass = JSHClass::Cast(thread->GlobalConstants()->GetTaggedArrayClass().GetTaggedObject());
    obj->SetClassWithoutBarrier(arrayClass);
    TaggedArray::Cast(obj)->InitializeWithSpecialValue(JSTaggedValue::Undefined(), arrayLen);
}

JSHandle<TaggedObject> SharedPartialGCTest::CreateSharedObjectsInOneRegion(std::shared_ptr<SharedTestSpace> space,
    double aliveRate)
{
    constexpr size_t TAGGED_TYPE_SIZE = 8;
    space->Expand(thread);
    size_t totalSize = space->GetEnd() - space->GetTop();
    size_t alive = totalSize * aliveRate;
    size_t arrayLen = alive / TAGGED_TYPE_SIZE;
    size_t size = TaggedArray::ComputeSize(TAGGED_TYPE_SIZE, arrayLen);
    TaggedObject *obj = reinterpret_cast<TaggedObject *>(space->Allocate(size));
    EXPECT_TRUE(obj != nullptr);
    InitTaggedArray(obj, arrayLen);
    return JSHandle<TaggedObject>(thread, obj);
}

HWTEST_F_L0(SharedPartialGCTest, PartialGCTest)
{
    constexpr double ALIVE_RATE = 0.1;
    constexpr size_t ARRAY_SIZE = SharedOldSpace::MIN_COLLECT_REGION_SIZE;
    instance->GetJSOptions().SetEnableForceGC(false);
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();
    ObjectFactory *factory = heap->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> localObj = factory->NewTaggedArray(ARRAY_SIZE, JSTaggedValue::Undefined(), false);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
    heap->GetHeapPrepare();
    SharedOldSpace *sOldSpace = sHeap->GetOldSpace();
    std::shared_ptr<SharedTestSpace> space= std::make_shared<SharedTestSpace>(sHeap);
    std::vector<std::pair<Region*, JSHandle<TaggedObject>>> checkObjList;
    for (size_t i = 0; i < SharedOldSpace::MIN_COLLECT_REGION_SIZE; i++) {
        auto obj = CreateSharedObjectsInOneRegion(space, ALIVE_RATE);
        Region *region = Region::ObjectAddressToRange(*obj);
        checkObjList.emplace_back(region, obj);
        sOldSpace->AddRegion(region);
    }
    space->FillBumpPointer();
    EXPECT_TRUE(sHeap->CheckCanTriggerConcurrentMarking(thread));
    sHeap->TriggerConcurrentMarking<TriggerGCType::SHARED_PARTIAL_GC, MarkReason::OTHER>(thread);
    while (!thread->HasSuspendRequest());
    thread->CheckSafepointIfSuspended();
    if (thread->IsSharedConcurrentMarkingOrFinished()) {
        EXPECT_TRUE(sOldSpace->GetCollectSetRegionCount() > 0);
        Region *localRegion = Region::ObjectAddressToRange(*localObj);
        for (uint32_t i = 0; i < SharedOldSpace::MIN_COLLECT_REGION_SIZE; i++) {
            auto each = checkObjList[i];
            Region *checkRegion = each.first;
            JSHandle<TaggedObject> checkObj = each.second;
            EXPECT_TRUE(checkRegion->InSCollectSet());
            localObj->Set(thread, i, checkObj);
            JSTaggedType *localSlot = localObj->GetData() + i;
            EXPECT_TRUE(localRegion->TestLocalToShare(reinterpret_cast<uintptr_t>(localSlot)));
        }
    }
    sHeap->WaitGCFinished(thread);
}
} // namespace panda::test
