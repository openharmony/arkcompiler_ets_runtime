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

#include <fuzzer/FuzzedDataProvider.h>
#include "jsvaluereffoundationvalue_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
Local<JSValueRef> FunCallback(JsiRuntimeCallInfo *info)
{
    EscapeLocalScope scope(info->GetVM());
    return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
}

void JSValueRefFoundationValueFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        std::cout << "illegal input!";
        return;
    }

    FuzzedDataProvider fdp(data, size);
    int key = fdp.ConsumeIntegral<int>();
    uint32_t inputUnit32 = fdp.ConsumeIntegral<uint32_t>();
    bool inputBool = fdp.ConsumeBool();
    std::string inputStr = fdp.ConsumeRandomLengthString(1024);
    const int32_t bufferSize = fdp.ConsumeIntegralInRange<int32_t>(0, 1024);
    const int32_t byteOffset = fdp.ConsumeIntegral<int32_t>();
    const int32_t length = fdp.ConsumeIntegral<int32_t>();
    NativePointerCallback deleter = nullptr;
    FunctionCallback nativeFunc = FunCallback;
    void *ptr = static_cast<void *>(const_cast<char *>(inputStr.data()));
    double timeRef = fdp.ConsumeFloatingPoint<double>();

    Local<IntegerRef> intValue = IntegerRef::New(vm, key);
    Local<NumberRef> resUnit32 = NumberRef::New(vm, inputUnit32);
    Local<BooleanRef> resBool = BooleanRef::New(vm, inputBool);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm, inputStr.data());
    Local<JSValueRef> tag = IntegerRef::New(vm, key);
    Local<ArrayBufferRef> ref = ArrayBufferRef::New(vm, bufferSize);
    Local<Uint32ArrayRef> typedArray = Uint32ArrayRef::New(vm, ref, byteOffset, length);
    Local<FunctionRef> obj(FunctionRef::NewClassFunction(vm, nativeFunc, deleter, ptr));
    Local<DateRef> dateRef = DateRef::New(vm, timeRef);
    Local<JSValueRef> error = Exception::Error(vm, stringUtf8);
    Local<JSValueRef> toTarget(stringUtf8);

    intValue->IsNumber();
    resUnit32->IsNumber();
    resBool->IsNumber();
    stringUtf8->IsNumber();
    tag->IsNumber();
    ref->IsNumber();
    typedArray->IsNumber();
    obj->IsNumber();
    dateRef->IsNumber();
    error->IsNumber();
    toTarget->IsNumber();

    intValue->IsString(vm);
    resUnit32->IsString(vm);
    resBool->IsString(vm);
    stringUtf8->IsString(vm);
    tag->IsString(vm);
    ref->IsString(vm);
    typedArray->IsString(vm);
    obj->IsString(vm);
    dateRef->IsString(vm);
    error->IsString(vm);
    toTarget->IsString(vm);

    intValue->WithinInt32();
    resUnit32->WithinInt32();
    resBool->WithinInt32();
    stringUtf8->WithinInt32();
    tag->WithinInt32();
    ref->WithinInt32();
    typedArray->WithinInt32();
    obj->WithinInt32();
    dateRef->WithinInt32();
    error->WithinInt32();
    toTarget->WithinInt32();

    intValue->IsFunction(vm);
    resUnit32->IsFunction(vm);
    resBool->IsFunction(vm);
    stringUtf8->IsFunction(vm);
    tag->IsFunction(vm);
    ref->IsFunction(vm);
    typedArray->IsFunction(vm);
    obj->IsFunction(vm);
    dateRef->IsFunction(vm);
    error->IsFunction(vm);
    toTarget->IsFunction(vm);

    intValue->IsArrayBuffer(vm);
    resUnit32->IsArrayBuffer(vm);
    resBool->IsArrayBuffer(vm);
    stringUtf8->IsArrayBuffer(vm);
    tag->IsArrayBuffer(vm);
    ref->IsArrayBuffer(vm);
    typedArray->IsArrayBuffer(vm);
    obj->IsArrayBuffer(vm);
    dateRef->IsArrayBuffer(vm);
    error->IsArrayBuffer(vm);
    toTarget->IsArrayBuffer(vm);

    intValue->IsTypedArray(vm);
    resUnit32->IsTypedArray(vm);
    resBool->IsTypedArray(vm);
    stringUtf8->IsTypedArray(vm);
    tag->IsTypedArray(vm);
    ref->IsTypedArray(vm);
    typedArray->IsTypedArray(vm);
    obj->IsTypedArray(vm);
    dateRef->IsTypedArray(vm);
    error->IsTypedArray(vm);
    toTarget->IsTypedArray(vm);

    intValue->IsDate(vm);
    resUnit32->IsDate(vm);
    resBool->IsDate(vm);
    stringUtf8->IsDate(vm);
    tag->IsDate(vm);
    ref->IsDate(vm);
    typedArray->IsDate(vm);
    obj->IsDate(vm);
    dateRef->IsDate(vm);
    error->IsDate(vm);
    toTarget->IsDate(vm);

    intValue->IsError(vm);
    resUnit32->IsError(vm);
    resBool->IsError(vm);
    stringUtf8->IsError(vm);
    tag->IsError(vm);
    ref->IsError(vm);
    typedArray->IsError(vm);
    obj->IsError(vm);
    dateRef->IsError(vm);
    error->IsError(vm);
    toTarget->IsError(vm);

    intValue->ToString(vm);
    resUnit32->ToString(vm);
    resBool->ToString(vm);
    stringUtf8->ToString(vm);
    tag->ToString(vm);
    ref->ToString(vm);
    typedArray->ToString(vm);
    obj->ToString(vm);
    dateRef->ToString(vm);
    error->ToString(vm);
    toTarget->ToString(vm);

    JSNApi::DestroyJSVM(vm);
}
}


// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::JSValueRefFoundationValueFuzzTest(data, size);
    return 0;
}
