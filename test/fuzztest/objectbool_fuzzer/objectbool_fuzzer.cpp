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

#include "objectbool_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
void ObjectBoolGetFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    Local<ObjectRef> object = ObjectRef::New(vm);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm, (char *)data, (int)size);
    object->Get(vm, key);
    JSNApi::DestroyJSVM(vm);
}

void ObjectBoolHasFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    Local<ObjectRef> object = ObjectRef::New(vm);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm, (char *)data, (int)size);
    object->Has(vm, key);
    JSNApi::DestroyJSVM(vm);
}

void ObjectBoolSetFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    Local<ObjectRef> object = ObjectRef::New(vm);
    Local<JSValueRef> value(JSValueRef::Undefined(vm));
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm, (char *)data, (int)size);
    object->Set(vm, key, value);
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::ObjectBoolGetFuzzTest(data, size);
    OHOS::ObjectBoolHasFuzzTest(data, size);
    OHOS::ObjectBoolSetFuzzTest(data, size);
    return 0;
}