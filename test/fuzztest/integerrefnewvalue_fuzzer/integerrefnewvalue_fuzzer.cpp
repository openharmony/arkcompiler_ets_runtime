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

#include "integerrefnewvalue_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

#define MAXBYTELEN sizeof(uint32_t)
#define MAXBYTELENS sizeof(int32_t)

namespace OHOS {
void IntegerRefNewValueFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    unsigned int number = 123;
    if (data == nullptr || size <= 0) {
        std::cout << "illegal input!";
        return;
    }
    if (size > MAXBYTELEN) {
        size = MAXBYTELEN;
    }
    if (memcpy_s(&number, MAXBYTELEN, data, size) != 0) {
        std::cout << "memcpy_s failed!";
        UNREACHABLE();
    }
    Local<JSValueRef> targetUInt = IntegerRef::NewFromUnsigned(vm, number);
    targetUInt->Uint32Value(vm);

    int value = 0;
    if (memcpy_s(&value, MAXBYTELENS, data, size) != 0) {
        std::cout << "memcpy_s failed!";
        UNREACHABLE();
    }
    Local<IntegerRef> res = IntegerRef::New(vm, value);
    res->Value();
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::IntegerRefNewValueFuzzTest(data, size);
    return 0;
}