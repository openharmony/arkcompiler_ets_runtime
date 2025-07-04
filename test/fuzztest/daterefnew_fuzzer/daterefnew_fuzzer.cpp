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

#include "daterefnew_fuzzer.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/ecma_string-inl.h"

using namespace panda;
using namespace panda::ecmascript;

#define MAXBYTELEN sizeof(double)

namespace OHOS {
    void DateRefNewFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        auto vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        double input = 0;
        if (size > MAXBYTELEN) {
            size = MAXBYTELEN;
        }
        if (memcpy_s(&input, MAXBYTELEN, data, size) != 0) {
            std::cout << "memcpy_s failed";
            UNREACHABLE();
        }
        if (std::isnan(input)) {
            input = ecmascript::base::NAN_VALUE;
        }
        DateRef::New(vm, input);
        JSNApi::DestroyJSVM(vm);
    }

    void DateRefGetTimeFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        auto vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        double input = 0;
        if (size > MAXBYTELEN) {
            size = MAXBYTELEN;
        }
        if (memcpy_s(&input, MAXBYTELEN, data, size) != 0) {
            std::cout << "memcpy_s failed";
            UNREACHABLE();
        }
        if (std::isnan(input)) {
            input = ecmascript::base::NAN_VALUE;
        }
        Local<DateRef> date = DateRef::New(vm, input);
        date->GetTime(vm);
        JSNApi::DestroyJSVM(vm);
    }

    void DateRefToStringFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        auto vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        double input = 0;
        if (size > MAXBYTELEN) {
            size = MAXBYTELEN;
        }
        if (memcpy_s(&input, MAXBYTELEN, data, size) != 0) {
            std::cout << "memcpy_s failed";
            UNREACHABLE();
        }
        if (std::isnan(input)) {
            input = ecmascript::base::NAN_VALUE;
        }
        Local<DateRef> date = DateRef::New(vm, input);
        date->ToString(vm);
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::DateRefNewFuzzTest(data, size);
    OHOS::DateRefGetTimeFuzzTest(data, size);
    OHOS::DateRefToStringFuzzTest(data, size);
    return 0;
}