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
#include "uint32arrayrefbytelength_fuzzer.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
void Uint32ArrayRefByteLengthFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (size <= 0) {
        return;
    }

    FuzzedDataProvider fdp(data, size);
    const int32_t bufferSize = fdp.ConsumeIntegralInRange<int32_t>(0, 1024);
    const int32_t byteOffset = fdp.ConsumeIntegral<int32_t>();
    const int32_t length = fdp.ConsumeIntegral<int32_t>();

    Local<ArrayBufferRef> ref = ArrayBufferRef::New(vm, bufferSize);
    Local<Uint32ArrayRef> typedArray = Uint32ArrayRef::New(vm, ref, byteOffset, length);
    typedArray->ByteLength(vm);
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::Uint32ArrayRefByteLengthFuzzTest(data, size);
    return 0;
}