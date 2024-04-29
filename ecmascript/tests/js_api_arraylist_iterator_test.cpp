/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_api/js_api_arraylist_iterator.h"
#include "ecmascript/global_env.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSAPIArrayListIteratorTest :  public BaseTestWithScope<false> {
protected:
    JSAPIArrayList *CreateArrayList()
    {
        return EcmaContainerCommon::CreateArrayList(thread);
    }
};

/**
 * @tc.name: Next
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIArrayListIteratorTest, Next)
{
    JSHandle<JSAPIArrayList> arrayList(thread, CreateArrayList());
    uint32_t elementsNum = 256;
    for (uint32_t i = 0; i < elementsNum; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIArrayList::Add(thread, arrayList, value);
    }
    JSHandle<JSAPIArrayListIterator> arrayListIterator(thread, JSAPIArrayList::GetIteratorObj(thread, arrayList));
    JSHandle<JSTaggedValue> valueStr = thread->GlobalConstants()->GetHandledValueString();
    uint32_t capacity = JSAPIArrayList::GetCapacity(thread, arrayList);
    for (uint32_t i = 0; i < capacity; i++) {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(arrayListIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPIArrayListIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSObject> resultObj(thread, result);
        if (i < elementsNum) {
            EXPECT_EQ(arrayListIterator->GetNextIndex(), i + 1U);
            EXPECT_EQ((JSObject::GetProperty(thread, resultObj, valueStr).GetValue()).GetTaggedValue(),
                JSTaggedValue(i));
        } else {
            EXPECT_EQ(arrayListIterator->GetIteratedArrayList(), JSTaggedValue::Undefined());
            EXPECT_EQ((JSObject::GetProperty(thread, resultObj, valueStr).GetValue()).GetTaggedValue(),
                JSTaggedValue::Undefined());
        }
    }

    // test Next exception
    {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPIArrayListIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::Exception());
        EXPECT_EXCEPTION();
    }
}
} // namespace panda::test
