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

#include "objectref_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;
using FunctionForRef = Local<JSValueRef>(*)(JsiRuntimeCallInfo*);
namespace OHOS {
    Local<JSValueRef> FuncRefNewCallbackForTest(JsiRuntimeCallInfo* info)
    {
        LOG_ECMA(ERROR) << "runing FuncRefNewCallbackForTest";
        EscapeLocalScope scope(info->GetVM());
        return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
    }
    void SetAccessorPropertyFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<JSValueRef> key = StringRef::NewFromUtf8(vm, (char*)data, (int)size);
        FunctionForRef nativeFunc = FuncRefNewCallbackForTest;
        Local<FunctionRef> getter = FunctionRef::New(vm, nativeFunc);
        Local<FunctionRef> setter = FunctionRef::New(vm, nativeFunc);
        Local<ObjectRef> object = ObjectRef::New(vm);
        object->SetAccessorProperty(vm, key, getter, setter);
        JSNApi::DestroyJSVM(vm);
    }

    void DefinePropertyFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        Local<ObjectRef> object = ObjectRef::New(vm);
        Local<JSValueRef> key = StringRef::NewFromUtf8(vm, (char*)data, (int)size);
        Local<JSValueRef> value = ObjectRef::New(vm);
        PropertyAttribute attribute(value, true, true, true);
        object->DefineProperty(vm, key, attribute);
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::SetAccessorPropertyFuzzTest(data, size);
    OHOS::DefinePropertyFuzzTest(data, size);
    return 0;
}