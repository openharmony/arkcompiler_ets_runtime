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

#include "functionrefname_fuzzer.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
Local<JSValueRef> FuncRefConCallbackForTest(JsiRuntimeCallInfo *info)
{
    EscapeLocalScope scope(info->GetVM());
    return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
}

void FunctionRefSetNameFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    FunctionCallback nativeFunc = FuncRefConCallbackForTest;
    NativePointerCallback deleter = nullptr;
    Local<FunctionRef> func = FunctionRef::New(vm, nativeFunc, deleter, (void *)(data + size));
    Local<StringRef> name = StringRef::NewFromUtf8(vm, (char *)data, (int)size);
    func->SetName(vm, name);
    JSNApi::DestroyJSVM(vm);
}

void FunctionRefGetNameFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    FunctionCallback nativeFunc = FuncRefConCallbackForTest;
    NativePointerCallback deleter = nullptr;
    Local<FunctionRef> func = FunctionRef::New(vm, nativeFunc, deleter, (void *)(data + size));
    Local<StringRef> name = StringRef::NewFromUtf8(vm, (char *)data, (int)size);
    func->SetName(vm, name);
    func->GetName(vm);
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::FunctionRefSetNameFuzzTest(data, size);
    OHOS::FunctionRefGetNameFuzzTest(data, size);
    return 0;
}