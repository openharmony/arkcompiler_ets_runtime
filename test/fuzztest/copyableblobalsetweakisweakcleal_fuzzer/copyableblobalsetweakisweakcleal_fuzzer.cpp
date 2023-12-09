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

#include "copyableblobalsetweakisweakcleal_fuzzer.h"

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

template <typename T> void FreeGlobalCallBack(void *ptr)
{
    T *i = static_cast<T *>(ptr);
    UNUSED(i);
}
template <typename T> void NativeFinalizeCallback(void *ptr)
{
    T *i = static_cast<T *>(ptr);
    delete i;
}

namespace OHOS {
    void CopyAbleblobalSetweakIsweakCleal_FuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        EcmaVM *vm_ = JSNApi::CreateJSVM(option);
        [[maybe_unused]] auto date1 = data;
        if (size <= 0) {
            return;
        }
        std::string testUtf8 = "Hello world";
        Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
        CopyableGlobal<JSValueRef> copyGlobalback(vm_, stringUtf8);
        copyGlobalback.IsWeak();
        copyGlobalback.SetWeak();
        copyGlobalback.ClearWeak();
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::CopyAbleblobalSetweakIsweakCleal_FuzzTest(data, size);
    return 0;
}