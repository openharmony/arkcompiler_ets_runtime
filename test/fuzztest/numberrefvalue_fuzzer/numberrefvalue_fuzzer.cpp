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

#include "numberrefvalue_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

#define MAXBYTELEN sizeof(double)

namespace OHOS {
void NumberRefValueFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    auto vm = JSNApi::CreateJSVM(option);
    double input = 0;
    if (data == nullptr || size <= 0) {
        std::cout << "illegal input!";
        return;
    }
    if (size > MAXBYTELEN) {
        size = MAXBYTELEN;
    }
    if (memcpy_s(&input, MAXBYTELEN, data, size) != 0) {
        std::cout << "memcpy_s failed !" << std::endl;
        UNREACHABLE();
    }
    Local<NumberRef> number = NumberRef::New(vm, input);
    number->Value();
    JSNApi::DestroyJSVM(vm);
}
}


// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::NumberRefValueFuzzTest(data, size);
    return 0;
}