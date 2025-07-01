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

#include "common_components/heap/collector/trace_collector.h"
#include "common_components/heap/heap_manager.h"
#include "common_components/heap/w_collector/w_collector.h"
#include "common_components/mutator/mutator_manager.h"
#include "common_components/tests/test_helper.h"
#include <cstdint>

using namespace common;

namespace common::test {
class TestWCollector : public WCollector {
public:
    using WCollector::WCollector;

    void PublicMergeMutatorRoots(WorkStack& workStack)
    {
        MergeMutatorRoots(workStack);
    }
};

class TraceCollectorTest : public common::test::BaseTestWithScope {
protected:
    static void SetUpTestCase()
    {
        BaseRuntime::GetInstance()->Init();
    }

    static void TearDownTestCase()
    {
        BaseRuntime::GetInstance()->Fini();
    }
    StaticRootTable rootTable_;
    bool ContainsRoot(StaticRootTable& table, const StaticRootTable::StaticRootArray* array, uint32_t size)
    {
        bool found = false;
        auto visitor = [&found, array, size](RefField<>& root) {
            for (uint32_t i = 0; i < size; ++i) {
                if (&root == array->content[i]) {
                    found = true;
                    return;
                }
            }
        };
        table.VisitRoots(visitor);
        return found;
    }
};

HWTEST_F_L0(TraceCollectorTest, UnregisterRoots_Exists_ShouldEraseAndDecreaseCount)
{
    StaticRootTable::StaticRootArray dummyArray;
    const uint32_t size = 10;
    rootTable_.RegisterRoots(&dummyArray, size);
    EXPECT_TRUE(ContainsRoot(rootTable_, &dummyArray, size));
    rootTable_.UnregisterRoots(&dummyArray, size);
    EXPECT_FALSE(ContainsRoot(rootTable_, &dummyArray, size));
}

HWTEST_F_L0(TraceCollectorTest, UnregisterRoots_NotExists_ShouldNotModify)
{
    const uint32_t size = 10;
    StaticRootTable::StaticRootArray anotherDummyArray;
    rootTable_.UnregisterRoots(&anotherDummyArray, size);
    EXPECT_FALSE(ContainsRoot(rootTable_, &anotherDummyArray, size));
}

std::unique_ptr<TestWCollector> GetWCollector()
{
    CollectorResources& resources = Heap::GetHeap().GetCollectorResources();
    Allocator& allocator = Heap::GetHeap().GetAllocator();
    return std::make_unique<TestWCollector>(allocator, resources);
}

HWTEST_F_L0(TraceCollectorTest, MergeMutatorRoots_WorldNotStopped_EnterBothIfs)
{
    std::unique_ptr<TestWCollector> wCollector = GetWCollector();
    ASSERT_TRUE(wCollector != nullptr);
    TraceCollector::WorkStack workStack;
    auto& mutatorManager = MutatorManager::Instance();
    EXPECT_FALSE(mutatorManager.WorldStopped());
    wCollector->PublicMergeMutatorRoots(workStack);
    EXPECT_TRUE(mutatorManager.TryAcquireMutatorManagementWLock());
    mutatorManager.MutatorManagementWUnlock();
}
}