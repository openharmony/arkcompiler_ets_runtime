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
#include "common_components/heap/collector/collector_proxy.h"
#include "common_components/heap/heap_manager.h"
#include "common_components/heap/allocator/region_desc.h"

using namespace common;

namespace common::test {
class WCollectorTest : public BaseTestWithScope {
protected:
    static void SetUpTestCase()
    {
        BaseRuntime::GetInstance()->Init();
    }

    static void TearDownTestCase()
    {}

    void SetUp() override
    {
        MutatorManager::Instance().CreateRuntimeMutator(ThreadType::GC_THREAD);
    }

    void TearDown() override
    {
        MutatorManager::Instance().DestroyRuntimeMutator(ThreadType::GC_THREAD);
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
    EXPECT_FALSE(wcollector->ResurrectObject(obj));
}

HWTEST_F_L0(WCollectorTest, IsUnmovableFromObjectTest2)
{
    std::unique_ptr<WCollector> wcollector = GetWCollector();
    ASSERT_TRUE(wcollector != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::PINNED_OBJECT, true);
    BaseObject *obj = reinterpret_cast<BaseObject *>(addr);

    new (obj) BaseObject();

    RegionDesc *region = RegionDesc::GetRegionDescAt(addr);

    bool isMarked = region->GetOrAllocResurrectBitmap()->MarkBits(0);
    region->SetResurrectedRegionFlag(1);
    region->SetRegionType(RegionDesc::RegionType::EXEMPTED_FROM_REGION);

    EXPECT_FALSE(isMarked);

    EXPECT_TRUE(wcollector->IsUnmovableFromObject(obj));
    EXPECT_TRUE(wcollector->ResurrectObject(obj));
}

HWTEST_F_L0(WCollectorTest, MarkNewObjectTest0)
{
    std::unique_ptr<WCollector> wcollector = GetWCollector();
    ASSERT_TRUE(wcollector != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject *obj = reinterpret_cast<BaseObject *>(addr);

    new (obj) BaseObject();

    Mutator::GetMutator()->SetMutatorPhase(GCPhase::GC_PHASE_MARK);

    obj->SetForwardState(BaseStateWord::ForwardState::FORWARDED);
    EXPECT_TRUE(obj->IsForwarded());

    wcollector->MarkNewObject(obj);
    EXPECT_TRUE(RegionSpace::IsMarkedObject(obj));
}

HWTEST_F_L0(WCollectorTest, MarkNewObjectTest1)
{
    std::unique_ptr<WCollector> wcollector = GetWCollector();
    ASSERT_TRUE(wcollector != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject *obj = reinterpret_cast<BaseObject *>(addr);

    new (obj) BaseObject();

    Mutator::GetMutator()->SetMutatorPhase(GCPhase::GC_PHASE_ENUM);

    obj->SetForwardState(BaseStateWord::ForwardState::FORWARDED);
    EXPECT_TRUE(obj->IsForwarded());

    wcollector->MarkNewObject(obj);
    EXPECT_TRUE(RegionSpace::IsMarkedObject(obj));
}
HWTEST_F_L0(WCollectorTest, MarkNewObjectTest2)
{
    std::unique_ptr<WCollector> wcollector = GetWCollector();
    ASSERT_TRUE(wcollector != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject *obj = reinterpret_cast<BaseObject *>(addr);

    new (obj) BaseObject();

    Mutator::GetMutator()->SetMutatorPhase(GCPhase::GC_PHASE_REMARK_SATB);

    obj->SetForwardState(BaseStateWord::ForwardState::FORWARDED);
    EXPECT_TRUE(obj->IsForwarded());

    wcollector->MarkNewObject(obj);
    EXPECT_TRUE(RegionSpace::IsMarkedObject(obj));
}

HWTEST_F_L0(WCollectorTest, MarkNewObjectTest3)
{
    std::unique_ptr<WCollector> wcollector = GetWCollector();
    ASSERT_TRUE(wcollector != nullptr);

    HeapAddress addr = HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject *obj = reinterpret_cast<BaseObject *>(addr);

    new (obj) BaseObject();

    Mutator::GetMutator()->SetMutatorPhase(GCPhase::GC_PHASE_FINAL_MARK);

    obj->SetForwardState(BaseStateWord::ForwardState::FORWARDED);
    EXPECT_TRUE(obj->IsForwarded());

    wcollector->MarkNewObject(obj);
    EXPECT_FALSE(RegionSpace::IsMarkedObject(obj));
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
}  // namespace common::test
