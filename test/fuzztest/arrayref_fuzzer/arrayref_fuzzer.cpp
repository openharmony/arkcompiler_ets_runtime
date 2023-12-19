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

#include "arrayref_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    void ArrayRefNewFuzzerTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        ArrayRef::New(vm, (uint32_t)size);
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayRefGetValueAtFuzzerTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        Local<ObjectRef> globalObject = JSNApi::GetGlobalObject(vm);
        Local<ArrayRef> property = ArrayRef::New(vm, (uint32_t)size);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        [[maybe_unused]]Local<JSValueRef> value = property->GetValueAt(vm, globalObject, (uint32_t)size);
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayRefLengthFuzzerTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<ArrayRef> property = ArrayRef::New(vm, (uint32_t)size);
        property->Length(vm);
        JSNApi::DestroyJSVM(vm);
    }

    void ArrayRefSetValueAtFuzzerTest([[maybe_unused]]const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<ObjectRef> globalObject = JSNApi::GetGlobalObject(vm);
        Local<ArrayRef> property = ArrayRef::New(vm, (uint32_t)size);
        Local<JSValueRef> value = ObjectRef::New(vm);
        [[maybe_unused]]bool result = property->SetValueAt(vm, globalObject, (uint32_t)size, value);
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::ArrayRefNewFuzzerTest(data, size);
    OHOS::ArrayRefGetValueAtFuzzerTest(data, size);
    OHOS::ArrayRefLengthFuzzerTest(data, size);
    OHOS::ArrayRefSetValueAtFuzzerTest(data, size);
    return 0;
}