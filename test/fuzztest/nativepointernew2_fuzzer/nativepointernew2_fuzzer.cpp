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

#include <fuzzer/FuzzedDataProvider.h>
#include "nativepointernew2_fuzzer.h"

#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    void NativePointerNew2FuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            return;
        }
        FuzzedDataProvider fdp(data, size);
        std::string str1 = fdp.ConsumeRandomLengthString(1024);
        void *ptr1 = static_cast<void *>(const_cast<char *>(str1.data()));
        std::string str2 = fdp.ConsumeRandomLengthString(1024);
        void *ptr2 = static_cast<void *>(const_cast<char *>(str2.data()));
        NativePointerCallback callBack = nullptr;
        NativePointerRef::New(vm, ptr1, callBack, ptr2);
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::NativePointerNew2FuzzTest(data, size);
    return 0;
}