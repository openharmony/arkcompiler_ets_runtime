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

#include "bufferrefnew_fuzzer.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
void BufferRefNewFuzzTest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    BufferRef::New(vm, (int32_t)size);
    JSNApi::DestroyJSVM(vm);
}

void BufferRefMultiParamNewFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    NativePointerCallback deleter = nullptr;
    BufferRef::New(vm, (void *)data, (int32_t)size, deleter, (void *)data);
    JSNApi::DestroyJSVM(vm);
}

void BufferRefByteLengthFuzzTest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    Local<BufferRef> buffer = BufferRef::New(vm, (int32_t)size);
    buffer->ByteLength(vm);
    JSNApi::DestroyJSVM(vm);
}

void BufferRefGetBufferFuzzTest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    Local<BufferRef> buffer = BufferRef::New(vm, (int32_t)size);
    buffer->GetBuffer(vm);
    JSNApi::DestroyJSVM(vm);
}

void BufferToStringCallbackFuzzTest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    Local<BufferRef> buffer = BufferRef::New(vm, (int32_t)size);
    buffer->ToString(vm);
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::BufferRefNewFuzzTest(data, size);
    OHOS::BufferRefMultiParamNewFuzzTest(data, size);
    OHOS::BufferRefByteLengthFuzzTest(data, size);
    OHOS::BufferRefGetBufferFuzzTest(data, size);
    OHOS::BufferToStringCallbackFuzzTest(data, size);
    return 0;
}