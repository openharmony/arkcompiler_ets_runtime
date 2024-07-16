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

#ifndef CONTAINERSDEQUECOMMON_FUZZER_H
#define CONTAINERSDEQUECOMMON_FUZZER_H

#include "ecmascript/containers/containers_deque.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_deque.h"
#include "ecmascript/js_api/js_api_deque_iterator.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"

#define MAXBYTELEN sizeof(unsigned int)

namespace panda::ecmascript {
using namespace panda::ecmascript::containers;
class ContainersDequeFuzzTestHelper {
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

    static JSHandle<JSAPIDeque> CreateJSAPIDeque(JSThread *thread)
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
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::Deque))); // 0 means the argument
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);

        JSHandle<JSFunction> newTarget(thread, result);
        auto objCallInfo2 = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
        objCallInfo2->SetFunction(newTarget.GetTaggedValue());
        objCallInfo2->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo2->SetThis(JSTaggedValue::Undefined());
        objCallInfo2->SetCallArg(0, JSTaggedValue::Undefined());

        JSTaggedValue list = ContainersDeque::DequeConstructor(objCallInfo2);
        JSHandle<JSAPIDeque> deque(thread, list);
        return deque;
    }

    static void ContainersDequeInsertFrontFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPIDeque> deque = CreateJSAPIDeque(thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(deque.GetTaggedValue());
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            std::string str(data, data + size);
            JSTaggedValue value = factory->NewFromStdString(str).GetTaggedValue();
            callInfo->SetCallArg(0, value);
            ContainersDeque::InsertFront(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    class TestClass : public base::BuiltinsBase {
        public:
            static JSTaggedValue TestForEachFunc(EcmaRuntimeCallInfo *argv)
            {
                JSThread *thread = argv->GetThread();
                JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
                JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
                JSHandle<JSTaggedValue> deque = GetCallArg(argv, 2); // 2 means the secode arg
                if (!deque->IsUndefined()) {
                    if (index->IsNumber() && value->IsNumber()) {
                        JSHandle<JSAPIDeque>::Cast(deque)->Set(thread, index->GetInt(),
                            JSTaggedValue(value->GetInt() * 2));    // 2 means mul by 2
                    }
                }
                return JSTaggedValue::True();
            }
    };

    static void ContainersDequeForEachFuzzTest(const uint8_t* data, size_t size)
    {
        uint32_t input = 0;
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

        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();

            constexpr uint32_t NODE_NUMBERS = 8;
            JSHandle<JSAPIDeque> deque = CreateJSAPIDeque(thread);
            for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
                auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
                callInfo->SetFunction(JSTaggedValue::Undefined());
                callInfo->SetThis(deque.GetTaggedValue());
                callInfo->SetCallArg(0, JSTaggedValue(i + input));
                ContainersDeque::InsertFront(callInfo);
            }

            JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
            JSHandle<JSFunction> func = thread->GetEcmaVM()->GetFactory()->NewJSFunction(env,
                reinterpret_cast<void *>(TestClass::TestForEachFunc));
            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(deque.GetTaggedValue());
            callInfo->SetCallArg(0, func.GetTaggedValue());
            callInfo->SetCallArg(1, deque.GetTaggedValue());
            ContainersDeque::ForEach(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersDequeGetFirstFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();

            JSHandle<JSAPIDeque> deque = CreateJSAPIDeque(thread);
            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(deque.GetTaggedValue());

            unsigned int input = 0;
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
            callInfo->SetCallArg(0, JSTaggedValue(input));
            ContainersDeque::InsertFront(callInfo);
            auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 4);
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(deque.GetTaggedValue());
            ContainersDeque::GetFirst(callInfo1);
        }
        JSNApi::DestroyJSVM(vm);
        return;
    }

    static void ContainersDequeGetLastFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();

            JSHandle<JSAPIDeque> deque = CreateJSAPIDeque(thread);
            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(deque.GetTaggedValue());

            unsigned int input = 0;
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
            callInfo->SetCallArg(0, JSTaggedValue(input));

            ContainersDeque::InsertFront(callInfo);

            auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 4);
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(deque.GetTaggedValue());
            ContainersDeque::GetLast(callInfo1);
        }
        JSNApi::DestroyJSVM(vm);
        return;
    }

    static void ContainersDequeInsertEndFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();

            JSHandle<JSAPIDeque> deque = CreateJSAPIDeque(thread);
            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(deque.GetTaggedValue());

            unsigned int input = 0;
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
            callInfo->SetCallArg(0, JSTaggedValue(input));
            ContainersDeque::InsertEnd(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
        return;
    }

    static void ContainersDequeHasFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();

            JSHandle<JSAPIDeque> deque = CreateJSAPIDeque(thread);
            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(deque.GetTaggedValue());

            unsigned int input = 0;
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
            callInfo->SetCallArg(0, JSTaggedValue(input));

            ContainersDeque::InsertEnd(callInfo);

            auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 8);
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(deque.GetTaggedValue());
            callInfo1->SetCallArg(0, JSTaggedValue(input));
            ContainersDeque::Has(callInfo1);
        }
        JSNApi::DestroyJSVM(vm);
        return;
    }

    static void ContainersDequePopFirstFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();

            JSHandle<JSAPIDeque> deque = CreateJSAPIDeque(thread);
            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(deque.GetTaggedValue());

            unsigned int input = 0;
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
            callInfo->SetCallArg(0, JSTaggedValue(input));
            ContainersDeque::InsertFront(callInfo);
            auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 4);
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(deque.GetTaggedValue());
            ContainersDeque::PopFirst(callInfo1);
        }
        JSNApi::DestroyJSVM(vm);
        return;
    }

    static void ContainersDequePopLastFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();

            JSHandle<JSAPIDeque> deque = CreateJSAPIDeque(thread);
            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(deque.GetTaggedValue());

            unsigned int input = 0;
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
            callInfo->SetCallArg(0, JSTaggedValue(input));
            ContainersDeque::InsertFront(callInfo);
            auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 4);
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(deque.GetTaggedValue());
            ContainersDeque::PopLast(callInfo1);
        }
        JSNApi::DestroyJSVM(vm);
        return;
    }

    static void ContainersDequeIteratorFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();

            uint32_t input = 0;
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

            constexpr uint32_t NODE_NUMBERS = 8;
            JSHandle<JSAPIDeque> deque = CreateJSAPIDeque(thread);
            for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
                auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
                callInfo->SetFunction(JSTaggedValue::Undefined());
                callInfo->SetThis(deque.GetTaggedValue());
                callInfo->SetCallArg(0, JSTaggedValue(i + input));

                ContainersDeque::InsertEnd(callInfo);
            }

            auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 4);
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(deque.GetTaggedValue());
            JSHandle<JSTaggedValue> iterValues(thread, ContainersDeque::GetIteratorObj(callInfo1));

            JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
            for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
                auto callInfo = CreateEcmaRuntimeCallInfo(thread, 4);
                callInfo->SetFunction(JSTaggedValue::Undefined());
                callInfo->SetThis(iterValues.GetTaggedValue());
                result.Update(JSAPIDequeIterator::Next(callInfo));
            }
        }
        JSNApi::DestroyJSVM(vm);
        return;
    }
};
}
#endif