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

#include <fuzzer/FuzzedDataProvider.h>
#include "containersfastbufferwritereadvalue_fuzzer.h"

#include "ecmascript/containers/containers_buffer.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_buffer.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/object_factory.h"

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
    JSTaggedValue InitializeFastBufferConstructor(JSThread *thread)
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
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::FastBuffer)));
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        return result;
    }

    JSHandle<JSAPIFastBuffer> CreateJSAPIFastBuffer(JSThread *thread, uint32_t length)
    {
        JSHandle<JSFunction> newTarget(thread, InitializeFastBufferConstructor(thread));
        auto objCallInfo = CreateEcmaRuntimeCallInfo(thread, 6);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());
        objCallInfo->SetCallArg(0, JSTaggedValue(length));

        JSTaggedValue result = ContainersBuffer::BufferConstructor(objCallInfo);
        JSHandle<JSAPIFastBuffer> buffer(thread, result);
        return buffer;
    }

    void ContainersFastBufferWriteReadValueOfFuzzTest(const uint8_t* data, size_t size)
    {
        constexpr size_t STRING_MAX_LENGTH = 100;
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        {
            JsiFastNativeScope scope(vm);
            auto thread = vm->GetAssociatedJSThread();

            const int32_t MaxMenory = 1024;
            if (size > MaxMenory) {
                size = MaxMenory;
            }
            FuzzedDataProvider fdp(data, size);
            JSHandle<JSAPIFastBuffer> buf1 = CreateJSAPIFastBuffer(thread, STRING_MAX_LENGTH);
                int32_t offset = fdp.ConsumeIntegral<int32_t>();
                int32_t value = fdp.ConsumeIntegral<int32_t>();
                double doubleValue = fdp.ConsumeFloatingPoint<double>();

                EcmaRuntimeCallInfo *callInfo1 = CreateEcmaRuntimeCallInfo(thread, 8);
                callInfo1->SetFunction(JSTaggedValue::Undefined());
                callInfo1->SetThis(buf1.GetTaggedValue());
                // 0 : means the first parameter
                callInfo1->SetCallArg(0, JSTaggedValue(value));
                // 1 : means the second parameter
                callInfo1->SetCallArg(1, JSTaggedValue(offset));
                [[maybe_unused]] JSTaggedValue ret = ContainersBuffer::WriteUInt8(callInfo1);
                ret = ContainersBuffer::WriteUInt8(callInfo1);
                ret = ContainersBuffer::WriteUInt16BE(callInfo1);
                ret = ContainersBuffer::WriteUInt16LE(callInfo1);
                ret = ContainersBuffer::WriteUInt32BE(callInfo1);
                ret = ContainersBuffer::WriteUInt32LE(callInfo1);
                ret = ContainersBuffer::WriteInt8(callInfo1);
                ret = ContainersBuffer::WriteInt16BE(callInfo1);
                ret = ContainersBuffer::WriteInt16LE(callInfo1);
                ret = ContainersBuffer::WriteInt32BE(callInfo1);
                ret = ContainersBuffer::WriteInt32LE(callInfo1);
                EcmaRuntimeCallInfo *callInfo2 = CreateEcmaRuntimeCallInfo(thread, 8);
                callInfo2->SetFunction(JSTaggedValue::Undefined());
                callInfo2->SetThis(buf1.GetTaggedValue());
                // 0 : means the first parameter
                callInfo2->SetCallArg(0, JSTaggedValue(doubleValue));
                // 1 : means the second parameter
                callInfo2->SetCallArg(1, JSTaggedValue(offset));
                ret = ContainersBuffer::WriteFloat32BE(callInfo2);
                ret = ContainersBuffer::WriteFloat32LE(callInfo2);
                ret = ContainersBuffer::WriteFloat64BE(callInfo2);
                ret = ContainersBuffer::WriteFloat64LE(callInfo2);
                
                EcmaRuntimeCallInfo *callInfo3 = CreateEcmaRuntimeCallInfo(thread, 8);
                callInfo3->SetFunction(JSTaggedValue::Undefined());
                callInfo3->SetThis(buf1.GetTaggedValue());
                // 0 : means the first parameter
                callInfo3->SetCallArg(0, JSTaggedValue(offset));
                ret = ContainersBuffer::ReadUInt8(callInfo3);
                ret = ContainersBuffer::ReadUInt16BE(callInfo3);
                ret = ContainersBuffer::ReadUInt16LE(callInfo3);
                ret = ContainersBuffer::ReadUInt32BE(callInfo3);
                ret = ContainersBuffer::ReadUInt32LE(callInfo3);
                ret = ContainersBuffer::ReadInt8(callInfo3);
                ret = ContainersBuffer::ReadInt16BE(callInfo3);
                ret = ContainersBuffer::ReadInt16LE(callInfo3);
                ret = ContainersBuffer::ReadInt32BE(callInfo3);
                ret = ContainersBuffer::ReadInt32LE(callInfo3);
                ret = ContainersBuffer::ReadFloat32BE(callInfo3);
                ret = ContainersBuffer::ReadFloat32LE(callInfo3);
                ret = ContainersBuffer::ReadFloat64BE(callInfo3);
                ret = ContainersBuffer::ReadFloat64LE(callInfo3);
        }
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::ContainersFastBufferWriteReadValueOfFuzzTest(data, size);
    return 0;
}