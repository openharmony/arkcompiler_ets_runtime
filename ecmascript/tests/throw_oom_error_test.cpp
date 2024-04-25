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
#include "ecmascript/js_handle.h"
#include "ecmascript/mem/mem_common.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class ThrowOOMErrorTest : public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
        thread->GetEcmaVM()->SetEnableForceGC(false);
        const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->SetMarkType(MarkType::MARK_FULL);
    }
};

HWTEST_F_L0(ThrowOOMErrorTest, ThrowNonMovableOOMError)
{
#ifdef NDEBUG
    static constexpr size_t SIZE = 100_KB / 8;
    [[maybe_unused]] ecmascript::EcmaHandleScope scope(thread);
    for (int i = 0; i < 800; i++) {
        [[maybe_unused]] JSHandle<TaggedArray> array =
            thread->GetEcmaVM()->GetFactory()->NewTaggedArray(SIZE, JSTaggedValue::Hole(), MemSpaceType::NON_MOVABLE);
    }

    EXPECT_TRUE(thread->HasPendingException());
    JSType errorType = thread->GetException().GetTaggedObject()->GetClass()->GetObjectType();
    EXPECT_EQ(errorType, JSType::JS_OOM_ERROR);
#endif
}

HWTEST_F_L0(ThrowOOMErrorTest, ThrowOldSpaceMergeOOMError)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto oldSpace = heap->GetOldSpace();
    oldSpace->SetMaximumCapacity(6_MB);

    static constexpr size_t SIZE = 100_KB / 8;
    [[maybe_unused]] ecmascript::EcmaHandleScope scope(thread);

    for (int i = 0; i < 46; i++) {
        [[maybe_unused]] JSHandle<TaggedArray> array =
            thread->GetEcmaVM()->GetFactory()->NewTaggedArray(SIZE, JSTaggedValue::Hole(), MemSpaceType::OLD_SPACE);
    }

    EXPECT_TRUE(!thread->HasPendingException());

    for (int i = 0; i < 4; i++) {
        [[maybe_unused]] JSHandle<TaggedArray> array =
            thread->GetEcmaVM()->GetFactory()->NewTaggedArray(SIZE, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
    }

    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    heap->Prepare();
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    heap->Prepare();

    EXPECT_TRUE(thread->HasPendingException());
    JSType errorType = thread->GetException().GetTaggedObject()->GetClass()->GetObjectType();
    EXPECT_EQ(errorType, JSType::JS_OOM_ERROR);
}
}  // namespace panda::test
