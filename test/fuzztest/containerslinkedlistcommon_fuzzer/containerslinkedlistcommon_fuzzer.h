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

#ifndef CONTAINERSLINKEDLISTCOMMON_FUZZER_H
#define CONTAINERSLINKEDLISTCOMMON_FUZZER_H

#include "ecmascript/containers/containers_linked_list.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_linked_list.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_thread.h"

#define MAXBYTELEN sizeof(int)

namespace panda::ecmascript {
class ContainersLinkedListFuzzTestHelper {
public:
    class TestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue TestForEachFunc(EcmaRuntimeCallInfo *argv)
        {
            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
            JSHandle<JSTaggedValue> list = GetCallArg(argv, 2); // 2 means the secode arg
            if (!list->IsUndefined()) {
                if (index->IsNumber() && value->IsNumber()) {
                    JSHandle<JSTaggedValue> newValue(thread, JSTaggedValue(value->GetInt() * 2)); // 2 means mul by 2
                    JSAPILinkedList::Set(thread, JSHandle<JSAPILinkedList>::Cast(list), index->GetInt(), newValue);
                }
            }
            return JSTaggedValue::True();
        }
    };
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

    static JSHandle<JSAPILinkedList> CreateJSAPILinkedList(JSThread *thread)
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
        // 0 means the argument
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(containers::ContainerTag::LinkedList)));
        JSTaggedValue result = containers::ContainersPrivate::Load(objCallInfo);

        JSHandle<JSFunction> newTarget(thread, result);
        auto objCallInfo2 = CreateEcmaRuntimeCallInfo(thread, 6);
        objCallInfo2->SetFunction(newTarget.GetTaggedValue());
        objCallInfo2->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo2->SetThis(JSTaggedValue::Undefined());
        objCallInfo2->SetCallArg(0, JSTaggedValue::Undefined());

        JSTaggedValue list = containers::ContainersLinkedList::LinkedListConstructor(objCallInfo2);
        JSHandle<JSAPILinkedList> linkedList(thread, list);
        return linkedList;
    }

    static void LinkedListAdd(JSHandle<JSAPILinkedList> &linkedList, JSTaggedValue value, JSThread *thread)
    {
        auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(linkedList.GetTaggedValue());
        callInfo->SetCallArg(0, value);
        containers::ContainersLinkedList::Add(callInfo);
    }

    static void ContainersLinkedListAddFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            int value = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&value, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            LinkedListAdd(linkedList, JSTaggedValue(value), thread);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListGetFirstFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            int value = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&value, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            LinkedListAdd(linkedList, JSTaggedValue(value), thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(linkedList.GetTaggedValue());
            containers::ContainersLinkedList::GetFirst(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListGetLastFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            int value = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&value, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            LinkedListAdd(linkedList, JSTaggedValue(value), thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(linkedList.GetTaggedValue());
            containers::ContainersLinkedList::GetLast(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListAddFirstFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(linkedList.GetTaggedValue());
            int value = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&value, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            callInfo->SetCallArg(0, JSTaggedValue(value));

            containers::ContainersLinkedList::AddFirst(callInfo);
            }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListClearFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            int value = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&value, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            LinkedListAdd(linkedList, JSTaggedValue(value), thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(linkedList.GetTaggedValue());
            containers::ContainersLinkedList::Clear(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListCloneFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            int value = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&value, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            LinkedListAdd(linkedList, JSTaggedValue(value), thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(linkedList.GetTaggedValue());
            containers::ContainersLinkedList::Clone(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListGetFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            int value = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&value, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            LinkedListAdd(linkedList, JSTaggedValue(value), thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(linkedList.GetTaggedValue());
            int index = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&index, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            callInfo->SetCallArg(0, JSTaggedValue(index));
            containers::ContainersLinkedList::Get(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListGetIndexOfFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            int value = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&value, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            LinkedListAdd(linkedList, JSTaggedValue(value), thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(linkedList.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(value));
            containers::ContainersLinkedList::GetIndexOf(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListGetLastIndexOfFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            int value = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&value, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            LinkedListAdd(linkedList, JSTaggedValue(value), thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(linkedList.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(value));
            containers::ContainersLinkedList::GetLastIndexOf(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListHasFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            int value = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&value, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            LinkedListAdd(linkedList, JSTaggedValue(value), thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(linkedList.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(value));
            containers::ContainersLinkedList::Has(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListInsertFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(linkedList.GetTaggedValue());
            int index = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&index, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            callInfo->SetCallArg(0, JSTaggedValue(index));
            callInfo->SetCallArg(1, JSTaggedValue(index + 1));

            containers::ContainersLinkedList::Insert(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListRemoveByIndexFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            int value = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&value, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            LinkedListAdd(linkedList, JSTaggedValue(value), thread);
            int index = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&index, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(linkedList.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(index));
            containers::ContainersLinkedList::RemoveByIndex(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListRemoveFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            int value = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&value, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            LinkedListAdd(linkedList, JSTaggedValue(value), thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(linkedList.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(value));
            containers::ContainersLinkedList::Remove(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListRemoveFirstFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            int value = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&value, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            LinkedListAdd(linkedList, JSTaggedValue(value), thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(linkedList.GetTaggedValue());
            containers::ContainersLinkedList::RemoveFirst(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListRemoveLastFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            int value = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&value, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            LinkedListAdd(linkedList, JSTaggedValue(value), thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(linkedList.GetTaggedValue());
            containers::ContainersLinkedList::RemoveLast(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListRemoveFirstFoundFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            int value = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&value, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            LinkedListAdd(linkedList, JSTaggedValue(value), thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(linkedList.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(value));
            containers::ContainersLinkedList::RemoveFirstFound(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListRemoveLastFoundFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            int value = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&value, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            LinkedListAdd(linkedList, JSTaggedValue(value), thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(linkedList.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(value));
            containers::ContainersLinkedList::RemoveLastFound(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListSetFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(linkedList.GetTaggedValue());
            int index = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&index, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            callInfo->SetCallArg(0, JSTaggedValue(index));
            callInfo->SetCallArg(1, JSTaggedValue(index + 1));

            containers::ContainersLinkedList::Set(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListLengthFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            int value = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&value, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            LinkedListAdd(linkedList, JSTaggedValue(value), thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(linkedList.GetTaggedValue());
            containers::ContainersLinkedList::Length(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListConvertToArrayFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            int value = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&value, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            LinkedListAdd(linkedList, JSTaggedValue(value), thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(linkedList.GetTaggedValue());
            containers::ContainersLinkedList::ConvertToArray(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListForEachFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            int value = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&value, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            LinkedListAdd(linkedList, JSTaggedValue(value), thread);

            JSHandle<JSAPILinkedList> newLinkedlist = CreateJSAPILinkedList(thread);
            auto callInfo2 = CreateEcmaRuntimeCallInfo(thread, 8);
            JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
            auto factory = vm->GetFactory();
            JSHandle<JSFunction> func =
                factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForEachFunc));
            callInfo2->SetFunction(JSTaggedValue::Undefined());
            callInfo2->SetThis(linkedList.GetTaggedValue());
            callInfo2->SetCallArg(0, func.GetTaggedValue());
            callInfo2->SetCallArg(1, newLinkedlist.GetTaggedValue());
            containers::ContainersLinkedList::ForEach(callInfo2);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersLinkedListGetIteratorObjFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList(thread);
            int value = 0;
            if (size <= 0) {
                return;
            }
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&value, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }
            LinkedListAdd(linkedList, JSTaggedValue(value), thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(linkedList.GetTaggedValue());
            containers::ContainersLinkedList::GetIteratorObj(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }
};
}
#endif