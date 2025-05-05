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

#include "containerslightweightmapsetandget_fuzzer.h"

#include "ecmascript/containers/containers_lightweightmap.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::test;
using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

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

    JSTaggedValue InitializeLightWeightMapConstructor(JSThread *thread)
    {
        auto factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::LightWeightMap)));
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);

        return result;
    }
    
    JSHandle<JSAPILightWeightMap> CreateJSAPILightWeightMap(JSThread *thread)
    {
        JSHandle<JSFunction> newTarget(thread, InitializeLightWeightMapConstructor(thread));
        auto objCallInfo = CreateEcmaRuntimeCallInfo(thread, 4);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());

        JSTaggedValue result = ContainersLightWeightMap::LightWeightMapConstructor(objCallInfo);
        JSHandle<JSAPILightWeightMap> map(thread, result);
        return map;
    }

    void ContainersLightWeightMapSetAndGetFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();

            if (size <= 0) {
                return;
            }
            double input = 0;
            const double maxByteLen = 4;
            if (size > maxByteLen) {
                size = maxByteLen;
            }
            if (memcpy_s(&input, maxByteLen, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPILightWeightMap> lightWeightMap = CreateJSAPILightWeightMap(thread);

            EcmaRuntimeCallInfo *callInfo1 = CreateEcmaRuntimeCallInfo(thread, 8);
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(lightWeightMap.GetTaggedValue());
            callInfo1->SetCallArg(0, JSTaggedValue(input));
            callInfo1->SetCallArg(1, JSTaggedValue(input + 1));
            ContainersLightWeightMap::Set(callInfo1);
            
            EcmaRuntimeCallInfo *callInfo2 = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo2->SetFunction(JSTaggedValue::Undefined());
            callInfo2->SetThis(lightWeightMap.GetTaggedValue());
            callInfo2->SetCallArg(0, JSTaggedValue(input));
            ContainersLightWeightMap::Get(callInfo2);

            {
                EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
                callInfo->SetFunction(JSTaggedValue::Undefined());
                callInfo->SetThis(lightWeightMap.GetTaggedValue());
                callInfo->SetCallArg(0, JSTaggedValue(input));
                ContainersLightWeightMap::GetIndexOfKey(callInfo);
            }

            {
                EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
                callInfo->SetFunction(JSTaggedValue::Undefined());
                callInfo->SetThis(lightWeightMap.GetTaggedValue());
                callInfo->SetCallArg(0, JSTaggedValue(input));
                ContainersLightWeightMap::GetIndexOfValue(callInfo);
            }
        }
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::ContainersLightWeightMapSetAndGetFuzzTest(data, size);
    return 0;
}