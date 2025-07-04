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

#ifndef CONTAINERSVECTORCOMMON_FUZZER_H
#define CONTAINERSVECTORCOMMON_FUZZER_H

#include "ecmascript/containers/containers_vector.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_vector.h"
#include "ecmascript/js_api/js_api_vector_iterator.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/js_array.h"

#define MAXBYTELEN sizeof(unsigned int)

namespace panda::ecmascript {
using namespace panda::ecmascript::containers;
class ContainersVectorFuzzTestHelper {
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

    static JSHandle<JSAPIVector> CreateJSAPIVector(JSThread *thread)
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
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(containers::ContainerTag::Vector)));
        JSTaggedValue result = containers::ContainersPrivate::Load(objCallInfo);

        JSHandle<JSFunction> newTarget(thread, result);
        auto objCallInfo2 = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
        objCallInfo2->SetFunction(newTarget.GetTaggedValue());
        objCallInfo2->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo2->SetThis(JSTaggedValue::Undefined());
        objCallInfo2->SetCallArg(0, JSTaggedValue::Undefined());

        JSTaggedValue list = containers::ContainersVector::VectorConstructor(objCallInfo2);
        JSHandle<JSAPIVector> vector(thread, list);
        return vector;
    }

    static JSHandle<JSAPIVector> GetVectorWithData(JSThread *thread, const uint8_t* data, size_t size)
    {
        JSHandle<JSAPIVector> vector = CreateJSAPIVector(thread);

        constexpr int32_t ELEMENT_NUMS = 8;

        uint32_t input = 0;
        if (size > MAXBYTELEN) {
            size = MAXBYTELEN;
        }
        if (memcpy_s(&input, MAXBYTELEN, data, size) != 0) {
            std::cout << "memcpy_s failed!";
            UNREACHABLE();
        }

        for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(vector.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(i + input));

            ContainersVector::Add(callInfo);
        }

        return vector;
    }

    static void ContainersVectorAddFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPIVector> vector = CreateJSAPIVector(thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(vector.GetTaggedValue());
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            std::string str(data, data + size);
            JSTaggedValue value = factory->NewFromStdString(str).GetTaggedValue();
            callInfo->SetCallArg(0, value);

            ContainersVector::Add(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersVectorGetIndexOfFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPIVector> vector = CreateJSAPIVector(thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(vector.GetTaggedValue());
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            std::string str(data, data + size);
            JSTaggedValue value = factory->NewFromStdString(str).GetTaggedValue();
            callInfo->SetCallArg(0, value);

            ContainersVector::Add(callInfo);

            auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(vector.GetTaggedValue());
            callInfo1->SetCallArg(0, value);

            ContainersVector::GetIndexOf(callInfo1);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersVectorHasFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPIVector> vector = CreateJSAPIVector(thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(vector.GetTaggedValue());
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            std::string str(data, data + size);
            JSTaggedValue value = factory->NewFromStdString(str).GetTaggedValue();
            callInfo->SetCallArg(0, value);

            ContainersVector::Add(callInfo);

            auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(vector.GetTaggedValue());
            callInfo1->SetCallArg(0, value);

            ContainersVector::Has(callInfo1);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersVectorInsertFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPIVector> vector = CreateJSAPIVector(thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8); // 8 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(vector.GetTaggedValue());
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            std::string str(data, data + size);
            JSTaggedValue value = factory->NewFromStdString(str).GetTaggedValue();
            callInfo->SetCallArg(0, value);
            callInfo->SetCallArg(1, JSTaggedValue(0));

            ContainersVector::Insert(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersVectorRemoveFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPIVector> vector = CreateJSAPIVector(thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8); // 8 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(vector.GetTaggedValue());
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            std::string str(data, data + size);
            JSTaggedValue value = factory->NewFromStdString(str).GetTaggedValue();
            callInfo->SetCallArg(0, value);
            callInfo->SetCallArg(1, JSTaggedValue(0));

            ContainersVector::Insert(callInfo);

            auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(vector.GetTaggedValue());
            callInfo1->SetCallArg(0, value);
            ContainersVector::Remove(callInfo1);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersVectorSetFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPIVector> vector = CreateJSAPIVector(thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8); // 8 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(vector.GetTaggedValue());
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            std::string str(data, data + size);
            JSTaggedValue value = factory->NewFromStdString(str).GetTaggedValue();
            callInfo->SetCallArg(0, value);
            callInfo->SetCallArg(1, JSTaggedValue(0));

            ContainersVector::Insert(callInfo);

            auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 8); // 8 : means the argv length
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(vector.GetTaggedValue());
            callInfo1->SetCallArg(0, JSTaggedValue(0));
            callInfo1->SetCallArg(1, value);
            ContainersVector::Set(callInfo1);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersVectorGetLastIndexOfFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPIVector> vector = CreateJSAPIVector(thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(vector.GetTaggedValue());
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            std::string str(data, data + size);
            JSTaggedValue value = factory->NewFromStdString(str).GetTaggedValue();
            callInfo->SetCallArg(0, value);

            ContainersVector::Add(callInfo);

            auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(vector.GetTaggedValue());
            callInfo1->SetCallArg(0, value);
            ContainersVector::GetLastIndexOf(callInfo1);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersVectorGetLastIndexFromFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPIVector> vector = CreateJSAPIVector(thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(vector.GetTaggedValue());
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            std::string str(data, data + size);
            JSTaggedValue value = factory->NewFromStdString(str).GetTaggedValue();
            callInfo->SetCallArg(0, value);

            ContainersVector::Add(callInfo);

            auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 8); // 8 : means the argv length
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(vector.GetTaggedValue());
            callInfo1->SetCallArg(0, value);
            callInfo1->SetCallArg(1, JSTaggedValue(0));
            ContainersVector::GetLastIndexFrom(callInfo1);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersVectorGetIndexFromFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();
            JSHandle<JSAPIVector> vector = CreateJSAPIVector(thread);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(vector.GetTaggedValue());
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            std::string str(data, data + size);
            JSTaggedValue value = factory->NewFromStdString(str).GetTaggedValue();
            callInfo->SetCallArg(0, value);

            ContainersVector::Add(callInfo);

            auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 8); // 8 : means the argv length
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(vector.GetTaggedValue());
            callInfo1->SetCallArg(0, value);
            callInfo1->SetCallArg(1, JSTaggedValue(0));
            ContainersVector::GetIndexFrom(callInfo1);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersVectorRemoveByRangeFuzzTest(const uint8_t* data, size_t size)
    {
        if (size <= 0) {
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();

            JSHandle<JSAPIVector> vector = CreateJSAPIVector(thread);

            constexpr int32_t ELEMENT_NUMS = 8;

            uint32_t input = 0;
            if (size > MAXBYTELEN) {
                size = MAXBYTELEN;
            }
            if (memcpy_s(&input, MAXBYTELEN, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }

            for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
                auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
                callInfo->SetFunction(JSTaggedValue::Undefined());
                callInfo->SetThis(vector.GetTaggedValue());
                callInfo->SetCallArg(0, JSTaggedValue(i + input));

                ContainersVector::Add(callInfo);
            }

            auto callInfo1 = CreateEcmaRuntimeCallInfo(thread, 8); // 8 : means the argv length
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(vector.GetTaggedValue());
            callInfo1->SetCallArg(0, JSTaggedValue(input % ELEMENT_NUMS));
            callInfo1->SetCallArg(1, JSTaggedValue((input + 1) % ELEMENT_NUMS));
            ContainersVector::RemoveByRange(callInfo1);
        }
        JSNApi::DestroyJSVM(vm);
    }

    class TestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue TestForEachFunc(EcmaRuntimeCallInfo *argv)
        {
            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> key = GetCallArg(argv, 1);
            JSHandle<JSTaggedValue> vector = GetCallArg(argv, 2); // 2 means the secode arg
            if (!vector->IsUndefined()) {
                if (value->IsNumber()) {
                    TaggedArray *elements = TaggedArray::Cast(JSAPIVector::Cast(vector.GetTaggedValue().
                                            GetTaggedObject())->GetElements(thread).GetTaggedObject());
                    elements->Get(thread, key->GetInt());
                }
            }
            return JSTaggedValue::Undefined();
        }

        static JSTaggedValue TestReplaceAllElementsFunc(EcmaRuntimeCallInfo *argv)
        {
            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
            JSHandle<JSTaggedValue> vector = GetCallArg(argv, 2); // 2 means the secode arg
            if (!vector->IsUndefined()) {
                if (value->IsNumber()) {
                    JSHandle<JSTaggedValue> newValue(thread, JSTaggedValue(value->GetInt() * 2)); // 2 means mul by 2
                    JSHandle<JSAPIVector>::Cast(vector)->Set(thread, index->GetNumber(), newValue.GetTaggedValue());
                    return newValue.GetTaggedValue();
                }
            }
            return JSTaggedValue::Undefined();
        }
    };

    static void ContainersVectorSubVectorFuzzTest(const uint8_t* data, size_t size)
    {
        if (size <= 0) {
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();

            JSHandle<JSAPIVector> vector = GetVectorWithData(thread, data, size);

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 8); // 8 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(vector.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(0));
            callInfo->SetCallArg(1, JSTaggedValue(2)); // 2 : means the third value
            ContainersVector::SubVector(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersVectorSetLengthFuzzTest(const uint8_t* data, size_t size)
    {
        if (size <= 0) {
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();

            JSHandle<JSAPIVector> vector = GetVectorWithData(thread, data, size);

            uint32_t length = 0;
            if (size > MAXBYTELEN - 1) {
                size = MAXBYTELEN - 1;
            }
            if (memcpy_s(&length, MAXBYTELEN - 1, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(vector.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(length));
            ContainersVector::SetLength(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersVectorIncreaseCapacityToFuzzTest(const uint8_t* data, size_t size)
    {
        if (size <= 0) {
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();

            JSHandle<JSAPIVector> vector = GetVectorWithData(thread, data, size);

            uint32_t capacity = 0;
            if (size > MAXBYTELEN - 1) {
                size = MAXBYTELEN - 1;
            }
            if (memcpy_s(&capacity, MAXBYTELEN - 1, data, size) != 0) {
                std::cout << "memcpy_s failed!";
                UNREACHABLE();
            }

            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(vector.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(capacity));
            ContainersVector::IncreaseCapacityTo(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }

    static void ContainersVectorCopyToArrayFuzzTest(const uint8_t* data, size_t size)
    {
        if (size <= 0) {
            return;
        }
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();

            JSHandle<JSAPIVector> vector = GetVectorWithData(thread, data, size);

            constexpr int32_t ELEMENT_NUMS = 8;
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            JSHandle<JSArray> array = factory->NewJSArray();
            JSHandle<TaggedArray> arrayElement = factory->NewTaggedArray(ELEMENT_NUMS, JSTaggedValue::Hole());
            array->SetElements(thread, arrayElement);
            array->SetArrayLength(thread, static_cast<uint32_t>(ELEMENT_NUMS));
            auto callInfo = CreateEcmaRuntimeCallInfo(thread, 6); // 6 : means the argv length
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(vector.GetTaggedValue());
            callInfo->SetCallArg(0, array.GetTaggedValue());
            ContainersVector::CopyToArray(callInfo);
        }
        JSNApi::DestroyJSVM(vm);
    }
};
}
#endif