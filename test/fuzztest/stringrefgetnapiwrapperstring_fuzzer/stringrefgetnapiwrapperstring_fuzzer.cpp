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

#include <fuzzer/FuzzedDataProvider.h>
#include "common_components/base/utf_helper.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "stringrefgetnapiwrapperstring_fuzzer.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace common::utf_helper;

namespace OHOS {
    void StringRefGetNapiWrapperStringFuzzTest(const uint8_t* data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);
        const int arkProp = fdp.ConsumeIntegral<int>();
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        option.SetArkProperties(arkProp);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        StringRef::GetNapiWrapperString(vm);
        JSNApi::DestroyJSVM(vm);
        return;
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::StringRefGetNapiWrapperStringFuzzTest(data, size);
    return 0;
}