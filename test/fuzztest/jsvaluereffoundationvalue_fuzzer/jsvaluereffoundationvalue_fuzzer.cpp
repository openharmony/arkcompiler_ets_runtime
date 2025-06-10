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

#include "jsvaluereffoundationvalue_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

#define MAXBYTELEN sizeof(int32_t)

namespace OHOS {
void JSValueRefIsNumberValueFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    int key = 0;
    uint32_t inputUnit32 = 32;
    if (data == nullptr || size <= 0) {
        std::cout << "illegal input!";
        return;
    }
    if (size > MAXBYTELEN) {
        size = MAXBYTELEN;
    }
    if (memcpy_s(&key, MAXBYTELEN, data, size) != EOK) {
        std::cout << "memcpy_s failed!";
        UNREACHABLE();
    }
    Local<IntegerRef> intValue = IntegerRef::New(vm, key);
    if (memcpy_s(&inputUnit32, MAXBYTELEN, data, size) != EOK) {
        std::cout << "memcpy_s failed!";
        UNREACHABLE();
    }
    Local<NumberRef> resUnit32 = NumberRef::New(vm, inputUnit32);
    bool inputBool = true;
    if (size == 0 || data == nullptr) {
        inputBool = false;
    }
    Local<BooleanRef> resBool = BooleanRef::New(vm, inputBool);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm, (char *)data, (int)size);
    intValue->IsNumber();
    resUnit32->IsNumber();
    resBool->IsNumber();
    stringUtf8->IsNumber();
    JSNApi::DestroyJSVM(vm);
}

void JSValueRefIsStringValueFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        std::cout << "illegal input!";
        return;
    }
    Local<JSValueRef> tag = StringRef::NewFromUtf8(vm, (char *)data, (int)size);
    tag->IsString(vm);
    JSNApi::DestroyJSVM(vm);
}

void JSValueRefWithinInt32ValueFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    int number = 0;
    if (data == nullptr || size <= 0) {
        std::cout << "illegal input!";
        return;
    }
    if (size > MAXBYTELEN) {
        size = MAXBYTELEN;
    }
    if (memcpy_s(&number, MAXBYTELEN, data, size) != 0) {
        std::cout << "memcpy_s failed!";
        UNREACHABLE();
    }
    Local<JSValueRef> tag = IntegerRef::New(vm, number);
    tag->WithinInt32();
    JSNApi::DestroyJSVM(vm);
}

Local<JSValueRef> FunCallback(JsiRuntimeCallInfo *info)
{
    EscapeLocalScope scope(info->GetVM());
    return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
}

void JSValueRefIsFunctionValueFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        std::cout << "illegal input!";
        return;
    }
    NativePointerCallback deleter = nullptr;
    FunctionCallback nativeFunc = FunCallback;
    Local<FunctionRef> obj(FunctionRef::NewClassFunction(vm, nativeFunc, deleter, (void *)(data + size)));
    (void)obj->IsFunction(vm);
    JSNApi::DestroyJSVM(vm);
}

void JSValueRefIsTypedArrayValueFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    int number = 123;
    if (data == nullptr || size <= 0) {
        std::cout << "illegal input!";
        return;
    }
    if (size > MAXBYTELEN) {
        size = MAXBYTELEN;
    }
    if (memcpy_s(&number, MAXBYTELEN, data, size) != 0) {
        std::cout << "memcpy_s failed!";
        UNREACHABLE();
    }
    Local<JSValueRef> targetUInt = IntegerRef::New(vm, number);
    targetUInt->IsTypedArray(vm);
    int32_t input;
    if (memcpy_s(&input, MAXBYTELEN, data, size) != 0) {
        std::cout << "memcpy_s failed!";
        UNREACHABLE();
    }
    const int32_t MaxMenory = 1024;
    if (input > MaxMenory) {
        input = MaxMenory;
    }
    Local<ArrayBufferRef> ref = ArrayBufferRef::New(vm, input);
    ref->IsArrayBuffer(vm);
    Local<Uint32ArrayRef> typedArray = Uint32ArrayRef::New(vm, ref, (int32_t)size, (int32_t)size);
    typedArray->IsTypedArray(vm);
    JSNApi::DestroyJSVM(vm);
}

void JSValueRefIsDateValueFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    int key = 0;
    uint32_t inputUnit32 = 32;
    if (data == nullptr || size <= 0) {
        std::cout << "illegal input!";
        return;
    }
    if (size > MAXBYTELEN) {
        size = MAXBYTELEN;
    }
    if (memcpy_s(&key, MAXBYTELEN, data, size) != EOK) {
        std::cout << "memcpy_s failed!";
        UNREACHABLE();
    }
    Local<IntegerRef> intValue = IntegerRef::New(vm, key);
    if (memcpy_s(&inputUnit32, MAXBYTELEN, data, size) != EOK) {
        std::cout << "memcpy_s failed!";
        UNREACHABLE();
    }
    Local<NumberRef> resUnit32 = NumberRef::New(vm, inputUnit32);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm, (char *)data, (int)size);

    double timeRef = 1.1;
    size_t maxByteLen = 8;
    if (size > maxByteLen) {
        size = maxByteLen;
    }
    if (memcpy_s(&timeRef, maxByteLen, data, size) != EOK) {
        std::cout << "memcpy_s failed!";
        UNREACHABLE();
    }
    Local<DateRef> dateRef = DateRef::New(vm, timeRef);
    resUnit32->IsDate(vm);
    intValue->IsDate(vm);
    stringUtf8->IsDate(vm);
    dateRef->IsDate(vm);
    JSNApi::DestroyJSVM(vm);
}

void JSValueRefIsErrorValueFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    int key = 0;
    uint32_t inputUnit32 = 32;
    if (data == nullptr || size <= 0) {
        std::cout << "illegal input!";
        return;
    }
    if (size > MAXBYTELEN) {
        size = MAXBYTELEN;
    }
    if (memcpy_s(&key, MAXBYTELEN, data, size) != EOK) {
        std::cout << "memcpy_s failed!";
        UNREACHABLE();
    }
    Local<IntegerRef> intValue = IntegerRef::New(vm, key);
    if (memcpy_s(&inputUnit32, MAXBYTELEN, data, size) != EOK) {
        std::cout << "memcpy_s failed!";
        UNREACHABLE();
    }
    Local<NumberRef> resUnit32 = NumberRef::New(vm, inputUnit32);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm, (char *)data, (int)size);
    Local<JSValueRef> error = Exception::Error(vm, stringUtf8);
    resUnit32->IsError(vm);
    intValue->IsError(vm);
    stringUtf8->IsError(vm);
    error->IsError(vm);
    JSNApi::DestroyJSVM(vm);
}

void JSValueRefToStringValueFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    int key = 0;
    uint32_t inputUnit32 = 32;
    if (data == nullptr || size <= 0) {
        std::cout << "illegal input!";
        return;
    }
    if (size > MAXBYTELEN) {
        size = MAXBYTELEN;
    }
    if (memcpy_s(&key, MAXBYTELEN, data, size) != EOK) {
        std::cout << "memcpy_s failed!";
        UNREACHABLE();
    }
    Local<IntegerRef> intValue = IntegerRef::New(vm, key);
    if (memcpy_s(&inputUnit32, MAXBYTELEN, data, size) != EOK) {
        std::cout << "memcpy_s failed!";
        UNREACHABLE();
    }
    Local<NumberRef> resUnit32 = NumberRef::New(vm, inputUnit32);
    bool inputBool = true;
    if (size == 0 || data == nullptr) {
        inputBool = false;
    }
    Local<BooleanRef> resBool = BooleanRef::New(vm, inputBool);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm, (char *)data, (int)size);
    Local<JSValueRef> toTarget(stringUtf8);
    intValue->ToString(vm);
    resUnit32->ToString(vm);
    resBool->ToString(vm);
    toTarget->ToString(vm);
    JSNApi::DestroyJSVM(vm);
}
}


// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::JSValueRefIsNumberValueFuzzTest(data, size);
    OHOS::JSValueRefIsStringValueFuzzTest(data, size);
    OHOS::JSValueRefWithinInt32ValueFuzzTest(data, size);
    OHOS::JSValueRefIsFunctionValueFuzzTest(data, size);
    OHOS::JSValueRefIsTypedArrayValueFuzzTest(data, size);
    OHOS::JSValueRefIsDateValueFuzzTest(data, size);
    OHOS::JSValueRefIsErrorValueFuzzTest(data, size);
    OHOS::JSValueRefToStringValueFuzzTest(data, size);
    return 0;
}
