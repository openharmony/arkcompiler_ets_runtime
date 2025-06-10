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

#ifndef CONTAINERSPRIVATECOMMON_FUZZER_H
#define CONTAINERSPRIVATECOMMON_FUZZER_H

#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
class ContainersPrivateFuzzTestHelper {
public:
    static JSFunction *JSObjectCreate(JSThread *thread)
    {
        EcmaVM *ecmaVM = thread->GetEcmaVM();
        JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
        return globalEnv->GetObjectFunction().GetObject<JSFunction>();
    }

    static EcmaRuntimeCallInfo *CreateEcmaRuntimeCallInfo(JSThread *thread, uint32_t numArgs)
    {
        auto factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSTaggedValue> hclass(thread, JSObjectCreate(thread));
        JSHandle<JSTaggedValue> callee(factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        EcmaRuntimeCallInfo *objCallInfo =
            EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, callee, undefined, numArgs);
        return objCallInfo;
    }

    static void ContainersPrivateCommonFuzzTest([[maybe_unused]] const uint8_t* data, size_t size, uint8_t tag)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();

            if (size <= 0) {
                return;
            }

            auto factory = thread->GetEcmaVM()->GetFactory();
            JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
            JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
            JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
            JSHandle<JSTaggedValue> value =
                JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

            auto objCallInfo = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            objCallInfo->SetFunction(JSTaggedValue::Undefined());
            objCallInfo->SetThis(value.GetTaggedValue());
            objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(tag))); // 0 means the argument
            containers::ContainersPrivate::Load(objCallInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }
};
}
#endif