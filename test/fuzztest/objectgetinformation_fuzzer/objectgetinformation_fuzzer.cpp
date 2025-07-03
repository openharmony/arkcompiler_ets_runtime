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
#include "objectgetinformation_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
void ObjectGetAllPropertyNamesFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    std::string str1 = fdp.ConsumeRandomLengthString(1024);
    void *ptr1 = static_cast<void *>(const_cast<char *>(str1.data()));
    std::string str2 = fdp.ConsumeRandomLengthString(1024);
    void *ptr2 = static_cast<void *>(const_cast<char *>(str2.data()));
    const int32_t index = fdp.ConsumeIntegralInRange<int32_t>(0, 1024);
    Local<ObjectRef> object = ObjectRef::New(vm);
    NativePointerCallback callBack = nullptr;
    object->SetNativePointerField(vm, index, ptr1, callBack, ptr2);
    object->GetAllPropertyNames(vm, index);
    JSNApi::DestroyJSVM(vm);
}

void ObjectGetNativePointerFieldCountFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    const int32_t key = fdp.ConsumeIntegralInRange<int32_t>(0, 1024);
    Local<ObjectRef> object = ObjectRef::New(vm);
    object->SetNativePointerFieldCount(vm, key);
    object->GetNativePointerFieldCount(vm);
    JSNApi::DestroyJSVM(vm);
}

void ObjectGetOwnEnumerablePropertyNamesFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    std::string str1 = fdp.ConsumeRandomLengthString(1024);
    void *ptr1 = static_cast<void *>(const_cast<char *>(str1.data()));
    std::string str2 = fdp.ConsumeRandomLengthString(1024);
    void *ptr2 = static_cast<void *>(const_cast<char *>(str2.data()));
    const int32_t index = fdp.ConsumeIntegralInRange<int32_t>(0, 1024);
    Local<ObjectRef> object = ObjectRef::New(vm);
    NativePointerCallback callBack = nullptr;
    object->SetNativePointerField(vm, index, ptr1, callBack, ptr2);
    object->GetOwnEnumerablePropertyNames(vm);
    JSNApi::DestroyJSVM(vm);
}

void ObjectGetOwnPropertyNamesFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    std::string str1 = fdp.ConsumeRandomLengthString(1024);
    void *ptr1 = static_cast<void *>(const_cast<char *>(str1.data()));
    std::string str2 = fdp.ConsumeRandomLengthString(1024);
    void *ptr2 = static_cast<void *>(const_cast<char *>(str2.data()));
    const int32_t index = fdp.ConsumeIntegralInRange<int32_t>(0, 1024);
    Local<ObjectRef> object = ObjectRef::New(vm);
    NativePointerCallback callBack = nullptr;
    object->SetNativePointerField(vm, index, ptr1, callBack, ptr2);
    object->GetOwnPropertyNames(vm);
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::ObjectGetAllPropertyNamesFuzzTest(data, size);
    OHOS::ObjectGetNativePointerFieldCountFuzzTest(data, size);
    OHOS::ObjectGetOwnEnumerablePropertyNamesFuzzTest(data, size);
    OHOS::ObjectGetOwnPropertyNamesFuzzTest(data, size);
    return 0;
}