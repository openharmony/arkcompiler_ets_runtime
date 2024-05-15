/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef TEST_COMMON_H
#define TEST_COMMON_H
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/global_env.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
namespace panda::test {
using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;
using ecmascript::base::BuiltinsBase;
using BuiltinsRegExp = builtins::BuiltinsRegExp;

class TestCommon {
public:
    static JSTaggedValue CreateContainerTaggedValue(JSThread *thread, ContainerTag tag)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);  // 6 means the value
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(tag)));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }

    static JSTaggedValue CreateJSRegexpByPatternAndFlags(JSThread *thread, const JSHandle<EcmaString> &pattern,
                                                         const JSHandle<EcmaString> &flags)
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> regexp(env->GetRegExpFunction());
        JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
        // 8 : test case
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*regexp), 8);
        ecmaRuntimeCallInfo->SetFunction(regexp.GetTaggedValue());
        ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(0, pattern.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(1, flags.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        // call RegExpConstructor method
        JSTaggedValue result = BuiltinsRegExp::RegExpConstructor(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }

    static JSTaggedValue TestCompareFunction(EcmaRuntimeCallInfo *argv)
    {
        JSThread *thread = argv->GetThread();
        JSHandle<JSTaggedValue> valueX = BuiltinsBase::GetCallArg(argv, 0);
        JSHandle<JSTaggedValue> valueY = BuiltinsBase::GetCallArg(argv, 1);

        if (valueX->IsString() && valueY->IsString()) {
            auto xHandle = JSHandle<EcmaString>(valueX);
            auto yHandle = JSHandle<EcmaString>(valueY);
            int result = EcmaStringAccessor::Compare(thread->GetEcmaVM(), xHandle, yHandle);
            if (result < 0) {
                return JSTaggedValue(1);
            }
            if (result == 0) {
                return JSTaggedValue(0);
            }
            return JSTaggedValue(-1);
        }

        if (valueX->IsNumber() && valueY->IsString()) {
            return JSTaggedValue(1);
        }
        if (valueX->IsString() && valueY->IsNumber()) {
            return JSTaggedValue(-1);
        }

        ComparisonResult res = ComparisonResult::UNDEFINED;
        if (valueX->IsNumber() && valueY->IsNumber()) {
            res = JSTaggedValue::StrictNumberCompare(valueY->GetNumber(), valueX->GetNumber());
        } else {
            res = JSTaggedValue::Compare(thread, valueY, valueX);
        }
        return res == ComparisonResult::GREAT ? JSTaggedValue(1)
                                              : (res == ComparisonResult::LESS ? JSTaggedValue(-1) : JSTaggedValue(0));
    }
};
};  // namespace panda::test

#endif
