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

#include "common_components/heap/collector/task_queue.h"
#include "common_components/heap/collector/collector_proxy.h"

#include "common_components/tests/test_helper.h"

using namespace common;

// ==================== Mock Classes ====================

class MockCollectorProxy : public CollectorProxy {
public:
    explicit MockCollectorProxy(Allocator& allocator, CollectorResources& resources)
        : CollectorProxy(allocator, resources), runCalled_(false),
          lastGcIndex_(0), lastReason_(GC_REASON_INVALID) {}

    void RunGarbageCollection(uint64_t gcIndex, GCReason reason) override
    {
        runCalled_ = true;
        lastGcIndex_ = gcIndex;
        lastReason_ = reason;
    }

    bool WasRunCalled() const
    {
        return runCalled_;
    }
    uint64_t GetLastGcIndex() const
    {
        return lastGcIndex_;
    }
    GCReason GetLastReason() const
    {
        return lastReason_;
    }

    void Reset()
    {
        runCalled_ = false;
        lastGcIndex_ = 0;
        lastReason_ = GC_REASON_INVALID;
    }

private:
    mutable bool runCalled_;
    uint64_t lastGcIndex_;
    GCReason lastReason_;
};

constexpr size_t DEFAULT_MAX_CAPACITY_SIZE = 10 * 1024 * 1024;
constexpr size_t DEFAULT_CAPACITY_SIZE = 5 * 1024 * 1024;

class TestAllocator : public Allocator {
public:
    HeapAddress Allocate(size_t size, AllocType type) override
    {
        return 0;
    }
    HeapAddress AllocateNoGC(size_t size, AllocType type) override
    {
        return 0;
    }
    bool ForEachObject(const std::function<void(BaseObject*)>& callback, bool safe) const override
    {
        return true;
    }
    size_t ReclaimGarbageMemory(bool releaseAll) override
    {
        return 0;
    }
    void FeedHungryBuffers() override {}
    size_t LargeObjectSize() const override
    {
        return 0;
    }
    size_t GetAllocatedBytes() const override
    {
        return 0;
    }
    void Init(const RuntimeParam& param) override {}

    size_t GetMaxCapacity() const override
    {
        return DEFAULT_MAX_CAPACITY_SIZE;
    }
    size_t GetCurrentCapacity() const override
    {
        return DEFAULT_CAPACITY_SIZE;
    }
    size_t GetUsedPageSize() const override
    {
        return 0;
    }
    HeapAddress GetSpaceStartAddress() const override
    {
        return 0;
    }
    HeapAddress GetSpaceEndAddress() const override
    {
        return 0;
    }

#ifndef NDEBUG
    bool IsHeapObject(HeapAddress) const override { return false; }
#endif
};

class TestCollectorResources : public CollectorResources {
public:
    explicit TestCollectorResources(CollectorProxy& proxy) : CollectorResources(proxy) {}
    void Init() {}
    void Fini() {}
};

// ==================== Fix: Use correct GCTask interface ====================

class MockGCTask : public GCTask {
public:
    explicit MockGCTask(GCTaskType type, uint64_t index, GCReason reason)
        : GCTask(type), taskIndex_(index), gcReason_(reason) {}

    uint64_t GetGCIndex() const
    {
        return taskIndex_;
    }
    GCReason GetGCReason() const
    {
        return gcReason_;
    }

    bool Execute(void* owner) override
    {
        if (owner == nullptr) {
            return false;
        }

        auto* proxy = reinterpret_cast<CollectorProxy*>(owner);
        switch (GetTaskType()) {
            case GCTaskType::GC_TASK_TERMINATE_GC:
                return false;
            case GCTaskType::GC_TASK_INVOKE_GC:
                proxy->RunGarbageCollection(taskIndex_, gcReason_);
                return true;
            default:
                return true;
        }
    }

private:
    uint64_t taskIndex_;
    GCReason gcReason_;
};

// ==================== Test Fixture ====================

// class GCRunnerTest : public BaseTestWithScope {

namespace common::test {
class GCRunnerTest : public common::test::BaseTestWithScope {
protected:
    void SetUp() override
    {
        resources_.reset(new TestCollectorResources(proxy_));
        mockProxy_ = new MockCollectorProxy(allocator_, *resources_);
    }

    void TearDown() override
    {
        delete mockProxy_;
        mockProxy_ = nullptr;
        resources_.reset();
    }

    TestAllocator allocator_;
    std::unique_ptr<TestCollectorResources> resources_;
    MockCollectorProxy* mockProxy_;
    CollectorProxy proxy_{allocator_, *resources_};
};

// ==================== Test Cases ====================

/**
 * @tc.name: GCRunner_Execute_Terminate
 * @tc.desc: Test GC_TASK_TERMINATE_GC task type.
 * @tc.type: FUNC
 */
HWTEST_F_L0(GCRunnerTest, Execute_Terminate) {
    // Arrange
    MockGCTask task(GCTask::GCTaskType::GC_TASK_TERMINATE_GC, 0, GC_REASON_INVALID);

    // Act
    bool result = task.Execute(mockProxy_);

    // Assert
    EXPECT_FALSE(result);  // Should return false to terminate thread
}

/**
 * @tc.name: GCRunner_Execute_InvokeGC
 * @tc.desc: Test GC_TASK_INVOKE_GC triggers RunGarbageCollection.
 * @tc.type: FUNC
 */
HWTEST_F_L0(GCRunnerTest, Execute_InvokeGC) {
    // Arrange
    mockProxy_->Reset();

    MockGCTask task(GCTask::GCTaskType::GC_TASK_INVOKE_GC, 123, GC_REASON_FORCE);

    // Act
    bool result = task.Execute(mockProxy_);

    // Assert
    EXPECT_TRUE(result);  // Thread should continue
    EXPECT_TRUE(mockProxy_->WasRunCalled());
    EXPECT_EQ(mockProxy_->GetLastGcIndex(), 123U);
    EXPECT_EQ(mockProxy_->GetLastReason(), GC_REASON_FORCE);
}
}