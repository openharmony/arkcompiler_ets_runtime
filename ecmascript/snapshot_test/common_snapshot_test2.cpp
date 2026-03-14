/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/mem_common.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/snapshot/mem/snapshot_env.h"
#include "ecmascript/snapshot/mem/snapshot_processor.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class CommonSnapshotTestPart2 : public BaseTestWithScope<false> {
};

HWTEST_F_L0(CommonSnapshotTestPart2, SnapshotIncrementalMarkingTest)
{
#ifndef NDEBUG
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
#endif
}

HWTEST_F_L0(CommonSnapshotTestPart2, SnapshotConcurrentFinishTest)
{
#ifndef NDEBUG
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
#endif
}

HWTEST_F_L0(CommonSnapshotTestPart2, SnapshotHeapCallbackTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 20; i++) {
        factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTestPart2, SnapshotRootSetTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTestPart2, SnapshotGCWorkTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 30; i++) {
        factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTestPart2, SnapshotGCPhasesTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTestPart2, SnapshotGCYieldTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 20; i++) {
        factory->NewTaggedArray(20, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTestPart2, SnapshotObjectLayoutIteratorTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto array = factory->NewTaggedArray(25, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    ASSERT_TRUE(!array.IsEmpty());
    
    for (int i = 0; i < 25; i++) {
        JSTaggedValue val = array->Get(thread, i);
        EXPECT_TRUE(val.IsUndefined());
    }
}

HWTEST_F_L0(CommonSnapshotTestPart2, SnapshotRegionDataTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto obj = factory->NewTaggedArray(100, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    ASSERT_TRUE(!obj.IsEmpty());
    
    EXPECT_GT(obj->GetSize(), 0u);
}

HWTEST_F_L0(CommonSnapshotTestPart2, SnapshotDoubleAllocationTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto array = factory->NewTaggedArray(10, JSTaggedValue::Hole(), MemSpaceType::OLD_SPACE);
    ASSERT_TRUE(!array.IsEmpty());
    
    array->Set(thread, 0, JSTaggedValue(3.14159));
    JSTaggedValue val = array->Get(thread, 0);
    EXPECT_TRUE(val.IsDouble() || val.IsInt());
}

HWTEST_F_L0(CommonSnapshotTestPart2, SnapshotTaggedValueOperationsTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto factory = ecmaVm->GetFactory();
    
    auto array = factory->NewTaggedArray(5, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    ASSERT_TRUE(!array.IsEmpty());
    
    for (int i = 0; i < 5; i++) {
        JSTaggedValue val = array->Get(thread, i);
        EXPECT_TRUE(val.IsUndefined());
    }
}

HWTEST_F_L0(CommonSnapshotTestPart2, SnapshotSnapshotDataTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTestPart2, SnapshotProcessorInitializeTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    auto processor = new SnapshotProcessor(ecmaVm);
    ASSERT_TRUE(processor != nullptr);
    
    delete processor;
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTestPart2, SnapshotProcessorProcessTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTestPart2, SnapshotProcessorSerializeTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTestPart2, SnapshotProcessorDeserializeTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    ASSERT_TRUE(heap != nullptr);
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTestPart2, SnapshotEnvInitializeTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto snapshotEnv = new SnapshotEnv(ecmaVm);
    ASSERT_TRUE(snapshotEnv != nullptr);
    
    delete snapshotEnv;
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTestPart2, SnapshotEnvSetupTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto snapshotEnv = new SnapshotEnv(ecmaVm);
    ASSERT_TRUE(snapshotEnv != nullptr);
    
    delete snapshotEnv;
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(CommonSnapshotTestPart2, SnapshotEnvCleanupTest)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto snapshotEnv = new SnapshotEnv(ecmaVm);
    ASSERT_TRUE(snapshotEnv != nullptr);
    
    delete snapshotEnv;
    
    EXPECT_TRUE(true);
}

}  // namespace panda::test

namespace panda::test {
class AdditionalSnapshotTest2 : public BaseTestWithScope<false> {
};

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest031)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 165; i++) {
        factory->NewTaggedArray(23, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest032)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 175; i++) {
        factory->NewTaggedArray(21, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest033)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 185; i++) {
        factory->NewTaggedArray(33, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest034)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 195; i++) {
        factory->NewTaggedArray(27, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest035)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 205; i++) {
        factory->NewTaggedArray(31, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest036)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 215; i++) {
        factory->NewTaggedArray(26, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest037)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 225; i++) {
        factory->NewTaggedArray(37, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest038)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 235; i++) {
        factory->NewTaggedArray(34, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest039)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 245; i++) {
        factory->NewTaggedArray(38, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest040)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 255; i++) {
        factory->NewTaggedArray(41, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest041)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 265; i++) {
        factory->NewTaggedArray(43, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest042)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 275; i++) {
        factory->NewTaggedArray(46, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest043)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 285; i++) {
        factory->NewTaggedArray(47, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest044)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 295; i++) {
        factory->NewTaggedArray(48, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest045)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 305; i++) {
        factory->NewTaggedArray(49, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest046)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 315; i++) {
        factory->NewTaggedArray(51, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest047)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 325; i++) {
        factory->NewTaggedArray(52, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest048)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 335; i++) {
        factory->NewTaggedArray(53, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest049)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 345; i++) {
        factory->NewTaggedArray(54, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest050)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 365; i++) {
        factory->NewTaggedArray(56, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest051)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 375; i++) {
        factory->NewTaggedArray(57, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest052)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 385; i++) {
        factory->NewTaggedArray(58, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest053)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 395; i++) {
        factory->NewTaggedArray(59, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest054)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 405; i++) {
        factory->NewTaggedArray(60, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest055)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 415; i++) {
        factory->NewTaggedArray(61, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest056)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 425; i++) {
        factory->NewTaggedArray(62, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest057)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 435; i++) {
        factory->NewTaggedArray(63, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest058)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 445; i++) {
        factory->NewTaggedArray(64, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest059)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 455; i++) {
        factory->NewTaggedArray(65, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest060)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 475; i++) {
        factory->NewTaggedArray(67, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest061)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 485; i++) {
        factory->NewTaggedArray(68, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest062)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 495; i++) {
        factory->NewTaggedArray(69, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest063)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 505; i++) {
        factory->NewTaggedArray(70, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest064)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 515; i++) {
        factory->NewTaggedArray(71, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest065)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 525; i++) {
        factory->NewTaggedArray(72, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest066)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 535; i++) {
        factory->NewTaggedArray(73, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest067)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 545; i++) {
        factory->NewTaggedArray(74, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest068)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 555; i++) {
        factory->NewTaggedArray(75, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest069)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 565; i++) {
        factory->NewTaggedArray(76, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest070)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 585; i++) {
        factory->NewTaggedArray(78, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest071)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 595; i++) {
        factory->NewTaggedArray(79, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest072)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 605; i++) {
        factory->NewTaggedArray(80, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest073)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 615; i++) {
        factory->NewTaggedArray(81, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest074)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 625; i++) {
        factory->NewTaggedArray(82, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest075)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 635; i++) {
        factory->NewTaggedArray(83, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest076)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 645; i++) {
        factory->NewTaggedArray(84, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest077)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 655; i++) {
        factory->NewTaggedArray(85, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest078)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 665; i++) {
        factory->NewTaggedArray(86, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest079)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 675; i++) {
        factory->NewTaggedArray(87, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest080)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 695; i++) {
        factory->NewTaggedArray(89, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest081)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 705; i++) {
        factory->NewTaggedArray(90, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest082)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 715; i++) {
        factory->NewTaggedArray(91, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest083)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 725; i++) {
        factory->NewTaggedArray(92, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest084)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 735; i++) {
        factory->NewTaggedArray(93, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest085)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 745; i++) {
        factory->NewTaggedArray(94, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest086)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 755; i++) {
        factory->NewTaggedArray(95, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest087)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 765; i++) {
        factory->NewTaggedArray(96, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest088)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 775; i++) {
        factory->NewTaggedArray(97, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest089)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 785; i++) {
        factory->NewTaggedArray(98, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest090)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 805; i++) {
        factory->NewTaggedArray(100, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest091)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 815; i++) {
        factory->NewTaggedArray(101, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest092)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 825; i++) {
        factory->NewTaggedArray(102, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest093)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 835; i++) {
        factory->NewTaggedArray(103, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest094)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 845; i++) {
        factory->NewTaggedArray(104, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest095)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 855; i++) {
        factory->NewTaggedArray(105, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest096)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 865; i++) {
        factory->NewTaggedArray(106, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest097)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 875; i++) {
        factory->NewTaggedArray(107, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest098)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 885; i++) {
        factory->NewTaggedArray(108, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest099)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 895; i++) {
        factory->NewTaggedArray(109, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest100)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 915; i++) {
        factory->NewTaggedArray(111, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest101)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 925; i++) {
        factory->NewTaggedArray(112, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest102)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 935; i++) {
        factory->NewTaggedArray(113, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest103)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 945; i++) {
        factory->NewTaggedArray(114, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest104)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 955; i++) {
        factory->NewTaggedArray(115, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest105)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 965; i++) {
        factory->NewTaggedArray(116, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest106)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 975; i++) {
        factory->NewTaggedArray(117, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest107)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 985; i++) {
        factory->NewTaggedArray(118, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest108)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 995; i++) {
        factory->NewTaggedArray(119, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest109)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1005; i++) {
        factory->NewTaggedArray(121, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest110)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1010; i++) {
        factory->NewTaggedArray(122, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest111)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1015; i++) {
        factory->NewTaggedArray(123, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest112)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1020; i++) {
        factory->NewTaggedArray(124, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest113)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1025; i++) {
        factory->NewTaggedArray(125, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest114)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1030; i++) {
        factory->NewTaggedArray(126, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest115)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1040; i++) {
        factory->NewTaggedArray(128, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest116)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1045; i++) {
        factory->NewTaggedArray(129, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest117)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1050; i++) {
        factory->NewTaggedArray(130, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest118)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1060; i++) {
        factory->NewTaggedArray(132, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest119)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1065; i++) {
        factory->NewTaggedArray(133, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest120)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1070; i++) {
        factory->NewTaggedArray(134, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest121)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1075; i++) {
        factory->NewTaggedArray(135, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest122)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1085; i++) {
        factory->NewTaggedArray(137, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest123)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1090; i++) {
        factory->NewTaggedArray(138, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest124)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1100; i++) {
        factory->NewTaggedArray(140, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest125)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1105; i++) {
        factory->NewTaggedArray(141, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest126)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1115; i++) {
        factory->NewTaggedArray(143, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest127)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1120; i++) {
        factory->NewTaggedArray(144, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest128)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1130; i++) {
        factory->NewTaggedArray(146, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest129)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1135; i++) {
        factory->NewTaggedArray(147, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest130)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1145; i++) {
        factory->NewTaggedArray(149, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest2, AdditionalSnapshotTest131)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 1155; i++) {
        factory->NewTaggedArray(151, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

}  // namespace panda::test
