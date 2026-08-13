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

#include <algorithm>

#include "ecmascript/ecma_vm.h"
#include "ecmascript/tests/test_helper.h"
#include "gtest/gtest.h"

using namespace panda::ecmascript;

namespace panda::test {
namespace {
// Mirrors the private EcmaVM constants in ecmascript/ecma_vm.h.
constexpr int32_t MIN_HANDLE_STORAGE_SIZE = 2;
constexpr int32_t MIN_PRIMITIVE_STORAGE_SIZE = 2;

// Shrink retains SHRINK_HEADROOM_FACTOR * active usage nodes (>= MIN).
constexpr int32_t SHRINK_HEADROOM_FACTOR = 2;

// Reset helper grows MIN + RESET_GROW_MARGIN nodes so the shrink collapses to MIN.
// Must be large enough that toDelete >= the production fixed floor (SHRINK_MIN_FREE_NODES).
constexpr int32_t RESET_GROW_MARGIN = 4;
constexpr size_t RESET_BASELINE_NODES = static_cast<size_t>(MIN_HANDLE_STORAGE_SIZE);

// prevIndex values driving each scenario.
constexpr int32_t PREV_INDEX_LOW = 1;                // usage 2 -> target 4
constexpr int32_t PREV_INDEX_PROPORTIONAL = 4;       // usage 5 -> target 10
constexpr int32_t PREV_INDEX_HEADROOM_EXCEEDS = 9;   // usage 10 -> target 20 == total
constexpr int32_t PREV_INDEX_HYSTERESIS_BAND = 7;     // usage 8 -> target 16, frees 4 < total/4=5

// Growth targets (final node counts) per scenario.
constexpr size_t GROW_NODES_SMALL = 10;
constexpr size_t GROW_NODES_LARGE = 30;
constexpr size_t GROW_NODES_NO_SHRINK = 20;            // headroom * (prev+1) == total
constexpr size_t GROW_NODES_HYSTERESIS_BELOW = 13;     // prev=4: frees 3 < max(3,4)=4 -> no shrink
constexpr size_t GROW_NODES_HYSTERESIS_AT = 14;        // prev=4: frees 4 >= max(3,4)=4 -> shrink

// Expected retained node count after a proportional shrink to 2x active usage.
inline size_t ShrinkTargetNodes(int32_t prevIndex, int32_t minNodes)
{
    return static_cast<size_t>(std::max(minNodes, (prevIndex + 1) * SHRINK_HEADROOM_FACTOR));
}

void GrowHandleStorageTo(EcmaVM *vm, size_t target)
{
    while (vm->GetHandleStorageNodesSize() < target) {
        vm->ExpandHandleStorage();
    }
}

void GrowPrimitiveStorageTo(EcmaVM *vm, size_t target)
{
    while (vm->GetPrimitiveStorageNodesSize() < target) {
        vm->ExpandPrimitiveStorage();
    }
}

// Collapse to a deterministic baseline of MIN nodes (index 0) regardless of
// the non-deterministic node count left by VM initialization.
void ResetHandleStorageToMin(EcmaVM *vm)
{
    GrowHandleStorageTo(vm, static_cast<size_t>(MIN_HANDLE_STORAGE_SIZE + RESET_GROW_MARGIN));
    vm->ShrinkHandleStorage(0);
}

void ResetPrimitiveStorageToMin(EcmaVM *vm)
{
    GrowPrimitiveStorageTo(vm, static_cast<size_t>(MIN_PRIMITIVE_STORAGE_SIZE + RESET_GROW_MARGIN));
    vm->ShrinkPrimitiveStorage(0);
}
}  // namespace

class EcmaVMStorageShrinkTest : public BaseTestWithScope<false> {
};

// --- Handle storage ---

HWTEST_F_L0(EcmaVMStorageShrinkTest, HandleStorage_LowUsageShrinkToHeadroom)
{
    // Low usage still shrinks, but to 2x usage (floored at MIN), not to MIN+1.
    ResetHandleStorageToMin(instance);
    GrowHandleStorageTo(instance, GROW_NODES_SMALL);
    ASSERT_EQ(instance->GetHandleStorageNodesSize(), GROW_NODES_SMALL);

    instance->ShrinkHandleStorage(PREV_INDEX_LOW);

    EXPECT_EQ(instance->GetCurrentHandleStorageIndex(), PREV_INDEX_LOW);
    EXPECT_EQ(instance->GetHandleStorageNodesSize(), ShrinkTargetNodes(PREV_INDEX_LOW, MIN_HANDLE_STORAGE_SIZE));
    EXPECT_LT(instance->GetCurrentHandleStorageIndex(),
              static_cast<int32_t>(instance->GetHandleStorageNodesSize()));
}

HWTEST_F_L0(EcmaVMStorageShrinkTest, HandleStorage_ProportionalShrink)
{
    // usage <= 3/8 of total -> frees >= total/4 -> shrink to 2x usage.
    ResetHandleStorageToMin(instance);
    GrowHandleStorageTo(instance, GROW_NODES_LARGE);
    ASSERT_EQ(instance->GetHandleStorageNodesSize(), GROW_NODES_LARGE);

    instance->ShrinkHandleStorage(PREV_INDEX_PROPORTIONAL);

    EXPECT_EQ(instance->GetCurrentHandleStorageIndex(), PREV_INDEX_PROPORTIONAL);
    EXPECT_EQ(instance->GetHandleStorageNodesSize(),
              ShrinkTargetNodes(PREV_INDEX_PROPORTIONAL, MIN_HANDLE_STORAGE_SIZE));
    EXPECT_LT(instance->GetCurrentHandleStorageIndex(),
              static_cast<int32_t>(instance->GetHandleStorageNodesSize()));
}

HWTEST_F_L0(EcmaVMStorageShrinkTest, HandleStorage_NoShrinkWhenHeadroomExceedsTotal)
{
    // 2x usage == total -> nothing to free.
    ResetHandleStorageToMin(instance);
    GrowHandleStorageTo(instance, GROW_NODES_NO_SHRINK);
    ASSERT_EQ(instance->GetHandleStorageNodesSize(), GROW_NODES_NO_SHRINK);

    instance->ShrinkHandleStorage(PREV_INDEX_HEADROOM_EXCEEDS);

    EXPECT_EQ(instance->GetCurrentHandleStorageIndex(), PREV_INDEX_HEADROOM_EXCEEDS);
    EXPECT_EQ(instance->GetHandleStorageNodesSize(), GROW_NODES_NO_SHRINK);
    EXPECT_LT(instance->GetCurrentHandleStorageIndex(),
              static_cast<int32_t>(instance->GetHandleStorageNodesSize()));
}

HWTEST_F_L0(EcmaVMStorageShrinkTest, HandleStorage_NoShrinkBelowHysteresis)
{
    // Frees a positive but < total/4 amount -> hysteresis keeps it to avoid churn.
    ResetHandleStorageToMin(instance);
    GrowHandleStorageTo(instance, GROW_NODES_NO_SHRINK);
    ASSERT_EQ(instance->GetHandleStorageNodesSize(), GROW_NODES_NO_SHRINK);

    instance->ShrinkHandleStorage(PREV_INDEX_HYSTERESIS_BAND);

    EXPECT_EQ(instance->GetCurrentHandleStorageIndex(), PREV_INDEX_HYSTERESIS_BAND);
    EXPECT_EQ(instance->GetHandleStorageNodesSize(), GROW_NODES_NO_SHRINK);
    EXPECT_LT(instance->GetCurrentHandleStorageIndex(),
              static_cast<int32_t>(instance->GetHandleStorageNodesSize()));
}

HWTEST_F_L0(EcmaVMStorageShrinkTest, HandleStorage_BelowMinGuardNoShrink)
{
    // lastIndex <= MIN -> outer guard (lastIndex > MIN) is false, no shrink.
    ResetHandleStorageToMin(instance);
    ASSERT_EQ(instance->GetHandleStorageNodesSize(), RESET_BASELINE_NODES);

    instance->ShrinkHandleStorage(0);

    EXPECT_EQ(instance->GetCurrentHandleStorageIndex(), 0);
    EXPECT_EQ(instance->GetHandleStorageNodesSize(), RESET_BASELINE_NODES);
}

HWTEST_F_L0(EcmaVMStorageShrinkTest, HandleStorage_HysteresisBoundary)
{
    // prev=4 -> target 10. total=13: frees 3 < max(3,4)=4 -> no shrink.
    ResetHandleStorageToMin(instance);
    GrowHandleStorageTo(instance, GROW_NODES_HYSTERESIS_BELOW);
    ASSERT_EQ(instance->GetHandleStorageNodesSize(), GROW_NODES_HYSTERESIS_BELOW);
    instance->ShrinkHandleStorage(PREV_INDEX_PROPORTIONAL);
    EXPECT_EQ(instance->GetHandleStorageNodesSize(), GROW_NODES_HYSTERESIS_BELOW);
    EXPECT_EQ(instance->GetCurrentHandleStorageIndex(), PREV_INDEX_PROPORTIONAL);

    // total=14: frees 4 >= max(3,4)=4 -> shrink to 2x usage.
    GrowHandleStorageTo(instance, GROW_NODES_HYSTERESIS_AT);
    ASSERT_EQ(instance->GetHandleStorageNodesSize(), GROW_NODES_HYSTERESIS_AT);
    instance->ShrinkHandleStorage(PREV_INDEX_PROPORTIONAL);
    EXPECT_EQ(instance->GetHandleStorageNodesSize(),
              ShrinkTargetNodes(PREV_INDEX_PROPORTIONAL, MIN_HANDLE_STORAGE_SIZE));
    EXPECT_EQ(instance->GetCurrentHandleStorageIndex(), PREV_INDEX_PROPORTIONAL);
    EXPECT_LT(instance->GetCurrentHandleStorageIndex(),
              static_cast<int32_t>(instance->GetHandleStorageNodesSize()));
}

HWTEST_F_L0(EcmaVMStorageShrinkTest, HandleStorage_IndependentOfPrimitiveIndex)
{
    // Handle shrink must use the handle index, never the primitive index.
    ResetHandleStorageToMin(instance);
    ResetPrimitiveStorageToMin(instance);
    // Inflate primitive storage so currentPrimitiveStorageIndex_ is large and unrelated.
    GrowPrimitiveStorageTo(instance, GROW_NODES_LARGE);
    GrowHandleStorageTo(instance, GROW_NODES_LARGE);
    ASSERT_EQ(instance->GetHandleStorageNodesSize(), GROW_NODES_LARGE);

    instance->ShrinkHandleStorage(PREV_INDEX_PROPORTIONAL);

    EXPECT_EQ(instance->GetHandleStorageNodesSize(),
              ShrinkTargetNodes(PREV_INDEX_PROPORTIONAL, MIN_HANDLE_STORAGE_SIZE));
    EXPECT_EQ(instance->GetCurrentHandleStorageIndex(), PREV_INDEX_PROPORTIONAL);
}

// --- Primitive storage ---

HWTEST_F_L0(EcmaVMStorageShrinkTest, PrimitiveStorage_LowUsageShrinkToHeadroom)
{
    ResetPrimitiveStorageToMin(instance);
    GrowPrimitiveStorageTo(instance, GROW_NODES_SMALL);
    ASSERT_EQ(instance->GetPrimitiveStorageNodesSize(), GROW_NODES_SMALL);

    instance->ShrinkPrimitiveStorage(PREV_INDEX_LOW);

    EXPECT_EQ(instance->GetCurrentPrimitiveStorageIndex(), PREV_INDEX_LOW);
    EXPECT_EQ(instance->GetPrimitiveStorageNodesSize(), ShrinkTargetNodes(PREV_INDEX_LOW, MIN_PRIMITIVE_STORAGE_SIZE));
    EXPECT_LT(instance->GetCurrentPrimitiveStorageIndex(),
              static_cast<int32_t>(instance->GetPrimitiveStorageNodesSize()));
}

HWTEST_F_L0(EcmaVMStorageShrinkTest, PrimitiveStorage_ProportionalShrink)
{
    ResetPrimitiveStorageToMin(instance);
    GrowPrimitiveStorageTo(instance, GROW_NODES_LARGE);
    ASSERT_EQ(instance->GetPrimitiveStorageNodesSize(), GROW_NODES_LARGE);

    instance->ShrinkPrimitiveStorage(PREV_INDEX_PROPORTIONAL);

    EXPECT_EQ(instance->GetCurrentPrimitiveStorageIndex(), PREV_INDEX_PROPORTIONAL);
    EXPECT_EQ(instance->GetPrimitiveStorageNodesSize(),
              ShrinkTargetNodes(PREV_INDEX_PROPORTIONAL, MIN_PRIMITIVE_STORAGE_SIZE));
    EXPECT_LT(instance->GetCurrentPrimitiveStorageIndex(),
              static_cast<int32_t>(instance->GetPrimitiveStorageNodesSize()));
}

HWTEST_F_L0(EcmaVMStorageShrinkTest, PrimitiveStorage_NoShrinkWhenHeadroomExceedsTotal)
{
    ResetPrimitiveStorageToMin(instance);
    GrowPrimitiveStorageTo(instance, GROW_NODES_NO_SHRINK);
    ASSERT_EQ(instance->GetPrimitiveStorageNodesSize(), GROW_NODES_NO_SHRINK);

    instance->ShrinkPrimitiveStorage(PREV_INDEX_HEADROOM_EXCEEDS);

    EXPECT_EQ(instance->GetCurrentPrimitiveStorageIndex(), PREV_INDEX_HEADROOM_EXCEEDS);
    EXPECT_EQ(instance->GetPrimitiveStorageNodesSize(), GROW_NODES_NO_SHRINK);
    EXPECT_LT(instance->GetCurrentPrimitiveStorageIndex(),
              static_cast<int32_t>(instance->GetPrimitiveStorageNodesSize()));
}

HWTEST_F_L0(EcmaVMStorageShrinkTest, PrimitiveStorage_NoShrinkBelowHysteresis)
{
    ResetPrimitiveStorageToMin(instance);
    GrowPrimitiveStorageTo(instance, GROW_NODES_NO_SHRINK);
    ASSERT_EQ(instance->GetPrimitiveStorageNodesSize(), GROW_NODES_NO_SHRINK);

    instance->ShrinkPrimitiveStorage(PREV_INDEX_HYSTERESIS_BAND);

    EXPECT_EQ(instance->GetCurrentPrimitiveStorageIndex(), PREV_INDEX_HYSTERESIS_BAND);
    EXPECT_EQ(instance->GetPrimitiveStorageNodesSize(), GROW_NODES_NO_SHRINK);
    EXPECT_LT(instance->GetCurrentPrimitiveStorageIndex(),
              static_cast<int32_t>(instance->GetPrimitiveStorageNodesSize()));
}

HWTEST_F_L0(EcmaVMStorageShrinkTest, PrimitiveStorage_BelowMinGuardNoShrink)
{
    // lastIndex <= MIN -> outer guard (lastIndex > MIN) is false, no shrink.
    ResetPrimitiveStorageToMin(instance);
    ASSERT_EQ(instance->GetPrimitiveStorageNodesSize(),
              static_cast<size_t>(MIN_PRIMITIVE_STORAGE_SIZE));

    instance->ShrinkPrimitiveStorage(0);

    EXPECT_EQ(instance->GetCurrentPrimitiveStorageIndex(), 0);
    EXPECT_EQ(instance->GetPrimitiveStorageNodesSize(),
              static_cast<size_t>(MIN_PRIMITIVE_STORAGE_SIZE));
}

HWTEST_F_L0(EcmaVMStorageShrinkTest, PrimitiveStorage_HysteresisBoundary)
{
    ResetPrimitiveStorageToMin(instance);
    GrowPrimitiveStorageTo(instance, GROW_NODES_HYSTERESIS_BELOW);
    ASSERT_EQ(instance->GetPrimitiveStorageNodesSize(), GROW_NODES_HYSTERESIS_BELOW);
    instance->ShrinkPrimitiveStorage(PREV_INDEX_PROPORTIONAL);
    EXPECT_EQ(instance->GetPrimitiveStorageNodesSize(), GROW_NODES_HYSTERESIS_BELOW);
    EXPECT_EQ(instance->GetCurrentPrimitiveStorageIndex(), PREV_INDEX_PROPORTIONAL);

    GrowPrimitiveStorageTo(instance, GROW_NODES_HYSTERESIS_AT);
    ASSERT_EQ(instance->GetPrimitiveStorageNodesSize(), GROW_NODES_HYSTERESIS_AT);
    instance->ShrinkPrimitiveStorage(PREV_INDEX_PROPORTIONAL);
    EXPECT_EQ(instance->GetPrimitiveStorageNodesSize(),
              ShrinkTargetNodes(PREV_INDEX_PROPORTIONAL, MIN_PRIMITIVE_STORAGE_SIZE));
    EXPECT_EQ(instance->GetCurrentPrimitiveStorageIndex(), PREV_INDEX_PROPORTIONAL);
    EXPECT_LT(instance->GetCurrentPrimitiveStorageIndex(),
              static_cast<int32_t>(instance->GetPrimitiveStorageNodesSize()));
}

// P0 regression: ShrinkPrimitiveStorage must key off currentPrimitiveStorageIndex_,
// not currentHandleStorageIndex_. The buggy version computed targetNodes from the
// handle index; with a small handle index it deleted primitive nodes still in use
// (use-after-free), and with a large handle index it skipped shrinking. In both
// cases the resulting node count differs from 2x the primitive usage.
HWTEST_F_L0(EcmaVMStorageShrinkTest, PrimitiveStorage_UsesPrimitiveIndexNotHandleIndex)
{
    // Pin handle storage to index 0 so the buggy code path (which read the handle
    // index) would compute a minimal target and delete the active primitive node.
    ResetHandleStorageToMin(instance);
    ResetPrimitiveStorageToMin(instance);
    GrowPrimitiveStorageTo(instance, GROW_NODES_LARGE);
    ASSERT_EQ(instance->GetPrimitiveStorageNodesSize(), GROW_NODES_LARGE);
    ASSERT_EQ(instance->GetCurrentHandleStorageIndex(), 0);

    instance->ShrinkPrimitiveStorage(PREV_INDEX_PROPORTIONAL);

    // Fixed: target = headroom * (prevIndex + 1).
    // Buggy (handle index 0): target collapses to MIN and the active primitive
    // node dangles; the node count differs from the proportional target.
    EXPECT_EQ(instance->GetPrimitiveStorageNodesSize(),
              ShrinkTargetNodes(PREV_INDEX_PROPORTIONAL, MIN_PRIMITIVE_STORAGE_SIZE));
    EXPECT_EQ(instance->GetCurrentPrimitiveStorageIndex(), PREV_INDEX_PROPORTIONAL);
    // Active primitive index must stay within the allocated node range.
    EXPECT_LT(instance->GetCurrentPrimitiveStorageIndex(),
              static_cast<int32_t>(instance->GetPrimitiveStorageNodesSize()));
}

}  // namespace panda::test
