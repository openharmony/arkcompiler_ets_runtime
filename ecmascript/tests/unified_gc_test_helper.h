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

#ifndef ECMASCRIPT_TESTS_UNIFIED_GC_TEST_HELPER_H
#define ECMASCRIPT_TESTS_UNIFIED_GC_TEST_HELPER_H

#include <vector>

#ifdef PANDA_JS_ETS_HYBRID_MODE
#include "ecmascript/cross_vm/cross_vm_operator.h"
#endif  // PANDA_JS_ETS_HYBRID_MODE
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

constexpr int32_t INT_VALUE_0 = 0;
constexpr int32_t INT_VALUE_1 = 1;
constexpr int32_t INT_VALUE_2 = 2;
constexpr int32_t INT_VALUE_4 = 4;

namespace panda::test {
using panda::ecmascript::EcmaVM;
using panda::ecmascript::JSArray;
using panda::ecmascript::JSTaggedNumber;
using panda::ecmascript::JSObject;
using panda::ecmascript::ECMAObject;
using panda::ecmascript::JSTaggedType;
using panda::ecmascript::JSTaggedValue;
using panda::ecmascript::JSThread;

#ifdef PANDA_JS_ETS_HYBRID_MODE
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

    bool StartXGCBarrier(const NoWorkPred &func) override
    {
        return !func || func();
    }
    bool WaitForConcurrentMark(const NoWorkPred &func) override
    {
        return !func || func();
    }
    void RemarkStartBarrier() override {}
    bool WaitForRemark(const NoWorkPred &func) override
    {
        return !func || func();
    }
    void FinishXGCBarrier() override {}
};

// Fake Cross Reference Object for Unified GC test
class CrossReferenceObjectBuilder {
public:
    CrossReferenceObjectBuilder(EcmaVM *vm, JSThread *thread)
    {
        [[maybe_unused]] EcmaHandleScope ecmaHandleScope(thread);
        size_t nativeBindingSize = INT_VALUE_0;

        // |JSObject in ArkTS               |SharedReference in STS  |
        // |--------------------------------|------------------------|
        // |RootSet(GlobalNodeList)         |                        |
        // |    |                           |                        |
        // |    v                           |                        |
        // |arrayInRoot                     |                        |
        // |    |                           |                        |
        // |    v                           |                        |
        // |jsXRefObjectRefByRoot --------- |--> sharedRefRoot       |
        // |                                |                        |
        // |jsXRefObjectNormal -------------|--> sharedRefNormal     |
        // |                                |                        |
        // |XRefRootSet(XRefGlobalNodeList) |                        |
        // |    |                           |                        |
        // |    v                           |                        |
        // |arrayInXRefRoot                 |                        |
        // |    |                           |                        |
        // |    v                           |                        |
        // |jsXRefObjectRefByXRefRoot ------|--> sharedRefXRefRoot   |
        // |--------------------------------|------------------------|
        SharedReferenceTest* sharedRefRoot = new SharedReferenceTest();
        SharedReferenceTest* sharedRefNormal = new SharedReferenceTest();
        SharedReferenceTest* sharedRefXRefRoot = new SharedReferenceTest();
        sharedRefsNeedMark_.push_back(sharedRefRoot);
        sharedRefsNoNeedMark_.push_back(sharedRefNormal);
        sharedRefsNoNeedMark_.push_back(sharedRefXRefRoot);

        JSHandle<JSTaggedValue> arrayInRoot = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_1));
        thread->NewGlobalHandle(arrayInRoot.GetTaggedType());
        JSHandle<JSObject> jsXRefObjectRefByRoot = vm->GetFactory()->NewJSXRefObject();
        JSArray::FastSetPropertyByValue(thread, arrayInRoot, INT_VALUE_0,
            JSHandle<JSTaggedValue>(jsXRefObjectRefByRoot));
        ECMAObject::SetNativePointerFieldCount(thread, jsXRefObjectRefByRoot, INT_VALUE_1);
        ECMAObject::SetNativePointerField(thread, jsXRefObjectRefByRoot, INT_VALUE_0,
            sharedRefRoot, nullptr, nullptr, nativeBindingSize);

        JSHandle<JSObject> jsXRefObjectNormal = vm->GetFactory()->NewJSXRefObject();
        ECMAObject::SetNativePointerFieldCount(thread, jsXRefObjectNormal, INT_VALUE_1);
        ECMAObject::SetNativePointerField(thread, jsXRefObjectNormal, INT_VALUE_0,
            sharedRefNormal, nullptr, nullptr, nativeBindingSize);

        JSHandle<JSTaggedValue> arrayInXRefRoot = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_1));
        thread->NewXRefGlobalHandle(arrayInXRefRoot.GetTaggedType());
        JSHandle<JSObject> jsXRefObjectRefByXRefRoot = vm->GetFactory()->NewJSXRefObject();
        JSArray::FastSetPropertyByValue(thread, arrayInXRefRoot, INT_VALUE_0,
            JSHandle<JSTaggedValue>(jsXRefObjectRefByXRefRoot));
        ECMAObject::SetNativePointerFieldCount(thread, jsXRefObjectRefByXRefRoot, INT_VALUE_1);
        ECMAObject::SetNativePointerField(thread, jsXRefObjectRefByXRefRoot, INT_VALUE_0,
            sharedRefXRefRoot, nullptr, nullptr, nativeBindingSize);
    }

    ~CrossReferenceObjectBuilder()
    {
        for (auto sharedRef : sharedRefsNeedMark_) {
            delete sharedRef;
        }
        for (auto sharedRef : sharedRefsNoNeedMark_) {
            delete sharedRef;
        }
    }

    void CheckResultAfterUnifiedGC()
    {
        for (auto sharedRef : sharedRefsNeedMark_) {
            EXPECT_TRUE(sharedRef->isMarked());
        }
        for (auto sharedRef : sharedRefsNoNeedMark_) {
            EXPECT_TRUE(!sharedRef->isMarked());
        }
    }

    void CheckResultAfterUnifiedGCTriggerFail()
    {
        for (auto sharedRef : sharedRefsNeedMark_) {
            EXPECT_TRUE(!sharedRef->isMarked());
        }
        for (auto sharedRef : sharedRefsNoNeedMark_) {
            EXPECT_TRUE(!sharedRef->isMarked());
        }
    }

private:
    std::vector<SharedReferenceTest*> sharedRefsNeedMark_;
    std::vector<SharedReferenceTest*> sharedRefsNoNeedMark_;
};
#endif  // PANDA_JS_ETS_HYBRID_MODE
}  // namespace panda::test

#endif  // ECMASCRIPT_TESTS_UNIFIED_GC_TEST_HELPER_H