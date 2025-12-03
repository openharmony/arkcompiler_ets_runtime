/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/idle_gc_trigger.h"
#include "ecmascript/mem/verification.h"

using namespace panda::ecmascript;

namespace panda::test {
class LocalCCTest : public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        instance->GetJSOptions().SetEnableForceGC(false);
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
        instance->SetEnableForceGC(false);
        heap_ = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
        heap_->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::ENABLE);
    }

    JSHandle<TaggedArray> AllocateWithoutGC(size_t size)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        // Disallow garbage collection
        heap_->SetSensitiveStatus(AppSensitiveStatus::ENTER_HIGH_SENSITIVE);
        heap_->SetOnSerializeEvent(true);
        size_t arrayLength = size / sizeof(JSTaggedType);
        auto ret = factory->NewTaggedArray(arrayLength, JSTaggedValue::Hole());
        heap_->SetOnSerializeEvent(false);
        heap_->SetSensitiveStatus(AppSensitiveStatus::NORMAL_SCENE);
        return ret;
    }

    Heap *heap_ {nullptr};
};

#ifdef PANDA_TARGET_64
HWTEST_F_L0(LocalCCTest, IdleTriggerTest)
{
    IdleGCTrigger *idleGCTrigger = heap_->GetIdleGCTrigger();
    idleGCTrigger->TryTriggerLocalCC();
    EXPECT_TRUE(!heap_->IsCCMark());
    constexpr size_t OBJ_SIZE = IDLE_LOCAL_CC_LIMIT;
    AllocateWithoutGC(OBJ_SIZE);
    EXPECT_TRUE(heap_->GetHeapObjectSize() >= IDLE_LOCAL_CC_LIMIT);

    idleGCTrigger->TryTriggerLocalCC();
    EXPECT_TRUE(heap_->IsCCMark());
    idleGCTrigger->TryTriggerLocalCC();
    EXPECT_TRUE(thread->IsConcurrentCopying());
    idleGCTrigger->TryTriggerLocalCC();
    EXPECT_TRUE(!thread->IsConcurrentCopying());
}

HWTEST_F_L0(LocalCCTest, GCTest)
{
    constexpr size_t OBJ_SIZE = 3_KB;
    uint32_t slotId = 0;
    JSHandle<TaggedArray> array1 = AllocateWithoutGC(OBJ_SIZE);
    JSHandle<TaggedArray> value = AllocateWithoutGC(OBJ_SIZE);
    JSTaggedValue weakValue = AllocateWithoutGC(OBJ_SIZE).GetTaggedValue();
    weakValue.CreateWeakRef();
    array1->Set(thread, slotId++, value.GetTaggedValue()); // slotid 0
    array1->Set(thread, slotId++, weakValue); // slotid 1
    uint8_t strList[6] = {1, 23, 45, 97, 127}; // 6 : char str size.
    uint32_t strLen = sizeof(strList) - 1;
    EcmaString *string =
        EcmaStringAccessor::CreateFromUtf8(instance, &strList[0], strLen, true);
    array1->Set(thread, slotId++, JSTaggedValue(string));  // slotid 2
    {
        EcmaHandleScope scope(thread);
        JSTaggedValue deadValue = AllocateWithoutGC(OBJ_SIZE).GetTaggedValue();
        deadValue.CreateWeakRef();
        array1->Set(thread, slotId++, deadValue);  // slotid 3
    }
    bool succ = heap_->TryTriggerCCMarking(MarkReason::OTHER);
    EXPECT_TRUE(succ);
    heap_->CollectFromCCMark(GCReason::OTHER);
    EXPECT_TRUE(thread->IsConcurrentCopying());
    heap_->WaitAndHandleCCFinished();
    EXPECT_NE(array1->Get(thread, 0).GetRawData(), JSTaggedValue::Undefined().GetRawData());
    EXPECT_TRUE(array1->Get(thread, 1).IsWeak());
    EXPECT_TRUE(array1->Get(thread, 2).IsInSharedHeap()); // 2 : second object in array
    EXPECT_TRUE(array1->Get(thread, 3).IsUndefined());  // 3 : third object in array
}

HWTEST_F_L0(LocalCCTest, ReadBarrierTest)
{
    constexpr size_t OBJ_SIZE = 3_KB;
    uint32_t slotId = 0;
    JSHandle<TaggedArray> array1 = AllocateWithoutGC(OBJ_SIZE);
    JSHandle<TaggedArray> value = AllocateWithoutGC(OBJ_SIZE);
    JSTaggedValue weakValue = AllocateWithoutGC(OBJ_SIZE).GetTaggedValue();
    weakValue.CreateWeakRef();
    array1->Set(thread, slotId++, value.GetTaggedValue()); // slotid 0
    array1->Set(thread, slotId++, weakValue); // slotid 1
    bool succ = heap_->TryTriggerCCMarking(MarkReason::OTHER);
    EXPECT_TRUE(succ);
    heap_->CollectFromCCMark(GCReason::OTHER);
    EXPECT_TRUE(thread->IsConcurrentCopying());
    JSTaggedType *data = array1->GetData();
    constexpr uintptr_t TEST_OFFSET0 = 0;
    constexpr uintptr_t TEST_OFFSET1 = 8;
    uintptr_t slotAddress = ToUintPtr(data) + TEST_OFFSET0;
    JSTaggedType slotObj = Barriers::ReadBarrierForObject(thread, slotAddress);
    EXPECT_NE(slotObj, JSTaggedValue::Undefined().GetRawData());
    uintptr_t slotAddress1 = ToUintPtr(data) + TEST_OFFSET1;
    JSTaggedValue slotValue1 = ObjectSlot(slotAddress1).GetTaggedValue();
    JSTaggedType slotObj1 = ReadBarrier(thread, slotAddress1, slotValue1);
    EXPECT_TRUE(JSTaggedValue(slotObj1).IsWeak());
}
#endif

HWTEST_F_L0(LocalCCTest, DisableTest)
{
    heap_->DisableLocalCC();
    EXPECT_FALSE(heap_->LocalCCEnabled());
    bool succ = heap_->TryTriggerCCMarking(MarkReason::OTHER);
    EXPECT_FALSE(succ);
}
}  // namespace panda::test
