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

#include "ecmascript/containers/containers_private.h"
#include "ecmascript/containers/containers_treemap.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/napi/include/jsnapi.h"

namespace panda::ecmascript {
using namespace panda::ecmascript::containers;
class ContainersTreeMapFuzzTestHelper {
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

    static JSTaggedValue InitializeTreeMapConstructor(JSThread *thread)
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
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::TreeMap)));
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        return result;
    }

    static JSHandle<JSAPITreeMap> CreateJSAPITreeMap(JSThread *thread)
    {
        JSHandle<JSFunction> newTarget(thread, InitializeTreeMapConstructor(thread));
        auto objCallInfo = CreateEcmaRuntimeCallInfo(thread, 6);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());

        JSTaggedValue result = ContainersTreeMap::TreeMapConstructor(objCallInfo);
        JSHandle<JSAPITreeMap> map(thread, result);
        return map;
    }

    static bool InitFuzzTest(const uint8_t *data, size_t &size, int32_t &key, EcmaVM *&vm, JSThread *&thread)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        vm = JSNApi::CreateJSVM(option);
        thread = vm->GetJSThread();
        if (size <= 0) {
            JSNApi::DestroyJSVM(vm);
            return false;
        }
        size_t maxByteLen = 4;
        if (size > maxByteLen) {
            size = maxByteLen;
        }
        if (memcpy_s(&key, maxByteLen, data, size) != EOK) {
            std::cout << "memcpy_s failed!";
            UNREACHABLE();
        }
        return true;
    }

    static void ContainersTreeMapClearFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        int32_t key = 0;
        if (!InitFuzzTest(data, size, key, vm, thread)) {
            return;
        }

        JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(key));
        ContainersTreeMap::Set(callInfo);

        auto objcallInfo = CreateEcmaRuntimeCallInfo(thread, 4);
        objcallInfo->SetFunction(JSTaggedValue::Undefined());
        objcallInfo->SetThis(tmap.GetTaggedValue());
        ContainersTreeMap::Clear(objcallInfo);

        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersTreeMapConstructorFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        int32_t key = 0;
        if (!InitFuzzTest(data, size, key, vm, thread)) {
            return;
        }

        JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(key));
        ContainersTreeMap::TreeMapConstructor(callInfo);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersTreeMapEntriesFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        int32_t key = 0;
        if (!InitFuzzTest(data, size, key, vm, thread)) {
            return;
        }

        JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(key));
        ContainersTreeMap::Set(callInfo);

        auto objCallInfo = CreateEcmaRuntimeCallInfo(thread, 4);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(tmap.GetTaggedValue());
        ContainersTreeMap::Entries(objCallInfo);

        JSNApi::DestroyJSVM(vm);
    }

    static JSTaggedValue TestForEachFunc([[maybe_unused]] EcmaRuntimeCallInfo *argv)
    {
        return JSTaggedValue::True();
    }

    static void ContainersTreeMapForEachFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        int32_t key = 0;
        if (!InitFuzzTest(data, size, key, vm, thread)) {
            return;
        }

        JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(key));
        ContainersTreeMap::Set(callInfo);

        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSAPITreeMap> dmap = CreateJSAPITreeMap(thread);
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestForEachFunc));
        auto objCallInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(tmap.GetTaggedValue());
        objCallInfo->SetCallArg(0, func.GetTaggedValue());
        objCallInfo->SetCallArg(1, dmap.GetTaggedValue());
        ContainersTreeMap::ForEach(objCallInfo);

        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersTreeMapGetFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        int32_t key = 0;
        if (!InitFuzzTest(data, size, key, vm, thread)) {
            return;
        }

        JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap(thread);
        auto objcallInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        objcallInfo->SetFunction(JSTaggedValue::Undefined());
        objcallInfo->SetThis(tmap.GetTaggedValue());
        objcallInfo->SetCallArg(0, JSTaggedValue(0));
        objcallInfo->SetCallArg(1, JSTaggedValue(key));
        ContainersTreeMap::Get(objcallInfo);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersTreeMapGetFirstKeyFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        int32_t key = 0;
        if (!InitFuzzTest(data, size, key, vm, thread)) {
            return;
        }

        JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(key));
        ContainersTreeMap::Set(callInfo);

        auto objCallInfo = CreateEcmaRuntimeCallInfo(thread, 4);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(tmap.GetTaggedValue());
        ContainersTreeMap::GetFirstKey(objCallInfo);

        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersTreeMapGetHigherKeyFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        int32_t key = 0;
        if (!InitFuzzTest(data, size, key, vm, thread)) {
            return;
        }

        JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(key));
        ContainersTreeMap::Set(callInfo);

        auto objcallInfo = CreateEcmaRuntimeCallInfo(thread, 6);
        objcallInfo->SetFunction(JSTaggedValue::Undefined());
        objcallInfo->SetThis(tmap.GetTaggedValue());
        objcallInfo->SetCallArg(0, JSTaggedValue(key));
        ContainersTreeMap::GetHigherKey(objcallInfo);

        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersTreeMapGetLastKeyFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        int32_t key = 0;
        if (!InitFuzzTest(data, size, key, vm, thread)) {
            return;
        }

        JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(key));
        ContainersTreeMap::Set(callInfo);

        auto objCallInfo = CreateEcmaRuntimeCallInfo(thread, 4);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(tmap.GetTaggedValue());
        ContainersTreeMap::GetLastKey(objCallInfo);

        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersTreeMapGetLengthFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        int32_t key = 0;
        if (!InitFuzzTest(data, size, key, vm, thread)) {
            return;
        }

        JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(key));
        ContainersTreeMap::Set(callInfo);

        JSHandle<JSAPITreeMap> map = CreateJSAPITreeMap(thread);
        auto objCallInfo = CreateEcmaRuntimeCallInfo(thread, 6);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(map.GetTaggedValue());
        ContainersTreeMap::GetLength(objCallInfo);

        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersTreeMapGetLowerKeyFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        int32_t key = 0;
        if (!InitFuzzTest(data, size, key, vm, thread)) {
            return;
        }

        JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(key));
        ContainersTreeMap::Set(callInfo);

        auto objcallInfo = CreateEcmaRuntimeCallInfo(thread, 6);
        objcallInfo->SetFunction(JSTaggedValue::Undefined());
        objcallInfo->SetThis(tmap.GetTaggedValue());
        objcallInfo->SetCallArg(0, JSTaggedValue(key));
        ContainersTreeMap::GetLowerKey(objcallInfo);

        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersTreeMapCheckKeyFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        int32_t key = 0;
        if (!InitFuzzTest(data, size, key, vm, thread)) {
            return;
        }

        JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(key));

        ContainersTreeMap::HasKey(callInfo);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersTreeMapCheckValueFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        int32_t key = 0;
        if (!InitFuzzTest(data, size, key, vm, thread)) {
            return;
        }

        JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(key));

        ContainersTreeMap::HasValue(callInfo);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersTreeMapIsEmptyFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        int32_t key = 0;
        if (!InitFuzzTest(data, size, key, vm, thread)) {
            return;
        }

        JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(key));
        ContainersTreeMap::Set(callInfo);
        ContainersTreeMap::IsEmpty(callInfo);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersTreeMapKeysFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        int32_t key = 0;
        if (!InitFuzzTest(data, size, key, vm, thread)) {
            return;
        }

        JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(key));
        ContainersTreeMap::Set(callInfo);

        auto objcallInfo = CreateEcmaRuntimeCallInfo(thread, 4);
        objcallInfo->SetFunction(JSTaggedValue::Undefined());
        objcallInfo->SetThis(tmap.GetTaggedValue());
        ContainersTreeMap::Keys(objcallInfo);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersTreeMapRemoveFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        int32_t key = 0;
        if (!InitFuzzTest(data, size, key, vm, thread)) {
            return;
        }

        JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(key));
        ContainersTreeMap::Set(callInfo);

        JSHandle<JSAPITreeMap> map = CreateJSAPITreeMap(thread);
        auto objcallInfo = CreateEcmaRuntimeCallInfo(thread, 6);
        objcallInfo->SetFunction(JSTaggedValue::Undefined());
        objcallInfo->SetThis(map.GetTaggedValue());
        objcallInfo->SetCallArg(0, JSTaggedValue(key));
        ContainersTreeMap::Remove(objcallInfo);

        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersTreeMapReplaceFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        int32_t key = 0;
        if (!InitFuzzTest(data, size, key, vm, thread)) {
            return;
        }

        constexpr int nodeNumbers = 8;
        JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap(thread);
        for (int i = 0; i < nodeNumbers; i++) {
            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(tmap.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(key));
            callInfo->SetCallArg(1, JSTaggedValue(key));
            ContainersTreeMap::Set(callInfo);

            auto objCallInfo = CreateEcmaRuntimeCallInfo(thread, 8);
            objCallInfo->SetFunction(JSTaggedValue::Undefined());
            objCallInfo->SetThis(tmap.GetTaggedValue());
            objCallInfo->SetCallArg(0, JSTaggedValue(nodeNumbers / 2));  // 2 : half of nodeNumbers
            objCallInfo->SetCallArg(1, JSTaggedValue(nodeNumbers));
            ContainersTreeMap::Replace(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersTreeMapSetFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        int32_t key = 0;
        if (!InitFuzzTest(data, size, key, vm, thread)) {
            return;
        }

        JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(key));

        ContainersTreeMap::Set(callInfo);
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersTreeMapSetAllFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        int32_t key = 0;
        if (!InitFuzzTest(data, size, key, vm, thread)) {
            return;
        }

        JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(key));
        ContainersTreeMap::Set(callInfo);

        JSHandle<JSAPITreeMap> map = CreateJSAPITreeMap(thread);
        auto objcallInfo = CreateEcmaRuntimeCallInfo(thread, 6);
        objcallInfo->SetFunction(JSTaggedValue::Undefined());
        objcallInfo->SetThis(map.GetTaggedValue());
        objcallInfo->SetCallArg(0, tmap.GetTaggedValue());
        ContainersTreeMap::SetAll(objcallInfo);

        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersTreeMapValuesFuzzTest(const uint8_t *data, size_t size)
    {
        EcmaVM *vm = nullptr;
        JSThread *thread = nullptr;
        int32_t key = 0;
        if (!InitFuzzTest(data, size, key, vm, thread)) {
            return;
        }

        JSHandle<JSAPITreeMap> tmap = CreateJSAPITreeMap(thread);
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tmap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(key));
        ContainersTreeMap::Set(callInfo);

        auto objcallInfo = CreateEcmaRuntimeCallInfo(thread, 4);
        objcallInfo->SetFunction(JSTaggedValue::Undefined());
        objcallInfo->SetThis(tmap.GetTaggedValue());
        ContainersTreeMap::Values(objcallInfo);

        JSNApi::DestroyJSVM(vm);
    }
};
}  // namespace panda::ecmascript
#endif