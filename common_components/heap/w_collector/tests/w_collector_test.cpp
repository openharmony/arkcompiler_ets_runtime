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
#include "common_components/mutator/mutator_manager-inl.h"

using namespace common;

namespace common::test {
using SuspensionType = MutatorBase::SuspensionType;
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

    RegionDesc *region = RegionDesc::GetRegionDescAt(addr);

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
    STWParam stwParam{"flip-test"};
    mutatorManager.FlipMutators(stwParam, stwTest, &mutatorTest);
}

HWTEST_F_L0(WCollectorTest, FlipTest)
{
    std::thread t1(FlipTest);
    t1.join();
}
}  // namespace common::test
