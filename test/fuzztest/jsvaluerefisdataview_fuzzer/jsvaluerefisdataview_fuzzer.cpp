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

#include <fuzzer/FuzzedDataProvider.h>
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "jsvaluerefisdataview_fuzzer.h"

using namespace panda;
using namespace panda::ecmascript;
namespace OHOS {
void JSValueRefIsDataViewFuzzerTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    const int32_t bufferSize = fdp.ConsumeIntegralInRange<int32_t>(0, 1024);
    const int32_t dataSize = fdp.ConsumeIntegralInRange<int32_t>(0, 1023);
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, bufferSize);
    Local<DataViewRef> dataView = DataViewRef::New(vm, arrayBuffer, 0, dataSize);
    dataView->IsDataView(vm);
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