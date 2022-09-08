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

#include "objectdelete_fuzzer.h"

#include "ecmascript/base/string_helper.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    void ObjectDeleteFuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            return;
        }
        uint32_t key = 0;
        size_t maxByteLen = 4;
        if (size > maxByteLen) {
            size = maxByteLen;
        }
        if (memcpy_s(&key, maxByteLen, data, size) != EOK) {
            std::cout << "memcpy_s failed!";
            UNREACHABLE();
        }
        Local<ObjectRef> object = ObjectRef::New(vm);
        Local<JSValueRef> value(JSValueRef::Undefined(vm));
        object->Set(vm, key, value);
        object->Delete(vm, key);
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::ObjectDeleteFuzzTest(data, size);
    return 0;
}