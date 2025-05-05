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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "maprefgetsetkeyvaluesize_fuzzer.h"

using namespace panda;
using namespace panda::ecmascript;
namespace OHOS {
void Int32GetSizeFuzzerTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm_ = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    char *value = new char[size]();
    if (memcpy_s(value, size, data, size) != EOK) {
        LOG_ECMA(ERROR) << "memcpy_s failed!";
    }
    Local<MapRef> object = MapRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, value, (int32_t)size);
    Local<JSValueRef> objectvalue = StringRef::NewFromUtf8(vm_, value, (int32_t)size);
    object->Set(vm_, key, objectvalue);
    object->GetSize(vm_);
    object->GetTotalElements(vm_);
    object->Get(vm_, key);
    object->GetKey(vm_, 0);
    object->GetValue(vm_, 0);
    JSNApi::DestroyJSVM(vm_);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::Int32GetSizeFuzzerTest(data, size);
    return 0;
}