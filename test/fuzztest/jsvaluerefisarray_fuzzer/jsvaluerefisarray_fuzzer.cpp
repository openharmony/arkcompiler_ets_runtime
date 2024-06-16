/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "jsvaluerefisarray_fuzzer.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
void IsInt8ArrayFuzztest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    NativePointerCallback deleter = nullptr;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, (void *)data, (int32_t)size, deleter, (void *)data);
    Local<JSValueRef> typedArray = Int8ArrayRef::New(vm, arrayBuffer, 0, (int32_t)size);
    typedArray->IsInt8Array(vm);
    JSNApi::DestroyJSVM(vm);
}

void IsUint8ArrayFuzztest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    NativePointerCallback deleter = nullptr;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, (void *)data, (int32_t)size, deleter, (void *)data);
    Local<JSValueRef> typedArray = Uint8ArrayRef::New(vm, arrayBuffer, 0, (int32_t)size);
    typedArray->IsUint8Array(vm);
    JSNApi::DestroyJSVM(vm);
}

void IsUint8ClampedArrayFuzztest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    NativePointerCallback deleter = nullptr;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, (void *)data, (int32_t)size, deleter, (void *)data);
    Local<JSValueRef> typedArray = Uint8ClampedArrayRef::New(vm, arrayBuffer, 0, (int32_t)size);
    typedArray->IsUint8ClampedArray(vm);
    JSNApi::DestroyJSVM(vm);
}

void IsInt16ArrayFuzztest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    NativePointerCallback deleter = nullptr;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, (void *)data, (int32_t)size, deleter, (void *)data);
    int32_t length = size / sizeof(int16_t);
    Local<JSValueRef> typedArray = Int16ArrayRef::New(vm, arrayBuffer, 0, length);
    typedArray->IsInt16Array(vm);
    JSNApi::DestroyJSVM(vm);
}

void IsUint16ArrayFuzztest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    NativePointerCallback deleter = nullptr;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, (void *)data, (int32_t)size, deleter, (void *)data);
    int32_t length = size / sizeof(uint16_t);
    Local<JSValueRef> typedArray = Uint16ArrayRef::New(vm, arrayBuffer, 0, length);
    typedArray->IsUint16Array(vm);
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::IsInt8ArrayFuzztest(data, size);
    OHOS::IsUint8ArrayFuzztest(data, size);
    OHOS::IsUint8ClampedArrayFuzztest(data, size);
    OHOS::IsInt16ArrayFuzztest(data, size);
    OHOS::IsUint16ArrayFuzztest(data, size);
    return 0;
}