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

#include "stringrefwriteutf16_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "common_components/base/utf_helper.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace common::utf_helper;

namespace OHOS {
    void StringRefWriteUtf16FuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (data == nullptr || size <= 0) {
            LOG_ECMA(ERROR) << "illegal input!";
            return;
        }
        int length = size / sizeof(char16_t);
        char16_t* buffer = new char16_t[length];
        if (memset_s(buffer, length, 0, length) != EOK) {
            LOG_ECMA(ERROR) << "memset_s fail!";
            UNREACHABLE();
        }
        Local<StringRef> res = StringRef::NewFromUtf16(vm, (char16_t*)data, length);
        if (length == 1) {
            buffer[0] = '\0';
        } else if (length != 0) {
            int count = res->WriteUtf16(vm, buffer, length - 1);
            buffer[count] = '\0';
        }
        delete[] buffer;
        buffer = nullptr;
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::StringRefWriteUtf16FuzzTest(data, size);
    return 0;
}