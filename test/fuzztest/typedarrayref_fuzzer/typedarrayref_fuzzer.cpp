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

#include "typedarrayref_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    void TypedArrayrefArrayLengthFuzzerTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        constexpr int byteOffset = 5;
        constexpr int length = 6;
        Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, (int32_t)size);
        Local<Int8ArrayRef> typedArray = Int8ArrayRef::New(vm, arrayBuffer, byteOffset, length);
        typedArray->ArrayLength(vm);
        JSNApi::DestroyJSVM(vm);
    }

    void TypedArrayrefByteOffSetFuzzerTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        constexpr int byteOffset = 5;
        constexpr int length = 6;
        Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, (int32_t)size);
        Local<Int8ArrayRef> typedArray = Int8ArrayRef::New(vm, arrayBuffer, byteOffset, length);
        typedArray->ByteOffset(vm);
        JSNApi::DestroyJSVM(vm);
    }

    void TypedArrayrefGetArrayBufferFuzzerTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        constexpr int byteOffset = 5;
        constexpr int length = 6;
        Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, (int32_t)size);
        Local<Int8ArrayRef> typedArray = Int8ArrayRef::New(vm, arrayBuffer, byteOffset, length);
        typedArray->GetArrayBuffer(vm);
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::TypedArrayrefArrayLengthFuzzerTest(data, size);
    OHOS::TypedArrayrefByteOffSetFuzzerTest(data, size);
    OHOS::TypedArrayrefGetArrayBufferFuzzerTest(data, size);
    return 0;
}