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

#ifndef CONTAINERSTREEMAPCOMMON_FUZZER_H
#define CONTAINERSTREEMAPCOMMON_FUZZER_H

#include "ecmascript/containers/containers_hashmap.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/napi/include/jsnapi.h"

namespace panda::ecmascript {
using namespace panda::ecmascript::containers;
class ContainersHashMapFuzzTestHelper {
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
        JSHandle<JSTaggedValue> callee(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        EcmaRuntimeCallInfo *objCallInfo =
            EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, callee, undefined, numArgs);
        return objCallInfo;
    }

    static JSTaggedValue InitializeHashMapConstructor(JSThread *thread)
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
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::HashMap)));
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        return result;
    }

    static JSHandle<JSAPIHashMap> CreateJSAPIHashMap(JSThread *thread)
    {
        JSHandle<JSFunction> newTarget(thread, InitializeHashMapConstructor(thread));
        auto objCallInfo = CreateEcmaRuntimeCallInfo(thread, 4);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());

        JSTaggedValue result = ContainersHashMap::HashMapConstructor(objCallInfo);
        JSHandle<JSAPIHashMap> map(thread, result);
        return map;
    }

    static bool InitializeFuzzTest(const uint8_t *data, size_t size, double &input, EcmaVM *&vm,
                                   JSThread *&thread)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        vm = JSNApi::CreateJSVM(option);
        thread = vm->GetAssociatedJSThread();
        if (vm == nullptr || thread == nullptr) {
            return false;
        }
        if (size == 0) {
            JSNApi::DestroyJSVM(vm);
            return false;
        }

        size_t maxByteLen = sizeof(uint32_t);
        if (size > maxByteLen) {
            size = maxByteLen;
        }
        if (memcpy_s(&input, maxByteLen, data, size) != 0) {
            std::cout << "memcpy_s failed!";
            UNREACHABLE();
        }
        return true;
    }

    static void ContainersHashMapEntriesFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        double input = 0;
        if (!InitializeFuzzTest(data, size, input, vm, thread)) {
            return;
        }

        JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(input));
        callInfo->SetCallArg(1, JSTaggedValue(input));
        ContainersHashMap::Set(callInfo);

        auto callInfo2 = CreateEcmaRuntimeCallInfo(thread, 4);
        callInfo2->SetFunction(JSTaggedValue::Undefined());
        callInfo2->SetThis(tMap.GetTaggedValue());
        ContainersHashMap::Entries(callInfo2);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersHashMapFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        double input = 0;
        if (!InitializeFuzzTest(data, size, input, vm, thread)) {
            return;
        }
        JSHandle<JSAPIHashMap> stack = CreateJSAPIHashMap(thread);
        EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(stack.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(input));
        ContainersHashMap::HashMapConstructor(callInfo);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersHashMapClearFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        double input = 0;
        if (!InitializeFuzzTest(data, size, input, vm, thread)) {
            return;
        }

        JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(input));
        callInfo->SetCallArg(1, JSTaggedValue(input));
        ContainersHashMap::Set(callInfo);
        auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 4);
        callInfo1->SetFunction(JSTaggedValue::Undefined());
        callInfo1->SetThis(tMap.GetTaggedValue());
        ContainersHashMap::Clear(callInfo1);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersHashMapForEachFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        double input = 0;
        if (!InitializeFuzzTest(data, size, input, vm, thread)) {
            return;
        }

        JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(input));
        callInfo->SetCallArg(1, JSTaggedValue(input));
        ContainersHashMap::Set(callInfo);
        JSHandle<JSAPIHashMap> dMap = CreateJSAPIHashMap(thread);
        auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo1->SetFunction(JSTaggedValue::Undefined());
        callInfo1->SetThis(tMap.GetTaggedValue());
        callInfo1->SetCallArg(0, JSTaggedValue::Undefined());
        callInfo1->SetCallArg(1, dMap.GetTaggedValue());
        ContainersHashMap::ForEach(callInfo1);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersHashMapGetFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        double input = 0;
        if (!InitializeFuzzTest(data, size, input, vm, thread)) {
            return;
        }

        JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(input));
        callInfo->SetCallArg(1, JSTaggedValue(input + 1));
        ContainersHashMap::Set(callInfo);

        auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 6);
        callInfo1->SetFunction(JSTaggedValue::Undefined());
        callInfo1->SetThis(tMap.GetTaggedValue());
        callInfo1->SetCallArg(0, JSTaggedValue(input));
        ContainersHashMap::Get(callInfo1);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersHashMapGetLengthFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        double input = 0;
        if (!InitializeFuzzTest(data, size, input, vm, thread)) {
            return;
        }

        JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(input));
        callInfo->SetCallArg(1, JSTaggedValue(input));
        ContainersHashMap::Set(callInfo);

        auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 4);
        callInfo1->SetFunction(JSTaggedValue::Undefined());
        callInfo1->SetThis(tMap.GetTaggedValue());
        ContainersHashMap::GetLength(callInfo1);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersHashMapHasKeyFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        double input = 0;
        if (!InitializeFuzzTest(data, size, input, vm, thread)) {
            return;
        }

        JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(input));
        callInfo->SetCallArg(1, JSTaggedValue(input));
        ContainersHashMap::Set(callInfo);

        EcmaRuntimeCallInfo *callInfo1 = CreateEcmaRuntimeCallInfo(thread, 6);
        callInfo1->SetFunction(JSTaggedValue::Undefined());
        callInfo1->SetThis(tMap.GetTaggedValue());
        callInfo1->SetCallArg(0, JSTaggedValue(input));
        ContainersHashMap::HasKey(callInfo1);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersHashMapHasValueFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        double input = 0;
        if (!InitializeFuzzTest(data, size, input, vm, thread)) {
            return;
        }

        JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(input));
        callInfo->SetCallArg(1, JSTaggedValue(input));
        ContainersHashMap::Set(callInfo);

        EcmaRuntimeCallInfo *callInfo1 = CreateEcmaRuntimeCallInfo(thread, 6);
        callInfo1->SetFunction(JSTaggedValue::Undefined());
        callInfo1->SetThis(tMap.GetTaggedValue());
        callInfo1->SetCallArg(0, JSTaggedValue(input));
        ContainersHashMap::HasValue(callInfo1);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersHashMapIsEmptyFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        double input = 0;
        if (!InitializeFuzzTest(data, size, input, vm, thread)) {
            return;
        }

        JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(input));
        callInfo->SetCallArg(1, JSTaggedValue(input));
        ContainersHashMap::Set(callInfo);
        EcmaRuntimeCallInfo *callInfo2 = CreateEcmaRuntimeCallInfo(thread, 4);
        callInfo2->SetFunction(JSTaggedValue::Undefined());
        callInfo2->SetThis(tMap.GetTaggedValue());
        ContainersHashMap::IsEmpty(callInfo2);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersHashMapKeysFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        double input = 0;
        if (!InitializeFuzzTest(data, size, input, vm, thread)) {
            return;
        }

        JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(input));
        callInfo->SetCallArg(1, JSTaggedValue(input));
        ContainersHashMap::Set(callInfo);

        auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 4);
        callInfo1->SetFunction(JSTaggedValue::Undefined());
        callInfo1->SetThis(tMap.GetTaggedValue());
        ContainersHashMap::Keys(callInfo1);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersHashMapRemoveFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        double input = 0;
        if (!InitializeFuzzTest(data, size, input, vm, thread)) {
            return;
        }

        JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(input));
        callInfo->SetCallArg(1, JSTaggedValue(input));
        ContainersHashMap::Set(callInfo);

        auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 6);
        callInfo1->SetFunction(JSTaggedValue::Undefined());
        callInfo1->SetThis(tMap.GetTaggedValue());
        callInfo1->SetCallArg(0, JSTaggedValue(input + 1));
        ContainersHashMap::Remove(callInfo1);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersHashMapReplaceFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        double input = 0;
        if (!InitializeFuzzTest(data, size, input, vm, thread)) {
            return;
        }

        JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(input));
        callInfo->SetCallArg(1, JSTaggedValue(input));
        ContainersHashMap::Set(callInfo);

        EcmaRuntimeCallInfo *callInfo1 = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo1->SetFunction(JSTaggedValue::Undefined());
        callInfo1->SetThis(tMap.GetTaggedValue());
        callInfo1->SetCallArg(0, JSTaggedValue(input + 1));
        callInfo1->SetCallArg(1, JSTaggedValue(input));
        ContainersHashMap::Replace(callInfo1);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersHashMapSetFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        double input = 0;
        if (!InitializeFuzzTest(data, size, input, vm, thread)) {
            return;
        }

        JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(input));
        callInfo->SetCallArg(1, JSTaggedValue(input + 1));
        ContainersHashMap::Set(callInfo);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersHashMapSetAllFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        double input = 0;
        if (!InitializeFuzzTest(data, size, input, vm, thread)) {
            return;
        }

        JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(input));
        callInfo->SetCallArg(1, JSTaggedValue(input));
        ContainersHashMap::Set(callInfo);

        JSHandle<JSAPIHashMap> dMap = CreateJSAPIHashMap(thread);
        auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 6);
        callInfo1->SetFunction(JSTaggedValue::Undefined());
        callInfo1->SetThis(dMap.GetTaggedValue());
        callInfo1->SetCallArg(0, tMap.GetTaggedValue());
        ContainersHashMap::SetAll(callInfo1);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersHashMapValuesFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        double input = 0;
        if (!InitializeFuzzTest(data, size, input, vm, thread)) {
            return;
        }

        JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(input));
        callInfo->SetCallArg(1, JSTaggedValue(input));
        ContainersHashMap::Set(callInfo);

        auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 4);
        callInfo1->SetFunction(JSTaggedValue::Undefined());
        callInfo1->SetThis(tMap.GetTaggedValue());
        ContainersHashMap::Values(callInfo1);
        JSNApi::DestroyJSVM(vm);
    }
};
}  // namespace panda::ecmascript
#endif