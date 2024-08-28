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

#include "ecmascript/builtins/builtins_ark_tools.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/full_gc.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/stw_young_gc.h"
#include "ecmascript/mem/partial_gc.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class GCTest : public BaseTestWithScope<false> {
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

HWTEST_F_L0(GCTest, ArkToolsHintGC)
{
    Heap *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::CONFIG_DISABLE);
    auto getSizeAfterCreateAndCallHintGC = [this, heap] (size_t &newSize, size_t &finalSize) -> bool {
        {
            [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
            for (int i = 0; i < 500; i++) {
                [[maybe_unused]] JSHandle<TaggedArray> obj = thread->GetEcmaVM()->GetFactory()->
                    NewTaggedArray(10 * 1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
            }
            newSize = heap->GetCommittedSize();
        }

        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 0);
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = builtins::BuiltinsArkTools::HintGC(ecmaRuntimeCallInfo);
        finalSize = heap->GetCommittedSize();
        TestHelper::TearDownFrame(thread, prev);

        return result.ToBoolean();
    };
    {
        // Test HintGC() when sensitive.
        heap->CollectGarbage(TriggerGCType::FULL_GC);
        heap->NotifyHighSensitive(true);
        size_t originSize = heap->GetCommittedSize();
        size_t newSize = 0;
        size_t finalSize = 0;
        bool res = getSizeAfterCreateAndCallHintGC(newSize, finalSize);
        EXPECT_FALSE(res);
        EXPECT_TRUE(newSize > originSize);
        EXPECT_TRUE(finalSize == newSize);
        heap->NotifyHighSensitive(false);
    }
    {
        // Test HintGC() when in background.
        heap->CollectGarbage(TriggerGCType::FULL_GC);
        heap->ChangeGCParams(true);
        size_t originSize = heap->GetCommittedSize();
        size_t newSize = 0;
        size_t finalSize = 0;
        bool res = getSizeAfterCreateAndCallHintGC(newSize, finalSize);
        EXPECT_TRUE(res);
        EXPECT_TRUE(newSize > originSize);
        EXPECT_TRUE(finalSize < newSize);
        heap->ChangeGCParams(false);
    }
}

HWTEST_F_L0(GCTest, LargeOverShootSizeTest)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    size_t originalYoungSize = heap->GetNewSpace()->GetCommittedSize();

    EXPECT_FALSE(heap->GetNewSpace()->CommittedSizeIsLarge());
    heap->GetConcurrentMarker()->ConfigConcurrentMark(false);
    heap->NotifyHighSensitive(true);
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 500; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                10 * 1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        }
    }
    size_t newYoungSize = heap->GetNewSpace()->GetCommittedSize();
    size_t originalOverShootSize = heap->GetNewSpace()->GetOvershootSize();
    EXPECT_TRUE(heap->GetNewSpace()->CommittedSizeIsLarge());
    EXPECT_TRUE(originalYoungSize < newYoungSize);

    heap->NotifyHighSensitive(false);
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    newYoungSize = heap->GetNewSpace()->GetCommittedSize();
    size_t newOverShootSize = heap->GetNewSpace()->GetOvershootSize();

    EXPECT_TRUE(originalYoungSize < newYoungSize);
    EXPECT_TRUE(originalOverShootSize < newOverShootSize);
    EXPECT_TRUE(0 < newOverShootSize);

    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 2049; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(
                1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        }
    }
    size_t newSize = heap->GetNewSpace()->GetCommittedSize();
    size_t newShootSize = heap->GetNewSpace()->GetOvershootSize();
    EXPECT_TRUE(originalYoungSize <= newSize);
    EXPECT_TRUE(newYoungSize > newSize);
    EXPECT_TRUE(newOverShootSize > newShootSize);
}
} // namespace panda::test
