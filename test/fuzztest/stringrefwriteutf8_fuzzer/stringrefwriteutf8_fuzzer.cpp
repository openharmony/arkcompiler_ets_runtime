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

#include "stringrefwriteutf8_fuzzer.h"

#include "ecmascript/base/string_helper.h"
#include "common_components/base/utf_helper.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace common::utf_helper;

bool createstatus = true;
namespace OHOS {
    void StringRefWriteUtf8FuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            return;
        }
        Local<StringRef> res = StringRef::StringRef::NewFromUtf8(vm, (char*)data, (int)size);
        char *value = new char[size]();
        if (memcpy_s(value, size, data, size) != EOK) {
            std::cout << "memcpy_s failed!";
            UNREACHABLE();
        }
        res->WriteUtf8(vm, value, (int)size);
        delete[] value;
        value = nullptr;
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::StringRefWriteUtf8FuzzTest(data, size);
    return 0;
}