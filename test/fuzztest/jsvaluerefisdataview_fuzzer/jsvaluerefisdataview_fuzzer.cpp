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
#include "jsvaluerefisdataview_fuzzer.h"

using namespace panda;
using namespace panda::ecmascript;
#define MAXBYTELEN sizeof(uint32_t)

namespace OHOS {
void JSValueRefIsDataViewFuzzerTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    [[maybe_unused]] auto date1 = data;
    if (size <= 0) {
        return;
    }
    if (size > MAXBYTELEN) {
        size = MAXBYTELEN;
    }
    const int32_t length = 15;
    uint32_t byteOffset = 5;
    uint32_t byteLength = 7;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, length);
    Local<DataViewRef> dataView = DataViewRef::New(vm, arrayBuffer, byteOffset, byteLength);
    dataView->IsDataView();
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::JSValueRefIsDataViewFuzzerTest(data, size);
    return 0;
}