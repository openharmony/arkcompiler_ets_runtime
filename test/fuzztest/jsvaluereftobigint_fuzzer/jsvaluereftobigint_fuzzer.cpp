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

#include "jsvaluereftobigint_fuzzer.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
void JSValueRefToBigIntFuzztest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    char *input = new char[size]();
    if (memcpy_s(input, size, data, size) != 0) {
        LOG_ECMA(ERROR) << "memcpy_s failed!";
        UNREACHABLE();
    }
    Local<JSValueRef> message = StringRef::NewFromUtf8(vm, input, (int)size);
    message->ToBigInt(vm);
    delete[] input;
    input = nullptr;
    JSNApi::DestroyJSVM(vm);
}

void JSValueRefTypeofFuzztest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    char *input = new char[size]();
    if (memcpy_s(input, size, data, size) != 0) {
        LOG_ECMA(ERROR) << "memcpy_s failed!";
        UNREACHABLE();
    }
    Local<JSValueRef> message = StringRef::NewFromUtf8(vm, input, (int)size);
    message->Typeof(vm);
    delete[] input;
    input = nullptr;
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::JSValueRefToBigIntFuzztest(data, size);
    OHOS::JSValueRefTypeofFuzztest(data, size);
    return 0;
}