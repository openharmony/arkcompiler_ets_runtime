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

#include "mergewithexistprofile_fuzzer.h"
#include <fuzzer/FuzzedDataProvider.h>
#include "ecmascript/pgo_profiler/pgo_profiler_encoder.h"
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::pgo;

namespace OHOS {
    void MergeWithExistProfileFuzzTest(const uint8_t* data, size_t size)
    {
        FuzzedDataProvider dataProvider(data, size);

        // generate random output path
        std::string path = "/tmp/fuzz_test" + std::to_string(dataProvider.ConsumeIntegral<uint32_t>());

        // generat random hotnessThreshold and ApGenMode
        uint32_t hotnessThreshold = dataProvider.ConsumeIntegral<uint32_t>();
        PGOProfilerEncoder::ApGenMode mode = static_cast<PGOProfilerEncoder::ApGenMode>(
            dataProvider.ConsumeIntegralInRange<int>(0, 1)
        );

        auto info = std::make_shared<PGOInfo>(hotnessThreshold);
        PGOProfilerEncoder encoder(path, mode);
        encoder.Save(info);
        std::remove(path.c_str());
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::MergeWithExistProfileFuzzTest(data, size);
    return 0;
}