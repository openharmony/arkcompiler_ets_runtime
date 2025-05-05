/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "jsonparse_fuzzer.h"

#include "ecmascript/base/utf_helper.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::base::utf_helper;

#define MAXBYTELEN sizeof(uint32_t)

namespace OHOS {
    void JSONParseFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        const char *const test{R"({"orientation": "portrait"})"};
        int32_t input = 0;
        if (size <= 0) {
            return;
        }
        if (size > MAXBYTELEN) {
            JSNApi::DestroyJSVM(vm);
            return;
        }
        if (memcpy_s(&input, MAXBYTELEN, data, size) != 0) {
            std::cout << "memcpy_s failed!";
            UNREACHABLE();
        }
        {
            JsiFastNativeScope scope(vm);
            Local<StringRef> res = StringRef::NewFromUtf8(vm, test, (int)size);
            Local<JSValueRef> jsValue = JSON::Parse(vm, res);
            JSON::Stringify(vm, jsValue);
        }
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::JSONParseFuzzTest(data, size);
    return 0;
}