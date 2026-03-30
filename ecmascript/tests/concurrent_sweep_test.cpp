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

using namespace panda::ecmascript;

namespace panda::test {
class ConcurrentSweepTest : public BaseTestWithScope<false> {
};

TEST_F(ConcurrentSweepTest, ConcurrentSweep)
{
    const uint8_t *utf8 = reinterpret_cast<const uint8_t *>("test");
    JSHandle<EcmaString> test1(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, utf8, 4, true)); // 4 : utf8 encoding length
    if (instance->IsInitialized()) {
        instance->CollectGarbage(ecmascript::TriggerGCType::OLD_GC);
    }
    JSHandle<EcmaString> test2(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, utf8, 4, true)); // 4 : utf8 encoding length
    ASSERT_EQ(EcmaStringAccessor(test1).GetLength(), 4U);
    ASSERT_NE(test1.GetTaggedValue().GetTaggedObject(), test2.GetTaggedValue().GetTaggedObject());
}

HWTEST_F_L0(ConcurrentSweepTest, EnableConcurrentSweepTest001)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto sweeper = heap->GetSweeper();
    sweeper->ConfigConcurrentSweep(false);
    sweeper->EnableConcurrentSweep(EnableConcurrentSweepType::ENABLE);
    EXPECT_TRUE(sweeper->IsConfigDisabled());
}

HWTEST_F_L0(ConcurrentSweepTest, EnableConcurrentSweepTest002)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto sweeper = heap->GetSweeper();
    sweeper->ConfigConcurrentSweep(true);
    sweeper->EnableConcurrentSweep(EnableConcurrentSweepType::ENABLE);
    EXPECT_TRUE(sweeper->ConcurrentSweepEnabled());
}

HWTEST_F_L0(ConcurrentSweepTest, EnableConcurrentSweepTest003)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto sweeper = heap->GetSweeper();
    sweeper->ConfigConcurrentSweep(true);
    sweeper->EnableConcurrentSweep(EnableConcurrentSweepType::DISABLE);
    heap->CollectGarbage(TriggerGCType::FULL_GC, GCReason::OTHER);
    sweeper->EnableConcurrentSweep(EnableConcurrentSweepType::ENABLE);
    EXPECT_TRUE(sweeper->ConcurrentSweepEnabled());
}

HWTEST_F_L0(ConcurrentSweepTest, EnableConcurrentSweepTest004)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto sweeper = heap->GetSweeper();
    sweeper->ConfigConcurrentSweep(true);
    sweeper->EnableConcurrentSweep(EnableConcurrentSweepType::DISABLE);
    sweeper->EnsureAllTaskFinished();
    sweeper->EnableConcurrentSweep(EnableConcurrentSweepType::DISABLE);
    EXPECT_TRUE(sweeper->IsDisabled());
}

HWTEST_F_L0(ConcurrentSweepTest, EnableConcurrentSweepTest005)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto sweeper = heap->GetSweeper();
    sweeper->ConfigConcurrentSweep(true);
    heap->CollectGarbage(TriggerGCType::FULL_GC, GCReason::OTHER);
    sweeper->EnableConcurrentSweep(EnableConcurrentSweepType::ENABLE);
    EXPECT_TRUE(sweeper->ConcurrentSweepEnabled());
}

HWTEST_F_L0(ConcurrentSweepTest, EnableConcurrentSweepTest006)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto sweeper = heap->GetSweeper();
    sweeper->ConfigConcurrentSweep(true);
    sweeper->EnsureAllTaskFinished();
    sweeper->EnableConcurrentSweep(EnableConcurrentSweepType::DISABLE);
    EXPECT_TRUE(sweeper->IsDisabled());
}

HWTEST_F_L0(ConcurrentSweepTest, EnableConcurrentSweepTest007)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto sweeper = heap->GetSweeper();
    sweeper->ConfigConcurrentSweep(true);
    sweeper->EnableConcurrentSweep(EnableConcurrentSweepType::DISABLE);
    heap->CollectGarbage(TriggerGCType::FULL_GC, GCReason::OTHER);
    sweeper->EnableConcurrentSweep(EnableConcurrentSweepType::DISABLE);
    EXPECT_TRUE(sweeper->IsDisabled());
}

HWTEST_F_L0(ConcurrentSweepTest, EnableConcurrentSweepTest008)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto sweeper = heap->GetSweeper();
    sweeper->ConfigConcurrentSweep(true);
    heap->CollectGarbage(TriggerGCType::FULL_GC, GCReason::OTHER);
    sweeper->EnableConcurrentSweep(EnableConcurrentSweepType::DISABLE);
    EXPECT_TRUE(sweeper->IsRequestDisabled());
}

HWTEST_F_L0(ConcurrentSweepTest, EnableConcurrentSweepTest009)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    auto sweeper = heap->GetSweeper();
    sweeper->ConfigConcurrentSweep(true);
    heap->CollectGarbage(TriggerGCType::FULL_GC, GCReason::OTHER);
    sweeper->EnableConcurrentSweep(EnableConcurrentSweepType::DISABLE);
    sweeper->EnsureAllTaskFinished();
    EXPECT_TRUE(sweeper->IsDisabled());
}
}  // namespace panda::test
