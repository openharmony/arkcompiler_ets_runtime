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

#include "mergeapfilesformerger_fuzzer.h"
#include <fuzzer/FuzzedDataProvider.h>
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"
#include "ecmascript/mem/c_string.h"

#include <unordered_map>
#include <string>

using namespace panda;
using namespace panda::ecmascript::pgo;
using panda::ecmascript::CString;

namespace OHOS {
constexpr size_t MIN_FUZZ_INPUT = 8;
constexpr uint32_t FAKE_ABC_CHECKSUM_FOR_FUZZ = 123456;

void MergeApFilesForMergerFuzzTest(const uint8_t *data, size_t size)
{
    if (size < MIN_FUZZ_INPUT) {
        return;
    }
    // generate random file path
    std::string randomFilePath = "/tmp/fuzz_test_loadfull.profiler";

    // write file
    std::ofstream outFile(randomFilePath, std::ios::binary);
    if (outFile.is_open()) {
        outFile.write(reinterpret_cast<const char *>(data), size);
        outFile.close();
    } else {
        return;
    }

    uint32_t hotnessThreshold = 0;

    PGOProfilerDecoder decoder(randomFilePath, hotnessThreshold);

    std::unordered_map<CString, uint32_t> fileNameToChecksumMap;
    fileNameToChecksumMap["dummy.abc"] = FAKE_ABC_CHECKSUM_FOR_FUZZ;

    PGOProfilerManager::GetInstance()->MergeApFiles(fileNameToChecksumMap, decoder);

    std::remove(randomFilePath.c_str());

    return;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::MergeApFilesForMergerFuzzTest(data, size);
    return 0;
}
