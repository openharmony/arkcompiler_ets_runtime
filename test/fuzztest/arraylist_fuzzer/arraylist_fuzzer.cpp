/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "arraylist_fuzzer.h"

#include "ecmascript/containers/containers_arraylist.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"


using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

#define MAXBYTELEN sizeof(uint32_t)
namespace OHOS {

    JSFunction *JSObjectCreate(JSThread *thread)
    {
        JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
        return globalEnv->GetObjectFunction().GetObject<JSFunction>();
    }

    EcmaRuntimeCallInfo *CreateEcmaRuntimeCallInfo(JSThread *thread, uint32_t numArgs)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSTaggedValue> hclass(thread, JSObjectCreate(thread));
        JSHandle<JSTaggedValue> callee(factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        EcmaRuntimeCallInfo *objCallInfo =
            EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, callee, undefined, numArgs);
        return objCallInfo;
    }

    JSTaggedValue InitializeArrayListConstructor(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo = CreateEcmaRuntimeCallInfo(thread, 6);  // 6 : means the argv length
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::ArrayList)));
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        return result;
    }

    JSHandle<JSAPIArrayList> CreateJSAPIArrayList(JSThread *thread)
    {
        JSHandle<JSFunction> newTarget(thread, InitializeArrayListConstructor(thread));
        auto objCallInfo = CreateEcmaRuntimeCallInfo(thread, 4);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());
        JSTaggedValue result = ContainersArrayList::ArrayListConstructor(objCallInfo);
        JSHandle<JSAPIArrayList> arrayList(thread, result);
        return arrayList;
    }

    void ArrayListAdd(JSThread* thread, JSHandle<JSAPIArrayList> arrayList, JSTaggedValue value)
    {
        EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(arrayList.GetTaggedValue());
        callInfo->SetCallArg(0, value);

        ContainersArrayList::Add(callInfo);
    }

    static JSTaggedValue TestForEachFunc(EcmaRuntimeCallInfo *argv)
    {
        JSHandle<JSTaggedValue> value = argv->GetCallArg(0);
        JSHandle<JSTaggedValue> key = argv->GetCallArg(1);
        JSHandle<JSTaggedValue> arrayList = argv->GetCallArg(2); // 2 means the secode arg
        if (!arrayList->IsUndefined()) {
            if (value->IsNumber()) {
                TaggedArray *elements = TaggedArray::Cast(JSAPIArrayList::Cast(arrayList.GetTaggedValue().
                                                          GetTaggedObject())->GetElements().GetTaggedObject());
                [[maybe_unused]] JSTaggedValue result = elements->Get(key->GetInt());
            }
        }
        return JSTaggedValue::Undefined();
    }

    static JSTaggedValue TestReplaceAllElementsFunc(EcmaRuntimeCallInfo *argv)
    {
        JSThread *thread = argv->GetThread();
        JSHandle<JSTaggedValue> value = argv->GetCallArg(0);
        JSHandle<JSTaggedValue> index = argv->GetCallArg(1);
        JSHandle<JSTaggedValue> arrayList = argv->GetCallArg(2); // 2 means the secode arg
        if (!arrayList->IsUndefined()) {
            if (value->IsNumber()) {
                JSHandle<JSTaggedValue> newValue(thread, JSTaggedValue(value->GetInt() * 2)); // 2 means mul by 2
                JSHandle<JSAPIArrayList>::Cast(arrayList)->Set(thread, index->GetNumber(), newValue.GetTaggedValue());
                return newValue.GetTaggedValue();
            }
        }
        return JSTaggedValue::Undefined();
    }

    void ArrayListForEachFuzzTest(const uint8_t* data, size_t size)
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
            ObjectFactory *factory = vm->GetFactory();
            JSHandle<GlobalEnv> env = vm->GetGlobalEnv();

            uint32_t inputNum = 0;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
            info->SetFunction(JSTaggedValue::Undefined());
            info->SetThis(arrayList.GetTaggedValue());
            info->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Add(info);

            JSHandle<JSAPIArrayList> arrList = CreateJSAPIArrayList(thread);
            JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestForEachFunc));
            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            callInfo->SetCallArg(0, func.GetTaggedValue());
            callInfo->SetCallArg(1, arrList.GetTaggedValue());
            ContainersArrayList::ForEach(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListAddFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());

            unsigned int inputNum = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            callInfo->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Add(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListClearFuzzTest(const uint8_t* data, size_t size)
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

            uint32_t inputNum = 0;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
            info->SetFunction(JSTaggedValue::Undefined());
            info->SetThis(arrayList.GetTaggedValue());
            info->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Add(info);

            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            ContainersArrayList::Clear(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListCloneFuzzTest(const uint8_t* data, size_t size)
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

            uint32_t inputNum = 0;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
            info->SetFunction(JSTaggedValue::Undefined());
            info->SetThis(arrayList.GetTaggedValue());
            info->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Add(info);

            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            ContainersArrayList::Clone(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListConvertToArrayFuzzTest(const uint8_t* data, size_t size)
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

            uint32_t inputNum = 0;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
            info->SetFunction(JSTaggedValue::Undefined());
            info->SetThis(arrayList.GetTaggedValue());
            info->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Add(info);

            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            ContainersArrayList::ConvertToArray(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListGetFuzzTest(const uint8_t* data, size_t size)
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

            uint32_t inputNum = 0;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
            info->SetFunction(JSTaggedValue::Undefined());
            info->SetThis(arrayList.GetTaggedValue());
            info->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Add(info);

            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Get(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListGetCapacityFuzzTest(const uint8_t* data, size_t size)
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

            uint32_t inputNum = 0;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
            info->SetFunction(JSTaggedValue::Undefined());
            info->SetThis(arrayList.GetTaggedValue());
            info->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Add(info);

            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            ContainersArrayList::GetCapacity(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListGetIndexOfFuzzTest(const uint8_t* data, size_t size)
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

            uint32_t inputNum = 0;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
            info->SetFunction(JSTaggedValue::Undefined());
            info->SetThis(arrayList.GetTaggedValue());
            info->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Add(info);

            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::GetIndexOf(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListGetIteratorObjFuzzTest(const uint8_t* data, size_t size)
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

            uint32_t inputNum = 0;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
            info->SetFunction(JSTaggedValue::Undefined());
            info->SetThis(arrayList.GetTaggedValue());
            info->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Add(info);

            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            ContainersArrayList::GetIteratorObj(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListGetLastIndexOfFuzzTest(const uint8_t* data, size_t size)
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

            uint32_t inputNum = 0;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
            info->SetFunction(JSTaggedValue::Undefined());
            info->SetThis(arrayList.GetTaggedValue());
            info->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Add(info);

            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::GetLastIndexOf(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListGetSizeFuzzTest(const uint8_t* data, size_t size)
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

            uint32_t inputNum = 0;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
            info->SetFunction(JSTaggedValue::Undefined());
            info->SetThis(arrayList.GetTaggedValue());
            info->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Add(info);

            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            ContainersArrayList::GetSize(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListHasFuzzTest(const uint8_t* data, size_t size)
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

            uint32_t inputNum = 0;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
            info->SetFunction(JSTaggedValue::Undefined());
            info->SetThis(arrayList.GetTaggedValue());
            info->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Add(info);

            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Has(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListIncreaseCapacityToFuzzTest(const uint8_t* data, size_t size)
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

            uint32_t inputNum = 0;
            const uint32_t maxByteLen = 1;
            if (size > maxByteLen) {
                size = maxByteLen;
            }
            if (memcpy_s(&inputNum, maxByteLen, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
            info->SetFunction(JSTaggedValue::Undefined());
            info->SetThis(arrayList.GetTaggedValue());
            info->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Add(info);

            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::IncreaseCapacityTo(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListInsertFuzzTest(const uint8_t* data, size_t size)
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

            uint32_t inputNum = 0;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
            info->SetFunction(JSTaggedValue::Undefined());
            info->SetThis(arrayList.GetTaggedValue());
            info->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Add(info);

            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(inputNum));
            callInfo->SetCallArg(1, JSTaggedValue(inputNum));
            ContainersArrayList::Insert(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListIsEmptyFuzzTest(const uint8_t* data, size_t size)
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

            uint32_t inputNum = 0;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
            info->SetFunction(JSTaggedValue::Undefined());
            info->SetThis(arrayList.GetTaggedValue());
            info->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Add(info);

            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            ContainersArrayList::IsEmpty(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListRemoveByIndexFuzzTest(const uint8_t* data, size_t size)
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

            uint32_t inputNum = 0;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
            info->SetFunction(JSTaggedValue::Undefined());
            info->SetThis(arrayList.GetTaggedValue());
            info->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Add(info);

            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Remove(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListRemoveByRangeFuzzTest(const uint8_t* data, size_t size)
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

            uint32_t inputNum = 0;
            const uint32_t maxByteLen = 1;
            if (size > maxByteLen) {
                size = maxByteLen;
            }
            if (memcpy_s(&inputNum, maxByteLen, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            for (uint32_t i = 0; i <= inputNum; i++) {
                EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
                info->SetFunction(JSTaggedValue::Undefined());
                info->SetThis(arrayList.GetTaggedValue());
                info->SetCallArg(0, JSTaggedValue(inputNum));
                ContainersArrayList::Add(info);
            }

            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(0));
            callInfo->SetCallArg(1, JSTaggedValue(inputNum));
            ContainersArrayList::RemoveByRange(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListReplaceAllElementsFuzzTest(const uint8_t* data, size_t size)
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
            ObjectFactory *factory = vm->GetFactory();
            JSHandle<GlobalEnv> env = vm->GetGlobalEnv();

            uint32_t inputNum = 0;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&inputNum, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
            info->SetFunction(JSTaggedValue::Undefined());
            info->SetThis(arrayList.GetTaggedValue());
            info->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Add(info);

            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            JSHandle<JSFunction> func =
                factory->NewJSFunction(env, reinterpret_cast<void *>(TestReplaceAllElementsFunc));
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            callInfo->SetCallArg(0, func.GetTaggedValue());
            ContainersArrayList::ReplaceAllElements(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListSetFuzzTest(const uint8_t* data, size_t size)
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

            uint32_t inputNum = 0;
            const uint32_t maxByteLen = 1;
            if (size > maxByteLen) {
                size = maxByteLen;
            }
            if (memcpy_s(&inputNum, maxByteLen, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            for (uint32_t i = 0; i <= inputNum; i++) {
                EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
                info->SetFunction(JSTaggedValue::Undefined());
                info->SetThis(arrayList.GetTaggedValue());
                info->SetCallArg(0, JSTaggedValue(inputNum));
                ContainersArrayList::Add(info);
            }

            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(inputNum / 2)); // 2 : half
            callInfo->SetCallArg(1, JSTaggedValue(inputNum));
            ContainersArrayList::Set(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListSortFuzzTest(const uint8_t* data, size_t size)
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

            uint32_t inputNum = 0;
            const uint32_t maxByteLen = 1;
            if (size > maxByteLen) {
                size = maxByteLen;
            }
            if (memcpy_s(&inputNum, maxByteLen, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
            info->SetFunction(JSTaggedValue::Undefined());
            info->SetThis(arrayList.GetTaggedValue());
            info->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Add(info);

            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue::Undefined());
            ContainersArrayList::Sort(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListSubArrayListFuzzTest(const uint8_t* data, size_t size)
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

            uint32_t inputNum = 0;
            const uint32_t maxByteLen = 1;
            if (size > maxByteLen) {
                size = maxByteLen;
            }
            if (memcpy_s(&inputNum, maxByteLen, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            for (uint32_t i = 0; i <= inputNum; i++) {
                EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
                info->SetFunction(JSTaggedValue::Undefined());
                info->SetThis(arrayList.GetTaggedValue());
                info->SetCallArg(0, JSTaggedValue(inputNum));
                ContainersArrayList::Add(info);
            }

            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(0)); // start
            callInfo->SetCallArg(1, JSTaggedValue(inputNum / 2)); // end 2 : half
            ContainersArrayList::SubArrayList(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayListTrimToCurrentLengthFuzzTest(const uint8_t* data, size_t size)
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

            uint32_t inputNum = 0;
            const uint32_t maxByteLen = 1;
            if (size > maxByteLen) {
                size = maxByteLen;
            }
            if (memcpy_s(&inputNum, maxByteLen, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList(thread);
            EcmaRuntimeCallInfo *info = CreateEcmaRuntimeCallInfo(thread, 6);
            info->SetFunction(JSTaggedValue::Undefined());
            info->SetThis(arrayList.GetTaggedValue());
            info->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::Add(info);

            EcmaRuntimeCallInfo *callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(arrayList.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(inputNum));
            ContainersArrayList::TrimToCurrentLength(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    void JSValueRefInstanceOfValueFuzzTest([[maybe_unused]] const uint8_t *data, [[maybe_unused]] size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            Local<ObjectRef> origin = ObjectRef::New(vm);
            JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
            JSHandle<JSFunction> constructor(globalEnv->GetObjectFunction());
            JSHandle<JSTaggedValue> arryListTag = JSHandle<JSTaggedValue>::Cast(constructor);
            Local<JSValueRef> value = JSNApiHelper::ToLocal<JSValueRef>(arryListTag);
            (void)origin->InstanceOf(vm, value);
        }
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::ArrayListAddFuzzTest(data, size);
    OHOS::ArrayListForEachFuzzTest(data, size);
    OHOS::ArrayListIsEmptyFuzzTest(data, size);
    OHOS::ArrayListInsertFuzzTest(data, size);
    OHOS::ArrayListIncreaseCapacityToFuzzTest(data, size);
    OHOS::ArrayListHasFuzzTest(data, size);
    OHOS::ArrayListGetSizeFuzzTest(data, size);
    OHOS::ArrayListGetLastIndexOfFuzzTest(data, size);
    OHOS::ArrayListGetIteratorObjFuzzTest(data, size);
    OHOS::ArrayListGetIndexOfFuzzTest(data, size);
    OHOS::ArrayListGetCapacityFuzzTest(data, size);
    OHOS::ArrayListGetFuzzTest(data, size);
    OHOS::ArrayListConvertToArrayFuzzTest(data, size);
    OHOS::ArrayListCloneFuzzTest(data, size);
    OHOS::ArrayListClearFuzzTest(data, size);
    OHOS::ArrayListRemoveByIndexFuzzTest(data, size);
    OHOS::ArrayListRemoveByRangeFuzzTest(data, size);
    OHOS::ArrayListReplaceAllElementsFuzzTest(data, size);
    OHOS::ArrayListSetFuzzTest(data, size);
    OHOS::ArrayListSortFuzzTest(data, size);
    OHOS::ArrayListSubArrayListFuzzTest(data, size);
    OHOS::ArrayListTrimToCurrentLengthFuzzTest(data, size);
    OHOS::JSValueRefInstanceOfValueFuzzTest(data, size);
    return 0;
}