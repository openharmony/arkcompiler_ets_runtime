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

#include "arraybufferrefnewwithtwoparameters_fuzzer.h"

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

#define MAXBYTELEN sizeof(int32_t)

namespace OHOS {
void ArrayBufferRefNewWithTwoParametersFuzzTest(const uint8_t* data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    int32_t input;
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
    ArrayBufferRef::New(vm, input);
    JSNApi::DestroyJSVM(vm);
}

void ArrayBufferRef_New_IsDetach_Detach_ByteLength_GetBuffer_FuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    int32_t input;
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
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, input);
    arrayBuffer->IsDetach(vm);
    arrayBuffer->Detach(vm);
    arrayBuffer->ByteLength(vm);
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::ArrayBufferRefNewWithTwoParametersFuzzTest(data, size);
    OHOS::ArrayBufferRef_New_IsDetach_Detach_ByteLength_GetBuffer_FuzzTest(data, size);
    return 0;
}