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

#include <fuzzer/FuzzedDataProvider.h>
#include "jsvaluerefisarray_fuzzer.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
constexpr int32_t MAX_TYPED_ARRAY_BYTES = 1024;

void IsInt8ArrayFuzztest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    FuzzedDataProvider fdp(data, size);
    int32_t byteLength = fdp.ConsumeIntegralInRange<int32_t>(1, MAX_TYPED_ARRAY_BYTES);
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, byteLength);
    Local<JSValueRef> typedArray = Int8ArrayRef::New(vm, arrayBuffer, 0, byteLength);
    typedArray->IsInt8Array(vm);
    JSNApi::DestroyJSVM(vm);
}

void IsUint8ArrayFuzztest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    FuzzedDataProvider fdp(data, size);
    int32_t byteLength = fdp.ConsumeIntegralInRange<int32_t>(1, MAX_TYPED_ARRAY_BYTES);
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, byteLength);
    Local<JSValueRef> typedArray = Uint8ArrayRef::New(vm, arrayBuffer, 0, byteLength);
    typedArray->IsUint8Array(vm);
    JSNApi::DestroyJSVM(vm);
}

void IsUint8ClampedArrayFuzztest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    FuzzedDataProvider fdp(data, size);
    int32_t byteLength = fdp.ConsumeIntegralInRange<int32_t>(1, MAX_TYPED_ARRAY_BYTES);
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, byteLength);
    Local<JSValueRef> typedArray = Uint8ClampedArrayRef::New(vm, arrayBuffer, 0, byteLength);
    typedArray->IsUint8ClampedArray(vm);
    JSNApi::DestroyJSVM(vm);
}

void IsInt16ArrayFuzztest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    FuzzedDataProvider fdp(data, size);
    int32_t byteLength = fdp.ConsumeIntegralInRange<int32_t>(static_cast<int32_t>(sizeof(int16_t)),
        MAX_TYPED_ARRAY_BYTES);
    int32_t length = byteLength / static_cast<int32_t>(sizeof(int16_t));
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, byteLength);
    Local<JSValueRef> typedArray = Int16ArrayRef::New(vm, arrayBuffer, 0, length);
    typedArray->IsInt16Array(vm);
    JSNApi::DestroyJSVM(vm);
}

void IsUint16ArrayFuzztest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    FuzzedDataProvider fdp(data, size);
    int32_t byteLength = fdp.ConsumeIntegralInRange<int32_t>(static_cast<int32_t>(sizeof(uint16_t)),
        MAX_TYPED_ARRAY_BYTES);
    int32_t length = byteLength / static_cast<int32_t>(sizeof(uint16_t));
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, byteLength);
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
