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

#include "jsvaluerefiscorrect_fuzzer.h"
#include "common_components/base/utf_helper.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace common::utf_helper;

namespace OHOS {
void JSValueRefIsFalseFuzzTest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (size <= 0) {
        return;
    }
    Local<JSValueRef> object = JSValueRef::False(vm);
    object->IsFalse();
    JSNApi::DestroyJSVM(vm);
    return;
}

void JSValueRefIsTrueFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    int value;
    size = size > sizeof(int) ? sizeof(int) : size;
    if (memcpy_s(&value, sizeof(int), data, size) != EOK) {
        LOG_ECMA(ERROR) << "memcpy_s failed !";
        UNREACHABLE();
    }
    Local<JSValueRef> object = IntegerRef::New(vm, value);
    object->IsTrue();
    JSNApi::DestroyJSVM(vm);
    return;
}

void JSValueRefIsHoleFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    int value;
    size = size > sizeof(int) ? sizeof(int) : size;
    if (memcpy_s(&value, sizeof(int), data, size) != EOK) {
        LOG_ECMA(ERROR) << "memcpy_s failed!";
        UNREACHABLE();
    }
    Local<JSValueRef> object = IntegerRef::New(vm, value);
    object->IsHole();
    JSNApi::DestroyJSVM(vm);
    return;
}

void JSValueRefIsUndefinedFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    Local<JSValueRef> tag = StringRef::NewFromUtf8(vm, (char *)data, (int)size);
    tag->IsUndefined();
    JSNApi::DestroyJSVM(vm);
    return;
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::JSValueRefIsFalseFuzzTest(data, size);
    OHOS::JSValueRefIsTrueFuzzTest(data, size);
    OHOS::JSValueRefIsHoleFuzzTest(data, size);
    OHOS::JSValueRefIsUndefinedFuzzTest(data, size);
    return 0;
}