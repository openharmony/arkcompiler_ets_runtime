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

#include "jsnapiexception_fuzzer.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
void JSNApiThrowExceptionFuzztest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    Local<StringRef> message = StringRef::NewFromUtf8(vm, (char *)data, (int)size);
    Local<JSValueRef> error = Exception::Error(vm, message);
    JSNApi::ThrowException(vm, error); // Throwing anomalies
    JSNApi::PrintExceptionInfo(vm);    // Abnormal printing
    JSNApi::GetUncaughtException(vm);  // Get uncaught exceptions
    JSNApi::HasPendingException(vm);   // There are pending exceptions
    TryCatch tryCatch(vm);             // Capture exceptions
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::JSNApiThrowExceptionFuzztest(data, size);
    return 0;
}