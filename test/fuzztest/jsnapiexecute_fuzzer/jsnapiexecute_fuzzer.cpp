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

#include "jsnapiexecute_fuzzer.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
constexpr size_t DIVISOR = 2;

void JSNApiExecuteFuzztest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    char *value = new char[size + 1]();
    if (memset_s(value, size + 1, 0, size + 1) != EOK) {
        LOG_ECMA(ERROR) << "memset_s failed!";
        UNREACHABLE();
    }
    if (memcpy_s(value, size + 1, data, size) != EOK) {
        LOG_ECMA(ERROR) << "memcpy_s failed!";
        UNREACHABLE();
    }
    value[size] = '\0';
    const std::string fileName = value;
    bool needUpdate = size % DIVISOR ? true : false; // 2:Cannot divide by 2 as true, otherwise it is false
    JSNApi::Execute(vm, fileName, fileName, needUpdate);
    delete[] value;
    value = nullptr;
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::JSNApiExecuteFuzztest(data, size);
    return 0;
}