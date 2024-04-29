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
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda::ecmascript;

namespace panda::test {
class WeakRefSemiGCTest : public BaseTestWithScope<false> {
};

static TaggedArray *ArrayTestCreate(JSThread *thread)
{
    [[maybe_unused]] ecmascript::EcmaHandleScope scope(thread);
    // 2 : test case
    JSHandle<TaggedArray> array = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(2);
    return *array;
}

HWTEST_F_L0(WeakRefSemiGCTest, ArrayUndefined)
{
    EcmaTestCommon::ArrayUndefinedGcCommon(thread, TriggerGCType::YOUNG_GC);
}

HWTEST_F_L0(WeakRefSemiGCTest, ArrayKeep)
{
    EcmaTestCommon::ArrayKeepGcCommon(thread, TriggerGCType::YOUNG_GC);
}

HWTEST_F_L0(WeakRefSemiGCTest, ObjectUndefined)
{
    JSHandle<JSObject> newObj1(thread, EcmaContainerCommon::JSObjectTestCreate(thread));
    JSTaggedValue array(ArrayTestCreate(thread));
    array.CreateWeakRef();
    newObj1->SetElements(thread, array);
    EXPECT_EQ(newObj1->GetElements(), array);
    thread->GetEcmaVM()->CollectGarbage(TriggerGCType::YOUNG_GC);
    EXPECT_EQ(newObj1->GetElements(), JSTaggedValue::Undefined());
}

HWTEST_F_L0(WeakRefSemiGCTest, ObjectKeep)
{
    JSHandle<JSObject> newObj1(thread, EcmaContainerCommon::JSObjectTestCreate(thread));
    JSHandle<TaggedArray> array(thread, ArrayTestCreate(thread));
    JSTaggedValue value = array.GetTaggedValue();
    value.CreateWeakRef();
    newObj1->SetElements(thread, value);
    EXPECT_EQ(newObj1->GetElements(), value);
    thread->GetEcmaVM()->CollectGarbage(TriggerGCType::YOUNG_GC);
    value = array.GetTaggedValue();
    value.CreateWeakRef();
    EXPECT_EQ(newObj1->GetElements(), value);
}
}  // namespace panda::test
