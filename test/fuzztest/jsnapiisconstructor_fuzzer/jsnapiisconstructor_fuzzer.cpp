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


#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "jsnapiisconstructor_fuzzer.h"

using namespace panda;
using namespace panda::ecmascript;
namespace OHOS {
void JSNApiIsConstructorFuzzTest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *vm_ = JSNApi::CreateJSVM(option);
    if (size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    Local<IntegerRef> intValue = IntegerRef::New(vm_, (int)size);
    intValue->IsConstructor(vm_);
    JSNApi::DestroyJSVM(vm_);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::JSNApiIsConstructorFuzzTest(data, size);
    return 0;
}