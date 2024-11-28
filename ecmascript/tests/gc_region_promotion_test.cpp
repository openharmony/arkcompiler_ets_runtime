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

#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class GCTest : public BaseTestWithScope<false> {
};

class NewToOldPromotionCase {
public:
    explicit NewToOldPromotionCase(Heap *heap) : heap_(heap) {}

    void Initialize();

    Heap *heap_ {nullptr};
    Region *regionNewToNew_ {nullptr};
    Region *regionNewToOld_ {nullptr};
    uintptr_t weakGlobal_ {0};
    JSHandle<TaggedArray> newToNewArray_;
    JSHandle<TaggedArray> newToOldArray_;
    JSHandle<TaggedArray> newArray_;
    JSHandle<TaggedArray> oldArray_;
};

void NewToOldPromotionCase::Initialize()
{
    // Disallow garbage collection
    heap_->SetOnSerializeEvent(true);
    JSThread *thread = heap_->GetJSThread();
    ObjectFactory *factory = heap_->GetEcmaVM()->GetFactory();
    SemiSpace *newSpace = heap_->GetNewSpace();
    // newToNew region
    regionNewToNew_ = newSpace->GetCurrentRegion();
    regionNewToNew_->ClearGCFlag(RegionGCFlags::HAS_AGE_MARK);
    constexpr size_t arraySize = 125 * 1024;
    constexpr size_t arrayLength = arraySize / sizeof(JSTaggedType);
    newToNewArray_ = factory->NewTaggedArray(arrayLength, JSTaggedValue::Undefined(), false);
    factory->NewTaggedArray(arrayLength, JSTaggedValue::Undefined(), false);
    // newToOld region
    newToOldArray_ = factory->NewTaggedArray(arrayLength, JSTaggedValue::Undefined(), false);
    factory->NewTaggedArray(arrayLength, JSTaggedValue::Undefined(), false);
    JSTaggedValue weakDeadObj;
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        JSHandle<JSHClass> hclass(thread, thread->GlobalConstants()->GetObjectClass().GetTaggedObject());
        JSHandle<JSObject> newToOldObj = factory->NewJSObject(hclass);
        weakDeadObj = newToOldObj.GetTaggedValue().CreateAndGetWeakRef();
    }
    regionNewToOld_ = newSpace->GetCurrentRegion();
    regionNewToOld_->SetGCFlag(RegionGCFlags::BELOW_AGE_MARK);
    // new region with aliveRate 10%
    constexpr size_t arraySize2 = 25 * 1024;
    constexpr size_t arrayLength2 = arraySize2 / sizeof(JSTaggedType);
    newArray_ = factory->NewTaggedArray(arrayLength2, JSTaggedValue::Undefined(), false);
    // old region
    constexpr size_t arraySize3 = 24;
    constexpr size_t arrayLength3 = arraySize3 / sizeof(JSTaggedType);
    oldArray_ = factory->NewOldSpaceTaggedArray(arrayLength3);
    // create reference
    weakGlobal_ = thread->NewGlobalHandle(reinterpret_cast<JSTaggedType>(weakDeadObj.GetWeakReferent()));
    weakGlobal_ = thread->SetWeak(weakGlobal_);
    uint32_t idx = 0;
    newToNewArray_->Set(thread, idx, weakDeadObj);
    newArray_->Set(thread, idx, weakDeadObj);
    oldArray_->Set(thread, idx, weakDeadObj);
    idx++;
    newToNewArray_->Set(thread, idx, newToOldArray_.GetTaggedValue());
    newArray_->Set(thread, idx, newToOldArray_.GetTaggedValue());
    oldArray_->Set(thread, idx, newToOldArray_.GetTaggedValue());
    heap_->SetOnSerializeEvent(false);
}

HWTEST_F_L0(GCTest, NewToOldPromotionYoungGCTest)
{
    instance->GetJSOptions().SetEnableForceGC(false);
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    NewToOldPromotionCase testCase(heap);
    testCase.Initialize();
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    heap->Prepare();
    EXPECT_TRUE(testCase.regionNewToNew_->InYoungSpace());
    EXPECT_TRUE(testCase.regionNewToOld_->InOldSpace());
    EXPECT_FALSE(testCase.regionNewToOld_->InNewToOldSet());
    EXPECT_TRUE(testCase.newToNewArray_->Get(0).IsUndefined());
    EXPECT_TRUE(testCase.newArray_->Get(0).IsUndefined());
    EXPECT_TRUE(testCase.oldArray_->Get(0).IsUndefined());
    EXPECT_EQ(*reinterpret_cast<JSTaggedType*>(testCase.weakGlobal_), JSTaggedValueInternals::VALUE_UNDEFINED);
    JSTaggedType newToOldArrayFromArray = testCase.newToNewArray_->Get(1).GetRawData();
    JSTaggedType newToOldArray = testCase.newToOldArray_.GetTaggedValue().GetRawData();
    EXPECT_EQ(newToOldArrayFromArray, newToOldArray);
    heap->GetOldSpace()->EnumerateRegions([](Region *region) {
        EXPECT_TRUE(!region->IsGCFlagSet(RegionGCFlags::HAS_BEEN_SWEPT));
        EXPECT_TRUE(!region->InNewToOldSet());
    });
}

HWTEST_F_L0(GCTest, NewToOldPromotionOldGCTest)
{
    instance->GetJSOptions().SetEnableForceGC(false);
    Heap *heap = const_cast<Heap *>(instance->GetHeap());
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    NewToOldPromotionCase testCase(heap);
    testCase.Initialize();
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    heap->Prepare();
    EXPECT_TRUE(testCase.regionNewToNew_->InYoungSpace());
    EXPECT_TRUE(testCase.regionNewToOld_->InOldSpace());
    EXPECT_FALSE(testCase.regionNewToOld_->InNewToOldSet());
    EXPECT_TRUE(testCase.newToNewArray_->Get(0).IsUndefined());
    EXPECT_TRUE(testCase.newArray_->Get(0).IsUndefined());
    EXPECT_TRUE(testCase.oldArray_->Get(0).IsUndefined());
    EXPECT_EQ(*reinterpret_cast<JSTaggedType*>(testCase.weakGlobal_), JSTaggedValueInternals::VALUE_UNDEFINED);
    JSTaggedType newToOldArrayFromArray = testCase.newToNewArray_->Get(1).GetRawData();
    JSTaggedType newToOldArray = testCase.newToOldArray_.GetTaggedValue().GetRawData();
    EXPECT_EQ(newToOldArrayFromArray, newToOldArray);
    heap->GetOldSpace()->EnumerateRegions([](Region *region) {
        EXPECT_TRUE(!region->IsGCFlagSet(RegionGCFlags::HAS_BEEN_SWEPT));
        EXPECT_TRUE(!region->InNewToOldSet());
    });
}
} // namespace panda::test