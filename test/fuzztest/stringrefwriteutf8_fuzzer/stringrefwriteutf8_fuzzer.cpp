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

#include "ecmascript/base/utf_helper.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/tooling/debugger_service.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::base::utf_helper;

bool createstatus = true;
namespace OHOS {
    void StringRefWriteUtf8FuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0 || size >= 5) { // 5:Utf8 character size
            JSNApi::DestroyJSVM(vm);
            return;
        }
        std::vector<uint8_t> vec;
        for (size_t i = 0; i<size; i++) {
            vec.push_back(data[i]);
        }

        if (!IsValidUTF8(vec)) {
            JSNApi::DestroyJSVM(vm);
            return;
        }
        Local<StringRef> res = StringRef::StringRef::NewFromUtf8(vm, (char*)data, (int)size);
        res->WriteUtf8((char*)(data), (int)size);
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