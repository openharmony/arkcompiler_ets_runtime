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

#include "ecmascript/builtins/builtins_ark_tools.h"
#include "ecmascript/builtins/builtins_gc.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/idle_gc_trigger.h"
#include "ecmascript/mem/local_cmc/cc_evacuator-inl.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/stubs/runtime_stubs.h"

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
        instance->GetJSOptions().SetOpenArkTools(true);
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
        instance->SetEnableForceGC(false);
        heap_ = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
        heap_->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::ENABLE);
        heap_->Prepare();
        if (heap_->CheckOngoingConcurrentMarking()) {
            heap_->GetConcurrentMarker()->Reset();
        }
    }

    class IgnoreGCScope {
    public:
        explicit IgnoreGCScope(Heap *heap) : heap_(heap)
        {
            heap_->SetSensitiveStatus(AppSensitiveStatus::ENTER_HIGH_SENSITIVE);
            heap_->SetOnSerializeEvent(true);
        }

        ~IgnoreGCScope()
        {
            heap_->SetOnSerializeEvent(false);
            heap_->SetSensitiveStatus(AppSensitiveStatus::NORMAL_SCENE);
        }
    private:
        Heap *heap_ {nullptr};
    };

    JSHandle<TaggedArray> AllocateWithoutGC(size_t size)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        // Disallow garbage collection
        IgnoreGCScope scope(heap_);
        size_t arrayLength = size / sizeof(JSTaggedType);
        auto ret = factory->NewTaggedArray(arrayLength, JSTaggedValue::Hole());
        return ret;
    }

    Heap *heap_ {nullptr};
};

#ifdef PANDA_TARGET_64
void SimulateWorker()
{
    JSRuntimeOptions options;
    options.SetEnableForceGC(false);
    EcmaVM *ecmaVm = JSNApi::CreateEcmaVM(options);
    EXPECT_TRUE(ecmaVm != nullptr) << "Cannot create Runtime";
    JSThread *thread = ecmaVm->GetJSThread();
    thread->ManagedCodeBegin();
    Heap *heap = const_cast<Heap*>(ecmaVm->GetHeap());
    IdleGCTrigger *idleGCTrigger = heap->GetIdleGCTrigger();
    idleGCTrigger->TryTriggerLocalCC();
    EXPECT_FALSE(heap->IsCCMark());
    thread->ManagedCodeEnd();
    JSNApi::DestroyJSVM(ecmaVm);
}

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
    // Test when triggered cc at leave frame not null.
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, undefined, undefined, 0);
    JSTaggedType *testLeaveFrame = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
    thread->SetLastLeaveFrame(testLeaveFrame);
    idleGCTrigger->TryTriggerLocalCC();
    EXPECT_FALSE(heap_->IsCCMark());
    thread->SetLastLeaveFrame(nullptr);
    heap_->SetFullMarkRequestedState(true);
    if (heap_->TryTriggerConcurrentMarking()) {
        // TryTriggerLocalCC when other concurrent mark is triggered.
        idleGCTrigger->TryTriggerLocalCC();
        EXPECT_FALSE(heap_->IsCCMark());
    }
}

HWTEST_F_L0(LocalCCTest, SimulateTriggerCCInWorkerTest)
{
    std::thread t1(SimulateWorker);
    t1.join();
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
    heap_->CollectGarbageFromCCMark(GCReason::OTHER);
    EXPECT_TRUE(thread->IsConcurrentCopying());
    heap_->SetNearGCInSensitive(true);
    heap_->WaitAndHandleCCFinished();
    EXPECT_NE(array1->Get(thread, 0).GetRawData(), JSTaggedValue::Undefined().GetRawData());
    EXPECT_TRUE(array1->Get(thread, 1).IsWeak());
    EXPECT_TRUE(array1->Get(thread, 2).IsInSharedHeap()); // 2 : third object in array
    EXPECT_TRUE(array1->Get(thread, 3).IsUndefined());  // 3 : forth object in array
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
    uint8_t strList[6] = {1, 23, 45, 97, 127}; // 6 : char str size.
    uint32_t strLen = sizeof(strList) - 1;
    EcmaString *string =
        EcmaStringAccessor::CreateFromUtf8(instance, &strList[0], strLen, true);
    array1->Set(thread, slotId++, JSTaggedValue(string));  // slotid 2
    bool succ = heap_->TryTriggerCCMarking(MarkReason::OTHER);
    EXPECT_TRUE(succ);
    heap_->CollectGarbageFromCCMark(GCReason::OTHER);
    EXPECT_TRUE(thread->IsConcurrentCopying());
    JSTaggedType *data = array1->GetData();
    constexpr uintptr_t TEST_OFFSET0 = 0;
    constexpr uintptr_t TEST_OFFSET1 = sizeof(JSTaggedType);
    constexpr uintptr_t TEST_OFFSET2 = sizeof(JSTaggedType) * 2; // 2 : the third data offset
    uintptr_t slotAddress = ToUintPtr(data) + TEST_OFFSET0;
    JSTaggedType slotObj = Barriers::ReadBarrierForObject(thread, slotAddress);
    EXPECT_NE(slotObj, JSTaggedValue::Undefined().GetRawData());
    uintptr_t slotAddress1 = ToUintPtr(data) + TEST_OFFSET1;
    JSTaggedValue slotValue1 = ObjectSlot(slotAddress1).GetTaggedValue();
    JSTaggedType slotObj1 = ReadBarrier(thread, slotAddress1, slotValue1);
    EXPECT_TRUE(JSTaggedValue(slotObj1).IsWeak());
    uintptr_t slotAddress2 = ToUintPtr(data) + TEST_OFFSET2;
    JSTaggedValue slotValue2 = ObjectSlot(slotAddress2).GetTaggedValue();
    JSTaggedType slotObj2 = ReadBarrier(thread, slotAddress2, slotValue2);
    EXPECT_TRUE(JSTaggedValue(slotObj2).IsInSharedHeap());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArrayBuffer> arraybuffer = factory->NewJSArrayBuffer(8);
    JSTaggedType hclassSlot = arraybuffer.GetTaggedValue().GetRawData();
    JSTaggedValue hclassValue(arraybuffer->GetClass());
    JSTaggedType hclass = ReadBarrier(thread, hclassSlot, hclassValue);
    EXPECT_TRUE(JSTaggedValue(hclass).IsJSHClass());
    JSTaggedType nullValue(0);
    uintptr_t nullSlot = ToUintPtr(&nullValue);
    JSTaggedType nullObject = ReadBarrierImpl(thread, nullSlot);
    EXPECT_EQ(nullObject, static_cast<JSTaggedType>(0));
}

HWTEST_F_L0(LocalCCTest, BCDebugStubSwitchScopeTest)
{
    heap_->WaitCCFinished();
    instance->GetJsDebuggerManager()->SetDebugMode(true);
    thread->CheckSwitchDebuggerBCStub();
    thread->SwitchAllStub(false);
    EXPECT_EQ(thread->GetBCStubStatus(), BCStubStatus::NORMAL_BC_STUB);
    EXPECT_EQ(thread->GetCommonStubStatus(), CommonStubStatus::NORMAL_COMMON_STUB);
    EXPECT_EQ(thread->GetBuiltinsStubStatus(), BuiltinsStubStatus::NORMAL_BUILTINS_STUB);
}

HWTEST_F_L0(LocalCCTest, CheckSafepointTest)
{
    constexpr size_t OBJ_SIZE = 3_KB;
    AllocateWithoutGC(OBJ_SIZE);
    heap_->TryTriggerCCMarking(MarkReason::OTHER);
    heap_->CheckOngoingConcurrentMarking();
    EXPECT_TRUE(thread->IsMarkFinished());
    thread->CheckSafepoint();
    EXPECT_TRUE(thread->IsConcurrentCopying());
    heap_->WaitCCFinished();
    thread->CheckSafepoint();
    EXPECT_FALSE(thread->IsConcurrentCopying());
}

HWTEST_F_L0(LocalCCTest, GCConflictTest)
{
    constexpr size_t OBJ_SIZE = 3_KB;
    AllocateWithoutGC(OBJ_SIZE);
    constexpr int enableGCLog = 0x805c;
    instance->GetJSOptions().SetArkProperties(enableGCLog);
    if (heap_->TryTriggerCCMarking(MarkReason::OTHER)) {
        heap_->CollectGarbage(TriggerGCType::OLD_GC, GCReason::OTHER);
        EXPECT_TRUE(thread->IsConcurrentCopying());
        heap_->CollectGarbage(TriggerGCType::OLD_GC, GCReason::OTHER);
        EXPECT_FALSE(thread->IsConcurrentCopying());
    }
}

HWTEST_F_L0(LocalCCTest, GCConflictTest2)
{
    constexpr size_t OBJ_SIZE = 3_KB;
    AllocateWithoutGC(OBJ_SIZE);
    if (heap_->TryTriggerCCMarking(MarkReason::OTHER)) {
        heap_->CollectGarbage(TriggerGCType::FULL_GC, GCReason::OTHER);
        EXPECT_FALSE(thread->IsConcurrentCopying());
        heap_->CollectGarbage(TriggerGCType::OLD_GC, GCReason::OTHER);
        EXPECT_FALSE(thread->IsConcurrentCopying());
    }
}

HWTEST_F_L0(LocalCCTest, GCConflictTest3)
{
    constexpr size_t OBJ_SIZE = 3_KB;
    AllocateWithoutGC(OBJ_SIZE);
    if (heap_->TryTriggerCCMarking(MarkReason::OTHER)) {
        heap_->CollectGarbage(TriggerGCType::OLD_GC, GCReason::OTHER);
        EXPECT_TRUE(thread->IsConcurrentCopying());
        SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
        EXPECT_FALSE(thread->IsConcurrentCopying());
    }
}

HWTEST_F_L0(LocalCCTest, GCConflictOOMTest)
{
    constexpr size_t OBJ_SIZE = 3_KB;
    AllocateWithoutGC(OBJ_SIZE);
    if (heap_->TryTriggerCCMarking(MarkReason::OTHER)) {
        heap_->ShouldThrowOOMError(true);
        heap_->CollectGarbage(TriggerGCType::OLD_GC, GCReason::OTHER);
        EXPECT_FALSE(thread->IsConcurrentCopying());
        heap_->CollectGarbage(TriggerGCType::OLD_GC, GCReason::OTHER);
        EXPECT_FALSE(thread->IsConcurrentCopying());
    }
}

HWTEST_F_L0(LocalCCTest, SkipGCTest)
{
    constexpr size_t OBJ_SIZE = 3_KB;
    AllocateWithoutGC(OBJ_SIZE);
    if (heap_->TryTriggerCCMarking(MarkReason::OTHER)) {
        IgnoreGCScope scope(heap_);
        heap_->CollectGarbage(TriggerGCType::OLD_GC, GCReason::OTHER);
        EXPECT_TRUE(thread->IsConcurrentMarkingOrFinished());
        EXPECT_FALSE(thread->IsConcurrentCopying());
    }
}

HWTEST_F_L0(LocalCCTest, VerifyTest)
{
    constexpr size_t OBJ_SIZE = 3_KB;
    uint32_t slotId = 0;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array1 = AllocateWithoutGC(OBJ_SIZE);
    JSHandle<TaggedArray> value = AllocateWithoutGC(OBJ_SIZE);
    JSTaggedValue weakValue = AllocateWithoutGC(OBJ_SIZE).GetTaggedValue();
    weakValue.CreateWeakRef();
    array1->Set(thread, slotId++, value.GetTaggedValue());
    array1->Set(thread, slotId++, weakValue);
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsWeakMapFunction();
    JSHandle<JSWeakMap>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor),
                                                                constructor));
    heap_->EnableHeapVerication(true);
    if (heap_->TryTriggerCCMarking(MarkReason::OTHER)) {
        heap_->CollectGarbage(TriggerGCType::OLD_GC, GCReason::OTHER);
        EXPECT_TRUE(thread->IsConcurrentCopying());
    }
    EXPECT_TRUE(array1->Get(thread, 1).IsWeak());
}

HWTEST_F_L0(LocalCCTest, EvacuatorCopyTest)
{
    constexpr size_t OBJ_SIZE = 3_KB;
    JSHandle<TaggedArray> testArray = AllocateWithoutGC(OBJ_SIZE);
    TaggedObject *testObject = testArray.GetTaggedValue().GetTaggedObject();
    MarkWord markWord(testObject);
    CCEvacuator *mainEvacuator = thread->GetLocalCCEvacuator();
    thread->SetReadBarrierState(true);
    thread->SetCCStatus(CCStatus::COPYING);
    TaggedObject *dst1 = mainEvacuator->Copy(testObject, markWord);
    TaggedObject *dst2 = mainEvacuator->Copy(testObject, markWord);
    EXPECT_EQ(dst1, dst2);
    thread->SetReadBarrierState(false);
    thread->SetCCStatus(CCStatus::READY);
}

HWTEST_F_L0(LocalCCTest, UpdateRecordJSWeakMapTest)
{
    constexpr size_t OBJ_SIZE = 3_KB;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> value = JSHandle<JSTaggedValue>::Cast(AllocateWithoutGC(OBJ_SIZE));
    JSHandle<GlobalEnv> env = instance->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsWeakMapFunction();
    JSHandle<JSWeakMap> weakMap =
        JSHandle<JSWeakMap>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor),
                                                                    constructor));
    JSHandle<LinkedHashMap> hashMap = LinkedHashMap::Create(thread);
    weakMap->SetLinkedMap(thread, hashMap);
    {
        EcmaHandleScope scope(thread);
        JSHandle<JSTaggedValue> key = JSHandle<JSTaggedValue>::Cast(AllocateWithoutGC(OBJ_SIZE));
        JSWeakMap::Set(thread, weakMap, key, value);
    }
    if (heap_->TryTriggerCCMarking(MarkReason::OTHER)) {
        heap_->CollectGarbageFromCCMark(GCReason::OTHER);
        EXPECT_TRUE(thread->IsConcurrentCopying());
    }
}

HWTEST_F_L0(LocalCCTest, UpdateRecordWeakRefTest)
{
    constexpr size_t OBJ_SIZE = 3_KB;
    uint32_t slotId = 0;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array1 = AllocateWithoutGC(OBJ_SIZE);
    JSHandle<TaggedArray> value = AllocateWithoutGC(OBJ_SIZE);
    JSTaggedValue weakValue = AllocateWithoutGC(OBJ_SIZE).GetTaggedValue();
    weakValue.CreateWeakRef();
    JSTaggedValue weakString = factory->NewFromUtf8("local_cc").GetTaggedValue();
    weakString.CreateWeakRef();
    array1->Set(thread, slotId++, value.GetTaggedValue());
    array1->Set(thread, slotId++, weakValue);
    array1->Set(thread, slotId++, weakString);
    array1->Set(thread, slotId, weakValue);
    if (heap_->TryTriggerCCMarking(MarkReason::OTHER)) {
        heap_->CheckOngoingConcurrentMarking();
        // change weakslot to strong reference.
        array1->Set(thread, slotId, value);
        heap_->CollectGarbageFromCCMark(GCReason::OTHER);
        EXPECT_TRUE(thread->IsConcurrentCopying());
    }
}

HWTEST_F_L0(LocalCCTest, UpdateWeakGlobalTest)
{
    constexpr size_t OBJ_SIZE = 3_KB;
    JSHandle<TaggedArray> value = AllocateWithoutGC(OBJ_SIZE);
    uintptr_t globalDeadValue = 0;
    {
        [[maybe_unused]] EcmaHandleScope scope(thread);
        JSHandle<TaggedArray> deadValue = AllocateWithoutGC(OBJ_SIZE);
        globalDeadValue = thread->NewGlobalHandle(deadValue.GetTaggedType());
        thread->SetWeak(globalDeadValue);
    }
    uintptr_t globalValue1 = thread->NewGlobalHandle(value.GetTaggedType());
    uintptr_t globalValue2 = thread->NewGlobalHandle(value.GetTaggedType());
    thread->SetWeak(globalValue1);
    thread->SetWeak(globalValue2);
    if (heap_->TryTriggerCCMarking(MarkReason::OTHER)) {
        heap_->CollectGarbageFromCCMark(GCReason::OTHER);
        EXPECT_TRUE(thread->IsConcurrentCopying());
        EXPECT_EQ(*reinterpret_cast<JSTaggedType*>(globalValue1), *reinterpret_cast<JSTaggedType*>(globalValue2));
        EXPECT_EQ(*reinterpret_cast<JSTaggedType*>(globalDeadValue), JSTaggedValue::Undefined().GetRawData());
        heap_->WaitCCFinished();
        EXPECT_TRUE(thread->IsConcurrentCopying());
        heap_->WaitAndHandleCCFinished();
        EXPECT_FALSE(thread->IsConcurrentCopying());
    }
}

HWTEST_F_L0(LocalCCTest, StringToGcTypeTest)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> gcTypeString = factory->NewFromUtf8("local_cc");
    auto cause = builtins::BuiltinsGc::StringToGcType(thread, gcTypeString.GetTaggedValue());
    EXPECT_EQ(cause, TriggerGCType::LOCAL_CC);
}

HWTEST_F_L0(LocalCCTest, ObjectCopyWithRbTest)
{
    constexpr size_t OBJ_SIZE = 3_KB;
    constexpr size_t COPY_COUNT = 10;
    uint32_t slotId = 0;
    JSHandle<TaggedArray> srcArray = AllocateWithoutGC(OBJ_SIZE);
    JSHandle<TaggedArray> value = AllocateWithoutGC(OBJ_SIZE);
    JSTaggedValue weakValue = AllocateWithoutGC(OBJ_SIZE).GetTaggedValue();
    weakValue.CreateWeakRef();
    srcArray->Set(thread, slotId++, value.GetTaggedValue()); // slotid 0
    srcArray->Set(thread, slotId++, weakValue); // slotid 1
    uint8_t strList[6] = {1, 23, 45, 97, 127}; // 6 : char str size.
    uint32_t strLen = sizeof(strList) - 1;
    EcmaString *string =
        EcmaStringAccessor::CreateFromUtf8(instance, &strList[0], strLen, true);
    srcArray->Set(thread, slotId++, JSTaggedValue(string));  // slotid 2

    JSHandle<TaggedArray> dstArray = AllocateWithoutGC(OBJ_SIZE);
    if (heap_->TryTriggerCCMarking(MarkReason::OTHER)) {
        heap_->CollectGarbageFromCCMark(GCReason::OTHER);
        EXPECT_TRUE(thread->IsConcurrentCopying());
        JSTaggedType *dstObject = reinterpret_cast<JSTaggedType *>(dstArray.GetTaggedValue().GetHeapObject());
        RuntimeStubs::ObjectCopy(reinterpret_cast<uintptr_t>(thread), dstObject, dstArray->GetData(),
            srcArray->GetData(), COPY_COUNT);
        EXPECT_NE(dstArray->Get(thread, 0).GetRawData(), JSTaggedValue::Undefined().GetRawData());
        EXPECT_TRUE(dstArray->Get(thread, 1).IsWeak());
        EXPECT_TRUE(dstArray->Get(thread, 2).IsInSharedHeap()); // 2 : third object in array
    }
}

HWTEST_F_L0(LocalCCTest, ObjectCopyWithoutRbTest)
{
    constexpr size_t OBJ_SIZE = 3_KB;
    constexpr size_t COPY_COUNT = 10;
    uint32_t slotId = 0;
    JSHandle<TaggedArray> srcArray = AllocateWithoutGC(OBJ_SIZE);
    JSHandle<TaggedArray> value = AllocateWithoutGC(OBJ_SIZE);
    JSTaggedValue weakValue = AllocateWithoutGC(OBJ_SIZE).GetTaggedValue();
    weakValue.CreateWeakRef();
    srcArray->Set(thread, slotId++, value.GetTaggedValue()); // slotid 0
    srcArray->Set(thread, slotId++, weakValue); // slotid 1
    uint8_t strList[6] = {1, 23, 45, 97, 127}; // 6 : char str size.
    uint32_t strLen = sizeof(strList) - 1;
    EcmaString *string =
        EcmaStringAccessor::CreateFromUtf8(instance, &strList[0], strLen, true);
    srcArray->Set(thread, slotId++, JSTaggedValue(string));  // slotid 2
    JSHandle<TaggedArray> dstArray = AllocateWithoutGC(OBJ_SIZE);

    heap_->CollectGarbage(TriggerGCType::FULL_GC, GCReason::OTHER);
    heap_->Prepare();
        
    JSTaggedType *dstObject = reinterpret_cast<JSTaggedType *>(dstArray.GetTaggedValue().GetHeapObject());
    RuntimeStubs::ObjectCopy(reinterpret_cast<uintptr_t>(thread), dstObject, dstArray->GetData(),
        srcArray->GetData(), COPY_COUNT);
    EXPECT_NE(dstArray->Get(thread, 0).GetRawData(), JSTaggedValue::Undefined().GetRawData());
    EXPECT_TRUE(dstArray->Get(thread, 1).IsWeak());
    EXPECT_TRUE(dstArray->Get(thread, 2).IsInSharedHeap()); // 2 : third object in array
}

HWTEST_F_L0(LocalCCTest, ReverseArrayWithRbTest)
{
    constexpr size_t OBJ_SIZE = 3_KB;
    constexpr size_t REVERSE_LENGTH = 10;
    uint32_t slotId = 0;
    JSHandle<TaggedArray> array = AllocateWithoutGC(OBJ_SIZE);
    JSHandle<TaggedArray> value = AllocateWithoutGC(OBJ_SIZE);
    JSTaggedValue weakValue = AllocateWithoutGC(OBJ_SIZE).GetTaggedValue();
    weakValue.CreateWeakRef();
    array->Set(thread, slotId++, value.GetTaggedValue()); // slotid 0
    array->Set(thread, slotId++, weakValue); // slotid 1
    uint8_t strList[6] = {1, 23, 45, 97, 127}; // 6 : char str size.
    uint32_t strLen = sizeof(strList) - 1;
    EcmaString *string =
        EcmaStringAccessor::CreateFromUtf8(instance, &strList[0], strLen, true);
    array->Set(thread, slotId++, JSTaggedValue(string));  // slotid 2

    if (heap_->TryTriggerCCMarking(MarkReason::OTHER)) {
        heap_->CollectGarbageFromCCMark(GCReason::OTHER);
        EXPECT_TRUE(thread->IsConcurrentCopying());
        RuntimeStubs::ReverseArray(reinterpret_cast<uintptr_t>(thread), array->GetData(), REVERSE_LENGTH);
        // reverse object 1.
        EXPECT_NE(array->Get(thread, REVERSE_LENGTH - 1).GetRawData(), JSTaggedValue::Undefined().GetRawData());
        // reverse object 2.
        EXPECT_TRUE(array->Get(thread, REVERSE_LENGTH - 2).IsWeak());
        // reverse object 3.
        EXPECT_TRUE(array->Get(thread, REVERSE_LENGTH - 3).IsInSharedHeap());
    }
}

HWTEST_F_L0(LocalCCTest, ReverseArrayWithoutRbTest)
{
    constexpr size_t OBJ_SIZE = 3_KB;
    constexpr size_t REVERSE_LENGTH = 10;
    uint32_t slotId = 0;
    JSHandle<TaggedArray> array = AllocateWithoutGC(OBJ_SIZE);
    JSHandle<TaggedArray> value = AllocateWithoutGC(OBJ_SIZE);
    JSTaggedValue weakValue = AllocateWithoutGC(OBJ_SIZE).GetTaggedValue();
    weakValue.CreateWeakRef();
    array->Set(thread, slotId++, value.GetTaggedValue()); // slotid 0
    array->Set(thread, slotId++, weakValue); // slotid 1
    uint8_t strList[6] = {1, 23, 45, 97, 127}; // 6 : char str size.
    uint32_t strLen = sizeof(strList) - 1;
    EcmaString *string =
        EcmaStringAccessor::CreateFromUtf8(instance, &strList[0], strLen, true);
    array->Set(thread, slotId++, JSTaggedValue(string));  // slotid 2

    heap_->CollectGarbage(TriggerGCType::FULL_GC, GCReason::OTHER);
    heap_->Prepare();
    RuntimeStubs::ReverseArray(reinterpret_cast<uintptr_t>(thread), array->GetData(), REVERSE_LENGTH);
    // reverse object 1.
    EXPECT_NE(array->Get(thread, REVERSE_LENGTH - 1).GetRawData(), JSTaggedValue::Undefined().GetRawData());
    // reverse object 2.
    EXPECT_TRUE(array->Get(thread, REVERSE_LENGTH - 2).IsWeak());
    // reverse object 3.
    EXPECT_TRUE(array->Get(thread, REVERSE_LENGTH - 3).IsInSharedHeap());
}

HWTEST_F_L0(LocalCCTest, ArkToolsTriggerTest)
{
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *runtimeInfo =
        EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, undefined, undefined, 0);
    builtins::BuiltinsArkTools::TriggerLocalCCGC(runtimeInfo);
    if (heap_->TryTriggerCCMarking(MarkReason::OTHER)) {
        EXPECT_TRUE(heap_->IsCCMark());
    }
}

HWTEST_F_L0(LocalCCTest, ArkToolsTriggerImplTest)
{
    constexpr size_t OBJ_SIZE = 3_KB;
    uint32_t slotId = 0;
    JSHandle<TaggedArray> array1 = AllocateWithoutGC(OBJ_SIZE);
    JSHandle<TaggedArray> value = AllocateWithoutGC(OBJ_SIZE);
    JSTaggedValue weakValue = AllocateWithoutGC(OBJ_SIZE).GetTaggedValue();
    weakValue.CreateWeakRef();
    array1->Set(thread, slotId++, value.GetTaggedValue());
    array1->Set(thread, slotId++, weakValue);
    heap_->SetFullMarkRequestedState(true);
    if (heap_->TryTriggerConcurrentMarking()) {
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        EcmaRuntimeCallInfo *runtimeInfo =
            EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, undefined, undefined, 0);
        builtins::BuiltinsArkTools::TriggerLocalCCGC(runtimeInfo);
    }
    if (heap_->TryTriggerCCMarking(MarkReason::OTHER)) {
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        EcmaRuntimeCallInfo *runtimeInfo =
            EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, undefined, undefined, 0);
        builtins::BuiltinsArkTools::TriggerLocalCCGC(runtimeInfo);
        EXPECT_TRUE(thread->IsConcurrentCopying());
        builtins::BuiltinsArkTools::TriggerLocalCCGC(runtimeInfo);
        EXPECT_FALSE(thread->IsConcurrentCopying());
    }
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
