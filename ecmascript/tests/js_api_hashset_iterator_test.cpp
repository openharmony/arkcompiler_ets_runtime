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
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_api/js_api_hashset.h"
#include "ecmascript/js_api/js_api_hashset_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSAPIHashSetIteratorTest : public BaseTestWithScope<false> {
protected:
    JSAPIHashSet *CreateHashSet()
    {
        return EcmaContainerCommon::CreateHashSet(thread);
    }
};

/**
 * @tc.name: Next and CreateHashSetIterator
 * @tc.desc: test special return of Next and CreateHashSetIterator,
 *           including throw exception and return undefined
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIHashSetIteratorTest, SpecailReturnOfNextCreateHashSetIterator)
{
    JSHandle<JSAPIHashSet> jsHashSet(thread, CreateHashSet());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIHashSetIterator> hashSetIterator = factory->NewJSAPIHashSetIterator(
        jsHashSet, IterationKind::KEY);
    hashSetIterator->SetIteratedHashSet(thread, JSTaggedValue::Undefined());

    // test Next exception
    {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPIHashSetIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::Exception());
        EXPECT_EXCEPTION();
    }

    // test Next return undefined
    {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(hashSetIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPIHashSetIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, thread->GetEcmaVM()->GetGlobalEnv()->GetUndefinedIteratorResult().GetTaggedValue());
    }

    // test CreateHashSetIterator exception
    {
        JSHandle<JSTaggedValue> undefined(thread, JSTaggedValue::Undefined());
        JSHandle<JSTaggedValue> result =
            JSAPIHashSetIterator::CreateHashSetIterator(thread, undefined, IterationKind::KEY);
        EXPECT_EQ(result.GetTaggedValue(), JSTaggedValue::Exception());
        EXPECT_EXCEPTION();
    }
}
}  // namespace panda::test
