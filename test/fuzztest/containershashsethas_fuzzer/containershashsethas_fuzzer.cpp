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

#include "containershashsethas_fuzzer.h"

#include "ecmascript/containers/containers_hashset.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

#define MAXBYTELEN sizeof(uint32_t)

namespace OHOS {

    JSFunction *JSObjectCreate(JSThread *thread)
    {
        EcmaVM *ecmaVM = thread->GetEcmaVM();
        JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
        return globalEnv->GetObjectFunction().GetObject<JSFunction>();
    }

    EcmaRuntimeCallInfo *CreateEcmaRuntimeCallInfo(JSThread *thread, uint32_t numArgs)
    {
        auto factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSTaggedValue> hclass(thread, JSObjectCreate(thread));
        JSHandle<JSTaggedValue> callee(factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        EcmaRuntimeCallInfo *objCallInfo =
            EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, callee, undefined, numArgs);
        return objCallInfo;
    }
    JSTaggedValue InitializeHashSetConstructor(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo = CreateEcmaRuntimeCallInfo(thread, 6);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::HashSet)));
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        return result;
    }

    JSHandle<JSAPIHashSet> CreateJSAPIHashSet(JSThread *thread)
    {
        JSHandle<JSFunction> newTarget(thread, InitializeHashSetConstructor(thread));
        auto objCallInfo = CreateEcmaRuntimeCallInfo(thread, 4);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());

        JSTaggedValue result = ContainersHashSet::HashSetConstructor(objCallInfo);
        JSHandle<JSAPIHashSet> map(thread, result);
        return map;
    }

    void ContainersHashSetHasFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();

            uint32_t input;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&input, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }

            JSHandle<JSAPIHashSet> hashSet = CreateJSAPIHashSet(thread);
            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(hashSet.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(input));
            [[maybe_unused]] JSTaggedValue resultAdd = ContainersHashSet::Add(callInfo);

            EcmaRuntimeCallInfo *callInfoHas = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfoHas->SetFunction(JSTaggedValue::Undefined());
            callInfoHas->SetThis(hashSet.GetTaggedValue());
            callInfoHas->SetCallArg(0, JSTaggedValue(input));
            [[maybe_unused]] JSTaggedValue resultHas = ContainersHashSet::Has(callInfoHas);
        }
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::ContainersHashSetHasFuzzTest(data, size);
    return 0;
}