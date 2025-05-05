/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "jsnapiset_fuzzer.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
void JSNApiSetAssetPathFuzzTest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (size <= 0) {
        return;
    }
    std::string str = "/data/storage/el1/bundle/moduleName/ets/modules.abc";
    JSNApi::SetAssetPath(vm, str);
    JSNApi::DestroyJSVM(vm);
}

void JSNApiSetBundleFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    uint8_t *ptr = nullptr;
    ptr = const_cast<uint8_t *>(data);
    JSNApi::SetBundle(vm, (bool)size);
    JSNApi::DestroyJSVM(vm);
}

void JSNApiSetHostEnqueueJobFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm, (char *)data, (int)size);
    JSNApi::SetHostEnqueueJob(vm, key);
    JSNApi::DestroyJSVM(vm);
}

void JSNApiSetMockModuleListFuzzTest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (size <= 0) {
        return;
    }
    std::map<std::string, std::string> str = { { "10", "20" } };
    JSNApi::SetMockModuleList(vm, str);
    JSNApi::DestroyJSVM(vm);
}

void JSNApiSetSourceMapTranslateCallbackFuzzTest([[maybe_unused]]const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (size <= 0) {
        return;
    }
    SourceMapTranslateCallback tag { nullptr };
    JSNApi::SetSourceMapTranslateCallback(vm, tag);
    vm->GetSourceMapTranslateCallback();
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::JSNApiSetAssetPathFuzzTest(data, size);
    OHOS::JSNApiSetBundleFuzzTest(data, size);
    OHOS::JSNApiSetHostEnqueueJobFuzzTest(data, size);
    OHOS::JSNApiSetMockModuleListFuzzTest(data, size);
    OHOS::JSNApiSetSourceMapTranslateCallbackFuzzTest(data, size);
    return 0;
}