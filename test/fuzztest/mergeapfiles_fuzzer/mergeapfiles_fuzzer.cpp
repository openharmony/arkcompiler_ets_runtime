/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "mergeapfiles_fuzzer.h"
#include <fuzzer/FuzzedDataProvider.h>
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"

using namespace panda;
using PGOProfilerManager = panda::ecmascript::pgo::PGOProfilerManager;
using ApGenMode = panda::ecmascript::pgo::ApGenMode;

namespace OHOS {
constexpr size_t MIN_FUZZ_INPUT = 8;

bool MergeApFilesFuzzTest(const uint8_t *data, size_t size)
{
    if (size < MIN_FUZZ_INPUT) {
        return false;
    }

    FuzzedDataProvider provider(data, size);

    std::string inFiles = "/tmp/fuzz_test_loadfull.ap";

    // write file
    std::ofstream outFile(inFiles, std::ios::binary);
    if (outFile.is_open()) {
        outFile.write(reinterpret_cast<const char *>(data), size);
        outFile.close();
    } else {
        return false;
    }

    std::string outPath = "/tmp/fuzz_test" + std::to_string(provider.ConsumeIntegral<uint32_t>());

    // generat random hotnessThreshold and ApGenMode
    uint32_t hotnessThreshold = provider.ConsumeIntegral<uint32_t>();
    ApGenMode mode = static_cast<ApGenMode>(provider.ConsumeIntegralInRange<int>(0, 1));

    bool result = PGOProfilerManager::GetInstance()->MergeApFiles(inFiles, outPath, hotnessThreshold, mode);

    std::remove(outPath.c_str());

    return result;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::MergeApFilesFuzzTest(data, size);
    return 0;
}