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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/partial_gc.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda::ecmascript;

namespace panda::test {
class WeakRefOldGCTest : public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        options.SetEnableEdenGC(true);
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
        auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
        heap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::ENABLE);
        heap->GetSweeper()->EnableConcurrentSweep(EnableConcurrentSweepType::ENABLE);
    }
};

static JSObject *JSObjectTestCreate(JSThread *thread)
{
    [[maybe_unused]] ecmascript::EcmaHandleScope scope(thread);
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    auto globalEnv = ecmaVM->GetGlobalEnv();
    JSFunction *jsFunc = globalEnv->GetObjectFunction().GetObject<JSFunction>();
    JSHandle<JSTaggedValue> jsFunc1(thread, jsFunc);
    JSHandle<JSObject> newObj =
        ecmaVM->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(jsFunc1), jsFunc1);
    return *newObj;
}

static TaggedArray *ArrayTestCreate(JSThread *thread)
{
    [[maybe_unused]] ecmascript::EcmaHandleScope scope(thread);
    // 2 : test case
    JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(2);
    return *array;
}

HWTEST_F_L0(WeakRefOldGCTest, ArrayNonMovable)
{
    auto vm = thread->GetEcmaVM();
    auto array = vm->GetFactory()->NewTaggedArray(2, JSTaggedValue::Undefined(), true);
    JSHandle<JSObject> newObj1(thread, JSObjectTestCreate(thread));
    array->Set(thread, 0, newObj1.GetTaggedValue());

    JSObject *newObj2 = JSObjectTestCreate(thread);
    JSTaggedValue value(newObj2);
    value.CreateWeakRef();
    array->Set(thread, 1, value);
    EXPECT_EQ(newObj1.GetTaggedValue(), array->Get(0));
    EXPECT_EQ(value, array->Get(1));
    vm->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_EQ(newObj1.GetTaggedValue(), array->Get(0));
    EXPECT_EQ(JSTaggedValue::Undefined(), array->Get(1));
}

HWTEST_F_L0(WeakRefOldGCTest, ArrayUndefined)
{
    EcmaTestCommon::ArrayUndefinedGcCommon(thread, TriggerGCType::OLD_GC);
}

HWTEST_F_L0(WeakRefOldGCTest, ArrayKeep)
{
    EcmaTestCommon::ArrayKeepGcCommon(thread, TriggerGCType::OLD_GC);
}

HWTEST_F_L0(WeakRefOldGCTest, ObjectUndefined)
{
    JSHandle<JSObject> newObj1(thread, JSObjectTestCreate(thread));
    JSTaggedValue array(ArrayTestCreate(thread));
    array.CreateWeakRef();
    newObj1->SetElements(thread, array);
    EXPECT_EQ(newObj1->GetElements(), array);
    thread->GetEcmaVM()->CollectGarbage(TriggerGCType::OLD_GC);
    EXPECT_EQ(newObj1->GetElements(), JSTaggedValue::Undefined());
}

HWTEST_F_L0(WeakRefOldGCTest, ObjectKeep)
{
    JSHandle<JSObject> newObj1(thread, JSObjectTestCreate(thread));
    JSHandle<TaggedArray> array(thread, ArrayTestCreate(thread));
    JSTaggedValue value = array.GetTaggedValue();
    value.CreateWeakRef();
    newObj1->SetElements(thread, value);
    EXPECT_EQ(newObj1->GetElements(), value);
    thread->GetEcmaVM()->CollectGarbage(TriggerGCType::OLD_GC);
    value = array.GetTaggedValue();
    value.CreateWeakRef();
    EXPECT_EQ(newObj1->GetElements(), value);
}

HWTEST_F_L0(WeakRefOldGCTest, WeakRefTest)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::vector<JSHandle<TaggedArray>> srcArrayHandleRecord;
    std::vector<JSHandle<TaggedArray>> dstOldArrayHandleRecord;
    std::vector<JSHandle<TaggedArray>> dstNewArrayHandleRecord;
    for (int i = 0; i < 128; i++) {
        JSHandle<TaggedArray> arrayHandle = factory->NewTaggedArray(64, JSTaggedValue::True(),
                                                                    MemSpaceType::OLD_SPACE);
        srcArrayHandleRecord.emplace_back(arrayHandle);
    }
    for (int i = 0; i < 128; i++) {
        JSHandle<TaggedArray> arrayHandle = factory->NewTaggedArray(128, JSTaggedValue::True(),
                                                                    MemSpaceType::OLD_SPACE);
        dstOldArrayHandleRecord.emplace_back(arrayHandle);
    }
    for (int i = 0; i < 128; i++) {
        JSHandle<TaggedArray> arrayHandle = factory->NewTaggedArray(128, JSTaggedValue::True(),
                                                                    MemSpaceType::SEMI_SPACE);
        dstNewArrayHandleRecord.emplace_back(arrayHandle);
    }
    for (auto it : srcArrayHandleRecord) {
        uint32_t countIndex = 0;
        std::random_device randomDevice;
        std::shuffle(dstOldArrayHandleRecord.begin(), dstOldArrayHandleRecord.end(), std::mt19937(randomDevice()));
        for (auto it2 : dstOldArrayHandleRecord) {
            if (Region::ObjectAddressToRange(it2.GetTaggedValue().GetTaggedObject())->InGeneralOldSpace()) {
                JSTaggedValue valueWeak = it2.GetTaggedValue().CreateAndGetWeakRef();
                it->Set<true>(thread, countIndex, valueWeak);
                if (++countIndex >= 40) {
                    break;
                }
            }
        }
    }
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    if (heap->CheckOngoingConcurrentMarking()) {
        heap->GetConcurrentMarker()->Reset();
    }
    heap->SetMarkType(MarkType::MARK_FULL);
    heap->TriggerConcurrentMarking();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    for (auto it : srcArrayHandleRecord) {
        uint32_t countIndex = 0;
        std::random_device randomDevice;
        std::shuffle(dstNewArrayHandleRecord.begin(), dstNewArrayHandleRecord.end(), std::mt19937(randomDevice()));
        for (auto it2 : dstNewArrayHandleRecord) {
            if (Region::ObjectAddressToRange(it2.GetTaggedValue().GetTaggedObject())->InGeneralNewSpace()) {
                JSTaggedValue valueWeak = it2.GetTaggedValue().CreateAndGetWeakRef();
                it->Set<true>(thread, countIndex, valueWeak);
                if (++countIndex >= 40) {
                    break;
                }
            }
        }
    }
    auto partialGc = heap->GetPartialGC();
    partialGc->RunPhases();
    for (auto it : dstOldArrayHandleRecord) {
        EXPECT_TRUE(it.GetTaggedValue() != JSTaggedValue::Undefined());
    }
    for (auto it : dstNewArrayHandleRecord) {
        EXPECT_TRUE(it.GetTaggedValue() != JSTaggedValue::Undefined());
    }
    for (auto it : srcArrayHandleRecord) {
        for (uint32_t i = 0; i < 40; i++) {
            EXPECT_TRUE(it->Get(i) != JSTaggedValue::Undefined());
        }
    }
}
}  // namespace panda::test
