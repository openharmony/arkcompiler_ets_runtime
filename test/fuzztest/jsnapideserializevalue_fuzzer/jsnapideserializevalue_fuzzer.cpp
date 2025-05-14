/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "jsnapideserializevalue_fuzzer.h"

#include "ecmascript/global_env.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    void JSNApiDeserializeValueFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            return;
        }
        double input = 0;
        const uint32_t doubleTypeSize = 8; // 8 : size of double type
        if (size > doubleTypeSize) {
            size = doubleTypeSize;
        }
        if (memcpy_s(&input, doubleTypeSize, data, size) != 0) {
            std::cout << "memcpy_s failed";
            UNREACHABLE();
        }
        Local<JSValueRef> value(NumberRef::New(vm, input));
        Local<JSValueRef> transfer(JSValueRef::Undefined(vm));
        void *serializationData = JSNApi::SerializeValue(vm, value, transfer, JSValueRef::Undefined(vm));
        void *hint = nullptr;
        JSNApi::DeserializeValue(vm, serializationData, hint);
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::JSNApiDeserializeValueFuzzTest(data, size);
    return 0;
}