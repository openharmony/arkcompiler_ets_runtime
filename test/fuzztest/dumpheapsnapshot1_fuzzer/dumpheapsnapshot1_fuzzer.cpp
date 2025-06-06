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

#include "dumpheapsnapshot1_fuzzer.h"

#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/ecma_string-inl.h"

using namespace panda;
using namespace panda::ecmascript;

#define MAXBYTELEN sizeof(int)

namespace OHOS {
    void DumpHeapSnapshot1FuzzTest(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        EcmaVM *vm = JSNApi::CreateJSVM(option);
        if (size <= 0) {
            return;
        }
        DumpFormat dumpFormat = DumpFormat::JSON;
        if (size > MAXBYTELEN) {
            size = MAXBYTELEN;
        }
        if (memcpy_s(&dumpFormat, MAXBYTELEN, data, size) != 0) {
            std::cout << "memcpy_s failed!";
            UNREACHABLE();
        }

        DumpSnapShotOption dumpOption;
        dumpOption.dumpFormat = dumpFormat;
        dumpOption.isVmMode = true;
        dumpOption.isPrivate = false;
        dumpOption.captureNumericValue = false;
        DFXJSNApi::DumpHeapSnapshot(vm, dumpOption);
        JSNApi::DestroyJSVM(vm);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::DumpHeapSnapshot1FuzzTest(data, size);
    return 0;
}