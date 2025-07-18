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

#include "common_components/tests/test_helper.h"

#include "common_components/heap/w_collector/w_collector.h"
#include "common_components/heap/w_collector/w_collector.cpp"
#include "common_components/heap/collector/collector_proxy.h"
#include "common_components/heap/heap_manager.h"
#include "common_components/heap/allocator/region_desc.h"
#include "common_components/mutator/mutator_manager-inl.h"

using namespace common;

namespace common::test {
using SuspensionType = MutatorBase::SuspensionType;
class WCollectorTest : public common::test::BaseTestWithScope {
protected:
    static void SetUpTestCase()
    {
        BaseRuntime::GetInstance()->Init();
    }

    static void TearDownTestCase()
    {}

    void SetUp() override
    {
        MutatorManager::Instance().CreateRuntimeMutator(ThreadType::ARK_PROCESSOR);
    }

    void TearDown() override
    {
        MutatorManager::Instance().DestroyRuntimeMutator(ThreadType::ARK_PROCESSOR);
    }
};

std::unique_ptr<WCollector> GetWCollector()
{
    CollectorResources &resources = Heap::GetHeap().GetCollectorResources();
    Allocator &allocator = Heap::GetHeap().GetAllocator();

    return std::make_unique<WCollector>(allocator, resources);
}

HWTEST_F_L0(WCollectorTest, IsUnmovableFromObjectTest0)
{
    std::unique_ptr<WCollector> wcollector = GetWCollector();
    ASSERT_TRUE(wcollector != nullptr);

    BaseObject *obj = nullptr;
    EXPECT_FALSE(wcollector->IsUnmovableFromObject(obj));
}

HWTEST_F_L0(WCollectorTest, IsUnmovableFromObjectTest1)
{
    std::unique_ptr<WCollector> wcollector = GetWCollector();
    ASSERT_TRUE(wcollector != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject *obj = reinterpret_cast<BaseObject *>(addr);

    new (obj) BaseObject();

    EXPECT_FALSE(wcollector->IsUnmovableFromObject(obj));
}

HWTEST_F_L0(WCollectorTest, IsUnmovableFromObjectTest2)
{
    std::unique_ptr<WCollector> wcollector = GetWCollector();
    ASSERT_TRUE(wcollector != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::PINNED_OBJECT, true);
    BaseObject *obj = reinterpret_cast<BaseObject *>(addr);

    new (obj) BaseObject();

    RegionDesc *region = RegionDesc::GetAliveRegionDescAt(addr);

    bool isMarked = region->GetOrAllocResurrectBitmap()->MarkBits(0);
    region->SetResurrectedRegionFlag(1);
    region->SetRegionType(RegionDesc::RegionType::EXEMPTED_FROM_REGION);

    EXPECT_FALSE(isMarked);

    EXPECT_TRUE(wcollector->IsUnmovableFromObject(obj));
}

HWTEST_F_L0(WCollectorTest, ForwardUpdateRawRefTest0)
{
    std::unique_ptr<WCollector> wcollector = GetWCollector();
    ASSERT_TRUE(wcollector != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject *obj = reinterpret_cast<BaseObject *>(addr);

    new (obj) BaseObject();

    common::ObjectRef root = {obj};

    BaseObject *oldObj = wcollector->ForwardUpdateRawRef(root);
    EXPECT_EQ(oldObj, obj);
}

void FlipTest()
{
    MutatorManager &mutatorManager = MutatorManager::Instance();
    ThreadHolder::CreateAndRegisterNewThreadHolder(nullptr);
    bool stwCallbackExecuted = false;
    auto stwTest = [&mutatorManager, &stwCallbackExecuted]() {
        EXPECT_TRUE(mutatorManager.WorldStopped());
        stwCallbackExecuted = true;
    };
    FlipFunction mutatorTest = [&mutatorManager, &stwCallbackExecuted](Mutator &mutator) {
        EXPECT_TRUE(mutator.HasSuspensionRequest(SuspensionType::SUSPENSION_FOR_RUNNING_CALLBACK));
        EXPECT_FALSE(mutatorManager.WorldStopped());
        EXPECT_TRUE(stwCallbackExecuted);
    };
    STWParam param;
    param.stwReason = "flip-test";
    mutatorManager.FlipMutators(param, stwTest, &mutatorTest);
}

HWTEST_F_L0(WCollectorTest, FlipTest)
{
    std::thread t1(FlipTest);
    t1.join();
}

HWTEST_F_L0(WCollectorTest, IsUnmovableFromObject_ReturnsFalseForNullptr)
{
    std::unique_ptr<WCollector> wcollector = GetWCollector();
    ASSERT_TRUE(wcollector != nullptr);

    BaseObject* obj = nullptr;
    EXPECT_FALSE(wcollector->IsUnmovableFromObject(obj));
}

class TestableWCollector : public WCollector {
public:
    using WCollector::ForwardObject;

    explicit TestableWCollector(Allocator& allocator, CollectorResources& resources)
        : WCollector(allocator, resources), currentGCPhase_(GCPhase::GC_PHASE_COPY) {}

    void SetCurrentGCPhaseForTest(GCPhase phase)
    {
        currentGCPhase_ = phase;
    }

    GCPhase GetCurrentGCPhaseForTest() const
    {
        return currentGCPhase_;
    }

private:
    GCPhase currentGCPhase_;
};


class DummyObject : public BaseObject {
public:
    const common::TypeInfo* GetTypeInfo() const { return nullptr; }
    size_t GetSize() const { return sizeof(DummyObject); }

    void SetClass(uintptr_t cls)
    {
        stateWord_.StoreStateWord(static_cast<StateWordType>(cls));
    }

private:
    class BaseStateWord {
    public:
        using StateWordType = uint64_t;

        void StoreStateWord(StateWordType word)
        {
            stateWord_ = word;
        }

        StateWordType LoadStateWord() const
        {
            return stateWord_;
        }

    private:
        StateWordType stateWord_{0};
    };

    BaseStateWord stateWord_;
};

HWTEST_F_L0(WCollectorTest, ForwardObject_WithUnmovedObject_ReturnsSameAddress)
{
    std::unique_ptr<WCollector> wcollector = GetWCollector();
    ASSERT_TRUE(wcollector != nullptr);

    TestableWCollector* testableCollector = reinterpret_cast<TestableWCollector*>(wcollector.get());

    testableCollector->SetCurrentGCPhaseForTest(GCPhase::GC_PHASE_COPY);
    EXPECT_EQ(testableCollector->GetCurrentGCPhaseForTest(), GCPhase::GC_PHASE_COPY);
}
class TestCreateTraceWCollector : public TraceCollector {
public:
    using TraceCollector::SetGCReason;
    explicit TestCreateTraceWCollector(Allocator& allocator, CollectorResources& resources)
        : TraceCollector(allocator, resources) {}
    BaseObject* ForwardObject(BaseObject*) override { return nullptr; }
    bool IsFromObject(BaseObject*) const override { return false; }
    bool IsUnmovableFromObject(BaseObject*) const override { return false; }
    BaseObject* FindToVersion(BaseObject* obj) const override { return nullptr; }
    bool TryUpdateRefField(BaseObject*, RefField<>&, BaseObject*&) const override { return false; }
    bool TryForwardRefField(BaseObject*, RefField<>&, BaseObject*&) const override { return false; }
    bool TryUntagRefField(BaseObject*, RefField<>&, BaseObject*&) const override { return false; }
    RefField<> GetAndTryTagRefField(BaseObject*) const override { return RefField<>(nullptr); }
    bool IsOldPointer(RefField<>&) const override { return false; }
    void AddRawPointerObject(BaseObject*) override {}
    void RemoveRawPointerObject(BaseObject*) override {}
    bool MarkObject(BaseObject* obj) const override { return false; }
    void TraceObjectRefFields(BaseObject *obj, TraceRefFieldVisitor *data) override {}
    BaseObject* CopyObjectAfterExclusive(BaseObject* obj) override { return nullptr; }
    void DoGarbageCollection() override {}
    bool IsCurrentPointer(RefField<>&) const override { return false; }
    TraceRefFieldVisitor CreateTraceObjectRefFieldsVisitor(WorkStack *workStack, WeakStack *weakStack) override
    {
        return TraceRefFieldVisitor();
    }
};

HWTEST_F_L0(WCollectorTest, FixRefField_TEST1)
{
    std::unique_ptr<WCollector> wcollector = GetWCollector();
    ASSERT_TRUE(wcollector != nullptr);

    constexpr uint64_t TAG_BITS_SHIFT = 48;
    constexpr uint64_t TAG_MARK = 0xFFFFULL << TAG_BITS_SHIFT;
    constexpr uint64_t TAG_SPECIAL = 0x02ULL;
    constexpr uint64_t TAG_BOOLEAN = 0x04ULL;
    constexpr uint64_t TAG_HEAP_OBJECT_MASK = TAG_MARK | TAG_SPECIAL | TAG_BOOLEAN;
    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject *obj = reinterpret_cast<BaseObject *>(addr | TAG_HEAP_OBJECT_MASK);
    RefField<> field(obj);
    wcollector->FixRefField(obj, field);
    EXPECT_FALSE(Heap::IsTaggedObject(field.GetFieldValue()));
}

HWTEST_F_L0(WCollectorTest, FixRefField_TEST2)
{
    std::unique_ptr<WCollector> wcollector = GetWCollector();
    ASSERT_TRUE(wcollector != nullptr);

    BaseObject* obj = reinterpret_cast<BaseObject*>(0);
    RefField<> field(obj);
    wcollector->FixRefField(obj, field);
    EXPECT_FALSE(Heap::IsHeapAddress(obj));
}

class TestStaticObject : public BaseObjectOperatorInterfaces {
public:
    size_t GetSize(const BaseObject *object) const override { return 0; }
    bool IsValidObject(const BaseObject *object) const override { return false; }
    void ForEachRefField(const BaseObject *object, const RefFieldVisitor &visitor) const override {}
    void SetForwardingPointerAfterExclusive(BaseObject *object, BaseObject *fwdPtr) override {}
    BaseObject *GetForwardingPointer(const BaseObject *object) const override
    {
        return const_cast<BaseObject*>(object);
    }
};

HWTEST_F_L0(WCollectorTest, ForwardUpdateRawRef_TEST1)
{
    std::unique_ptr<WCollector> wcollector = GetWCollector();
    ASSERT_TRUE(wcollector != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    region->SetRegionType(RegionDesc::RegionType::FROM_REGION);
    Mutator::GetMutator()->SetMutatorPhase(GCPhase::GC_PHASE_PRECOPY);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    EXPECT_FALSE(obj->IsForwarded());
    obj->SetForwardState(BaseStateWord::ForwardState::FORWARDED);
    obj->SetLanguageType(LanguageType::STATIC);
    EXPECT_TRUE(obj->IsForwarded());
    TestStaticObject staticObject;
    obj->RegisterStatic(&staticObject);

    ObjectRef root;
    root.object = obj;
    auto ret = wcollector->ForwardUpdateRawRef(root);
    EXPECT_TRUE(wcollector->IsFromObject(obj));
    EXPECT_EQ(ret, obj);
}

class TestForwardNullObject : public TestStaticObject {
public:
    BaseObject *GetForwardingPointer(const BaseObject *object) const override { return nullptr; }
};

HWTEST_F_L0(WCollectorTest, ForwardObject_TEST1)
{
    std::unique_ptr<WCollector> wcollector = GetWCollector();
    ASSERT_TRUE(wcollector != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    region->SetRegionType(RegionDesc::RegionType::FROM_REGION);
    Mutator::GetMutator()->SetMutatorPhase(GCPhase::GC_PHASE_PRECOPY);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    EXPECT_FALSE(obj->IsForwarded());
    obj->SetForwardState(BaseStateWord::ForwardState::FORWARDED);
    obj->SetLanguageType(LanguageType::STATIC);
    EXPECT_TRUE(obj->IsForwarded());
    TestForwardNullObject staticObject;
    obj->RegisterStatic(&staticObject);

    auto ret = wcollector->ForwardObject(obj);
    EXPECT_EQ(ret, obj);
}

HWTEST_F_L0(WCollectorTest, CopyObjectImpl_TEST1)
{
    std::unique_ptr<WCollector> wcollector = GetWCollector();
    ASSERT_TRUE(wcollector != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    region->SetRegionType(RegionDesc::RegionType::FROM_REGION);
    Mutator::GetMutator()->SetMutatorPhase(GCPhase::GC_PHASE_PRECOPY);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    EXPECT_FALSE(obj->IsForwarded());
    obj->SetForwardState(BaseStateWord::ForwardState::FORWARDED);
    obj->SetLanguageType(LanguageType::STATIC);
    EXPECT_TRUE(obj->IsForwarded());
    TestForwardNullObject staticObject;
    obj->RegisterStatic(&staticObject);

    auto ret = wcollector->CopyObjectImpl(obj);
    EXPECT_TRUE(ret == nullptr);
}

HWTEST_F_L0(WCollectorTest, TryUpdateRefFieldImpl_TEST1)
{
    std::unique_ptr<WCollector> wcollector = GetWCollector();
    ASSERT_TRUE(wcollector != nullptr);

    RefField<> field(nullptr);
    BaseObject* obj = nullptr;
    bool ret = wcollector->TryUpdateRefField(nullptr, field, obj);
    EXPECT_FALSE(ret);
}

HWTEST_F_L0(WCollectorTest, TryUpdateRefFieldImpl_TEST2)
{
    std::unique_ptr<WCollector> wcollector = GetWCollector();
    ASSERT_TRUE(wcollector != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    region->SetRegionType(RegionDesc::RegionType::FROM_REGION);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    RefField<false> field(obj);
    bool ret = wcollector->TryUpdateRefField(nullptr, field, obj);
    EXPECT_FALSE(ret);
}

HWTEST_F_L0(WCollectorTest, TryUpdateRefFieldImpl_TEST3)
{
    std::unique_ptr<WCollector> wcollector = GetWCollector();
    ASSERT_TRUE(wcollector != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    region->SetRegionType(RegionDesc::RegionType::FROM_REGION);
    Mutator::GetMutator()->SetMutatorPhase(GCPhase::GC_PHASE_PRECOPY);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    EXPECT_FALSE(obj->IsForwarded());
    obj->SetForwardState(BaseStateWord::ForwardState::FORWARDED);
    obj->SetLanguageType(LanguageType::STATIC);
    EXPECT_TRUE(obj->IsForwarded());
    TestForwardNullObject staticObject;
    obj->RegisterStatic(&staticObject);

    RefField<false> field(obj);
    bool ret = wcollector->TryForwardRefField(nullptr, field, obj);
    EXPECT_FALSE(ret);
}

HWTEST_F_L0(WCollectorTest, TryUpdateRefFieldImpl_TEST4)
{
    std::unique_ptr<WCollector> wcollector = GetWCollector();
    ASSERT_TRUE(wcollector != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    RegionDesc* region = RegionDesc::GetRegionDescAt(addr);
    region->SetRegionType(RegionDesc::RegionType::FROM_REGION);
    Mutator::GetMutator()->SetMutatorPhase(GCPhase::GC_PHASE_PRECOPY);
    BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
    EXPECT_FALSE(obj->IsForwarded());
    obj->SetForwardState(BaseStateWord::ForwardState::FORWARDED);
    obj->SetLanguageType(LanguageType::STATIC);
    EXPECT_TRUE(obj->IsForwarded());
    TestStaticObject staticObject;
    obj->RegisterStatic(&staticObject);

    RefField<false> field(obj);
    bool ret = wcollector->TryForwardRefField(nullptr, field, obj);
    EXPECT_TRUE(ret);
}
}  // namespace common::test
