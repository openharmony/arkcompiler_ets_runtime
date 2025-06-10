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

#include "jsnapistopdebugger_fuzzer.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
void JSNApiStopDebuggerFuzzTest(const uint8_t *data, size_t size)
{
    RuntimeOption option;
    option.SetLogLevel(common::LOG_LEVEL::ERROR);
    EcmaVM *vm = JSNApi::CreateJSVM(option);
    if (data == nullptr || size <= 0) {
        LOG_ECMA(ERROR) << "illegal input!";
        return;
    }
    int32_t instanceId = 0;
    const int32_t int32ByteLen = 4;
    if (size > int32ByteLen) {
        size = int32ByteLen;
    }
    if (memcpy_s(&instanceId, int32ByteLen, data, size) != 0) {
        LOG_ECMA(ERROR) << "memcpy_s failed !";
        UNREACHABLE();
    }
    JSNApi::DebugOption const debugOption = { nullptr, true, -1 };
    JSNApi::StartDebugger(vm, debugOption, instanceId);
    JSNApi::StopDebugger(vm);
    JSNApi::DestroyJSVM(vm);
}
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::JSNApiStopDebuggerFuzzTest(data, size);
    return 0;
}