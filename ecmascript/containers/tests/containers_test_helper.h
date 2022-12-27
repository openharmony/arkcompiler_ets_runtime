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

#ifndef ECMASCRIPT_TESTS_CONTAINERS_TEST_HELPER_H
#define ECMASCRIPT_TESTS_CONTAINERS_TEST_HELPER_H

#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_list.h"
#include "ecmascript/js_api/js_api_list_iterator.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_proxy.h"

namespace panda::ecmascript {
    static JSFunction *JSObjectTestCreate(JSThread *thread)
    {
        EcmaVM *ecmaVM = thread->GetEcmaVM();
        JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
        return globalEnv->GetObjectFunction().GetObject<JSFunction>();
    }
    [[maybe_unused]] static JSHandle<JSProxy> CreateJSProxyHandle(JSThread *thread)
    {
        JSHandle<JSTaggedValue> hclass(thread, JSObjectTestCreate(thread));
        JSHandle<JSTaggedValue> targetHandle(
            thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
        JSHandle<JSTaggedValue> handlerHandle(
            thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
        return JSProxy::ProxyCreate(thread, targetHandle, handlerHandle);
    }
} // namespace panda::ecmascript

namespace panda::test {
using panda::ecmascript::EcmaRuntimeCallInfo;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CONTAINERS_API_EXCEPTION_TEST(className, methodName, callInfoName)          \
{                                                                                   \
    [[maybe_unused]] auto testPrev = TestHelper::SetupFrame(thread, callInfoName);  \
    JSTaggedValue testResult = className::methodName(callInfoName);                 \
    TestHelper::TearDownFrame(thread, testPrev);                                    \
    EXPECT_EQ(testResult, JSTaggedValue::Exception());                              \
    EXPECT_EXCEPTION();                                                             \
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(className, methodName)   \
{                                                                            \
    auto callInfo = NewEmptyCallInfo(thread);                                \
    CONTAINERS_API_EXCEPTION_TEST(className, methodName, callInfo);          \
}

[[maybe_unused]] static EcmaRuntimeCallInfo* NewEmptyCallInfo(JSThread *thread)
{
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(JSTaggedValue::Undefined());
    callInfo->SetCallArg(0, JSTaggedValue::Hole());
    callInfo->SetCallArg(1, JSTaggedValue::Hole());
    return callInfo;
}
} // namespace panda::test

#endif // ECMASCRIPT_TESTS_CONTAINERS_TEST_HELPER_H
