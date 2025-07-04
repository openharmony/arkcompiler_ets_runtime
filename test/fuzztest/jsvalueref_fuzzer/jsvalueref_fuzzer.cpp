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

#include <fuzzer/FuzzedDataProvider.h>
#include "ecmascript/global_env.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "jsvalueref_fuzzer.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    void JSValueRefIsBooleanOrNullFuzzTest(const uint8_t* data, size_t size)
    {
        FuzzedDataProvider fdp(data, size);
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        bool inputBool = fdp.ConsumeBool();
        Local<JSValueRef> tag = BooleanRef::New(vm, inputBool);
        tag->IsBoolean();
        std::string str = fdp.ConsumeRandomLengthString(1024);
        Local<JSValueRef> obj = StringRef::NewFromUtf8(vm, str.data());
        obj->IsNull();
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    OHOS::JSValueRefIsBooleanOrNullFuzzTest(data, size);
    return 0;
}