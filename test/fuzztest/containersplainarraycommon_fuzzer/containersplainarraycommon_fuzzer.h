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
#ifndef CONTAINERS_PLAIN_ARRAY_COMMON_FUZZER_H
#define CONTAINERS_PLAIN_ARRAY_COMMON_FUZZER_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/containers/containers_plainarray.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_plain_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/napi/include/jsnapi.h"

namespace ContainersPlainArrayFuzzTest {
using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::base;
using namespace panda::ecmascript::containers;

class ContainersPlainArrayTest {
public:
    static JSFunction *JSObjectCreate(JSThread *thread)
    {
        JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
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

    static JSTaggedValue InitializePlainArrayConstructor(JSThread *thread)
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
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::PlainArray)));
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        return result;
    }
    
    static JSHandle<JSAPIPlainArray> CreateJSAPIPlainArray(JSThread *thread)
    {
        JSHandle<JSFunction> newTarget(thread, InitializePlainArrayConstructor(thread));
        auto objCallInfo = CreateEcmaRuntimeCallInfo(thread, 4);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());
        JSTaggedValue result = ContainersPlainArray::PlainArrayConstructor(objCallInfo);
        JSHandle<JSAPIPlainArray> array(thread, result);
        return array;
    }

    static void ContainersPlainArray_Add_Has_FuzzTest(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size <= 0) {
            std::cout << "illegal input!";
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            auto factory = vm->GetFactory();

            uint32_t inputNum = 0;
            std::string inputStr(data, data + size);
            const uint32_t MAXBYTELEN = 4;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIPlainArray> plainArray = CreateJSAPIPlainArray(thread);
            const uint32_t addTimes = 3;
            for (uint32_t i = 0; i < addTimes; i++) {
                JSHandle<EcmaString> inputEcmaStr = factory->NewFromStdString(inputStr);
                EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 8); // 8 : means the argv length
                callInfo->SetFunction(JSTaggedValue::Undefined());
                callInfo->SetThis(plainArray.GetTaggedValue());
                callInfo->SetCallArg(0, JSTaggedValue(static_cast<uint32_t>(inputNum + i))); // set key
                callInfo->SetCallArg(1, inputEcmaStr.GetTaggedValue()); // set value
                ContainersPlainArray::Add(callInfo);
            }

            for (uint32_t i = 0; i < addTimes; i++) {
                EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
                callInfo->SetFunction(JSTaggedValue::Undefined());
                callInfo->SetThis(plainArray.GetTaggedValue());
                callInfo->SetCallArg(0, JSTaggedValue(static_cast<uint32_t>(inputNum + i)));
                ContainersPlainArray::Has(callInfo);  // expected to return true
            }
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersPlainArray_Get_FuzzTest(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size <= 0) {
            std::cout << "illegal input!";
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            auto factory = vm->GetFactory();

            uint32_t inputNum = 0;
            std::string inputStr(data, data + size);
            const uint32_t MAXBYTELEN = 4;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIPlainArray> plainArray = CreateJSAPIPlainArray(thread);
            JSHandle<EcmaString> inputEcmaStr = factory->NewFromStdString(inputStr);
            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 8); // 8 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(plainArray.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(static_cast<uint32_t>(inputNum)));
            callInfo->SetCallArg(1, inputEcmaStr.GetTaggedValue());
            ContainersPlainArray::Add(callInfo);

            EcmaRuntimeCallInfo *cfForGet = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            cfForGet->SetFunction(JSTaggedValue::Undefined());
            cfForGet->SetThis(plainArray.GetTaggedValue());
            cfForGet->SetCallArg(0, JSTaggedValue(static_cast<uint32_t>(inputNum))); // set key get value
            ContainersPlainArray::Get(cfForGet); // expected to return value
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersPlainArray_GetIndexOfKey_FuzzTest(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size <= 0) {
            std::cout << "illegal input!";
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            auto factory = vm->GetFactory();

            uint32_t inputNum = 0;
            std::string inputStr(data, data + size);
            const uint32_t MAXBYTELEN = 4;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIPlainArray> plainArray = CreateJSAPIPlainArray(thread);
            JSHandle<EcmaString> inputEcmaStr = factory->NewFromStdString(inputStr);
            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 8); // 8 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(plainArray.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(static_cast<uint32_t>(inputNum)));
            callInfo->SetCallArg(1, inputEcmaStr.GetTaggedValue());
            ContainersPlainArray::Add(callInfo);

            EcmaRuntimeCallInfo *cfForGetIndexOfKey =
                CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            cfForGetIndexOfKey->SetFunction(JSTaggedValue::Undefined());
            cfForGetIndexOfKey->SetThis(plainArray.GetTaggedValue());
            cfForGetIndexOfKey->SetCallArg(0, inputEcmaStr.GetTaggedValue()); // value
            ContainersPlainArray::GetIndexOfKey(cfForGetIndexOfKey); // expected to return the index of key
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersPlainArray_GetIndexOfValue_FuzzTest(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size <= 0) {
            std::cout << "illegal input!";
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            auto factory = vm->GetFactory();

            uint32_t inputNum = 0;
            std::string inputStr(data, data + size);
            const uint32_t MAXBYTELEN = 4;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIPlainArray> plainArray = CreateJSAPIPlainArray(thread);
            JSHandle<EcmaString> inputEcmaStr = factory->NewFromStdString(inputStr);
            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 8); // 8 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(plainArray.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(static_cast<uint32_t>(inputNum)));
            callInfo->SetCallArg(1, inputEcmaStr.GetTaggedValue());
            ContainersPlainArray::Add(callInfo);

            EcmaRuntimeCallInfo *cfForGetIndexOfValue =
                CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            cfForGetIndexOfValue->SetFunction(JSTaggedValue::Undefined());
            cfForGetIndexOfValue->SetThis(plainArray.GetTaggedValue());
            cfForGetIndexOfValue->SetCallArg(0, JSTaggedValue(static_cast<uint32_t>(inputNum))); // key
            ContainersPlainArray::GetIndexOfValue(cfForGetIndexOfValue); // expected to return the index of value
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersPlainArray_GetKeyAt_FuzzTest(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size <= 0) {
            std::cout << "illegal input!";
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            auto factory = vm->GetFactory();

            uint32_t inputNum = 0;
            std::string inputStr(data, data + size);
            const uint32_t MAXBYTELEN = 4;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIPlainArray> plainArray = CreateJSAPIPlainArray(thread);
            JSHandle<EcmaString> inputEcmaStr = factory->NewFromStdString(inputStr);
            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 8); // 8 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(plainArray.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(static_cast<uint32_t>(inputNum)));
            callInfo->SetCallArg(1, inputEcmaStr.GetTaggedValue());
            ContainersPlainArray::Add(callInfo);

            EcmaRuntimeCallInfo *cfForGetKeyAt = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            cfForGetKeyAt->SetFunction(JSTaggedValue::Undefined());
            cfForGetKeyAt->SetThis(plainArray.GetTaggedValue());
            cfForGetKeyAt->SetCallArg(0, inputEcmaStr.GetTaggedValue()); // value
            ContainersPlainArray::GetKeyAt(cfForGetKeyAt); // expected to return the key
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersPlainArray_Remove_FuzzTest(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size <= 0) {
            std::cout << "illegal input!";
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            auto factory = vm->GetFactory();

            uint32_t inputNum = 0;
            std::string inputStr(data, data + size);
            const uint32_t MAXBYTELEN = 4;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIPlainArray> plainArray = CreateJSAPIPlainArray(thread);
            JSHandle<EcmaString> inputEcmaStr = factory->NewFromStdString(inputStr);
            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 8); // 8 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(plainArray.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(static_cast<uint32_t>(inputNum)));
            callInfo->SetCallArg(1, inputEcmaStr.GetTaggedValue());
            ContainersPlainArray::Add(callInfo);

            EcmaRuntimeCallInfo *cfForRemove = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            cfForRemove->SetFunction(JSTaggedValue::Undefined());
            cfForRemove->SetThis(plainArray.GetTaggedValue());
            cfForRemove->SetCallArg(0, JSTaggedValue(static_cast<uint32_t>(inputNum)));
            ContainersPlainArray::Remove(cfForRemove); // expected to return the value
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersPlainArray_RemoveAt_FuzzTest(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size <= 0) {
            std::cout << "illegal input!";
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            auto factory = vm->GetFactory();

            uint32_t inputNum = 0;
            std::string inputStr(data, data + size);
            const uint32_t MAXBYTELEN = 4;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIPlainArray> plainArray = CreateJSAPIPlainArray(thread);
            JSHandle<EcmaString> inputEcmaStr = factory->NewFromStdString(inputStr);
            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 8); // 8 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(plainArray.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(static_cast<uint32_t>(inputNum)));
            callInfo->SetCallArg(1, inputEcmaStr.GetTaggedValue());
            ContainersPlainArray::Add(callInfo);

            EcmaRuntimeCallInfo *cfForRemoveAt = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            cfForRemoveAt->SetFunction(JSTaggedValue::Undefined());
            cfForRemoveAt->SetThis(plainArray.GetTaggedValue());
            cfForRemoveAt->SetCallArg(0, JSTaggedValue(static_cast<uint32_t>(inputNum)));
            ContainersPlainArray::RemoveAt(cfForRemoveAt); // expected to return the value
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersPlainArray_RemoveRangeFrom_FuzzTest(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size <= 0) {
            std::cout << "illegal input!";
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            auto factory = vm->GetFactory();

            uint32_t inputNum = 0;
            std::string inputStr(data, data + size);
            const uint32_t MAXBYTELEN = 4;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIPlainArray> plainArray = CreateJSAPIPlainArray(thread);
            JSHandle<EcmaString> inputEcmaStr = factory->NewFromStdString(inputStr);
            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 8); // 8 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(plainArray.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(static_cast<uint32_t>(inputNum)));
            callInfo->SetCallArg(1, inputEcmaStr.GetTaggedValue());
            ContainersPlainArray::Add(callInfo);

            EcmaRuntimeCallInfo *cfForRemoveRangeFrom =
                CreateEcmaRuntimeCallInfo(thread, 8); // 8 : means the argv length
            cfForRemoveRangeFrom->SetFunction(JSTaggedValue::Undefined());
            cfForRemoveRangeFrom->SetThis(plainArray.GetTaggedValue());
            cfForRemoveRangeFrom->SetCallArg(0, JSTaggedValue(0)); // set index as the head of array
            cfForRemoveRangeFrom->SetCallArg(1, JSTaggedValue(static_cast<uint32_t>(inputNum))); // number to delete
            ContainersPlainArray::RemoveRangeFrom(cfForRemoveRangeFrom); // expected to return the safe size
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersPlainArray_SetValueAt_FuzzTest(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size <= 0) {
            std::cout << "illegal input!";
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            auto factory = vm->GetFactory();

            uint32_t inputNum = 0;
            std::string inputStr(data, data + size);
            const uint32_t MAXBYTELEN = 4;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIPlainArray> plainArray = CreateJSAPIPlainArray(thread);
            JSHandle<EcmaString> inputEcmaStr = factory->NewFromStdString(inputStr);
            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 8); // 8 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(plainArray.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(static_cast<uint32_t>(inputNum)));
            callInfo->SetCallArg(1, inputEcmaStr.GetTaggedValue());
            ContainersPlainArray::Add(callInfo);

            EcmaRuntimeCallInfo *cfForSetValueAt = CreateEcmaRuntimeCallInfo(thread, 8); // 8 : means the argv length
            cfForSetValueAt->SetFunction(JSTaggedValue::Undefined());
            cfForSetValueAt->SetThis(plainArray.GetTaggedValue());
            cfForSetValueAt->SetCallArg(0, JSTaggedValue(static_cast<uint32_t>(inputNum))); // set index to set
            cfForSetValueAt->SetCallArg(1, JSTaggedValue(static_cast<uint32_t>(inputNum))); // set new value
            ContainersPlainArray::SetValueAt(cfForSetValueAt); // expected to return undefined
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersPlainArray_GetValueAt_FuzzTest(const uint8_t* data, size_t size)
    {
        if (data == nullptr || size <= 0) {
            std::cout << "illegal input!";
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            auto factory = vm->GetFactory();

            uint32_t inputNum = 0;
            std::string inputStr(data, data + size);
            const uint32_t MAXBYTELEN = 4;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIPlainArray> plainArray = CreateJSAPIPlainArray(thread);
            JSHandle<EcmaString> inputEcmaStr = factory->NewFromStdString(inputStr);
            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 8); // 8 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(plainArray.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(static_cast<uint32_t>(inputNum)));
            callInfo->SetCallArg(1, inputEcmaStr.GetTaggedValue());
            ContainersPlainArray::Add(callInfo);

            EcmaRuntimeCallInfo *cfForGetValueAt = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            cfForGetValueAt->SetFunction(JSTaggedValue::Undefined());
            cfForGetValueAt->SetThis(plainArray.GetTaggedValue());
            cfForGetValueAt->SetCallArg(0, JSTaggedValue(static_cast<uint32_t>(inputNum))); // set index to get
            ContainersPlainArray::GetValueAt(cfForGetValueAt); // expected to return value
        }
        JSNApi::DestroyJSVM(vm);
    }
};
}

#endif