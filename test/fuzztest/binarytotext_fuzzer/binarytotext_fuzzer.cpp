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

#include "binarytotext_fuzzer.h"

#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"

using namespace panda;
using namespace panda::ecmascript;

namespace OHOS {
    void BinaryToTextFuzzTest(const uint8_t* data, size_t size)
    {
        mkdir("ark-profiler11/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        std::ofstream file("ark-profiler11/modules.text");
        std::string result(data, data + size);
        file.write(result.c_str(), result.size());
        file.close();
        panda::ecmascript::pgo::PGOProfilerManager::GetInstance()->BinaryToText(
            "ark-profiler11/modules.ap", "ark-profiler11/modules_recover.text", 2); // 2 : means the hotness threshold
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::BinaryToTextFuzzTest(data, size);
    return 0;
}