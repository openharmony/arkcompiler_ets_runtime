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

#include "ecmascript/tests/unified_gc_test_helper.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/mem/unified_gc/unified_gc_marker.h"

using namespace panda::ecmascript;

namespace panda::test {
class UnifiedGCTest : public BaseTestWithScope<false> {};

class UnifiedGCVerificationTest : public UnifiedGCTest {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        options.SetArkProperties(options.GetArkProperties() | ArkProperties::ENABLE_HEAP_VERIFY);
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
    }
};

HWTEST_F_L0(UnifiedGCTest, UnifiedGCMarkRootsScopeTest)
{
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<TaggedArray> weakRefArray = vm->GetFactory()->NewTaggedArray(INT_VALUE_2, JSTaggedValue::Hole());
    vm->SetEnableForceGC(false);
    {
        [[maybe_unused]] EcmaHandleScope ecmaHandleScope(thread);
        JSHandle<JSTaggedValue> arrayInXRefRoot = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_1));
        JSHandle<JSTaggedValue> arrayInRoot = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_1));
        thread->NewXRefGlobalHandle(arrayInXRefRoot.GetTaggedType());
        thread->NewGlobalHandle(arrayInRoot.GetTaggedType());
        weakRefArray->Set(thread, INT_VALUE_0, arrayInXRefRoot.GetTaggedValue().CreateAndGetWeakRef());
        weakRefArray->Set(thread, INT_VALUE_1, arrayInRoot.GetTaggedValue().CreateAndGetWeakRef());
    }
    [[maybe_unused]] UnifiedGCMarkRootsScope unifiedGCMarkRootsScope(thread);
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(weakRefArray->Get(INT_VALUE_0).IsUndefined());
    EXPECT_TRUE(!weakRefArray->Get(INT_VALUE_1).IsUndefined());
    vm->SetEnableForceGC(true);
}

#ifdef PANDA_JS_ETS_HYBRID_MODE
HWTEST_F_L0(UnifiedGCTest, DoHandshakeTest)
{
    EcmaVM *vm = thread->GetEcmaVM();
    // Construct fake stsVMInterface and ecmaVMInterface for test, ecmaVMInterface used for STSVM so can be nullptr.
    auto stsVMInterface = std::make_unique<STSVMInterfaceTest>();
    void *ecmaVMInterface = nullptr;
    CrossVMOperator::DoHandshake(vm, stsVMInterface.get(), &ecmaVMInterface);

    EXPECT_TRUE(vm->GetCrossVMOperator()->GetSTSVMInterface() == stsVMInterface.get());
    EXPECT_TRUE(vm->GetCrossVMOperator()->GetEcmaVMInterface() ==
        static_cast<arkplatform::EcmaVMInterface *>(ecmaVMInterface));
}

HWTEST_F_L0(UnifiedGCTest, TriggerUnifiedGCMarkTest1)
{
    EcmaVM *vm = thread->GetEcmaVM();
    auto stsVMInterface = std::make_unique<STSVMInterfaceTest>();
    void *ecmaVMInterface = nullptr;
    CrossVMOperator::DoHandshake(vm, stsVMInterface.get(), &ecmaVMInterface);

    CrossReferenceObjectBuilder CrossReferenceObject(vm, thread);
    SharedHeap::GetInstance()->TriggerUnifiedGCMark<TriggerGCType::UNIFIED_GC, GCReason::CROSSREF_CAUSE>(thread);
    while (!thread->HasSuspendRequest()) {}
    thread->WaitSuspension();

    CrossReferenceObject.CheckResultAfterUnifiedGC();
}

HWTEST_F_L0(UnifiedGCTest, TriggerUnifiedGCMarkTest2)
{
    EcmaVM *vm = thread->GetEcmaVM();
    auto stsVMInterface = std::make_unique<STSVMInterfaceTest>();
    void *ecmaVMInterface = nullptr;
    CrossVMOperator::DoHandshake(vm, stsVMInterface.get(), &ecmaVMInterface);

    auto heap = vm->GetHeap();
    heap->GetConcurrentMarker()->Mark();
    SharedHeap::GetInstance()->TriggerUnifiedGCMark<TriggerGCType::UNIFIED_GC, GCReason::CROSSREF_CAUSE>(thread);
    while (!thread->HasSuspendRequest()) {}
    thread->WaitSuspension();
}

HWTEST_F_L0(UnifiedGCTest, TriggerUnifiedGCMarkTest3)
{
    EcmaVM *vm = thread->GetEcmaVM();
    auto stsVMInterface = std::make_unique<STSVMInterfaceTest>();
    void *ecmaVMInterface = nullptr;
    CrossVMOperator::DoHandshake(vm, stsVMInterface.get(), &ecmaVMInterface);

    SharedHeap::GetInstance()->TriggerUnifiedGCMark<TriggerGCType::UNIFIED_GC, GCReason::CROSSREF_CAUSE>(thread);
    while (!thread->HasSuspendRequest()) {}
    thread->WaitSuspension();
    vm->CollectGarbage(TriggerGCType::YOUNG_GC);
}

static inline bool IsObjectMarked(TaggedObject *object)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    return objectRegion->Test(object);
}

HWTEST_F_L0(UnifiedGCTest, IsObjectAliveTest)
{
    EcmaVM *vm = thread->GetEcmaVM();
    vm->SetEnableForceGC(false);

    JSHandle<JSTaggedValue> arrayInXRefRoot = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_1));
    JSHandle<JSTaggedValue> arrayRefByXRefRoot = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_2));
    thread->NewXRefGlobalHandle(arrayInXRefRoot.GetTaggedType());
    JSArray::FastSetPropertyByValue(thread, arrayInXRefRoot, INT_VALUE_0, arrayRefByXRefRoot);

    auto oldArray = vm->GetFactory()->NewFromASCII("123");
    EXPECT_TRUE(!panda::JSNApi::IsObjectAlive(vm, reinterpret_cast<uintptr_t>(&oldArray)));
}

HWTEST_F_L0(UnifiedGCTest, IsValidHeapObjectTest)
{
    EcmaVM *vm = thread->GetEcmaVM();
    vm->SetEnableForceGC(false);

    JSHandle<JSTaggedValue> arrayInXRefRoot = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_1));
    JSHandle<JSTaggedValue> arrayRefByXRefRoot = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_2));
    thread->NewXRefGlobalHandle(arrayInXRefRoot.GetTaggedType());
    JSArray::FastSetPropertyByValue(thread, arrayInXRefRoot, INT_VALUE_0, arrayRefByXRefRoot);

    auto oldArray = vm->GetFactory()->NewFromASCII("123");
    EXPECT_TRUE(!panda::JSNApi::IsValidHeapObject(vm, reinterpret_cast<uintptr_t>(&oldArray)));
}

HWTEST_F_L0(UnifiedGCTest, MarkFromObjectTest)
{
    EcmaVM *vm = thread->GetEcmaVM();
    vm->SetEnableForceGC(false);

    JSHandle<JSTaggedValue> arrayInXRefRoot = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_1));
    JSHandle<JSTaggedValue> arrayRefByXRefRoot = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_2));
    thread->NewXRefGlobalHandle(arrayInXRefRoot.GetTaggedType());
    JSArray::FastSetPropertyByValue(thread, arrayInXRefRoot, INT_VALUE_0, arrayRefByXRefRoot);

    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    vm->GetCrossVMOperator()->MarkFromObject(arrayInXRefRoot->GetRawData());
    heap->WaitRunningTaskFinished();
    EXPECT_TRUE(IsObjectMarked(arrayInXRefRoot->GetHeapObject()));
    EXPECT_TRUE(IsObjectMarked(arrayRefByXRefRoot->GetHeapObject()));

    // Clear mark bit
    heap->Resume(TriggerGCType::UNIFIED_GC);
    heap->WaitClearTaskFinished();
    EXPECT_TRUE(!IsObjectMarked(arrayInXRefRoot->GetHeapObject()));
    EXPECT_TRUE(!IsObjectMarked(arrayRefByXRefRoot->GetHeapObject()));
    vm->SetEnableForceGC(true);
}

HWTEST_F_L0(UnifiedGCTest, TriggerUnifiedGCFailTest)
{
    EcmaVM *vm = thread->GetEcmaVM();
    auto stsVMInterface = std::make_unique<STSVMInterfaceTest>();
    void *ecmaVMInterface = nullptr;
    CrossVMOperator::DoHandshake(vm, stsVMInterface.get(), &ecmaVMInterface);

    CrossReferenceObjectBuilder CrossReferenceObject(vm, thread);
    SharedHeap::GetInstance()->TriggerUnifiedGCMark<TriggerGCType::UNIFIED_GC, GCReason::CROSSREF_CAUSE>(thread);
    vm->GetCrossVMOperator()->GetEcmaVMInterface()->NotifyXGCInterruption();
    while (!thread->HasSuspendRequest()) {}
    thread->WaitSuspension();
    CrossReferenceObject.CheckResultAfterUnifiedGCTriggerFail();
}

HWTEST_F_L0(UnifiedGCVerificationTest, VerifyUnifiedGCMarkTest)
{
    EcmaVM *vm = thread->GetEcmaVM();
    vm->SetEnableForceGC(false);
    auto stsVMInterface = std::make_unique<STSVMInterfaceTest>();
    void *ecmaVMInterface = nullptr;
    CrossVMOperator::DoHandshake(vm, stsVMInterface.get(), &ecmaVMInterface);
    SharedHeap::GetInstance()->TriggerUnifiedGCMark<TriggerGCType::UNIFIED_GC, GCReason::CROSSREF_CAUSE>(thread);
    while (!thread->HasSuspendRequest()) {}
    thread->WaitSuspension();
    vm->SetEnableForceGC(true);
}
#endif  // PANDA_JS_ETS_HYBRID_MODE
}  // namespace panda::test