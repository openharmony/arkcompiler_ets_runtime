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
#include "ecmascript/napi/include/jsnapi.h"
#include "objectrefsetprototype_fuzzer.h"

using namespace panda;
using namespace panda::ecmascript;
namespace OHOS {
void ObjectRefSetPrototypeFuzzTest([[maybe_unused]]const uint8_t *data, [[maybe_unused]]size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm_ = JSNApi::CreateJSVM(option);
    if (size <= 0) {
        LOG_ECMA(ERROR) << "Parameter out of range.";
        return;
    }
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<ObjectRef> prototype = object->GetPrototype(vm_);
    object->SetPrototype(vm_, prototype);
    JSNApi::DestroyJSVM(vm_);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::ObjectRefSetPrototypeFuzzTest(data, size);
    return 0;
}