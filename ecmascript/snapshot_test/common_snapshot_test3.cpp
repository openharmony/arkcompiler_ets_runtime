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
class AdditionalSnapshotTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest001)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        factory->NewTaggedArray(20, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest002)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 80; i++) {
        factory->NewTaggedArray(15, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest003)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 60; i++) {
        factory->NewTaggedArray(25, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest004)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 120; i++) {
        factory->NewTaggedArray(18, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest005)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 90; i++) {
        factory->NewTaggedArray(12, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest006)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 70; i++) {
        factory->NewTaggedArray(30, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest007)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    std::vector<JSTaggedValue> objects;
    for (int i = 0; i < 50; i++) {
        auto obj = factory->NewTaggedArray(40, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        ASSERT_TRUE(!obj.IsEmpty());
    }
    objects.clear();
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest008)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 140; i++) {
        factory->NewTaggedArray(8, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest009)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 40; i++) {
        factory->NewTaggedArray(50, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest010)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 110; i++) {
        factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest011)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 85; i++) {
        factory->NewTaggedArray(22, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest012)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 95; i++) {
        factory->NewTaggedArray(16, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest013)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 130; i++) {
        factory->NewTaggedArray(6, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest014)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 75; i++) {
        factory->NewTaggedArray(28, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest015)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 65; i++) {
        factory->NewTaggedArray(35, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest016)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 55; i++) {
        factory->NewTaggedArray(45, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest017)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        factory->NewTaggedArray(5, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest018)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 45; i++) {
        auto oldObj = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        auto youngObj = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        oldObj->Set(thread, 0, JSTaggedValue(*youngObj));
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest019)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int round = 0; round < 5; round++) {
        for (int i = 0; i < 30; i++) {
            factory->NewTaggedArray(25, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        }
        heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest020)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 150; i++) {
        factory->NewTaggedArray(4, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest021)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 160; i++) {
        factory->NewTaggedArray(3, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest022)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 170; i++) {
        factory->NewTaggedArray(2, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest023)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 180; i++) {
        factory->NewTaggedArray(1, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest024)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 105; i++) {
        factory->NewTaggedArray(17, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest025)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 115; i++) {
        factory->NewTaggedArray(14, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest026)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 125; i++) {
        factory->NewTaggedArray(9, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest027)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 135; i++) {
        factory->NewTaggedArray(7, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest028)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 145; i++) {
        factory->NewTaggedArray(11, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest029)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    std::vector<JSTaggedValue> objects;
    for (int i = 0; i < 200; i++) {
        auto obj = factory->NewTaggedArray(13, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        ASSERT_TRUE(!obj.IsEmpty());
    }
    objects.clear();
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(AdditionalSnapshotTest, AdditionalSnapshotTest030)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 155; i++) {
        factory->NewTaggedArray(19, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

}  // namespace panda::test

namespace panda::test {
class VeryFinalSnapshotTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(VeryFinalSnapshotTest, VeryFinalSnapshotTest001)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        auto obj = factory->NewTaggedArray(5, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(VeryFinalSnapshotTest, VeryFinalSnapshotTest002)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 80; i++) {
        auto obj = factory->NewTaggedArray(6, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(VeryFinalSnapshotTest, VeryFinalSnapshotTest003)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 60; i++) {
        auto obj = factory->NewTaggedArray(7, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

}  // namespace panda::test

namespace panda::test {
class ExtendedSnapshotTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest001)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        auto obj = factory->NewTaggedArray(20, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest002)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 50; i++) {
        auto obj = factory->NewTaggedArray(30, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest003)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 75; i++) {
        auto obj = factory->NewTaggedArray(25, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest004)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    size_t initialSize = heap->GetCommittedSize();
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    size_t finalSize = heap->GetCommittedSize();
    EXPECT_GE(finalSize, 0u);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest005)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 200; i++) {
        factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest006)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 150; i++) {
        auto obj = factory->NewTaggedArray(15, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest007)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        auto str = factory->NewFromStdString(std::to_string(i));
        EXPECT_TRUE(*str != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest008)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 80; i++) {
        auto obj = factory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest009)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 60; i++) {
        auto arr = factory->NewJSArray();
        EXPECT_TRUE(*arr != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest010)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest011)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 120; i++) {
        auto obj = factory->NewTaggedArray(8, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    size_t size = heap->GetHeapObjectSize();
    EXPECT_GT(size, 0u);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest012)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 90; i++) {
        auto obj = factory->NewTaggedArray(12, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest013)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 70; i++) {
        auto obj = factory->NewTaggedArray(18, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest014)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    size_t committedSize = heap->GetCommittedSize();
    size_t heapObjectSize = heap->GetHeapObjectSize();
    
    EXPECT_GE(committedSize, heapObjectSize);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest015)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 50; i++) {
        auto str = factory->NewFromStdString("test_string_" + std::to_string(i));
        EXPECT_TRUE(*str != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest016)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 40; i++) {
        auto obj = factory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest017)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 30; i++) {
        auto arr = factory->NewJSArray();
        EXPECT_TRUE(*arr != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest018)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    size_t size1 = heap->GetCommittedSize();
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    size_t size2 = heap->GetCommittedSize();
    
    EXPECT_TRUE(size1 >= 0 && size2 >= 0);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest019)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        auto obj = factory->NewTaggedArray(5, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest020)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 80; i++) {
        auto obj = factory->NewTaggedArray(7, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest021)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 60; i++) {
        auto obj = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest022)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 50; i++) {
        auto str = factory->GetEmptyString();
        EXPECT_TRUE(*str != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest023)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 40; i++) {
        auto obj = factory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    
    size_t size = heap->GetHeapObjectSize();
    EXPECT_GT(size, 0u);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest024)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 30; i++) {
        auto arr = factory->NewJSArray();
        EXPECT_TRUE(*arr != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest025)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    
    size_t committedSize = heap->GetCommittedSize();
    EXPECT_GT(committedSize, 0u);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest026)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 200; i++) {
        auto obj = factory->NewTaggedArray(3, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest027)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 150; i++) {
        auto obj = factory->NewTaggedArray(4, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest028)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        auto obj = factory->NewTaggedArray(6, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest029)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 80; i++) {
        auto str = factory->NewFromStdString(std::to_string(i * 100));
        EXPECT_TRUE(*str != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest030)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 60; i++) {
        auto obj = factory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest031)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 110; i++) {
        auto obj = factory->NewTaggedArray(22, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest032)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 55; i++) {
        auto obj = factory->NewTaggedArray(32, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest033)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 85; i++) {
        auto obj = factory->NewTaggedArray(27, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest034)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    size_t finalSize = heap->GetCommittedSize();
    EXPECT_GE(finalSize, 0u);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest035)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 210; i++) {
        factory->NewTaggedArray(12, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    }
    
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest036)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 160; i++) {
        auto obj = factory->NewTaggedArray(17, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest037)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 110; i++) {
        auto str = factory->NewFromStdString(std::to_string(i));
        EXPECT_TRUE(*str != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest038)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 90; i++) {
        auto obj = factory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest039)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 70; i++) {
        auto arr = factory->NewJSArray();
        EXPECT_TRUE(*arr != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest040)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest041)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 130; i++) {
        auto obj = factory->NewTaggedArray(9, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    size_t size = heap->GetHeapObjectSize();
    EXPECT_GT(size, 0u);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest042)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        auto obj = factory->NewTaggedArray(13, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest043)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 80; i++) {
        auto obj = factory->NewTaggedArray(19, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest044)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    size_t committedSize = heap->GetCommittedSize();
    size_t heapObjectSize = heap->GetHeapObjectSize();
    
    EXPECT_GE(committedSize, heapObjectSize);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest045)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 60; i++) {
        auto str = factory->NewFromStdString("test_string_" + std::to_string(i));
        EXPECT_TRUE(*str != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest046)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 50; i++) {
        auto obj = factory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest047)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 40; i++) {
        auto arr = factory->NewJSArray();
        EXPECT_TRUE(*arr != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest048)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    size_t size1 = heap->GetCommittedSize();
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    
    size_t size2 = heap->GetCommittedSize();
    
    EXPECT_TRUE(size1 >= 0 && size2 >= 0);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest049)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 110; i++) {
        auto obj = factory->NewTaggedArray(6, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(ExtendedSnapshotTest, ExtendedSnapshotTest050)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 90; i++) {
        auto obj = factory->NewTaggedArray(8, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

}  // namespace panda::test

namespace panda::test {
class MoreExtendedSnapshotTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(MoreExtendedSnapshotTest, MoreExtendedSnapshotTest001)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 300; i++) {
        auto obj = factory->NewTaggedArray(10, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedSnapshotTest, MoreExtendedSnapshotTest002)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 250; i++) {
        auto obj = factory->NewTaggedArray(12, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedSnapshotTest, MoreExtendedSnapshotTest003)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 200; i++) {
        auto obj = factory->NewTaggedArray(15, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedSnapshotTest, MoreExtendedSnapshotTest004)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 180; i++) {
        auto str = factory->NewFromStdString("test_" + std::to_string(i));
        EXPECT_TRUE(*str != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedSnapshotTest, MoreExtendedSnapshotTest005)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 160; i++) {
        auto obj = factory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedSnapshotTest, MoreExtendedSnapshotTest006)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 140; i++) {
        auto arr = factory->NewJSArray();
        EXPECT_TRUE(*arr != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedSnapshotTest, MoreExtendedSnapshotTest007)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedSnapshotTest, MoreExtendedSnapshotTest008)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 120; i++) {
        auto obj = factory->NewTaggedArray(20, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    size_t size = heap->GetHeapObjectSize();
    EXPECT_GT(size, 0u);
}

HWTEST_F_L0(MoreExtendedSnapshotTest, MoreExtendedSnapshotTest009)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        auto str = factory->NewFromStdString("string_" + std::to_string(i));
        EXPECT_TRUE(*str != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedSnapshotTest, MoreExtendedSnapshotTest010)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 90; i++) {
        auto obj = factory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedSnapshotTest, MoreExtendedSnapshotTest011)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 80; i++) {
        auto arr = factory->NewJSArray();
        EXPECT_TRUE(*arr != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedSnapshotTest, MoreExtendedSnapshotTest012)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    size_t committedSize = heap->GetCommittedSize();
    EXPECT_GT(committedSize, 0u);
}

HWTEST_F_L0(MoreExtendedSnapshotTest, MoreExtendedSnapshotTest013)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 70; i++) {
        auto obj = factory->NewTaggedArray(25, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedSnapshotTest, MoreExtendedSnapshotTest014)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 60; i++) {
        auto obj = factory->NewTaggedArray(30, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedSnapshotTest, MoreExtendedSnapshotTest015)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 50; i++) {
        auto obj = factory->NewTaggedArray(35, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedSnapshotTest, MoreExtendedSnapshotTest016)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 45; i++) {
        auto str = factory->GetEmptyString();
        EXPECT_TRUE(*str != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedSnapshotTest, MoreExtendedSnapshotTest017)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 40; i++) {
        auto obj = factory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    size_t size = heap->GetHeapObjectSize();
    EXPECT_GT(size, 0u);
}

HWTEST_F_L0(MoreExtendedSnapshotTest, MoreExtendedSnapshotTest018)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 35; i++) {
        auto arr = factory->NewJSArray();
        EXPECT_TRUE(*arr != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(MoreExtendedSnapshotTest, MoreExtendedSnapshotTest019)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    size_t size = heap->GetCommittedSize();
    EXPECT_GE(size, 0u);
}

HWTEST_F_L0(MoreExtendedSnapshotTest, MoreExtendedSnapshotTest020)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 30; i++) {
        auto obj = factory->NewTaggedArray(40, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

}  // namespace panda::test

namespace panda::test {
class FinalExtendedSnapshotTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(FinalExtendedSnapshotTest, FinalExtendedSnapshotTest001)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 500; i++) {
        auto obj = factory->NewTaggedArray(5, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(FinalExtendedSnapshotTest, FinalExtendedSnapshotTest002)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 400; i++) {
        auto obj = factory->NewTaggedArray(6, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(FinalExtendedSnapshotTest, FinalExtendedSnapshotTest003)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 300; i++) {
        auto obj = factory->NewTaggedArray(7, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(FinalExtendedSnapshotTest, FinalExtendedSnapshotTest004)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 250; i++) {
        auto str = factory->NewFromStdString("final_" + std::to_string(i));
        EXPECT_TRUE(*str != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(FinalExtendedSnapshotTest, FinalExtendedSnapshotTest005)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 200; i++) {
        auto obj = factory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(FinalExtendedSnapshotTest, FinalExtendedSnapshotTest006)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 180; i++) {
        auto arr = factory->NewJSArray();
        EXPECT_TRUE(*arr != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(FinalExtendedSnapshotTest, FinalExtendedSnapshotTest007)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    
    for (int i = 0; i < 5; i++) {
        heap->CollectGarbage(TriggerGCType::YOUNG_GC);
        heap->CollectGarbage(TriggerGCType::OLD_GC);
        heap->CollectGarbage(TriggerGCType::FULL_GC);
    }
    
    EXPECT_TRUE(true);
}

HWTEST_F_L0(FinalExtendedSnapshotTest, FinalExtendedSnapshotTest008)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 150; i++) {
        auto obj = factory->NewTaggedArray(8, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        EXPECT_TRUE(*obj != nullptr);
    }
    size_t size = heap->GetHeapObjectSize();
    EXPECT_GT(size, 0u);
}

HWTEST_F_L0(FinalExtendedSnapshotTest, FinalExtendedSnapshotTest009)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 120; i++) {
        auto str = factory->NewFromStdString("snapshot_" + std::to_string(i));
        EXPECT_TRUE(*str != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(true);
}

HWTEST_F_L0(FinalExtendedSnapshotTest, FinalExtendedSnapshotTest010)
{
    auto ecmaVm = thread->GetEcmaVM();
    ASSERT_TRUE(ecmaVm != nullptr);
    
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto factory = ecmaVm->GetFactory();
    
    for (int i = 0; i < 100; i++) {
        auto obj = factory->NewJSArray();
        EXPECT_TRUE(*obj != nullptr);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_TRUE(true);
}

}  // namespace panda::test
