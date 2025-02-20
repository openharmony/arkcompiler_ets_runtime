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

#ifdef PANDA_JS_ETS_HYBRID_MODE
#include "ecmascript/cross_vm/cross_vm_operator.h"
#endif  // PANDA_JS_ETS_HYBRID_MODE
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/mem/unified_gc/unified_gc_marker.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
constexpr int32_t INT_VALUE_0 = 0;
constexpr int32_t INT_VALUE_1 = 1;
constexpr int32_t INT_VALUE_2 = 2;

namespace panda::test {

class UnifiedGCTest : public BaseTestWithScope<false> {
public:
#ifdef PANDA_JS_ETS_HYBRID_MODE
    // Fake STSVMInterface implement for Unified GC test
    class STSVMInterfaceTest final : public arkplatform::STSVMInterface {
    public:
        NO_COPY_SEMANTIC(STSVMInterfaceTest);
        NO_MOVE_SEMANTIC(STSVMInterfaceTest);
        STSVMInterfaceTest() = default;
        ~STSVMInterfaceTest() override = default;

        void MarkFromObject(void *ref) override
        {
            ASSERT(ref != nullptr);
            auto *sharedRef = static_cast<SharedReferenceTest *>(ref);
            sharedRef->MarkIfNotMarked();
        };

        void OnVMAttach() override {}
        void OnVMDetach() override {}

        void StartXGCBarrier() override {}
        bool WaitForConcurrentMark(const NoWorkPred &func) override
        {
            if (func && !func()) {
                return false;
            }
            return true;
        }
        void RemarkStartBarrier() override {}
        bool WaitForRemark(const NoWorkPred &func) override
        {
            if (func && !func()) {
                return false;
            }
            return true;
        }
        void FinishXGCBarrier() override {}
    };

    // Fake SharedReference implement for Unified GC test
    class SharedReferenceTest {
    public:
        bool MarkIfNotMarked()
        {
            if (!isMarked_) {
                isMarked_ = true;
                return true;
            }
            return false;
        }

        bool isMarked()
        {
            return isMarked_;
        }

    private:
        bool isMarked_ {false};
    };
#endif  // PANDA_JS_ETS_HYBRID_MODE

    bool IsObjectMarked(TaggedObject *object)
    {
        Region *objectRegion = Region::ObjectAddressToRange(object);
        return objectRegion->Test(object);
    }
};

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
        JSHandle<JSTaggedValue> xRefArray = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_1));
        JSHandle<JSTaggedValue> normalArray = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_1));
        thread->NewXRefGlobalHandle(xRefArray.GetTaggedType());
        thread->NewGlobalHandle(normalArray.GetTaggedType());
        weakRefArray->Set(thread, INT_VALUE_0, xRefArray.GetTaggedValue().CreateAndGetWeakRef());
        weakRefArray->Set(thread, INT_VALUE_1, normalArray.GetTaggedValue().CreateAndGetWeakRef());
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

HWTEST_F_L0(UnifiedGCTest, TriggerUnifiedGCMarkTest)
{
    EcmaVM *vm = thread->GetEcmaVM();
    auto stsVMInterface = std::make_unique<STSVMInterfaceTest>();
    void *ecmaVMInterface = nullptr;
    CrossVMOperator::DoHandshake(vm, stsVMInterface.get(), &ecmaVMInterface);

    // |JSObject in ArkTS        |SharedReference in STS     |
    // |-------------------------|---------------------------|
    // |RootSet(GlobalNodeList)  |                           |
    // |    |                    |                           |
    // |    v                    |                           |
    // |arrayInRoot              |                           |
    // |    |                    |                           |
    // |    v                    |                           |
    // |jsXRefObjectRefByRoot ---|---> sharedRefNeedMark     |
    // |                         |                           |
    // |jsXRefObjectNormal ------|---> sharedRefNoNeedMark   |
    // |-------------------------|---------------------------|
    SharedReferenceTest sharedRefNeedMark;
    SharedReferenceTest sharedRefNoNeedMark;
    size_t nativeBindingSize = INT_VALUE_0;
    {
        [[maybe_unused]] EcmaHandleScope ecmaHandleScope(thread);
        JSHandle<JSObject> jsXRefObjectRefByRoot = vm->GetFactory()->NewJSXRefObject();
        JSHandle<JSTaggedValue> arrayInRoot = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_1));
        thread->NewGlobalHandle(arrayInRoot.GetTaggedType());
        JSArray::FastSetPropertyByValue(thread, arrayInRoot, INT_VALUE_0,
            JSHandle<JSTaggedValue>(jsXRefObjectRefByRoot));
        ECMAObject::SetNativePointerFieldCount(thread, jsXRefObjectRefByRoot, INT_VALUE_1);
        ECMAObject::SetNativePointerField(thread, jsXRefObjectRefByRoot, INT_VALUE_0,
            &sharedRefNeedMark, nullptr, nullptr, nativeBindingSize);

        JSHandle<JSObject> jsXRefObjectNormal = vm->GetFactory()->NewJSXRefObject();
        ECMAObject::SetNativePointerFieldCount(thread, jsXRefObjectNormal, INT_VALUE_1);
        ECMAObject::SetNativePointerField(thread, jsXRefObjectNormal, INT_VALUE_0,
            &sharedRefNoNeedMark, nullptr, nullptr, nativeBindingSize);
    }
    auto heap = vm->GetHeap();
    heap->TriggerUnifiedGCMark<TriggerGCType::UNIFIED_GC, GCReason::CROSSREF_CAUSE>();
    while (!thread->HasSuspendRequest()) {}
    thread->CheckSafepoint();

    EXPECT_TRUE(sharedRefNeedMark.isMarked());
    EXPECT_TRUE(!sharedRefNoNeedMark.isMarked());
}
#endif  // PANDA_JS_ETS_HYBRID_MODE

HWTEST_F_L0(UnifiedGCTest, MarkFromObjectTest)
{
    EcmaVM *vm = thread->GetEcmaVM();
    vm->SetEnableForceGC(false);

    JSHandle<JSTaggedValue> arrayInXRefRoot = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_1));
    JSHandle<JSTaggedValue> arrayRefByXRefRoot = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_2));
    thread->NewXRefGlobalHandle(arrayInXRefRoot.GetTaggedType());
    JSArray::FastSetPropertyByValue(thread, arrayInXRefRoot, INT_VALUE_0, arrayRefByXRefRoot);

    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetUnifiedGCMarker()->MarkFromObject(arrayInXRefRoot->GetHeapObject());
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

#ifdef PANDA_JS_ETS_HYBRID_MODE
HWTEST_F_L0(UnifiedGCVerificationTest, VerifyTest)
{
    EcmaVM *vm = thread->GetEcmaVM();
    vm->SetEnableForceGC(false);
    auto stsVMInterface = std::make_unique<STSVMInterfaceTest>();
    void *ecmaVMInterface = nullptr;
    CrossVMOperator::DoHandshake(vm, stsVMInterface.get(), &ecmaVMInterface);
    auto heap = vm->GetHeap();
    heap->TriggerUnifiedGCMark<TriggerGCType::UNIFIED_GC, GCReason::CROSSREF_CAUSE>();
    while (!thread->HasSuspendRequest()) {}
    thread->CheckSafepoint();
    vm->SetEnableForceGC(true);
}
#endif  // PANDA_JS_ETS_HYBRID_MODE
}