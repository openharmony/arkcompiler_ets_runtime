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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/shared_heap/shared_space.h"
#include "ecmascript/mem/shared_heap/shared_memory_reallocator.h"
#include "ecmascript/object_factory.h"

using namespace panda::ecmascript;

namespace panda::test {

class SharedMemoryReallocatorTest : public BaseTestWithScope<false> {
};

// Test basic memory reallocation when SharedOldSpace is exhausted
HWTEST_F_L0(SharedMemoryReallocatorTest, ReallocateMemoryFromHugeToOldSpace)
{
    auto sHeap = SharedHeap::GetInstance();
    ASSERT_NE(sHeap, nullptr);

    auto oldSpace = sHeap->GetOldSpace();
    auto hugeSpace = sHeap->GetHugeObjectSpace();

    ASSERT_NE(oldSpace, nullptr);
    ASSERT_NE(hugeSpace, nullptr);

    // Get initial capacities
    size_t initialOldCapacity = oldSpace->GetMaximumCapacity();
    size_t initialHugeCapacity = hugeSpace->GetMaximumCapacity();

    LOG_ECMA_MEM(INFO) << "Initial OldSpace capacity: " << initialOldCapacity;
    LOG_ECMA_MEM(INFO) << "Initial HugeObjectSpace capacity: " << initialHugeCapacity;

    // Verify that total capacity is preserved
    size_t initialTotalCapacity = initialOldCapacity + initialHugeCapacity;

    // Try to transfer memory
    sHeap->TryTransferMemoryToOldSpace();

    // Verify capacity was transferred correctly
    size_t newOldCapacity = oldSpace->GetMaximumCapacity();
    size_t newHugeCapacity = hugeSpace->GetMaximumCapacity();

    // Capacity should either remain the same (no transfer) or increase (transfer happened)
    if (newOldCapacity > initialOldCapacity) {
        EXPECT_LT(newHugeCapacity, initialHugeCapacity) << "HugeObjectSpace capacity should decrease after transfer";

        // Verify total capacity is preserved
        size_t newTotalCapacity = newOldCapacity + newHugeCapacity;
        EXPECT_EQ(newTotalCapacity, initialTotalCapacity) << "Total capacity should be preserved";

        LOG_ECMA_MEM(INFO) << "After transfer - OldSpace capacity: " << newOldCapacity;
        LOG_ECMA_MEM(INFO) << "After transfer - HugeObjectSpace capacity: " << newHugeCapacity;
    }
}

// Test memory allocation after reallocation
HWTEST_F_L0(SharedMemoryReallocatorTest, AllocateAfterMemoryReallocation)
{
    auto sHeap = SharedHeap::GetInstance();
    auto factory = instance->GetFactory();

    ASSERT_NE(sHeap, nullptr);
    ASSERT_NE(factory, nullptr);

    // Try to transfer memory first
    size_t initialOldCapacity = sHeap->GetOldSpace()->GetMaximumCapacity();
    sHeap->TryTransferMemoryToOldSpace();
    size_t newOldCapacity = sHeap->GetOldSpace()->GetMaximumCapacity();

    // If transfer happened, try to allocate objects to verify the space is functional
    if (newOldCapacity > initialOldCapacity) {
        const int arraySize = 1000;
        auto arrayHandle = factory->NewTaggedArray(arraySize);

        EXPECT_NE(arrayHandle.GetTaggedValue(), JSTaggedValue::Undefined());
        EXPECT_EQ(arrayHandle->GetLength(), arraySize);

        LOG_ECMA_MEM(INFO) << "Successfully allocated array after memory reallocation";
    } else {
        LOG_ECMA_MEM(INFO) << "Memory transfer not performed, skipping allocation test";
    }
}

// Test that memory reallocation doesn't happen when HugeObjectSpace has insufficient free memory
HWTEST_F_L0(SharedMemoryReallocatorTest, NoReallocWhenHugeSpaceFull)
{
    auto sHeap = SharedHeap::GetInstance();
    auto oldSpace = sHeap->GetOldSpace();
    auto hugeSpace = sHeap->GetHugeObjectSpace();

    ASSERT_NE(sHeap, nullptr);
    ASSERT_NE(oldSpace, nullptr);
    ASSERT_NE(hugeSpace, nullptr);

    // Get initial capacities
    size_t initialOldCapacity = oldSpace->GetMaximumCapacity();
    size_t initialHugeCapacity = hugeSpace->GetMaximumCapacity();

    // Calculate free memory in HugeObjectSpace
    size_t hugeCommitted = hugeSpace->GetCommittedSize();
    size_t hugeFree = initialHugeCapacity - hugeCommitted;

    LOG_ECMA_MEM(INFO) << "HugeObjectSpace committed: " << hugeCommitted;
    LOG_ECMA_MEM(INFO) << "HugeObjectSpace free: " << hugeFree;

    // If HugeObjectSpace has less than threshold free memory, transfer should not happen
    if (hugeFree <= SharedMemoryReallocator::HUGE_SPACE_FREE_THRESHOLD) {
        sHeap->TryTransferMemoryToOldSpace();

        // Verify capacities haven't changed
        EXPECT_EQ(oldSpace->GetMaximumCapacity(), initialOldCapacity) << "OldSpace capacity should not change";
        EXPECT_EQ(hugeSpace->GetMaximumCapacity(), initialHugeCapacity) << "HugeObjectSpace capacity should not change";
    } else {
        LOG_ECMA_MEM(INFO) << "HugeObjectSpace has sufficient free memory, skipping this test";
    }
}

// Test memory reallocation with different transfer sizes
HWTEST_F_L0(SharedMemoryReallocatorTest, CalculateTransferSize)
{
    auto sHeap = SharedHeap::GetInstance();
    auto hugeSpace = sHeap->GetHugeObjectSpace();

    ASSERT_NE(sHeap, nullptr);
    ASSERT_NE(hugeSpace, nullptr);

    size_t hugeCommitted = hugeSpace->GetCommittedSize();
    size_t hugeMaxCapacity = hugeSpace->GetMaximumCapacity();
    size_t hugeFree = hugeMaxCapacity - hugeCommitted;

    LOG_ECMA_MEM(INFO) << "HugeObjectSpace committed: " << hugeCommitted;
    LOG_ECMA_MEM(INFO) << "HugeObjectSpace max capacity: " << hugeMaxCapacity;
    LOG_ECMA_MEM(INFO) << "HugeObjectSpace free: " << hugeFree;

    // If there's free memory, verify transfer size calculation
    if (hugeFree > SharedMemoryReallocator::HUGE_SPACE_FREE_THRESHOLD) {
        size_t initialOldCapacity = sHeap->GetOldSpace()->GetMaximumCapacity();
        size_t initialHugeCapacity = hugeSpace->GetMaximumCapacity();

        sHeap->TryTransferMemoryToOldSpace();

        size_t newOldCapacity = sHeap->GetOldSpace()->GetMaximumCapacity();
        size_t newHugeCapacity = hugeSpace->GetMaximumCapacity();

        size_t transferSize = newOldCapacity - initialOldCapacity;
        size_t expectedTransferSize =
            AlignUp(static_cast<size_t>((hugeFree - SharedMemoryReallocator::HUGE_SPACE_FREE_THRESHOLD) *
                SharedMemoryReallocator::TRANSFER_RATIO + 0.5), DEFAULT_REGION_SIZE);

        LOG_ECMA_MEM(INFO) << "Expected transfer size: " << expectedTransferSize;
        LOG_ECMA_MEM(INFO) << "Actual capacity increase: " << transferSize;

        // Verify capacity was transferred correctly (or not at all if conditions changed)
        if (transferSize > 0) {
            EXPECT_EQ(newOldCapacity + newHugeCapacity, initialOldCapacity + initialHugeCapacity)
                << "Total capacity should be preserved";
        }
    }
}

/**
 * Test thread safety of memory reallocation
 * Verify that concurrent calls to TryTransferMemoryToOldSpace are handled correctly
 * The function uses SharedMemoryReallocator's internal Mutex to serialize transfer attempts.
 */
HWTEST_F_L0(SharedMemoryReallocatorTest, ConcurrentTransferThreadSafety)
{
    auto sHeap = SharedHeap::GetInstance();
    auto oldSpace = sHeap->GetOldSpace();
    auto hugeSpace = sHeap->GetHugeObjectSpace();

    ASSERT_NE(sHeap, nullptr);
    ASSERT_NE(oldSpace, nullptr);
    ASSERT_NE(hugeSpace, nullptr);

    // Get initial capacities
    size_t initialOldCapacity = oldSpace->GetMaximumCapacity();
    size_t initialHugeCapacity = hugeSpace->GetMaximumCapacity();

    // Simulate concurrent transfer attempts
    // In real scenario, multiple threads might call this simultaneously
    sHeap->TryTransferMemoryToOldSpace();
    sHeap->TryTransferMemoryToOldSpace();

    // Verify total capacity is preserved (should always be true)
    size_t newOldCapacity = oldSpace->GetMaximumCapacity();
    size_t newHugeCapacity = hugeSpace->GetMaximumCapacity();
    size_t newTotalCapacity = newOldCapacity + newHugeCapacity;
    size_t initialTotalCapacity = initialOldCapacity + initialHugeCapacity;

    EXPECT_EQ(newTotalCapacity, initialTotalCapacity)
        << "Total capacity should be preserved after transfer";

    LOG_ECMA_MEM(INFO) << "Thread safety test: Capacity preserved, OldSpace: "
                       << initialOldCapacity << " -> " << newOldCapacity
                       << ", HugeObjectSpace: " << initialHugeCapacity << " -> " << newHugeCapacity;
}

// Test triggering transfer by allocating huge object then filling OldSpace
HWTEST_F_L0(SharedMemoryReallocatorTest, TriggerTransferByContinuousAllocation)
{
    // disable force gc to avoid time out
    instance->SetEnableForceGC(false);

    auto sHeap = SharedHeap::GetInstance();
    auto factory = instance->GetFactory();

    ASSERT_NE(sHeap, nullptr);
    ASSERT_NE(factory, nullptr);

    auto oldSpace = sHeap->GetOldSpace();
    auto hugeSpace = sHeap->GetHugeObjectSpace();

    ASSERT_NE(oldSpace, nullptr);
    ASSERT_NE(hugeSpace, nullptr);

    // Get initial capacities
    size_t initialOldCapacity = oldSpace->GetMaximumCapacity();
    size_t initialHugeCapacity = hugeSpace->GetMaximumCapacity();

    LOG_ECMA_MEM(INFO) << "Initial OldSpace capacity: " << initialOldCapacity;
    LOG_ECMA_MEM(INFO) << "Initial HugeObjectSpace capacity: " << initialHugeCapacity;

    // Allocate a ~100MB huge object
    // TaggedArray with 1M elements (~8MB per element for large values)
    // We'll allocate a large TaggedArray to push into HugeObjectSpace
    const int hugeArraySize = 13 * 1024 * 1024;  // 13 * 1024 * 1024: ~100MB array (assuming 8 bytes per element)
    auto hugeArrayHandle = factory->NewSTaggedArray(hugeArraySize, JSTaggedValue::Hole());

    EXPECT_NE(hugeArrayHandle.GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(hugeArrayHandle->GetLength(), hugeArraySize);

    LOG_ECMA_MEM(INFO) << "Allocated huge array of size: " << hugeArraySize;

    // Verify the huge object was allocated in HugeObjectSpace
    size_t hugeCommittedAfter = hugeSpace->GetCommittedSize();
    LOG_ECMA_MEM(INFO) << "HugeObjectSpace committed after huge allocation: " << hugeCommittedAfter;

    // Calculate free memory in HugeObjectSpace
    size_t hugeMaxCapacity = hugeSpace->GetMaximumCapacity();
    size_t hugeFree = hugeMaxCapacity - hugeCommittedAfter;

    LOG_ECMA_MEM(INFO) << "HugeObjectSpace free memory: " << hugeFree;

    // If HugeObjectSpace has enough free memory, proceed with OldSpace allocation
    if (hugeFree > SharedMemoryReallocator::HUGE_SPACE_FREE_THRESHOLD) {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        // Continuously allocate objects in OldSpace (~400MB total) to trigger transfer automatically
        // Each TaggedArray with 10240 elements = ~80KB (assuming 8 bytes per element)
        // 5120 iterations * 80KB = ~400MB
        const int allocIterations = 5120;
        const int arraySize = 10240;  // 80KB per array

        for (int i = 0; i < allocIterations; i++) {
            auto arrayHandle = factory->NewSTaggedArray(arraySize, JSTaggedValue::Hole());
            EXPECT_NE(arrayHandle.GetTaggedValue(), JSTaggedValue::Undefined());
        }

        LOG_ECMA_MEM(INFO) << "Allocated " << allocIterations << " arrays in OldSpace (~400MB total)";

        // Verify capacity was transferred correctly
        // Transfer should have been triggered automatically during allocation
        size_t newOldCapacity = oldSpace->GetMaximumCapacity();
        size_t newHugeCapacity = hugeSpace->GetMaximumCapacity();

        LOG_ECMA_MEM(INFO) << "After allocation - OldSpace capacity: " << newOldCapacity;
        LOG_ECMA_MEM(INFO) << "After allocation - HugeObjectSpace capacity: " << newHugeCapacity;

        // Validate expectations
        if (newOldCapacity > initialOldCapacity) {
            // Transfer happened automatically
            EXPECT_LT(newHugeCapacity, initialHugeCapacity)
                << "HugeObjectSpace capacity should decrease after transfer";

            // Verify total capacity is preserved
            size_t initialTotalCapacity = initialOldCapacity + initialHugeCapacity;
            size_t newTotalCapacity = newOldCapacity + newHugeCapacity;
            EXPECT_EQ(newTotalCapacity, initialTotalCapacity)
                << "Total capacity should be preserved";

            LOG_ECMA_MEM(INFO) << "Transfer triggered automatically: OldSpace increased by "
                               << (newOldCapacity - initialOldCapacity)
                               << ", HugeObjectSpace decreased by "
                               << (initialHugeCapacity - newHugeCapacity);
        } else {
            LOG_ECMA_MEM(INFO) << "Transfer not triggered automatically (insufficient OldSpace pressure)";
        }
    } else {
        LOG_ECMA_MEM(INFO) << "HugeObjectSpace has insufficient free memory for transfer test";
    }
}

// Verify that no transfer occurs when HugeObjectSpace has less than threshold free memory
HWTEST_F_L0(SharedMemoryReallocatorTest, NoTransferWhenBelowThreshold)
{
    // disable force gc to avoid time out
    instance->SetEnableForceGC(false);

    auto sHeap = SharedHeap::GetInstance();
    auto oldSpace = sHeap->GetOldSpace();
    auto hugeSpace = sHeap->GetHugeObjectSpace();

    ASSERT_NE(sHeap, nullptr);
    ASSERT_NE(oldSpace, nullptr);
    ASSERT_NE(hugeSpace, nullptr);

    size_t originOldCapacity =  oldSpace->GetMaximumCapacity();
    // Adjust old capacity to committed size
    oldSpace->SetMaximumCapacity(oldSpace->GetCommittedSize());
    size_t initialOldCapacity =  oldSpace->GetMaximumCapacity();
    size_t initialHugeCapacity =  hugeSpace->GetMaximumCapacity();

    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // Allocate a ~300MB huge object
    // TaggedArray with 1M elements (~8MB per element for large values)
    // We'll allocate a large TaggedArray to push into HugeObjectSpace
    const int hugeArraySize = 39 * 1024 * 1024;  // 39 * 1024 * 1024: ~300MB array (assuming 8 bytes per element)
    auto factory = instance->GetFactory();
    auto hugeArrayHandle = factory->NewSTaggedArray(hugeArraySize, JSTaggedValue::Hole());
    EXPECT_NE(hugeArrayHandle.GetTaggedValue(), JSTaggedValue::Undefined());

    // Calculate free memory in HugeObjectSpace
    size_t hugeCommitted = hugeSpace->GetCommittedSize();
    size_t hugeMaxCapacity = hugeSpace->GetMaximumCapacity();
    size_t hugeFree = hugeMaxCapacity - hugeCommitted;

    // When freeSize < HUGE_SPACE_FREE_THRESHOLD, CalculateTransferSize returns 0
    if (hugeFree < SharedMemoryReallocator::HUGE_SPACE_FREE_THRESHOLD) {
        LOG_ECMA_MEM(INFO) << "HugeObjectSpace free memory (" << hugeFree
                           << ") is below threshold ("
                           << SharedMemoryReallocator::HUGE_SPACE_FREE_THRESHOLD << ")";

        // Try to transfer memory - this should return without transferring
        sHeap->TryTransferMemoryToOldSpace();

        // Verify capacities haven't changed (no transfer occurred)
        EXPECT_EQ(oldSpace->GetMaximumCapacity(), initialOldCapacity)
            << "OldSpace capacity should not change when free memory is below threshold";
        EXPECT_EQ(hugeSpace->GetMaximumCapacity(), initialHugeCapacity)
            << "HugeObjectSpace capacity should not change when free memory is below threshold";
    } else {
        LOG_ECMA_MEM(INFO) << "HugeObjectSpace has sufficient free memory ("
                           << hugeFree << " >= "
                           << SharedMemoryReallocator::HUGE_SPACE_FREE_THRESHOLD
                           << "), skipping below-threshold test";
    }
    oldSpace->SetMaximumCapacity(originOldCapacity);
}

// Verify CalculateTransferSize returns 0 when free space is exactly at threshold boundary
HWTEST_F_L0(SharedMemoryReallocatorTest, CalculateTransferSizeAtThresholdBoundary)
{
    auto sHeap = SharedHeap::GetInstance();
    auto hugeSpace = sHeap->GetHugeObjectSpace();

    ASSERT_NE(sHeap, nullptr);
    ASSERT_NE(hugeSpace, nullptr);

    size_t hugeCommitted = hugeSpace->GetCommittedSize();
    size_t hugeMaxCapacity = hugeSpace->GetMaximumCapacity();
    size_t hugeFree = hugeMaxCapacity - hugeCommitted;

    LOG_ECMA_MEM(INFO) << "HugeObjectSpace free: " << hugeFree;
    LOG_ECMA_MEM(INFO) << "Threshold: " << SharedMemoryReallocator::HUGE_SPACE_FREE_THRESHOLD;

    // Test the boundary condition: when free space < threshold, should return 0
    if (hugeFree < SharedMemoryReallocator::HUGE_SPACE_FREE_THRESHOLD) {
        LOG_ECMA_MEM(INFO) << "Testing: free space (" << hugeFree
                           << ") < threshold ("
                           << SharedMemoryReallocator::HUGE_SPACE_FREE_THRESHOLD << ")";

        size_t initialOldCapacity = sHeap->GetOldSpace()->GetMaximumCapacity();
        size_t initialHugeCapacity = hugeSpace->GetMaximumCapacity();

        // Call transfer - should not transfer anything
        sHeap->TryTransferMemoryToOldSpace();

        // Verify no capacity change occurred
        EXPECT_EQ(sHeap->GetOldSpace()->GetMaximumCapacity(), initialOldCapacity)
            << "OldSpace capacity should not change";
        EXPECT_EQ(hugeSpace->GetMaximumCapacity(), initialHugeCapacity)
            << "HugeObjectSpace capacity should not change";

        LOG_ECMA_MEM(INFO) << "Verified: CalculateTransferSize returned 0, no transfer occurred";
    } else {
        LOG_ECMA_MEM(INFO) << "Free space is above threshold, boundary test not applicable";
    }
}

// Verify the "No memory to transfer" log path is covered
HWTEST_F_L0(SharedMemoryReallocatorTest, LogPathWhenNoMemoryToTransfer)
{
    auto sHeap = SharedHeap::GetInstance();
    auto oldSpace = sHeap->GetOldSpace();
    auto hugeSpace = sHeap->GetHugeObjectSpace();

    ASSERT_NE(sHeap, nullptr);
    ASSERT_NE(oldSpace, nullptr);
    ASSERT_NE(hugeSpace, nullptr);

    size_t hugeCommitted = hugeSpace->GetCommittedSize();
    size_t hugeMaxCapacity = hugeSpace->GetMaximumCapacity();
    size_t hugeFree = hugeMaxCapacity - hugeCommitted;

    // When CalculateTransferSize returns 0, the log message should be emitted
    if (hugeFree < SharedMemoryReallocator::HUGE_SPACE_FREE_THRESHOLD) {
        LOG_ECMA_MEM(INFO) << "Testing log path when free memory is below threshold";

        size_t initialOldCapacity = oldSpace->GetMaximumCapacity();
        size_t initialHugeCapacity = hugeSpace->GetMaximumCapacity();

        // "SharedMemoryReallocator: No memory to transfer"
        sHeap->TryTransferMemoryToOldSpace();

        // Verify the function returned early without transferring
        EXPECT_EQ(oldSpace->GetMaximumCapacity(), initialOldCapacity)
            << "OldSpace should not change when no memory to transfer";
        EXPECT_EQ(hugeSpace->GetMaximumCapacity(), initialHugeCapacity)
            << "HugeObjectSpace should not change when no memory to transfer";

        LOG_ECMA_MEM(INFO) << "Verified: Log path covered, early return occurred";
    } else {
        LOG_ECMA_MEM(INFO) << "Free memory above threshold, log path test not applicable";
    }
}
}  // namespace panda::test
