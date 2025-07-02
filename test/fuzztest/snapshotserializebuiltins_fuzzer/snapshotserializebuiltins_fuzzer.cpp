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

#include <fuzzer/FuzzedDataProvider.h>
#include "snapshotserializebuiltins_fuzzer.h"

#include "ecmascript/log_wrapper.h"
#include "ecmascript/snapshot/mem/snapshot.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    void SnapshotSerializeBuiltinsFuzzTest(const uint8_t* data, size_t size)
    {
        // remove builtins.snapshot file first if exist
        const CString fileName = "builtins.snapshot";
        std::remove(fileName.c_str());
        // generate builtins.snapshot file
        FuzzedDataProvider fdp(data, size);
        const int arkProp1 = fdp.ConsumeIntegral<int>();
        RuntimeOption option1;
        option1.SetLogLevel(common::LOG_LEVEL::ERROR);
        option1.SetArkProperties(ArkProperties::ENABLE_SNAPSHOT_SERIALIZE | arkProp1);
        // create vm and generate builtins.snapshot file
        EcmaVM *vm1 = JSNApi::CreateJSVM(option1);
        if (size <= 0 || data == nullptr) {
            return;
        }
        JSNApi::DestroyJSVM(vm1);

        const int arkProp2 = fdp.ConsumeIntegral<int>();
        RuntimeOption option2;
        option2.SetLogLevel(common::LOG_LEVEL::ERROR);
        option2.SetArkProperties(ArkProperties::ENABLE_SNAPSHOT_DESERIALIZE | arkProp2);
        // create vm by deserialize builtins.snapshot file
        EcmaVM *vm2 = JSNApi::CreateJSVM(option2);
        JSNApi::DestroyJSVM(vm2);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::SnapshotSerializeBuiltinsFuzzTest(data, size);
    return 0;
}