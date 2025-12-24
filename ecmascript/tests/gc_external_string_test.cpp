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

#include "ecmascript/builtins/builtins_ark_tools.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/full_gc.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/partial_gc.h"
#include "ecmascript/serializer/serialize_chunk.h"
#include "ecmascript/tests/ecma_test_common.h"
#include "ecmascript/string/external_string.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class GCTest : public BaseTestWithScope<false> {
public:
    static constexpr uint32_t TYPE_INFO_SIZE = 4;
    static constexpr uint32_t UTF8_CACHED_DATA_SIZE = 4;
    static constexpr uint32_t UTF16_CACHED_DATA_SIZE = 8;
    static constexpr uint32_t UTF16_STRING_LENGTH = UTF16_CACHED_DATA_SIZE / sizeof(uint16_t);

    static inline void CallBackFn([[maybe_unused]] void *data, void *hint)
    {
        free(data);
    }

    void SetUp() override
    {
        JSRuntimeOptions options;
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

class TestData {
public:
    uint32_t GetCount()
    {
        return count_;
    }

    void AddCount()
    {
        count_++;
    }

private:
    uint32_t count_ {0U};
};

class ExternalData {
public:
    std::string GetName()
    {
        return name_;
    }

    std::string GetContext()
    {
        return context_;
    }

    void SetName(std::string name)
    {
        name_ = name;
    }

    void SetContext(std::string context)
    {
        context_ = context;
    }
private:
    std::string name_;
    std::string context_;
};

HWTEST_F_L0(GCTest, ExternalStringGCAddStringTest)
{
    auto sHeap = SharedHeap::GetInstance();
    EXPECT_EQ(sHeap->GetExternalStringTable()->GetListSize(), 0);
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        TestData *hint = new TestData();
        ExternalData *data = new ExternalData();
        EcmaString *ecmaString = EcmaStringAccessor::CreateFromExternalResource(
            instance, reinterpret_cast<void *>(data), UTF8_CACHED_DATA_SIZE,
            true, GCTest::CallBackFn, reinterpret_cast<void *>(hint));
        JSHandle<EcmaString> ecmaStrHandle(thread, ecmaString);
        EXPECT_NE(ecmaString, nullptr);
        EXPECT_EQ(sHeap->GetExternalStringTable()->GetListSize(), 1);
    }
 
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    EXPECT_EQ(sHeap->GetExternalStringTable()->GetListSize(), 0);
};

HWTEST_F_L0(GCTest, ExternalStringGCAddStringHandleTest)
{
    auto sHeap = SharedHeap::GetInstance();
    EXPECT_EQ(sHeap->GetExternalStringTable()->GetListSize(), 0);

    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        TestData *hint = new TestData();
        ExternalData *data = new ExternalData();
        EcmaString *ecmaString = EcmaStringAccessor::CreateFromExternalResource(
            instance, reinterpret_cast<void *>(data), UTF8_CACHED_DATA_SIZE,
            true, GCTest::CallBackFn, reinterpret_cast<void *>(hint));
        JSHandle<EcmaString> ecmaStrHandle(thread, ecmaString);
        EXPECT_NE(ecmaString, nullptr);
        EXPECT_EQ(sHeap->GetExternalStringTable()->GetListSize(), 1);

        sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
        EXPECT_EQ(sHeap->GetExternalStringTable()->GetListSize(), 1);
    }
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    EXPECT_EQ(sHeap->GetExternalStringTable()->GetListSize(), 0);
};

HWTEST_F_L0(GCTest, ExternalStringSharedFullGCCallBackSaveTest)
{
    auto sHeap = SharedHeap::GetInstance();
    TestData *hint = new TestData();
    auto callback = [] (void* data, void* hint) -> void {
        reinterpret_cast<TestData *>(hint)->AddCount();
        free(data);
    };
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    size_t oldCount = sHeap->GetExternalStringTable()->GetListSize();
    EXPECT_EQ(oldCount, 0);
    EXPECT_EQ(hint->GetCount(), 0);
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        for (int i = 0; i < 32; i++) {
            ExternalData *data = new ExternalData();
            EcmaString *ecmaString = EcmaStringAccessor::CreateFromExternalResource(
                instance, reinterpret_cast<void *>(data), UTF8_CACHED_DATA_SIZE,
                true, callback, reinterpret_cast<void *>(hint));

            JSHandle<EcmaString> ecmaStrHandle(thread, ecmaString);
            CachedExternalEcmaString *cachedEcmaString = CachedExternalEcmaString::Cast(ecmaString);
            CachedExternalString *cachedExternalString = cachedEcmaString->ToCachedExternalString();
            EXPECT_NE(cachedExternalString, nullptr);
        }
        EXPECT_EQ(hint->GetCount(), 0);
        EXPECT_EQ(sHeap->GetExternalStringTable()->GetListSize(), 32);

        sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);

        EXPECT_EQ(hint->GetCount(), 0);
        EXPECT_EQ(sHeap->GetExternalStringTable()->GetListSize(), 32);
    }
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    EXPECT_EQ(hint->GetCount(), 32);
    EXPECT_EQ(sHeap->GetExternalStringTable()->GetListSize(), 0);
};

HWTEST_F_L0(GCTest, ExternalStringSharedGCCallBackSaveTest)
{
    auto sHeap = SharedHeap::GetInstance();
    TestData *hint = new TestData();
    auto callback = [] (void* data, void* hint) -> void {
        reinterpret_cast<TestData *>(hint)->AddCount();
        free(data);
    };
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    size_t oldCount = sHeap->GetExternalStringTable()->GetListSize();
    EXPECT_EQ(oldCount, 0);
    EXPECT_EQ(hint->GetCount(), 0);
    {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        for (int i = 0; i < 32; i++) {
            ExternalData *data = new ExternalData();
            EcmaString *ecmaString = EcmaStringAccessor::CreateFromExternalResource(
                instance, reinterpret_cast<void *>(data), UTF8_CACHED_DATA_SIZE,
                true, callback, reinterpret_cast<void *>(hint));

            JSHandle<EcmaString> ecmaStrHandle(thread, ecmaString);
            CachedExternalEcmaString *cachedEcmaString = CachedExternalEcmaString::Cast(ecmaString);
            CachedExternalString *cachedExternalString = cachedEcmaString->ToCachedExternalString();
            EXPECT_NE(cachedExternalString, nullptr);
        }
        EXPECT_EQ(hint->GetCount(), 0);
        EXPECT_EQ(sHeap->GetExternalStringTable()->GetListSize(), 32);

        sHeap->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);

        EXPECT_EQ(hint->GetCount(), 0);
        EXPECT_EQ(sHeap->GetExternalStringTable()->GetListSize(), 32);
    }
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    EXPECT_EQ(hint->GetCount(), 32);
    EXPECT_EQ(sHeap->GetExternalStringTable()->GetListSize(), 0);
};

}
