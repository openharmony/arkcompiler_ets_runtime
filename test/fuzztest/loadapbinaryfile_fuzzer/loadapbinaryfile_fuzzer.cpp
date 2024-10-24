/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "loadapbinaryfile_fuzzer.h"
#include <fuzzer/FuzzedDataProvider.h>
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"

using namespace panda::ecmascript::pgo;

namespace OHOS {
    void LoadAPBinaryFileFuzzTest(const uint8_t* data, size_t size)
    {
        // generate random file path
        std::string randomFilePath = "/tmp/fuzz_test_loadfull.profiler";

        // write file
        std::ofstream outFile(randomFilePath, std::ios::binary);
        if (outFile.is_open()) {
            outFile.write(reinterpret_cast<const char*>(data), size);
            outFile.close();
        } else {
            return;
        }

        uint32_t hotnessThreshold = 0;
        PGOProfilerDecoder decoder(randomFilePath, hotnessThreshold);
        std::shared_ptr<PGOAbcFilePool> abcFilePool = std::make_shared<PGOAbcFilePool>();
        decoder.LoadFull(abcFilePool);

        // cleanup
        std::remove(randomFilePath.c_str());
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::LoadAPBinaryFileFuzzTest(data, size);
    return 0;
}