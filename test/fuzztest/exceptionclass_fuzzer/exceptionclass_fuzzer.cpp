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

#include "exceptionclass_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/base/utf_helper.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::base::utf_helper;

namespace OHOS {
    void ExpectionErrorFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<StringRef> message = StringRef::NewFromUtf8(vm, (char*)data, (int)size);
        [[maybe_unused]]Local<JSValueRef> error = Exception::Error(vm, message);
        JSNApi::DestroyJSVM(vm);
    }

    void ExpectionRangeErrorFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<StringRef> message = StringRef::NewFromUtf8(vm, (char*)data, (int)size);
        [[maybe_unused]]Local<JSValueRef> error = Exception::RangeError(vm, message);
        JSNApi::DestroyJSVM(vm);
    }

    void ReferenceErrorFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<StringRef> message = StringRef::NewFromUtf8(vm, (char*)data, (int)size);
        [[maybe_unused]]Local<JSValueRef> error = Exception::ReferenceError(vm, message);
        JSNApi::DestroyJSVM(vm);
    }

    void SYNTexErrorFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<StringRef> message = StringRef::NewFromUtf8(vm, (char*)data, (int)size);
        [[maybe_unused]]Local<JSValueRef> error = Exception::SyntaxError(vm, message);
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::ExpectionErrorFuzzTest(data, size);
    OHOS::ExpectionRangeErrorFuzzTest(data, size);
    OHOS::ReferenceErrorFuzzTest(data, size);
    OHOS::SYNTexErrorFuzzTest(data, size);
    return 0;
}