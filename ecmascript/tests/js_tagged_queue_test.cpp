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
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/tagged_queue.h"
#include "ecmascript/tagged_queue.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class JSTaggedQueueTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(JSTaggedQueueTest, Create)
{
    JSHandle<TaggedQueue> queue = thread->GetEcmaVM()->GetFactory()->NewTaggedQueue(0);
    EXPECT_TRUE(*queue != nullptr);
    EXPECT_TRUE(queue->Empty(thread));
    EXPECT_EQ(queue->Size(thread), 0U);
    EXPECT_EQ(queue->Front(thread), JSTaggedValue::Hole());
    EXPECT_EQ(queue->Back(thread), JSTaggedValue::Hole());
}

HWTEST_F_L0(JSTaggedQueueTest, PopAndPush)
{
    JSHandle<TaggedQueue> queue = thread->GetEcmaVM()->GetFactory()->NewTaggedQueue(0);
    EXPECT_TRUE(queue->Empty(thread));

    JSHandle<TaggedQueue> queue2(thread,
                                 TaggedQueue::Push(thread, queue, JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))));
    EXPECT_FALSE(queue2->Empty(thread));
    EXPECT_EQ(queue2->Size(thread), 1U);
    EXPECT_EQ(queue2->Front(thread), JSTaggedValue(0));
    EXPECT_EQ(queue2->Back(thread), JSTaggedValue(0));

    JSHandle<TaggedQueue> queue3(thread,
                                 TaggedQueue::Push(thread, queue2, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1))));
    EXPECT_EQ(queue3->Size(thread), 2U);
    EXPECT_EQ(queue3->Front(thread), JSTaggedValue(0));
    EXPECT_EQ(queue3->Back(thread), JSTaggedValue(1));
    EXPECT_NE(queue3.GetTaggedValue(), queue2.GetTaggedValue());

    JSHandle<TaggedQueue> queue4(thread,
                                 TaggedQueue::Push(thread, queue3, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2))));
    EXPECT_EQ(queue4->Size(thread), 3U);
    EXPECT_EQ(queue4->Front(thread), JSTaggedValue(0));
    EXPECT_EQ(queue4->Back(thread), JSTaggedValue(2));
    EXPECT_NE(queue4.GetTaggedValue(), queue3.GetTaggedValue());

    JSHandle<TaggedQueue> queue5(thread,
                                 TaggedQueue::Push(thread, queue4, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3))));
    EXPECT_EQ(queue5->Size(thread), 4U);
    EXPECT_EQ(queue5->Front(thread), JSTaggedValue(0));
    EXPECT_EQ(queue5->Back(thread), JSTaggedValue(3));
    EXPECT_NE(queue5.GetTaggedValue(), queue4.GetTaggedValue());

    EXPECT_EQ(queue5->Pop(thread), JSTaggedValue(0));
    EXPECT_EQ(queue5->Size(thread), 3U);
    EXPECT_EQ(queue5->Front(thread), JSTaggedValue(1));

    EXPECT_EQ(queue5->Pop(thread), JSTaggedValue(1));
    EXPECT_EQ(queue5->Size(thread), 2U);
    EXPECT_EQ(queue5->Front(thread), JSTaggedValue(2));

    EXPECT_EQ(queue5->Pop(thread), JSTaggedValue(2));
    EXPECT_EQ(queue5->Size(thread), 1U);
    EXPECT_EQ(queue5->Front(thread), JSTaggedValue(3));

    EXPECT_EQ(queue5->Pop(thread), JSTaggedValue(3));
    EXPECT_EQ(queue5->Size(thread), 0U);
    EXPECT_EQ(queue5->Front(thread), JSTaggedValue::Hole());
    EXPECT_TRUE(queue5->Empty(thread));
}
}  // namespace panda::test
