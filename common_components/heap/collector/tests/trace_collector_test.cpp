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
    class TableTraceCollctor : public TraceCollector {
    public:
        using TraceCollector::UpdateNativeThreshold;
    };
};

std::unique_ptr<TestWCollector> GetWCollector()
{
    CollectorResources& resources = Heap::GetHeap().GetCollectorResources();
    Allocator& allocator = Heap::GetHeap().GetAllocator();
    return std::make_unique<TestWCollector>(allocator, resources);
}

HWTEST_F_L0(TraceCollectorTest, RunGarbageCollection)
{
    TraceCollector& collector = reinterpret_cast<TraceCollector&>(Heap::GetHeap().GetCollector());
    Heap::GetHeap().SetGCReason(GCReason::GC_REASON_YOUNG);
    collector.RunGarbageCollection(0, GCReason::GC_REASON_USER, common::GC_TYPE_FULL);
    ASSERT_FALSE(Heap::GetHeap().GetCollector().GetGCStats().isYoungGC());

    Heap::GetHeap().SetGCReason(GCReason::GC_REASON_BACKUP);
    collector.RunGarbageCollection(0, GCReason::GC_REASON_OOM, common::GC_TYPE_FULL);
    ASSERT_FALSE(Heap::GetHeap().GetCollector().GetGCStats().isYoungGC());
}

HWTEST_F_L0(TraceCollectorTest, RunGarbageCollectionTest2)
{
    TraceCollector& collector = reinterpret_cast<TraceCollector&>(Heap::GetHeap().GetCollector());
    Heap::GetHeap().SetGCReason(GCReason::GC_REASON_YOUNG);
    collector.RunGarbageCollection(0, GCReason::GC_REASON_YOUNG, common::GC_TYPE_FULL);
    ASSERT_TRUE(Heap::GetHeap().GetCollector().GetGCStats().isYoungGC());
}

HWTEST_F_L0(TraceCollectorTest, UpdateNativeThresholdTest)
{
    TableTraceCollctor& collector = reinterpret_cast<TableTraceCollctor&>(Heap::GetHeap().GetCollector());
    GCParam gcParam;
    gcParam.minGrowBytes = 1024;
    Heap::GetHeap().SetNativeHeapThreshold(512);
    auto oldThreshold = Heap::GetHeap().GetNativeHeapThreshold();
    collector.UpdateNativeThreshold(gcParam);
    auto newThreshold = Heap::GetHeap().GetNativeHeapThreshold();
    EXPECT_NE(newThreshold, oldThreshold);
}
}