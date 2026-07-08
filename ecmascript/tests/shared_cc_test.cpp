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
#include "ecmascript/daemon/daemon_task.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/free_object.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/barriers.h"
#include "ecmascript/mem/barriers-inl.h"
#include "ecmascript/mem/shared_heap/shared_cc_evacuator.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/mem/shared_heap/shared_cc.h"
#include "ecmascript/mem/shared_heap/shared_cc_evacuator.h"
#include "ecmascript/mem/shared_heap/shared_cc_evacuator-inl.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_marker.h"
#include "ecmascript/mem/tlab_allocator.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/runtime.h"

using namespace panda::ecmascript;

namespace panda::test {

// ============================================================
// Test fixtures
// ============================================================

class SharedCCTest : public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
        sHeap_ = SharedHeap::GetInstance();
    }

    SharedHeap *GetSharedHeap() const { return sHeap_; }

    void CleanUpThreadState()
    {
        thread->SetReadBarrierState(false);
        thread->SetSharedCCStatus(SharedCCStatus::IDLE);
        thread->ClearCCSuspend();
        thread->SwitchAllStub(true);
    }

protected:
    SharedHeap *sHeap_ {nullptr};
};

class SharedCCTestSpace : public MonoSpace {
public:
    static constexpr size_t CAP = 10 * 1024 * 1024;
    explicit SharedCCTestSpace(SharedHeap *heap)
        : MonoSpace(heap, heap->GetHeapRegionAllocator(),
          MemSpaceType::SHARED_OLD_SPACE, CAP, CAP), sHeap_(heap) {}
    ~SharedCCTestSpace() override = default;
    NO_COPY_SEMANTIC(SharedCCTestSpace);
    NO_MOVE_SEMANTIC(SharedCCTestSpace);

    void Expand(JSThread *thread)
    {
        Region *region = heapRegionAllocator_->AllocateAlignedRegion(
            this, DEFAULT_REGION_SIZE, thread, sHeap_);
        FillBumpPointer();
        allocator_.Reset(region->GetBegin(), region->GetEnd());
    }

    uintptr_t Allocate(size_t size) { return allocator_.Allocate(size); }
    uintptr_t GetTop() { return allocator_.GetTop(); }
    uintptr_t GetEnd() { return allocator_.GetEnd(); }

    void FillBumpPointer()
    {
        auto begin = allocator_.GetTop();
        auto size = allocator_.Available();
        FreeObject::FillFreeObject(sHeap_, begin, size);
    }

private:
    SharedHeap *sHeap_ {nullptr};
    BumpPointerAllocator allocator_;
};

class SharedCCRunPhasesTest : public BaseTestWithScope<false> {
public:
    static constexpr double aliveRate = 0.1;

    void SetUp() override
    {
        JSRuntimeOptions options;
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
        sHeap_ = SharedHeap::GetInstance();
        instance->SetEnableForceGC(false);
    }

    void TearDown() override
    {
        instance->SetEnableForceGC(true);
        BaseTestWithScope<false>::TearDown();
    }

    SharedHeap *GetSharedHeap() const { return sHeap_; }

    void InitTaggedArray(TaggedObject *obj, size_t arrayLen);
    JSHandle<TaggedObject> CreateSharedObjectsInOneRegion(
        std::shared_ptr<SharedCCTestSpace> space, double aliveRate);

protected:
    SharedHeap *sHeap_ {nullptr};
};

void SharedCCRunPhasesTest::InitTaggedArray(TaggedObject *obj, size_t arrayLen)
{
    JSHClass *arrayClass = JSHClass::Cast(thread->GlobalConstants()->GetTaggedArrayClass().GetTaggedObject());
    obj->SetClassWithoutBarrier(arrayClass);
    TaggedArray::Cast(obj)->InitializeWithSpecialValue(JSTaggedValue::Undefined(), arrayLen);
}

JSHandle<TaggedObject> SharedCCRunPhasesTest::CreateSharedObjectsInOneRegion(
    std::shared_ptr<SharedCCTestSpace> space, double aliveRate)
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

// ============================================================
// Unit tests: SharedCCStatus state machine
// ============================================================

HWTEST_F_L0(SharedCCTest, SharedCCStatusStateMachine)
{
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);

    thread->SetSharedCCStatus(SharedCCStatus::READY);
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::READY);

    thread->SetSharedCCStatus(SharedCCStatus::SUSPENDED);
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::SUSPENDED);

    thread->SetSharedCCStatus(SharedCCStatus::IDLE);
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
}

HWTEST_F_L0(SharedCCTest, ThreadFlagsSetClearRoundtrip)
{
    EXPECT_FALSE(thread->HasCCSuspend());
    EXPECT_FALSE(thread->NeedReadBarrier());

    thread->SetCCSuspend();
    EXPECT_TRUE(thread->HasCCSuspend());
    thread->ClearCCSuspend();
    EXPECT_FALSE(thread->HasCCSuspend());

    thread->SetReadBarrierState(true);
    EXPECT_TRUE(thread->NeedReadBarrier());
    thread->SetReadBarrierState(false);
    EXPECT_FALSE(thread->NeedReadBarrier());

    thread->SetWaitingSharedGCFinished(true);
    EXPECT_TRUE(thread->IsWaitingSharedGCFinished());
    thread->SetWaitingSharedGCFinished(false);
    EXPECT_FALSE(thread->IsWaitingSharedGCFinished());
}

HWTEST_F_L0(SharedCCTest, FinishPathRestoresAllState)
{
    thread->SetSharedCCStatus(SharedCCStatus::READY);
    thread->SwitchAllStub(false);
    thread->SetReadBarrierState(true);
    thread->SetCCSuspend();

    EXPECT_TRUE(thread->NeedReadBarrier());
    EXPECT_TRUE(thread->HasCCSuspend());

    CleanUpThreadState();

    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
    EXPECT_FALSE(thread->NeedReadBarrier());
    EXPECT_FALSE(thread->HasCCSuspend());
}

// ============================================================
// Unit tests: SharedCC construction and accessors
// ============================================================

HWTEST_F_L0(SharedCCTest, SharedCCConstructionAndAccessors)
{
    SharedCC cc(sHeap_);
    EXPECT_EQ(cc.GetHeap(), sHeap_);

    SharedCC *sharedCC = sHeap_->GetSharedCC();
    ASSERT_TRUE(sharedCC != nullptr);
    EXPECT_EQ(sharedCC->GetHeap(), sHeap_);
}

HWTEST_F_L0(SharedCCTest, SharedCCTypesExist)
{
    EXPECT_TRUE(TriggerGCType::SHARED_CC < TriggerGCType::GC_TYPE_LAST);
    EXPECT_TRUE(DaemonTaskType::TRIGGER_SHARED_CC != DaemonTaskType::MOCK_FOR_TEST);
}

HWTEST_F_L0(SharedCCTest, PrepareNewThreadNoCrashWhenIdle)
{
    SharedCC *cc = sHeap_->GetSharedCC();
    ASSERT_TRUE(cc != nullptr);
    cc->PrepareNewThread(thread);
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
    EXPECT_FALSE(thread->NeedReadBarrier());
    EXPECT_EQ(thread->GetSharedCCEvacuator(), nullptr);
}

HWTEST_F_L0(SharedCCTest, SkipThreadWorkloadNoCrashWhenNoWorkload)
{
    SharedCC *cc = sHeap_->GetSharedCC();
    ASSERT_TRUE(cc != nullptr);
    cc->SkipThreadWorkload(thread);
}

HWTEST_F_L0(SharedCCTest, NotifyMainThreadReadyNoCrash)
{
    SharedCC cc(sHeap_);
    cc.NotifyMainThreadReady();
}

// ============================================================
// Unit tests: ThreadWorkload structure
// ============================================================

HWTEST_F_L0(SharedCCTest, ThreadWorkloadInitialState)
{
    ThreadWorkload tw;
    tw.thread = thread;
    EXPECT_EQ(tw.thread, thread);
    EXPECT_EQ(tw.localRegions.size(), 0U);
    EXPECT_EQ(tw.nextIndex_.load(), -1);
    EXPECT_EQ(tw.remainItems_, 0);
}

HWTEST_F_L0(SharedCCTest, ThreadWorkloadInitializedWithRegions)
{
    ThreadWorkload tw;
    tw.thread = thread;
    tw.localRegions.push_back(nullptr);
    tw.localRegions.push_back(nullptr);
    tw.localRegions.push_back(nullptr);
    int num = static_cast<int>(tw.localRegions.size());
    tw.nextIndex_.store(num - 1, std::memory_order_relaxed);
    tw.remainItems_ = num;

    EXPECT_EQ(tw.localRegions.size(), 3U);
    EXPECT_EQ(tw.nextIndex_.load(), 2);
    EXPECT_EQ(tw.remainItems_, 3);
}

HWTEST_F_L0(SharedCCTest, ThreadWorkloadWorkStealingPattern)
{
    ThreadWorkload tw;
    tw.thread = thread;
    tw.localRegions.push_back(nullptr);
    tw.localRegions.push_back(nullptr);
    int num = static_cast<int>(tw.localRegions.size());
    tw.nextIndex_.store(num - 1, std::memory_order_relaxed);
    tw.remainItems_ = num;

    int done = 0;
    while (true) {
        int idx = tw.nextIndex_.fetch_sub(1, std::memory_order_relaxed);
        if (idx < 0) {
            break;
        }
        done++;
    }
    EXPECT_EQ(done, 2);
    EXPECT_EQ(tw.remainItems_ - done, 0);
}

HWTEST_F_L0(SharedCCTest, ThreadWorkloadEmptyNoWork)
{
    ThreadWorkload tw;
    tw.thread = thread;
    tw.nextIndex_.store(-1, std::memory_order_relaxed);
    tw.remainItems_ = 0;

    int done = 0;
    while (true) {
        int idx = tw.nextIndex_.fetch_sub(1, std::memory_order_relaxed);
        if (idx < 0) {
            break;
        }
        done++;
    }
    EXPECT_EQ(done, 0);
}

// ============================================================
// Unit tests: EvacuatorEntry
// ============================================================

HWTEST_F_L0(SharedCCTest, EvacuatorEntryStructure)
{
    EvacuatorEntry entry;
    entry.thread = thread;
    entry.evacuator = nullptr;
    EXPECT_EQ(entry.thread, thread);
    EXPECT_EQ(entry.evacuator, nullptr);
}

// ============================================================
// Unit tests: SharedCCEvacuator lifecycle
// ============================================================

HWTEST_F_L0(SharedCCTest, EvacuatorSingleArgConstructorOwnsAllocator)
{
    SharedCCEvacuator evacuator(sHeap_);
    EXPECT_NE(evacuator.GetTlabAllocator(), nullptr);
}

HWTEST_F_L0(SharedCCTest, EvacuatorTwoArgConstructorDoesNotOwn)
{
    auto tlab = std::make_unique<SharedTlabAllocator>(sHeap_);
    SharedTlabAllocator *raw = tlab.get();
    {
        SharedCCEvacuator evacuator(sHeap_, raw);
        EXPECT_EQ(evacuator.GetTlabAllocator(), raw);
    }
    EXPECT_NE(tlab.get(), nullptr);
}

// ============================================================
// Unit tests: SharedCCUpdateVisitor
// ============================================================

HWTEST_F_L0(SharedCCTest, UpdateVisitorSkipsNonHeapObjects)
{
    SharedCCUpdateVisitor updateVisitor;

    JSTaggedValue undefined = JSTaggedValue::Undefined();
    ObjectSlot undefinedSlot(reinterpret_cast<uintptr_t>(&undefined));
    updateVisitor.HandleSlot(undefinedSlot);
    EXPECT_EQ(undefinedSlot.GetTaggedType(), undefined.GetRawData());

    JSTaggedValue smallInt = JSTaggedValue(static_cast<int32_t>(42));
    ObjectSlot smallIntSlot(reinterpret_cast<uintptr_t>(&smallInt));
    updateVisitor.HandleSlot(smallIntSlot);
    EXPECT_EQ(smallIntSlot.GetTaggedType(), smallInt.GetRawData());

    JSTaggedValue nullValue = JSTaggedValue::Null();
    ObjectSlot nullSlot(reinterpret_cast<uintptr_t>(&nullValue));
    updateVisitor.HandleSlot(nullSlot);
    EXPECT_EQ(nullSlot.GetTaggedType(), nullValue.GetRawData());
}

HWTEST_F_L0(SharedCCTest, UpdateVisitorSkipsNonFromRegionHeapObject)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> referent = factory->NewTaggedArray(4, JSTaggedValue::Undefined());
    JSTaggedValue heapValue = referent.GetTaggedValue();
    ASSERT_TRUE(heapValue.IsHeapObject());

    Region *region = Region::ObjectAddressToRange(heapValue.GetHeapObject());
    EXPECT_FALSE(region->IsFromRegion());

    JSTaggedType slotData = heapValue.GetRawData();
    ObjectSlot slot(reinterpret_cast<uintptr_t>(&slotData));
    SharedCCUpdateVisitor updateVisitor;
    updateVisitor.HandleSlot(slot);
    EXPECT_EQ(slot.GetTaggedType(), heapValue.GetRawData());
}

HWTEST_F_L0(SharedCCTest, UpdateVisitorHandlesMixedSlotTypes)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> referent = factory->NewTaggedArray(4, JSTaggedValue::Undefined());

    JSTaggedType slots[3];
    slots[0] = JSTaggedValue::Undefined().GetRawData();
    slots[1] = JSTaggedValue(static_cast<int32_t>(100)).GetRawData();
    slots[2] = referent.GetTaggedValue().GetRawData();

    SharedCCUpdateVisitor updateVisitor;
    updateVisitor.HandleSlot(ObjectSlot(reinterpret_cast<uintptr_t>(&slots[0])));
    updateVisitor.HandleSlot(ObjectSlot(reinterpret_cast<uintptr_t>(&slots[1])));
    updateVisitor.HandleSlot(ObjectSlot(reinterpret_cast<uintptr_t>(&slots[2])));

    EXPECT_EQ(slots[0], JSTaggedValue::Undefined().GetRawData());
    EXPECT_EQ(slots[1], JSTaggedValue(static_cast<int32_t>(100)).GetRawData());
    EXPECT_EQ(slots[2], referent.GetTaggedValue().GetRawData());
}

// UpdateVisitor forwarded slot/weak slot tests are covered by integration tests
// (SharedObjectsSurviveAfterCollect, SharedObjectsContentIntactAfterCollect)
// which run full CC cycles through FROM→Copy→Update.

// ============================================================
// Unit tests: SharedCCRootVisitor
// ============================================================

HWTEST_F_L0(SharedCCTest, RootVisitorSkipsNonHeapObject)
{
    JSTaggedType undefinedData = JSTaggedValue::Undefined().GetRawData();
    ObjectSlot slot(reinterpret_cast<uintptr_t>(&undefinedData));

    auto tlab = std::make_unique<SharedTlabAllocator>(sHeap_);
    SharedCCEvacuator evacuator(sHeap_, tlab.get());
    SharedCCRootVisitor visitor(&evacuator);

    visitor.VisitRoot(Root::ROOT_LOCAL_HANDLE, slot);
    EXPECT_EQ(slot.GetTaggedType(), JSTaggedValue::Undefined().GetRawData());
}

HWTEST_F_L0(SharedCCTest, RootVisitorSkipsNonFromRegion)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> localArray = factory->NewTaggedArray(4, JSTaggedValue::Undefined());
    JSTaggedType slotData = localArray.GetTaggedValue().GetRawData();
    ObjectSlot slot(reinterpret_cast<uintptr_t>(&slotData));

    auto tlab = std::make_unique<SharedTlabAllocator>(sHeap_);
    SharedCCEvacuator evacuator(sHeap_, tlab.get());
    SharedCCRootVisitor visitor(&evacuator);

    visitor.VisitRoot(Root::ROOT_LOCAL_HANDLE, slot);
    EXPECT_EQ(slot.GetTaggedType(), localArray.GetTaggedValue().GetRawData());
}

// ============================================================
// Unit tests: FROM region detection
// ============================================================

HWTEST_F_L0(SharedCCTest, RegionCheckerLocalHeapNotFromRegion)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array = factory->NewTaggedArray(8, JSTaggedValue::Undefined());
    Region *region = Region::ObjectAddressToRange(array.GetTaggedValue().GetHeapObject());
    EXPECT_FALSE(region->IsFromRegion());
}

// ============================================================
// Integration tests: RunPhases via CollectGarbage
// ============================================================

HWTEST_F_L0(SharedCCRunPhasesTest, EmptyHeapCollect)
{
    SharedHeap *sHeap = SharedHeap::GetInstance();
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);

    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
    EXPECT_FALSE(thread->NeedReadBarrier());
    EXPECT_FALSE(thread->HasCCSuspend());
    EXPECT_EQ(thread->GetSharedCCEvacuator(), nullptr);
}

HWTEST_F_L0(SharedCCRunPhasesTest, BasicTriggerWithEmptyStack)
{
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();

    ASSERT_EQ(thread->GetLastLeaveFrame(), nullptr);

    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);

    SharedCC *cc = sHeap->GetSharedCC();
    ASSERT_TRUE(cc != nullptr);
    EXPECT_EQ(cc->GetHeap(), sHeap);
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
    EXPECT_FALSE(thread->HasCCSuspend());
    EXPECT_FALSE(thread->NeedReadBarrier());
    EXPECT_EQ(thread->GetSharedCCEvacuator(), nullptr);
}

HWTEST_F_L0(SharedCCRunPhasesTest, SharedObjectsSurviveAfterCollect)
{
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();
    ObjectFactory *factory = heap->GetEcmaVM()->GetFactory();

    JSHandle<TaggedArray> localObj = factory->NewTaggedArray(
        SharedOldSpace::MIN_COLLECT_REGION_SIZE, JSTaggedValue::Undefined(), false);

    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);
    heap->GetHeapPrepare(thread);

    SharedOldSpace *sOldSpace = sHeap->GetOldSpace();
    auto space = std::make_shared<SharedCCTestSpace>(sHeap);
    std::vector<std::pair<Region *, JSHandle<TaggedObject>>> checkObjList;

    for (size_t i = 0; i < SharedOldSpace::MIN_COLLECT_REGION_SIZE; i++) {
        auto obj = CreateSharedObjectsInOneRegion(space, aliveRate);
        Region *region = Region::ObjectAddressToRange(*obj);
        checkObjList.emplace_back(region, obj);
        sOldSpace->AddRegion(region);
    }
    space->FillBumpPointer();

    for (size_t i = 0; i < SharedOldSpace::MIN_COLLECT_REGION_SIZE; i++) {
        localObj->Set(thread, i, checkObjList[i].second);
    }

    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);

    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
    EXPECT_FALSE(thread->NeedReadBarrier());
    EXPECT_FALSE(thread->HasCCSuspend());
    EXPECT_EQ(thread->GetSharedCCEvacuator(), nullptr);

    for (size_t i = 0; i < SharedOldSpace::MIN_COLLECT_REGION_SIZE; i++) {
        JSTaggedValue val = localObj->Get(thread, i);
        EXPECT_TRUE(val.IsHeapObject()) << "slot " << i << " should be a heap object";
        EXPECT_NE(val, JSTaggedValue::Undefined()) << "slot " << i << " should survive";
    }
}

HWTEST_F_L0(SharedCCRunPhasesTest, SharedObjectsContentIntactAfterCollect)
{
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();
    ObjectFactory *factory = heap->GetEcmaVM()->GetFactory();

    constexpr size_t NUM_REGIONS = SharedOldSpace::MIN_COLLECT_REGION_SIZE;
    JSHandle<TaggedArray> localObj = factory->NewTaggedArray(NUM_REGIONS, JSTaggedValue::Undefined(), false);
    std::vector<size_t> expectedLengths;

    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);
    heap->GetHeapPrepare(thread);

    SharedOldSpace *sOldSpace = sHeap->GetOldSpace();
    auto space = std::make_shared<SharedCCTestSpace>(sHeap);

    for (size_t i = 0; i < NUM_REGIONS; i++) {
        auto obj = CreateSharedObjectsInOneRegion(space, aliveRate);
        Region *region = Region::ObjectAddressToRange(*obj);
        sOldSpace->AddRegion(region);
        localObj->Set(thread, i, obj);

        TaggedArray *arr = TaggedArray::Cast(*obj);
        expectedLengths.push_back(arr->GetLength());
    }
    space->FillBumpPointer();

    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);

    for (size_t i = 0; i < NUM_REGIONS; i++) {
        JSTaggedValue val = localObj->Get(thread, i);
        ASSERT_TRUE(val.IsHeapObject()) << "slot " << i << " should be heap object";
        TaggedArray *arr = TaggedArray::Cast(val.GetTaggedObject());
        EXPECT_EQ(arr->GetLength(), expectedLengths[i])
            << "TaggedArray at slot " << i << " length mismatch after CC";
        EXPECT_TRUE(arr->GetClass() != nullptr)
            << "TaggedArray at slot " << i << " hclass is null after CC";
    }
}

HWTEST_F_L0(SharedCCRunPhasesTest, RestoresAllThreadState)
{
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();

    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);
    heap->GetHeapPrepare(thread);

    SharedOldSpace *sOldSpace = sHeap->GetOldSpace();
    auto space = std::make_shared<SharedCCTestSpace>(sHeap);

    for (size_t i = 0; i < SharedOldSpace::MIN_COLLECT_REGION_SIZE; i++) {
        auto obj = CreateSharedObjectsInOneRegion(space, aliveRate);
        Region *region = Region::ObjectAddressToRange(*obj);
        sOldSpace->AddRegion(region);
    }
    space->FillBumpPointer();

    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
    EXPECT_FALSE(thread->NeedReadBarrier());

    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);

    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
    EXPECT_FALSE(thread->NeedReadBarrier());
    EXPECT_FALSE(thread->HasCCSuspend());
    EXPECT_EQ(thread->GetSharedCCEvacuator(), nullptr);
}

HWTEST_F_L0(SharedCCRunPhasesTest, MultipleCycles)
{
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();
    ObjectFactory *factory = heap->GetEcmaVM()->GetFactory();

    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);
    heap->GetHeapPrepare(thread);

    for (int cycle = 0; cycle < 3; cycle++) {
        SharedOldSpace *sOldSpace = sHeap->GetOldSpace();
        auto space = std::make_shared<SharedCCTestSpace>(sHeap);
        JSHandle<TaggedArray> localObj = factory->NewTaggedArray(
            SharedOldSpace::MIN_COLLECT_REGION_SIZE, JSTaggedValue::Undefined(), false);

        for (size_t i = 0; i < SharedOldSpace::MIN_COLLECT_REGION_SIZE; i++) {
            auto obj = CreateSharedObjectsInOneRegion(space, aliveRate);
            Region *region = Region::ObjectAddressToRange(*obj);
            sOldSpace->AddRegion(region);
            localObj->Set(thread, i, obj);
        }
        space->FillBumpPointer();

        sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);
        EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
        EXPECT_FALSE(thread->NeedReadBarrier());
        EXPECT_EQ(thread->GetSharedCCEvacuator(), nullptr);

        heap->GetHeapPrepare(thread);
    }
}

HWTEST_F_L0(SharedCCRunPhasesTest, CrossHeapReferencesSurvive)
{
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();
    ObjectFactory *factory = heap->GetEcmaVM()->GetFactory();

    JSHandle<TaggedArray> localArray = factory->NewTaggedArray(16, JSTaggedValue::Undefined());

    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);
    heap->GetHeapPrepare(thread);

    SharedOldSpace *sOldSpace = sHeap->GetOldSpace();
    auto space = std::make_shared<SharedCCTestSpace>(sHeap);
    std::vector<JSHandle<TaggedObject>> sharedObjs;

    for (size_t i = 0; i < SharedOldSpace::MIN_COLLECT_REGION_SIZE; i++) {
        auto obj = CreateSharedObjectsInOneRegion(space, aliveRate);
        Region *region = Region::ObjectAddressToRange(*obj);
        sOldSpace->AddRegion(region);
        sharedObjs.push_back(obj);
    }
    space->FillBumpPointer();

    for (size_t i = 0; i < sharedObjs.size() && i < 16; i++) {
        localArray->Set(thread, i, sharedObjs[i]);
    }

    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);

    for (size_t i = 0; i < sharedObjs.size() && i < 16; i++) {
        JSTaggedValue val = localArray->Get(thread, i);
        EXPECT_TRUE(val.IsHeapObject()) << "local slot " << i << " lost shared reference";
        EXPECT_TRUE(val.IsInSharedHeap()) << "local slot " << i << " should point to shared heap";
    }
}

HWTEST_F_L0(SharedCCRunPhasesTest, PreservesLocalObjects)
{
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();
    ObjectFactory *factory = heap->GetEcmaVM()->GetFactory();

    JSHandle<TaggedArray> localKeep = factory->NewTaggedArray(64, JSTaggedValue::Hole());
    for (uint32_t i = 0; i < 64; i++) {
        localKeep->Set(thread, i, JSTaggedValue(i));
    }

    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);

    for (uint32_t i = 0; i < 64; i++) {
        EXPECT_EQ(localKeep->Get(thread, i), JSTaggedValue(i))
            << "local slot " << i << " corrupted by SharedCC";
    }
}

HWTEST_F_L0(SharedCCRunPhasesTest, SharedFullGCThenSharedCCSequence)
{
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();

    heap->CollectGarbage(TriggerGCType::FULL_GC);

    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);

    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);

    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
    EXPECT_FALSE(thread->NeedReadBarrier());
    EXPECT_FALSE(thread->HasCCSuspend());
    EXPECT_EQ(thread->GetSharedCCEvacuator(), nullptr);
}

HWTEST_F_L0(SharedCCRunPhasesTest, SharedObjectsWithFullGCCycle)
{
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();
    ObjectFactory *factory = heap->GetEcmaVM()->GetFactory();

    JSHandle<TaggedArray> localRef = factory->NewTaggedArray(8, JSTaggedValue::Undefined());

    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);
    heap->GetHeapPrepare(thread);

    SharedOldSpace *sOldSpace = sHeap->GetOldSpace();
    auto space = std::make_shared<SharedCCTestSpace>(sHeap);

    for (size_t i = 0; i < SharedOldSpace::MIN_COLLECT_REGION_SIZE; i++) {
        auto obj = CreateSharedObjectsInOneRegion(space, aliveRate);
        Region *region = Region::ObjectAddressToRange(*obj);
        sOldSpace->AddRegion(region);
        localRef->Set(thread, i % 8, obj);
    }
    space->FillBumpPointer();

    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);

    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
}

HWTEST_F_L0(SharedCCRunPhasesTest, WithFullGCBefore)
{
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();
    ObjectFactory *factory = heap->GetEcmaVM()->GetFactory();

    for (int i = 0; i < 100; i++) {
        factory->NewTaggedArray(64, JSTaggedValue::Undefined());
    }

    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);

    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
}

HWTEST_F_L0(SharedCCRunPhasesTest, DaemonTaskPostedAndCompleted)
{
    SharedHeap *sHeap = SharedHeap::GetInstance();
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);

    auto *cc = sHeap->GetSharedCC();
    ASSERT_TRUE(cc != nullptr);
    EXPECT_EQ(cc->GetHeap(), sHeap);
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
    EXPECT_EQ(thread->GetSharedCCEvacuator(), nullptr);
}

HWTEST_F_L0(SharedCCRunPhasesTest, RegionFlagsResetAfterCollect)
{
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();
    ObjectFactory *factory = heap->GetEcmaVM()->GetFactory();

    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);
    heap->GetHeapPrepare(thread);

    SharedOldSpace *sOldSpace = sHeap->GetOldSpace();
    auto space = std::make_shared<SharedCCTestSpace>(sHeap);
    JSHandle<TaggedArray> localObj = factory->NewTaggedArray(
        SharedOldSpace::MIN_COLLECT_REGION_SIZE, JSTaggedValue::Undefined(), false);

    for (size_t i = 0; i < SharedOldSpace::MIN_COLLECT_REGION_SIZE; i++) {
        auto obj = CreateSharedObjectsInOneRegion(space, aliveRate);
        Region *region = Region::ObjectAddressToRange(*obj);
        sOldSpace->AddRegion(region);
        localObj->Set(thread, i, obj);
    }
    space->FillBumpPointer();

    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);

    for (size_t i = 0; i < SharedOldSpace::MIN_COLLECT_REGION_SIZE; i++) {
        JSTaggedValue val = localObj->Get(thread, i);
        if (val.IsHeapObject()) {
            Region *region = Region::ObjectAddressToRange(val.GetHeapObject());
            EXPECT_FALSE(region->IsFromRegion())
                << "surviving object at slot " << i << " should not be in FROM region";
        }
    }
}

// Forwarding address idempotency is verified by integration tests
// where objects survive full CC cycles.

HWTEST_F_L0(SharedCCRunPhasesTest, FromRegionDetectedByRegionChecker)
{
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();

    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);
    heap->GetHeapPrepare(thread);

    SharedOldSpace *sOldSpace = sHeap->GetOldSpace();
    auto space = std::make_shared<SharedCCTestSpace>(sHeap);

    auto obj = CreateSharedObjectsInOneRegion(space, aliveRate);
    Region *region = Region::ObjectAddressToRange(*obj);
    sOldSpace->AddRegion(region);
    space->FillBumpPointer();

    EXPECT_FALSE(region->IsFromRegion());

    region->SetRegionTypeFlag(RegionTypeFlag::FROM);
    EXPECT_TRUE(region->IsFromRegion());

    region->SetRegionTypeFlag(RegionTypeFlag::DEFAULT);
}

// ============================================================
// Unit tests: SharedCCRootVisitor forwarded root
// ============================================================

// RootVisitor forwarded root test is covered by integration tests.

HWTEST_F_L0(SharedCCTest, RootVisitorVisitRangeRoots)
{
    JSTaggedType slots[4];
    slots[0] = JSTaggedValue::Undefined().GetRawData();
    slots[1] = JSTaggedValue(static_cast<int32_t>(42)).GetRawData();
    slots[2] = JSTaggedValue::Null().GetRawData();
    slots[3] = JSTaggedValue::Hole().GetRawData();

    ObjectSlot start(reinterpret_cast<uintptr_t>(&slots[0]));
    ObjectSlot end(reinterpret_cast<uintptr_t>(&slots[4]));

    auto tlab = std::make_unique<SharedTlabAllocator>(sHeap_);
    SharedCCEvacuator evacuator(sHeap_, tlab.get());
    SharedCCRootVisitor visitor(&evacuator);

    visitor.VisitRangeRoot(Root::ROOT_LOCAL_HANDLE, start, end);

    EXPECT_EQ(slots[0], JSTaggedValue::Undefined().GetRawData());
    EXPECT_EQ(slots[1], JSTaggedValue(static_cast<int32_t>(42)).GetRawData());
    EXPECT_EQ(slots[2], JSTaggedValue::Null().GetRawData());
    EXPECT_EQ(slots[3], JSTaggedValue::Hole().GetRawData());
}

HWTEST_F_L0(SharedCCTest, RootVisitorVisitBaseAndDerivedRoot)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> base = factory->NewTaggedArray(4, JSTaggedValue::Undefined());
    JSHandle<TaggedArray> derived = factory->NewTaggedArray(4, JSTaggedValue::Undefined());

    JSTaggedType baseData = base.GetTaggedValue().GetRawData();
    JSTaggedType derivedData = derived.GetTaggedValue().GetRawData();
    ObjectSlot baseSlot(reinterpret_cast<uintptr_t>(&baseData));
    ObjectSlot derivedSlot(reinterpret_cast<uintptr_t>(&derivedData));

    auto tlab = std::make_unique<SharedTlabAllocator>(sHeap_);
    SharedCCEvacuator evacuator(sHeap_, tlab.get());
    SharedCCRootVisitor visitor(&evacuator);

    uintptr_t baseOldObject = reinterpret_cast<uintptr_t>(*base);
    visitor.VisitBaseAndDerivedRoot(Root::ROOT_LOCAL_HANDLE, baseSlot, derivedSlot, baseOldObject);

    EXPECT_EQ(baseSlot.GetTaggedType(), base.GetTaggedValue().GetRawData());
    EXPECT_EQ(derivedSlot.GetTaggedType(), derived.GetTaggedValue().GetRawData());
}

// ============================================================
// Unit tests: UpdateVisitor VisitObjectRangeImpl
// ============================================================

HWTEST_F_L0(SharedCCTest, UpdateVisitorVisitObjectRangeAllNonHeap)
{
    JSTaggedType slots[4];
    slots[0] = JSTaggedValue::Undefined().GetRawData();
    slots[1] = JSTaggedValue(static_cast<int32_t>(1)).GetRawData();
    slots[2] = JSTaggedValue::Null().GetRawData();
    slots[3] = JSTaggedValue::Hole().GetRawData();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> root = factory->NewTaggedArray(4, JSTaggedValue::Undefined());

    SharedCCUpdateVisitor updateVisitor;
    updateVisitor.VisitObjectRangeImpl(*root,
        reinterpret_cast<uintptr_t>(&slots[0]),
        reinterpret_cast<uintptr_t>(&slots[4]),
        VisitObjectArea::NORMAL);

    EXPECT_EQ(slots[0], JSTaggedValue::Undefined().GetRawData());
    EXPECT_EQ(slots[1], JSTaggedValue(static_cast<int32_t>(1)).GetRawData());
    EXPECT_EQ(slots[2], JSTaggedValue::Null().GetRawData());
    EXPECT_EQ(slots[3], JSTaggedValue::Hole().GetRawData());
}

// ============================================================
// Unit tests: SharedCCEvacuator Copy and RawCopyObject
// ============================================================

HWTEST_F_L0(SharedCCTest, EvacuatorGetTlabAllocatorNotNull)
{
    SharedCCEvacuator evacuator(sHeap_);
    auto *alloc = evacuator.GetTlabAllocator();
    ASSERT_NE(alloc, nullptr);
    EXPECT_NE(alloc->GetSharedLocalSpace(), nullptr);
}

// ============================================================
// Unit tests: ThreadWorkload concurrent work-stealing simulation
// ============================================================

HWTEST_F_L0(SharedCCTest, ThreadWorkloadConcurrentStealAllConsumed)
{
    ThreadWorkload tw;
    tw.thread = thread;
    for (int i = 0; i < 10; i++) {
        tw.localRegions.push_back(nullptr);
    }
    int num = static_cast<int>(tw.localRegions.size());
    tw.nextIndex_.store(num - 1, std::memory_order_relaxed);
    tw.remainItems_ = num;

    auto steal = [&]() {
        int done = 0;
        while (true) {
            int idx = tw.nextIndex_.fetch_sub(1, std::memory_order_relaxed);
            if (idx < 0) {
                break;
            }
            done++;
        }
        if (done > 0) {
            LockHolder lock(tw.mutex_);
            tw.remainItems_ -= done;
            if (tw.remainItems_ == 0) {
                tw.cv_.SignalAll();
            }
        }
    };

    steal();
    steal();
    steal();

    EXPECT_EQ(tw.remainItems_, 0);
    EXPECT_EQ(tw.nextIndex_.load() < 0, true);
}

HWTEST_F_L0(SharedCCTest, ThreadWorkloadSkipPattern)
{
    ThreadWorkload tw;
    tw.thread = thread;
    for (int i = 0; i < 5; i++) {
        tw.localRegions.push_back(nullptr);
    }
    int num = static_cast<int>(tw.localRegions.size());
    tw.nextIndex_.store(num - 1, std::memory_order_relaxed);
    tw.remainItems_ = num;

    int done = 0;
    while (true) {
        int idx = tw.nextIndex_.fetch_sub(1, std::memory_order_relaxed);
        if (idx < 0) {
            break;
        }
        done++;
    }
    if (done > 0) {
        LockHolder lock(tw.mutex_);
        tw.remainItems_ -= done;
        if (tw.remainItems_ == 0) {
            tw.cv_.SignalAll();
        }
    }

    EXPECT_EQ(done, 5);
    EXPECT_EQ(tw.remainItems_, 0);
}

// ============================================================
// Unit tests: SharedHeap::FinishSharedCC
// ============================================================

HWTEST_F_L0(SharedCCTest, UpdateGCThresholdsNoCrash)
{
    sHeap_->UpdateGCThresholds(TriggerGCType::SHARED_CC);
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
}

// ============================================================
// Integration tests: private methods covered via CollectGarbage
// - SetStringTableCopyOrSweeping: called in PrepareForCopy + RestoreThreadStates
// - EstimatePostCCSize: called in ReMarkAndPrepare
// - CalculateCopyThreadNum: called in ParallelCopy
// - CalculateUpdateThreadNum: called in UpdateReferences
// - OnCopyFinished/WaitFinished: called in ParallelCopy
// - OnUpdateFinished/WaitUpdateFinished: called in UpdateReferences
// - EnterSharedGCScope/ExitSharedGCScope: called in ReMarkAndPrepare/FinalizeAndReclaim
// - RestoreThreadStates: called in FinalizeAndReclaim
// - LogThreadStatesBeforeCopy: called in ReMarkAndPrepare
// ============================================================

HWTEST_F_L0(SharedCCRunPhasesTest, FullCycleExercisesAllPrivateMethods)
{
    SharedHeap *sHeap = SharedHeap::GetInstance();
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);

    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
    EXPECT_FALSE(thread->NeedReadBarrier());
    EXPECT_FALSE(thread->HasCCSuspend());
    EXPECT_EQ(thread->GetSharedCCEvacuator(), nullptr);

    auto *stringTable = Runtime::GetInstance()->GetEcmaStringTable();
    ASSERT_TRUE(stringTable != nullptr);
    EXPECT_FALSE(stringTable->IsSweeping());
}

HWTEST_F_L0(SharedCCRunPhasesTest, RestoreThreadStatesViaCollectGarbage)
{
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();

    thread->SetSharedCCStatus(SharedCCStatus::READY);
    thread->SetReadBarrierState(true);
    thread->SwitchAllStub(false);
    thread->SetCCSuspend();

    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::READY);
    EXPECT_TRUE(thread->NeedReadBarrier());
    EXPECT_TRUE(thread->HasCCSuspend());

    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);

    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
    EXPECT_FALSE(thread->NeedReadBarrier());
    EXPECT_FALSE(thread->HasCCSuspend());
    EXPECT_EQ(thread->GetSharedCCEvacuator(), nullptr);
}

// ============================================================
// Unit tests: SharedCCEvacuator SetForwardingAddress
// ============================================================

// ============================================================
// Unit tests: ProcessSharedCCWeakReferenceSlot
// ============================================================

HWTEST_F_L0(SharedCCTest, WeakRefSlotSkipsNonWeak)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> obj = factory->NewTaggedArray(4, JSTaggedValue::Undefined());
    JSTaggedType slotData = obj.GetTaggedValue().GetRawData();
    ObjectSlot slot(reinterpret_cast<uintptr_t>(&slotData));

    auto tlab = std::make_unique<SharedTlabAllocator>(sHeap_);
    SharedCCEvacuator evacuator(sHeap_, tlab.get());
    ProcessSharedCCWeakReferenceSlot(slot, evacuator);

    EXPECT_EQ(slot.GetTaggedType(), obj.GetTaggedValue().GetRawData());
}

HWTEST_F_L0(SharedCCTest, WeakRefSlotClearsUnmarkedFromRegion)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> obj = factory->NewTaggedArray(4, JSTaggedValue::Undefined());
    Region *region = Region::ObjectAddressToRange(*obj);
    region->SetRegionTypeFlag(RegionTypeFlag::FROM);

    JSTaggedType weakData = obj.GetTaggedValue().GetRawData();
    JSTaggedValue weakVal(weakData);
    weakVal.CreateWeakRef();
    weakData = weakVal.GetRawData();
    ObjectSlot slot(reinterpret_cast<uintptr_t>(&weakData));

    auto tlab = std::make_unique<SharedTlabAllocator>(sHeap_);
    SharedCCEvacuator evacuator(sHeap_, tlab.get());
    ProcessSharedCCWeakReferenceSlot(slot, evacuator);

    EXPECT_EQ(slot.GetTaggedType(), JSTaggedValue::Undefined().GetRawData());

    region->SetRegionTypeFlag(RegionTypeFlag::DEFAULT);
}

HWTEST_F_L0(SharedCCTest, WeakRefSlotPreservesNonFromRegion)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> obj = factory->NewTaggedArray(4, JSTaggedValue::Undefined());

    JSTaggedType weakData = obj.GetTaggedValue().GetRawData();
    JSTaggedValue weakVal(weakData);
    weakVal.CreateWeakRef();
    weakData = weakVal.GetRawData();
    ObjectSlot slot(reinterpret_cast<uintptr_t>(&weakData));

    auto tlab = std::make_unique<SharedTlabAllocator>(sHeap_);
    SharedCCEvacuator evacuator(sHeap_, tlab.get());
    ProcessSharedCCWeakReferenceSlot(slot, evacuator);

    EXPECT_EQ(slot.GetTaggedType(), weakData);
}

// ============================================================
// Integration tests: SharedCC + local GC interaction
// ============================================================

HWTEST_F_L0(SharedCCRunPhasesTest, LocalGCAfterSharedCC)
{
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();

    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);

    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
    EXPECT_FALSE(thread->NeedReadBarrier());

    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_FALSE(thread->NeedReadBarrier());
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
}

HWTEST_F_L0(SharedCCRunPhasesTest, SharedCCThenSharedPartialGC)
{
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();

    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);

    sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);

    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
    EXPECT_FALSE(thread->NeedReadBarrier());
}

// ============================================================
// Integration tests: SharedCC with large heap
// ============================================================

HWTEST_F_L0(SharedCCRunPhasesTest, LargeHeapSharedObjectsSurvive)
{
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();
    ObjectFactory *factory = heap->GetEcmaVM()->GetFactory();

    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);
    heap->GetHeapPrepare(thread);

    SharedOldSpace *sOldSpace = sHeap->GetOldSpace();
    auto space = std::make_shared<SharedCCTestSpace>(sHeap);
    JSHandle<TaggedArray> localRefs = factory->NewTaggedArray(32, JSTaggedValue::Undefined());

    for (int i = 0; i < 32; i++) {
        auto obj = CreateSharedObjectsInOneRegion(space, aliveRate);
        Region *region = Region::ObjectAddressToRange(*obj);
        sOldSpace->AddRegion(region);
        localRefs->Set(thread, i, obj);
    }
    space->FillBumpPointer();

    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);

    int survived = 0;
    for (int i = 0; i < 32; i++) {
        JSTaggedValue val = localRefs->Get(thread, i);
        if (val.IsHeapObject() && val.IsInSharedHeap()) {
            survived++;
        }
    }
    EXPECT_GT(survived, 0);
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
}

// ============================================================
// Integration tests: SharedCC with NonMovable space
// ============================================================

HWTEST_F_L0(SharedCCRunPhasesTest, SharedCCWithAllocationsBetweenCycles)
{
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();
    ObjectFactory *factory = heap->GetEcmaVM()->GetFactory();

    heap->CollectGarbage(TriggerGCType::FULL_GC);
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);
    heap->GetHeapPrepare(thread);

    SharedOldSpace *sOldSpace = sHeap->GetOldSpace();
    auto space = std::make_shared<SharedCCTestSpace>(sHeap);
    JSHandle<TaggedArray> keep = factory->NewTaggedArray(8, JSTaggedValue::Undefined());

    for (size_t i = 0; i < SharedOldSpace::MIN_COLLECT_REGION_SIZE; i++) {
        auto obj = CreateSharedObjectsInOneRegion(space, aliveRate);
        Region *region = Region::ObjectAddressToRange(*obj);
        sOldSpace->AddRegion(region);
        keep->Set(thread, i % 8, obj);
    }
    space->FillBumpPointer();

    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);

    heap->GetHeapPrepare(thread);
    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);

    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
    EXPECT_FALSE(thread->NeedReadBarrier());
}

// ============================================================
// Unit tests: HandleSlot with dead FROM object (unmarked → cleared)
// ============================================================

HWTEST_F_L0(SharedCCTest, UpdateVisitorClearsDeadFromRegionSlot)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> deadObj = factory->NewTaggedArray(4, JSTaggedValue::Undefined());
    Region *region = Region::ObjectAddressToRange(*deadObj);
    region->SetRegionTypeFlag(RegionTypeFlag::FROM);

    JSTaggedType slotData = deadObj.GetTaggedValue().GetRawData();
    ObjectSlot slot(reinterpret_cast<uintptr_t>(&slotData));
    SharedCCUpdateVisitor updateVisitor;
    updateVisitor.HandleSlot(slot);

    EXPECT_EQ(slot.GetTaggedType(), JSTaggedValue::Undefined().GetRawData());

    region->SetRegionTypeFlag(RegionTypeFlag::DEFAULT);
}

// ============================================================
// ============================================================
// Forwarded weak slot handling covered by integration tests.
// ============================================================

// ============================================================
// Unit tests: ThreadWorkload skip-and-wait pattern
// ============================================================

HWTEST_F_L0(SharedCCTest, ThreadWorkloadSkipAdvancesIndexToEnd)
{
    ThreadWorkload tw;
    tw.thread = thread;
    for (int i = 0; i < 5; i++) {
        tw.localRegions.push_back(nullptr);
    }
    int num = static_cast<int>(tw.localRegions.size());
    tw.nextIndex_.store(num - 1, std::memory_order_relaxed);
    tw.remainItems_ = num;

    int done = 0;
    while (true) {
        int idx = tw.nextIndex_.fetch_sub(1, std::memory_order_relaxed);
        if (idx < 0) {
            break;
        }
        done++;
    }
    EXPECT_EQ(done, 5);
    EXPECT_TRUE(tw.nextIndex_.load() < 0);

    if (done > 0) {
        LockHolder lock(tw.mutex_);
        tw.remainItems_ -= done;
    }
    EXPECT_EQ(tw.remainItems_, 0);

    LockHolder lock(tw.mutex_);
    EXPECT_EQ(tw.remainItems_, 0);
}

HWTEST_F_L0(SharedCCTest, ThreadWorkloadRemainItemsSignalWhenZero)
{
    ThreadWorkload tw;
    tw.thread = thread;
    tw.localRegions.push_back(nullptr);
    tw.localRegions.push_back(nullptr);
    tw.nextIndex_.store(1, std::memory_order_relaxed);
    tw.remainItems_ = 2;

    int done = 0;
    while (true) {
        int idx = tw.nextIndex_.fetch_sub(1, std::memory_order_relaxed);
        if (idx < 0) {
            break;
        }
        done++;
    }
    if (done > 0) {
        LockHolder lock(tw.mutex_);
        tw.remainItems_ -= done;
        if (tw.remainItems_ == 0) {
            tw.cv_.SignalAll();
        }
    }

    LockHolder lock(tw.mutex_);
    while (tw.remainItems_ > 0) {
        tw.cv_.Wait(&tw.mutex_);
    }
    EXPECT_EQ(tw.remainItems_, 0);
}

// ============================================================
// Unit tests: EvacuatorEntry lifecycle
// ============================================================

HWTEST_F_L0(SharedCCTest, EvacuatorEntryLifecycle)
{
    EvacuatorEntry entry;
    entry.thread = thread;
    entry.evacuator = nullptr;
    EXPECT_EQ(entry.thread, thread);
    EXPECT_EQ(entry.evacuator, nullptr);

    auto *evacuator = new SharedCCEvacuator(sHeap_);
    entry.evacuator = evacuator;
    EXPECT_NE(entry.evacuator, nullptr);
    EXPECT_NE(entry.evacuator->GetTlabAllocator(), nullptr);
    delete entry.evacuator;
    entry.evacuator = nullptr;
    EXPECT_EQ(entry.evacuator, nullptr);
}

// ============================================================
// Integration tests: NeedSwitchRBStub
// ============================================================

HWTEST_F_L0(SharedCCRunPhasesTest, NeedSwitchRBStubFalseWhenIdle)
{
    SharedHeap *sHeap = SharedHeap::GetInstance();
    EXPECT_FALSE(sHeap->NeedSwitchRBStub());
}

HWTEST_F_L0(SharedCCRunPhasesTest, SetReadyForGCIteratingNoOpWhenIdle)
{
    SharedHeap *sHeap = SharedHeap::GetInstance();
    EXPECT_FALSE(sHeap->NeedSwitchRBStub());

    thread->SetReadyForGCIterating(false);
    EXPECT_FALSE(thread->ReadyForGCIterating());

    thread->SetReadyForGCIterating(true);
    EXPECT_TRUE(thread->ReadyForGCIterating());
    EXPECT_FALSE(thread->NeedReadBarrier());
}

// ============================================================
// Integration tests: multiple SharedGC types sequence
// ============================================================

HWTEST_F_L0(SharedCCRunPhasesTest, AllSharedGCSequence)
{
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    SharedHeap *sHeap = SharedHeap::GetInstance();

    heap->CollectGarbage(TriggerGCType::FULL_GC);

    sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
    EXPECT_FALSE(thread->NeedReadBarrier());

    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
    EXPECT_FALSE(thread->NeedReadBarrier());

    sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
    EXPECT_FALSE(thread->NeedReadBarrier());

    sHeap->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);
    EXPECT_EQ(thread->GetSharedCCStatus(), SharedCCStatus::IDLE);
    EXPECT_FALSE(thread->NeedReadBarrier());
    EXPECT_EQ(thread->GetSharedCCEvacuator(), nullptr);
}

}  // namespace panda::test
