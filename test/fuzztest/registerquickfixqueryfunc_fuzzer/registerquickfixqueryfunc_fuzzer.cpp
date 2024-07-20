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

#include "registerquickfixqueryfunc_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    bool QuickFixQueryFunc(std::string baseFileName, std::string &patchFileName, uint8_t **patchBuffer,
        size_t &patchBufferSize)
    {
        if (baseFileName != "multi_file/base/merge.abc") {
            return false;
        }
        patchFileName = "__index.pa";
        const char *data = R"(
            .function void foo() {}
        )";
        char *bufferData = new char[strlen(data) + 1];
        size_t dataSize = strlen(data) + 1;
        strcpy_s(bufferData, dataSize, data);
        *patchBuffer = reinterpret_cast<uint8_t *>(bufferData);
        patchBufferSize = strlen(data);
        return true;
    }
    void RegisterQuickFixQueryFuncFuzzerTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        JSNApi::RegisterQuickFixQueryFunc(vm, QuickFixQueryFunc);
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::RegisterQuickFixQueryFuncFuzzerTest(data, size);
    return 0;
}