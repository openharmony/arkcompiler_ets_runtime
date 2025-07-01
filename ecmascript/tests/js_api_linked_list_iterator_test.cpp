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
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_linked_list.h"
#include "ecmascript/js_api/js_api_linked_list_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_list.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;

using namespace panda::ecmascript;

using namespace panda::ecmascript::containers;

namespace panda::test {
class JSAPILinkedListIteratorTest : public BaseTestWithScope<false> {
protected:
    JSAPILinkedList *CreateLinkedList()
    {
        return EcmaContainerCommon::CreateLinkedList(thread);
    }
};

/**
 * @tc.name: Next and CreateLinkedListIterator
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPILinkedListIteratorTest, NextCreateLinkedListIterator)
{
    JSHandle<JSAPILinkedList> linkedList(thread, CreateLinkedList());
    uint32_t elementsNum = 256;
    for (uint32_t i = 0; i < elementsNum; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPILinkedList::Add(thread, linkedList, value);
    }
    JSHandle<JSTaggedValue> taggedValueHandle(thread, linkedList.GetTaggedValue());

    JSHandle<JSAPILinkedListIterator> linkedListIterator(
        thread, JSAPILinkedListIterator::CreateLinkedListIterator(thread, taggedValueHandle).GetTaggedValue());
    JSHandle<JSTaggedValue> valueStr = thread->GlobalConstants()->GetHandledValueString();
    uint32_t capacity = static_cast<uint32_t>(linkedList->Length(thread));
    for (uint32_t i = 0; i < capacity; i++) {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(linkedListIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPILinkedListIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSObject> resultObj(thread, result);
        if (i < elementsNum) {
            EXPECT_EQ(linkedListIterator->GetNextIndex(), i + 1U);
            EXPECT_EQ((JSObject::GetProperty(thread, resultObj, valueStr).GetValue()).GetTaggedValue(),
                JSTaggedValue(i));
        } else {
            EXPECT_EQ(linkedListIterator->GetIteratedLinkedList(thread), JSTaggedValue::Undefined());
            EXPECT_EQ((JSObject::GetProperty(thread, resultObj, valueStr).GetValue()).GetTaggedValue(),
                JSTaggedValue::Undefined());
        }
    }

    // test Next exception
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = JSAPILinkedListIterator::Next(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
    EXPECT_EXCEPTION();

    // test CreateLinkedListIterator exception
    JSHandle<JSTaggedValue> undefined(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> result1 = JSAPILinkedListIterator::CreateLinkedListIterator(thread, undefined);
    EXPECT_EQ(result1.GetTaggedValue(), JSTaggedValue::Exception());
    EXPECT_EXCEPTION();
}
}  // namespace panda::test
