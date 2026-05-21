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

#include "ecmascript/dfx/hprof/heap_sampling.h"
#include "ecmascript/mem/shared_heap/shared_gc.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class SharedTestSpace;
class SharedPartialGCTest : public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
    }
    JSHandle<TaggedObject> CreateSharedObjectsInOneRegion(std::shared_ptr<SharedTestSpace> space, double aliveRate);

    JSHandle<TaggedArray> AllocateSharedArray(std::shared_ptr<SharedTestSpace> space, size_t arrayLen);
    JSHandle<TaggedArray> AllocateSharedArrayNewRegion(std::shared_ptr<SharedTestSpace> space, size_t arrayLen);

    void InitializeBaseObjects(std::shared_ptr<SharedTestSpace> space);
    void InitTaggedArray(TaggedObject *obj, size_t arrayLen);
    void CreateTaggedArray();
};

class SharedTestSpace : public MonoSpace {
public:
    static constexpr size_t CAP = 10 * 1024 * 1024;
    explicit SharedTestSpace(SharedHeap *heap, JSThread *thread)
        : MonoSpace(heap, heap->GetHeapRegionAllocator(), MemSpaceType::SHARED_OLD_SPACE, CAP, CAP),
          sHeap_(heap), thread_(thread) {}
    ~SharedTestSpace() override = default;
    NO_COPY_SEMANTIC(SharedTestSpace);
    NO_MOVE_SEMANTIC(SharedTestSpace);

    void Expand()
    {
        Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE, thread_, sHeap_);
        FillBumpPointer();
        allocator_.Reset(region->GetBegin(), region->GetEnd());
        regionList_.push_back(region);
    }

    uintptr_t Allocate(size_t size)
    {
        uintptr_t obj = allocator_.Allocate(size);
        if (obj == 0) {
            Expand();
            obj = allocator_.Allocate(size);
        }
        return obj;
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

    uintptr_t Evacuate(TaggedObject *obj)
    {
        size_t size = obj->GetSize();
        uintptr_t toAddress = Allocate(size);
        ASSERT(toAddress != 0);
        if (memcpy_s(reinterpret_cast<void*>(toAddress), size, reinterpret_cast<void*>(obj), size) != EOK) {
            UNREACHABLE();
        }
        Barriers::SetPrimitive(obj, 0, MarkWord::FromForwardingAddress(toAddress));
        return toAddress;
    }

    void Merge()
    {
        FillBumpPointer();
        SharedOldSpace *sOldSpace = sHeap_->GetOldSpace();
        for (auto region : regionList_) {
            sOldSpace->AddRegion(region);
        }
        regionList_.clear();
    }
private:
    SharedHeap *sHeap_ {nullptr};
    BumpPointerAllocator allocator_;
    std::vector<Region*> regionList_ {};
    JSThread *thread_ {nullptr};
};

void SharedPartialGCTest::InitTaggedArray(TaggedObject *obj, size_t arrayLen)
{
    JSHClass *arrayClass = JSHClass::Cast(thread->GlobalConstants()->GetTaggedArrayClass().GetTaggedObject());
    obj->SetClassWithoutBarrier(arrayClass);
    TaggedArray::Cast(obj)->InitializeWithSpecialValue(JSTaggedValue::Undefined(), arrayLen);
}

void SharedPartialGCTest::InitializeBaseObjects(std::shared_ptr<SharedTestSpace> space)
{
    static constexpr size_t SMALL_PER_REGION_SIZE = 240 * 1024 * SharedOldSpace::CSET_REGION_ALIVE_RATIO;
    static constexpr size_t SMALL_ARRAY_LEN = SMALL_PER_REGION_SIZE / 8;
    static constexpr size_t BIG_ARRAY_LEN = 13 * 1024;
    static constexpr size_t SMALL_REGION_COUNT =
        static_cast<size_t>(SharedOldSpace::MAX_EVACUATION_SIZE) / SMALL_PER_REGION_SIZE + 1;
    size_t bigRegionCount = 1;
    if (SMALL_REGION_COUNT < SharedOldSpace::MIN_COLLECT_REGION_SIZE) {
        bigRegionCount = SharedOldSpace::MIN_COLLECT_REGION_SIZE - SMALL_REGION_COUNT;
    }
    for (size_t i = 0; i < SMALL_REGION_COUNT; i++) {
        AllocateSharedArrayNewRegion(space, SMALL_ARRAY_LEN);
    }
    for (size_t i = 0; i < bigRegionCount; i++) {
        AllocateSharedArrayNewRegion(space, BIG_ARRAY_LEN);
    }
}

JSHandle<TaggedObject> SharedPartialGCTest::CreateSharedObjectsInOneRegion(std::shared_ptr<SharedTestSpace> space,
    double aliveRate)
{
    constexpr size_t TAGGED_TYPE_SIZE = 8;
    space->Expand();
    size_t totalSize = space->GetEnd() - space->GetTop();
    size_t alive = totalSize * aliveRate;
    size_t arrayLen = alive / TAGGED_TYPE_SIZE;
    size_t size = TaggedArray::ComputeSize(TAGGED_TYPE_SIZE, arrayLen);
    TaggedObject *obj = reinterpret_cast<TaggedObject *>(space->Allocate(size));
    EXPECT_TRUE(obj != nullptr);
    InitTaggedArray(obj, arrayLen);
    return JSHandle<TaggedObject>(thread, obj);
}

JSHandle<TaggedArray> SharedPartialGCTest::AllocateSharedArray(std::shared_ptr<SharedTestSpace> space, size_t arrayLen)
{
    constexpr size_t TAGGED_TYPE_SIZE = 8;
    size_t size = TaggedArray::ComputeSize(TAGGED_TYPE_SIZE, arrayLen);
    TaggedObject *obj = reinterpret_cast<TaggedObject *>(space->Allocate(size));
    EXPECT_TRUE(obj != nullptr);
    InitTaggedArray(obj, arrayLen);
    return JSHandle<TaggedArray>(thread, obj);
}

JSHandle<TaggedArray> SharedPartialGCTest::AllocateSharedArrayNewRegion(std::shared_ptr<SharedTestSpace> space,
    size_t arrayLen)
{
    constexpr size_t TAGGED_TYPE_SIZE = 8;
    space->Expand();
    size_t size = TaggedArray::ComputeSize(TAGGED_TYPE_SIZE, arrayLen);
    TaggedObject *obj = reinterpret_cast<TaggedObject *>(space->Allocate(size));
    EXPECT_TRUE(obj != nullptr);
    InitTaggedArray(obj, arrayLen);
    return JSHandle<TaggedArray>(thread, obj);
}

HWTEST_F_L0(SharedPartialGCTest, BarrierTest)
{
    constexpr size_t ARRAY_LEN = 10;
    instance->GetJSOptions().SetEnableForceGC(false);
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();
    ObjectFactory *factory = heap->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> localArray = factory->NewTaggedArray(ARRAY_LEN, JSTaggedValue::Undefined(), false);
    JSHandle<TaggedArray> sNonmovableArray =
        factory->NewSTaggedArray(ARRAY_LEN, JSTaggedValue::Undefined(), MemSpaceType::SHARED_NON_MOVABLE);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
    heap->GetHeapPrepare(thread);

    std::shared_ptr<SharedTestSpace> space= std::make_shared<SharedTestSpace>(sHeap, thread);
    // sOld1 and sOld2 are in same region and sOld3 in another region.
    JSHandle<TaggedArray> sOld1 = AllocateSharedArray(space, ARRAY_LEN);
    JSHandle<TaggedArray> sOld2 = AllocateSharedArray(space, ARRAY_LEN);
    JSHandle<TaggedArray> sOld3 = AllocateSharedArrayNewRegion(space, ARRAY_LEN);
    InitializeBaseObjects(space);
    space->Merge();
    // trigger concurrent mark
    sHeap->TriggerConcurrentMarking<TriggerGCType::SHARED_PARTIAL_GC, MarkReason::OTHER>(thread);
    while (!thread->HasSuspendRequest());
    thread->CheckSafepointIfSuspended();
    if (thread->IsSharedConcurrentMarkingOrFinished()) {
        // test allocate new region during concurrent mark.
        AllocateSharedArrayNewRegion(space, ARRAY_LEN);
        space->Merge();
        // Mark Barrier Test
        localArray->Set(thread, 0, sOld1);
        auto weakArray = sOld3.GetTaggedValue();
        weakArray.CreateWeakRef();
        sOld1->Set(thread, 0, sOld2);
        sOld2->Set(thread, 0, weakArray);
        sOld2->Set(thread, 1, sOld3);
        sOld3->Set(thread, 0, sNonmovableArray);
        Region *region1 = Region::ObjectAddressToRange(*sOld1);
        auto crossRSet1 = region1->GetCrossRegionRememberedSet();
        Region *region2 = Region::ObjectAddressToRange(*sOld2);
        auto crossRSet2 = region1->GetCrossRegionRememberedSet();
        EXPECT_EQ(region1, region2);
        JSTaggedType *slot1 = sOld1->GetData();
        JSTaggedType *slot2 = sOld1->GetData() + 1;
        JSTaggedType *slot3 = sOld2->GetData();
        JSTaggedType *slot4 = sOld2->GetData() + 1;
        EXPECT_FALSE(crossRSet1->TestBit((uintptr_t)(region1), reinterpret_cast<uintptr_t>(slot1)));
        EXPECT_FALSE(crossRSet1->TestBit((uintptr_t)(region1), reinterpret_cast<uintptr_t>(slot2)));
        EXPECT_TRUE(crossRSet2->TestBit((uintptr_t)(region2), reinterpret_cast<uintptr_t>(slot3)));
        EXPECT_TRUE(crossRSet2->TestBit((uintptr_t)(region2), reinterpret_cast<uintptr_t>(slot4)));
        sHeap->WaitGCFinished(thread);
    }
}

HWTEST_F_L0(SharedPartialGCTest, ReadBarrierTest)
{
    constexpr size_t ARRAY_LEN = 10;
    instance->GetJSOptions().SetEnableForceGC(false);
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
    heap->GetHeapPrepare(thread);

    std::shared_ptr<SharedTestSpace> fromSpace= std::make_shared<SharedTestSpace>(sHeap, thread);
    std::shared_ptr<SharedTestSpace> toSpace= std::make_shared<SharedTestSpace>(sHeap, thread);
    JSHandle<TaggedArray> fromObj = AllocateSharedArray(fromSpace, ARRAY_LEN);
    Region *fromRegion = Region::ObjectAddressToRange(*fromObj);
    fromRegion->SetGCFlag(RegionGCFlags::IN_SHARED_COLLECT_SET);
    fromRegion->NonAtomicMark(*fromObj);
    uintptr_t toObj = toSpace->Evacuate(*fromObj);
    Region *toRegion = Region::ObjectAddressToRange(toObj);
    toRegion->SetRegionTypeFlag(RegionTypeFlag::TO);
    JSTaggedType toObjFromBarrier= Barriers::ReadBarrierForStringTableSlot(fromObj.GetTaggedType());
    JSTaggedType toObjFromBarrier2= Barriers::ReadBarrierForStringTableSlot(toObj);
    EXPECT_EQ(toObjFromBarrier, toObj);
    EXPECT_EQ(toObjFromBarrier2, toObj);
}

HWTEST_F_L0(SharedPartialGCTest, SensitiveTest)
{
    instance->GetJSOptions().SetEnableForceGC(false);
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
    heap->GetHeapPrepare(thread);
    std::shared_ptr<SharedTestSpace> space= std::make_shared<SharedTestSpace>(sHeap, thread);
    sHeap->SetSensitiveStatus(AppSensitiveStatus::ENTER_HIGH_SENSITIVE);
    sHeap->SetForceGC(true);
    InitializeBaseObjects(space);
    space->Merge();
    // trigger concurrent mark
    sHeap->TriggerConcurrentMarking<TriggerGCType::SHARED_PARTIAL_GC, MarkReason::OTHER>(thread);
    while (!thread->HasSuspendRequest());
    thread->CheckSafepointIfSuspended();
    sHeap->WaitGCFinished(thread);
    sHeap->SetSensitiveStatus(AppSensitiveStatus::EXIT_HIGH_SENSITIVE);
    EXPECT_FALSE(sHeap->InSensitiveStatus());
}

HWTEST_F_L0(SharedPartialGCTest, SharedOldSpace1)
{
    constexpr double ALIVE_RATE = 0.1;
    SharedHeap *sHeap = SharedHeap::GetInstance();
    SharedOldSpace *sOldSpace = sHeap->GetOldSpace();
    size_t maxOldSpaceCapacity = sHeap->GetOldSpace()->GetMaximumCapacity();
    std::shared_ptr<SharedTestSpace> space= std::make_shared<SharedTestSpace>(sHeap, thread);
    SharedLocalSpace *sLocalSpace = new SharedLocalSpace(sHeap, maxOldSpaceCapacity, maxOldSpaceCapacity);
    ASSERT_TRUE(sLocalSpace != nullptr);
    for (size_t i = 0; i < SharedOldSpace::MIN_COLLECT_REGION_SIZE; i++) {
        auto obj = CreateSharedObjectsInOneRegion(space, ALIVE_RATE);
        Region *region = Region::ObjectAddressToRange(*obj);
        EXPECT_TRUE(sLocalSpace->AddRegionToList(region));
    }
    sLocalSpace->Stop();
    sOldSpace->IncreaseCommitted(500 * 1024 * 1024);
    EXPECT_TRUE(sOldSpace->GetCommittedSize() > sOldSpace->GetOverShootMaximumCapacity())
        << "sOldSpace->GetCommittedSize() is " << sOldSpace->GetCommittedSize()
        << "sOldSpace->GetOverShootMaximumCapacity()" << sOldSpace->GetOverShootMaximumCapacity();
    sOldSpace->Merge(sLocalSpace);
    delete sLocalSpace;
    sLocalSpace = nullptr;
}

HWTEST_F_L0(SharedPartialGCTest, SharedOldSpace2)
{
    SharedHeap *sHeap = SharedHeap::GetInstance();
    sHeap->SetCanThrowOOMError(true);
    SharedOldSpace *sOldSpace = sHeap->GetOldSpace();
    size_t maxOldSpaceCapacity = sHeap->GetOldSpace()->GetMaximumCapacity();
    SharedLocalSpace *sLocalSpace = new SharedLocalSpace(sHeap, maxOldSpaceCapacity, maxOldSpaceCapacity);
    ASSERT_TRUE(sLocalSpace != nullptr);
    sLocalSpace->Stop();
    sOldSpace->IncreaseCommitted(500 * 1024 * 1024);
    EXPECT_TRUE(sOldSpace->GetCommittedSize() > sOldSpace->GetOverShootMaximumCapacity());
    sOldSpace->Merge(sLocalSpace);
    delete sLocalSpace;
    sLocalSpace = nullptr;
}

HWTEST_F_L0(SharedPartialGCTest, Allocate)
{
    SharedHeap *sHeap = SharedHeap::GetInstance();
    SharedSparseSpace *space = sHeap->GetSpaceWithType(MemSpaceType::SHARED_OLD_SPACE);
    space->PrepareSweeping();
    EXPECT_NE(space->Allocate(thread, 1000, true), 0);
}

HWTEST_F_L0(SharedPartialGCTest, InvokeAllocationInspector)
{
    SharedHeap *sHeap = SharedHeap::GetInstance();
    SharedSparseSpace *space = sHeap->GetSpaceWithType(MemSpaceType::SHARED_OLD_SPACE);
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto profiler = new HeapSampling(thread->GetEcmaVM(), heap, 10, 3);
    auto inspector = new AllocationInspector(heap, 10, profiler);
    space->AddAllocationInspector(inspector);
    space->InvokeAllocationInspector(10000, 100, 100);
}

HWTEST_F_L0(SharedPartialGCTest, CheckAndTriggerLocalFullMark)
{
    SharedHeap *sHeap = SharedHeap::GetInstance();
    SharedSparseSpace *space = sHeap->GetSpaceWithType(MemSpaceType::SHARED_OLD_SPACE);
    space->IncreaseLiveObjectSize(1000);
    space->CheckAndTriggerLocalFullMark();
}

HWTEST_F_L0(SharedPartialGCTest, Expand1)
{
    SharedHeap *sHeap = SharedHeap::GetInstance();
    SharedReadOnlySpace *sReadOnlySpace = sHeap->GetReadOnlySpace();
    sReadOnlySpace->SetInitialCapacity(1 * 1024 * 1024);
    EXPECT_TRUE(sReadOnlySpace->Expand(thread));
}

HWTEST_F_L0(SharedPartialGCTest, Expand2)
{
    SharedHeap *sHeap = SharedHeap::GetInstance();
    SharedReadOnlySpace *sReadOnlySpace = sHeap->GetReadOnlySpace();
    sReadOnlySpace->IncreaseCommitted(10000);
    EXPECT_FALSE(sReadOnlySpace->Expand(thread));
}
} // namespace panda::test
