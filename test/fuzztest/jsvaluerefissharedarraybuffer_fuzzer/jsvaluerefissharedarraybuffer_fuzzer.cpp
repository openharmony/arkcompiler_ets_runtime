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

#include "ecmascript/containers/containers_arraylist.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "jsvaluerefissharedarraybuffer_fuzzer.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;
namespace OHOS {
void JSValueRefIsWeakMapFuzzerTest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm_ = JSNApi::CreateJSVM(option);
    if (size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    Local<JSValueRef> sharedArrayBuffer = ArrayBufferRef::New(vm_, (int32_t)size);
    sharedArrayBuffer->IsSharedArrayBuffer(vm_);
    JSNApi::DestroyJSVM(vm_);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::JSValueRefIsWeakMapFuzzerTest(data, size);
    return 0;
}