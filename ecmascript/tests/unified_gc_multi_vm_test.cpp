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

#include <thread>

#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/tests/unified_gc_test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class UnifiedGCMultiVMTest : public BaseTestWithScope<false> {};

class UnifiedGCMultiVMVerificationTest : public UnifiedGCMultiVMTest {
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

#ifdef PANDA_JS_ETS_HYBRID_MODE
static inline bool IsObjectMarked(TaggedObject *object)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    return objectRegion->Test(object);
}

class UnifiedGCMultiVMTestSuite {
public:
    UnifiedGCMultiVMTestSuite(void *stsIface, SuspendBarrier *barrier)
        : stsVMInterface(stsIface), suspendBarrier(barrier) {}
    ~UnifiedGCMultiVMTestSuite() = default;

    void DoHandShakeTest()
    {
        Init();

        void *ecmaVMInterface = nullptr;
        CrossVMOperator::DoHandshake(vm, stsVMInterface, &ecmaVMInterface);

        EXPECT_TRUE(vm->GetCrossVMOperator()->GetSTSVMInterface() == stsVMInterface);
        EXPECT_TRUE(vm->GetCrossVMOperator()->GetEcmaVMInterface() ==
            static_cast<arkplatform::EcmaVMInterface *>(ecmaVMInterface));

        Destroy();
    }

    void NoHandshakeTest()
    {
        Init();

        while (!thread->HasSuspendRequest()) {}
        thread->WaitSuspension();

        Destroy();
    }

    void CheckUnifiedGCResultTest()
    {
        Init();

        void *ecmaVMInterface = nullptr;
        CrossVMOperator::DoHandshake(vm, stsVMInterface, &ecmaVMInterface);

        CrossReferenceObjectBuilder CrossReferenceObject(vm, thread);
        while (!thread->HasSuspendRequest()) {}
        thread->WaitSuspension();
        CrossReferenceObject.CheckResultAfterUnifiedGC();

        Destroy();
    }

    void TriggerConcurrentMarkTest()
    {
        Init();

        void *ecmaVMInterface = nullptr;
        CrossVMOperator::DoHandshake(vm, stsVMInterface, &ecmaVMInterface);

        CrossReferenceObjectBuilder CrossReferenceObject(vm, thread);
        auto heap = vm->GetHeap();
        heap->GetConcurrentMarker()->Mark();
        while (!thread->HasSuspendRequest()) {}
        thread->WaitSuspension();
        CrossReferenceObject.CheckResultAfterUnifiedGC();

        Destroy();
    }

    void TriggerYoungGCTest()
    {
        Init();

        void *ecmaVMInterface = nullptr;
        CrossVMOperator::DoHandshake(vm, stsVMInterface, &ecmaVMInterface);

        CrossReferenceObjectBuilder CrossReferenceObject(vm, thread);
        while (!thread->HasSuspendRequest()) {}
        thread->WaitSuspension();
        vm->CollectGarbage(TriggerGCType::YOUNG_GC);
        CrossReferenceObject.CheckResultAfterUnifiedGC();

        Destroy();
    }

    void HeapVerifyTest()
    {
        JSRuntimeOptions options;
        options.SetArkProperties(options.GetArkProperties() | ArkProperties::ENABLE_HEAP_VERIFY);
        vm = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(vm != nullptr) << "Cannot create EcmaVM";
        thread = vm->GetJSThread();
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
        {
            ThreadSuspensionScope suspensionScope(thread);
            suspendBarrier->PassStrongly();
            suspendBarrier->Wait();
        }

        void *ecmaVMInterface = nullptr;
        CrossVMOperator::DoHandshake(vm, stsVMInterface, &ecmaVMInterface);

        CrossReferenceObjectBuilder CrossReferenceObject(vm, thread);
        auto heap = vm->GetHeap();
        heap->GetConcurrentMarker()->Mark();
        while (!thread->HasSuspendRequest()) {}
        thread->WaitSuspension();
        CrossReferenceObject.CheckResultAfterUnifiedGC();

        Destroy();
    }

    void MarkFromObjectTest()
    {
        Init();

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

        Destroy();
    }

    void TriggerUnifiedGCFailTest()
    {
        Init();

        void *ecmaVMInterface = nullptr;
        CrossVMOperator::DoHandshake(vm, stsVMInterface, &ecmaVMInterface);

        CrossReferenceObjectBuilder CrossReferenceObject(vm, thread);
        while (!thread->HasSuspendRequest()) {}
        thread->WaitSuspension();
        CrossReferenceObject.CheckResultAfterUnifiedGCTriggerFail();

        Destroy();
    }

private:
    void Init()
    {
        JSRuntimeOptions options;
        vm = JSNApi::CreateEcmaVM(options);
        vm->SetEnableForceGC(false);
        ASSERT_TRUE(vm != nullptr) << "Cannot create EcmaVM";
        thread = vm->GetJSThread();
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
        // RuntimeLockHolder will trigger SuspendAll GC(sharedGC) during the initialization of builtins in
        // CreateEcmaVM. So switch the current jsThread to IS_SUSPENDED state to pass the SuspendAll barrier until
        // all SuspendAll GC finished. The suspendBarrier is used to make sure all SuspendAll GC finished and
        // sync all test vm for following unifiedGC test.
        {
            ThreadSuspensionScope suspensionScope(thread);
            suspendBarrier->PassStrongly();
            suspendBarrier->Wait();
        }
    }

    void Destroy()
    {
        TestHelper::DestroyEcmaVMWithScope(vm, scope);
    }

    EcmaVM *vm = nullptr;
    EcmaHandleScope *scope = nullptr;
    JSThread *thread = nullptr;
    void *stsVMInterface = nullptr;
    SuspendBarrier *suspendBarrier = nullptr;
};

HWTEST_F_L0(UnifiedGCMultiVMTest, MultiVMDoHandshakeTest)
{
    SuspendBarrier barrier(INT_VALUE_4);
    EcmaVM *vm = thread->GetEcmaVM();
    auto stsVMInterface = std::make_unique<STSVMInterfaceTest>();

    UnifiedGCMultiVMTestSuite testVM1(stsVMInterface.get(), &barrier);
    UnifiedGCMultiVMTestSuite testVM2(stsVMInterface.get(), &barrier);
    UnifiedGCMultiVMTestSuite testVM3(stsVMInterface.get(), &barrier);
    std::thread t1(&UnifiedGCMultiVMTestSuite::DoHandShakeTest, testVM1);
    std::thread t2(&UnifiedGCMultiVMTestSuite::DoHandShakeTest, testVM2);
    std::thread t3(&UnifiedGCMultiVMTestSuite::DoHandShakeTest, testVM3);
    {
        ThreadSuspensionScope suspensionScope(thread);
        barrier.PassStrongly();
        barrier.Wait();
    }

    void *ecmaVMInterface = nullptr;
    CrossVMOperator::DoHandshake(vm, stsVMInterface.get(), &ecmaVMInterface);
    EXPECT_TRUE(vm->GetCrossVMOperator()->GetSTSVMInterface() == stsVMInterface.get());
    EXPECT_TRUE(vm->GetCrossVMOperator()->GetEcmaVMInterface() ==
        static_cast<arkplatform::EcmaVMInterface *>(ecmaVMInterface));
    t1.join();
    t2.join();
    t3.join();
}

HWTEST_F_L0(UnifiedGCMultiVMTest, MultiVMUnifiedGCMarkTest1)
{
    SuspendBarrier barrier(INT_VALUE_4);
    EcmaVM *vm = thread->GetEcmaVM();
    auto stsVMInterface = std::make_unique<STSVMInterfaceTest>();

    UnifiedGCMultiVMTestSuite testVM1(stsVMInterface.get(), &barrier);
    UnifiedGCMultiVMTestSuite testVM2(stsVMInterface.get(), &barrier);
    UnifiedGCMultiVMTestSuite testVM3(stsVMInterface.get(), &barrier);
    std::thread t1(&UnifiedGCMultiVMTestSuite::CheckUnifiedGCResultTest, testVM1);
    std::thread t2(&UnifiedGCMultiVMTestSuite::CheckUnifiedGCResultTest, testVM2);
    std::thread t3(&UnifiedGCMultiVMTestSuite::CheckUnifiedGCResultTest, testVM3);
    {
        ThreadSuspensionScope suspensionScope(thread);
        barrier.PassStrongly();
        barrier.Wait();
    }

    void *ecmaVMInterface = nullptr;
    CrossVMOperator::DoHandshake(vm, stsVMInterface.get(), &ecmaVMInterface);
    CrossReferenceObjectBuilder CrossReferenceObject(vm, thread);
    SharedHeap::GetInstance()->TriggerUnifiedGCMark<TriggerGCType::UNIFIED_GC, GCReason::CROSSREF_CAUSE>(thread);
    while (!thread->HasSuspendRequest()) {}
    thread->WaitSuspension();
    CrossReferenceObject.CheckResultAfterUnifiedGC();
    t1.join();
    t2.join();
    t3.join();
}

HWTEST_F_L0(UnifiedGCMultiVMTest, MultiVMUnifiedGCMarkTest2)
{
    SuspendBarrier barrier(INT_VALUE_4);
    EcmaVM *vm = thread->GetEcmaVM();
    auto stsVMInterface = std::make_unique<STSVMInterfaceTest>();

    UnifiedGCMultiVMTestSuite testVM1(stsVMInterface.get(), &barrier);
    UnifiedGCMultiVMTestSuite testVM2(stsVMInterface.get(), &barrier);
    UnifiedGCMultiVMTestSuite testVM3(stsVMInterface.get(), &barrier);
    std::thread t1(&UnifiedGCMultiVMTestSuite::NoHandshakeTest, testVM1);
    std::thread t2(&UnifiedGCMultiVMTestSuite::NoHandshakeTest, testVM2);
    std::thread t3(&UnifiedGCMultiVMTestSuite::NoHandshakeTest, testVM3);
    {
        ThreadSuspensionScope suspensionScope(thread);
        barrier.PassStrongly();
        barrier.Wait();
    }

    void *ecmaVMInterface = nullptr;
    CrossVMOperator::DoHandshake(vm, stsVMInterface.get(), &ecmaVMInterface);
    CrossReferenceObjectBuilder CrossReferenceObject(vm, thread);
    SharedHeap::GetInstance()->TriggerUnifiedGCMark<TriggerGCType::UNIFIED_GC, GCReason::CROSSREF_CAUSE>(thread);
    while (!thread->HasSuspendRequest()) {}
    thread->WaitSuspension();
    CrossReferenceObject.CheckResultAfterUnifiedGC();
    t1.join();
    t2.join();
    t3.join();
}

HWTEST_F_L0(UnifiedGCMultiVMTest, MultiVMUnifiedGCMarkTest3)
{
    SuspendBarrier barrier(INT_VALUE_4);
    EcmaVM *vm = thread->GetEcmaVM();
    auto stsVMInterface = std::make_unique<STSVMInterfaceTest>();

    UnifiedGCMultiVMTestSuite testVM1(stsVMInterface.get(), &barrier);
    UnifiedGCMultiVMTestSuite testVM2(stsVMInterface.get(), &barrier);
    UnifiedGCMultiVMTestSuite testVM3(stsVMInterface.get(), &barrier);
    std::thread t1(&UnifiedGCMultiVMTestSuite::TriggerConcurrentMarkTest, testVM1);
    std::thread t2(&UnifiedGCMultiVMTestSuite::TriggerConcurrentMarkTest, testVM2);
    std::thread t3(&UnifiedGCMultiVMTestSuite::TriggerConcurrentMarkTest, testVM3);
    {
        ThreadSuspensionScope suspensionScope(thread);
        barrier.PassStrongly();
        barrier.Wait();
    }

    void *ecmaVMInterface = nullptr;
    CrossVMOperator::DoHandshake(vm, stsVMInterface.get(), &ecmaVMInterface);
    CrossReferenceObjectBuilder CrossReferenceObject(vm, thread);
    auto heap = vm->GetHeap();
    heap->GetConcurrentMarker()->Mark();
    SharedHeap::GetInstance()->TriggerUnifiedGCMark<TriggerGCType::UNIFIED_GC, GCReason::CROSSREF_CAUSE>(thread);
    while (!thread->HasSuspendRequest()) {}
    thread->WaitSuspension();
    CrossReferenceObject.CheckResultAfterUnifiedGC();
    t1.join();
    t2.join();
    t3.join();
}

HWTEST_F_L0(UnifiedGCMultiVMTest, MultiVMUnifiedGCMarkTest4)
{
    SuspendBarrier barrier(INT_VALUE_4);
    EcmaVM *vm = thread->GetEcmaVM();
    auto stsVMInterface = std::make_unique<STSVMInterfaceTest>();

    UnifiedGCMultiVMTestSuite testVM1(stsVMInterface.get(), &barrier);
    UnifiedGCMultiVMTestSuite testVM2(stsVMInterface.get(), &barrier);
    UnifiedGCMultiVMTestSuite testVM3(stsVMInterface.get(), &barrier);
    std::thread t1(&UnifiedGCMultiVMTestSuite::CheckUnifiedGCResultTest, testVM1);
    std::thread t2(&UnifiedGCMultiVMTestSuite::NoHandshakeTest, testVM2);
    std::thread t3(&UnifiedGCMultiVMTestSuite::TriggerConcurrentMarkTest, testVM3);
    {
        ThreadSuspensionScope suspensionScope(thread);
        barrier.PassStrongly();
        barrier.Wait();
    }

    void *ecmaVMInterface = nullptr;
    CrossVMOperator::DoHandshake(vm, stsVMInterface.get(), &ecmaVMInterface);
    CrossReferenceObjectBuilder CrossReferenceObject(vm, thread);
    auto heap = vm->GetHeap();
    heap->GetConcurrentMarker()->Mark();
    SharedHeap::GetInstance()->TriggerUnifiedGCMark<TriggerGCType::UNIFIED_GC, GCReason::CROSSREF_CAUSE>(thread);
    while (!thread->HasSuspendRequest()) {}
    thread->WaitSuspension();
    CrossReferenceObject.CheckResultAfterUnifiedGC();
    t1.join();
    t2.join();
    t3.join();
}

HWTEST_F_L0(UnifiedGCMultiVMTest, MultiVMUnifiedGCMarkTest5)
{
    SuspendBarrier barrier(INT_VALUE_4);
    EcmaVM *vm = thread->GetEcmaVM();
    auto stsVMInterface = std::make_unique<STSVMInterfaceTest>();

    UnifiedGCMultiVMTestSuite testVM1(stsVMInterface.get(), &barrier);
    UnifiedGCMultiVMTestSuite testVM2(stsVMInterface.get(), &barrier);
    UnifiedGCMultiVMTestSuite testVM3(stsVMInterface.get(), &barrier);
    std::thread t1(&UnifiedGCMultiVMTestSuite::TriggerYoungGCTest, testVM1);
    std::thread t2(&UnifiedGCMultiVMTestSuite::TriggerYoungGCTest, testVM2);
    std::thread t3(&UnifiedGCMultiVMTestSuite::TriggerYoungGCTest, testVM3);
    {
        ThreadSuspensionScope suspensionScope(thread);
        barrier.PassStrongly();
        barrier.Wait();
    }

    void *ecmaVMInterface = nullptr;
    CrossVMOperator::DoHandshake(vm, stsVMInterface.get(), &ecmaVMInterface);
    CrossReferenceObjectBuilder CrossReferenceObject(vm, thread);
    SharedHeap::GetInstance()->TriggerUnifiedGCMark<TriggerGCType::UNIFIED_GC, GCReason::CROSSREF_CAUSE>(thread);
    while (!thread->HasSuspendRequest()) {}
    thread->WaitSuspension();
    vm->CollectGarbage(TriggerGCType::YOUNG_GC);
    CrossReferenceObject.CheckResultAfterUnifiedGC();
    t1.join();
    t2.join();
    t3.join();
}

HWTEST_F_L0(UnifiedGCMultiVMTest, MultiVMMarkFromObjectTest)
{
    SuspendBarrier barrier(INT_VALUE_4);
    EcmaVM *vm = thread->GetEcmaVM();
    auto stsVMInterface = std::make_unique<STSVMInterfaceTest>();

    UnifiedGCMultiVMTestSuite testVM1(stsVMInterface.get(), &barrier);
    UnifiedGCMultiVMTestSuite testVM2(stsVMInterface.get(), &barrier);
    UnifiedGCMultiVMTestSuite testVM3(stsVMInterface.get(), &barrier);
    std::thread t1(&UnifiedGCMultiVMTestSuite::MarkFromObjectTest, testVM1);
    std::thread t2(&UnifiedGCMultiVMTestSuite::MarkFromObjectTest, testVM2);
    std::thread t3(&UnifiedGCMultiVMTestSuite::MarkFromObjectTest, testVM3);
    {
        ThreadSuspensionScope suspensionScope(thread);
        barrier.PassStrongly();
        barrier.Wait();
    }

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
    t1.join();
    t2.join();
    t3.join();
}

HWTEST_F_L0(UnifiedGCMultiVMTest, MultiVMTriggerUnifiedGCFailTest)
{
    SuspendBarrier barrier(INT_VALUE_4);
    EcmaVM *vm = thread->GetEcmaVM();
    auto stsVMInterface = std::make_unique<STSVMInterfaceTest>();

    UnifiedGCMultiVMTestSuite testVM1(stsVMInterface.get(), &barrier);
    UnifiedGCMultiVMTestSuite testVM2(stsVMInterface.get(), &barrier);
    UnifiedGCMultiVMTestSuite testVM3(stsVMInterface.get(), &barrier);
    std::thread t1(&UnifiedGCMultiVMTestSuite::TriggerUnifiedGCFailTest, testVM1);
    std::thread t2(&UnifiedGCMultiVMTestSuite::TriggerUnifiedGCFailTest, testVM2);
    std::thread t3(&UnifiedGCMultiVMTestSuite::TriggerUnifiedGCFailTest, testVM3);
    {
        ThreadSuspensionScope suspensionScope(thread);
        barrier.PassStrongly();
        barrier.Wait();
    }

    void *ecmaVMInterface = nullptr;
    CrossVMOperator::DoHandshake(vm, stsVMInterface.get(), &ecmaVMInterface);
    CrossReferenceObjectBuilder CrossReferenceObject(vm, thread);
    SharedHeap::GetInstance()->TriggerUnifiedGCMark<TriggerGCType::UNIFIED_GC, GCReason::CROSSREF_CAUSE>(thread);
    vm->GetCrossVMOperator()->GetEcmaVMInterface()->NotifyXGCInterruption();
    while (!thread->HasSuspendRequest()) {}
    thread->WaitSuspension();
    CrossReferenceObject.CheckResultAfterUnifiedGCTriggerFail();
    t1.join();
    t2.join();
    t3.join();
}

HWTEST_F_L0(UnifiedGCMultiVMVerificationTest, VerifyMultiVMUnifiedGCMarkTest)
{
    SuspendBarrier barrier(INT_VALUE_4);
    EcmaVM *vm = thread->GetEcmaVM();
    auto stsVMInterface = std::make_unique<STSVMInterfaceTest>();

    UnifiedGCMultiVMTestSuite testVM1(stsVMInterface.get(), &barrier);
    UnifiedGCMultiVMTestSuite testVM2(stsVMInterface.get(), &barrier);
    UnifiedGCMultiVMTestSuite testVM3(stsVMInterface.get(), &barrier);
    std::thread t1(&UnifiedGCMultiVMTestSuite::HeapVerifyTest, testVM1);
    std::thread t2(&UnifiedGCMultiVMTestSuite::HeapVerifyTest, testVM2);
    std::thread t3(&UnifiedGCMultiVMTestSuite::HeapVerifyTest, testVM3);
    {
        ThreadSuspensionScope suspensionScope(thread);
        barrier.PassStrongly();
        barrier.Wait();
    }

    void *ecmaVMInterface = nullptr;
    CrossVMOperator::DoHandshake(vm, stsVMInterface.get(), &ecmaVMInterface);
    CrossReferenceObjectBuilder CrossReferenceObject(vm, thread);
    auto heap = vm->GetHeap();
    heap->GetConcurrentMarker()->Mark();
    SharedHeap::GetInstance()->TriggerUnifiedGCMark<TriggerGCType::UNIFIED_GC, GCReason::CROSSREF_CAUSE>(thread);
    while (!thread->HasSuspendRequest()) {}
    thread->WaitSuspension();
    CrossReferenceObject.CheckResultAfterUnifiedGC();
    t1.join();
    t2.join();
    t3.join();
}
#endif  // PANDA_JS_ETS_HYBRID_MODE
}  // namespace panda::test