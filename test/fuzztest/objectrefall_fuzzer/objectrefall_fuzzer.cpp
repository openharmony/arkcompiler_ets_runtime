/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecmascript/base/string_helper.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "objectrefall_fuzzer.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    void ObjectGetPrototypeFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        uint8_t *ptr = nullptr;
        size_t temp = 0;
        ptr = const_cast<uint8_t*>(data);
        temp = size;
        Local<FunctionRef> object = ObjectRef::New(vm);
        object->GetPrototype(vm);
        JSNApi::DestroyJSVM(vm);
    }

    void ObjectSealFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        uint8_t *ptr = nullptr;
        size_t temp = 0;
        ptr = const_cast<uint8_t*>(data);
        temp = size;
        Local<ObjectRef> object = ObjectRef::New(vm);
        object->Seal(vm);
        JSNApi::DestroyJSVM(vm);
    }

    void ObjectFreezeFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        uint8_t *ptr = nullptr;
        size_t temp = 0;
        ptr = const_cast<uint8_t*>(data);
        temp = size;
        Local<ObjectRef> object = ObjectRef::New(vm);
        object->Freeze(vm);
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::ObjectGetPrototypeFuzzTest(data, size);
    OHOS::ObjectSealFuzzTest(data, size);
    OHOS::ObjectFreezeFuzzTest(data, size);
    return 0;
}