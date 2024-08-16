/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/ohos/aot_runtime_info.h"
#include "setruntimeinfo_fuzzer.h"
#include <fuzzer/FuzzedDataProvider.h>

using namespace panda::ecmascript::ohos;

namespace OHOS {

class AotRuntimeInfoTest : public AotRuntimeInfo {
public:
    void TestSetRuntimeInfo(const char *realOutPath, char lines[][BUFFER_SIZE], int length) const
    {
        SetRuntimeInfo(realOutPath, lines, length);
    }

    void TestGetRuntimeInfoByPath(char lines[][BUFFER_SIZE], const char *realOutPath, const char *soBuildId) const
    {
        GetRuntimeInfoByPath(lines, realOutPath, soBuildId);
    }

    virtual bool TestGetRuntimeBuildId(char *buildId, int length) const
    {
        return GetRuntimeBuildId(buildId, length);
    }
};

    void SetRuntimeInfoFuzzTest([[maybe_unused]] const uint8_t *data, size_t size)
    {
        FuzzedDataProvider dataProvider(data, size);

        // Generate a random output file path
        std::string realOutPath = "/tmp/fuzz_test_" + std::to_string(dataProvider.ConsumeIntegral<uint32_t>());

        // Generate random lines content
        constexpr int bufferSize = 4096;
        constexpr int maxLength = 255;
        char lines[maxLength][bufferSize] = {};

        int lineCount = dataProvider.ConsumeIntegralInRange<int>(1, maxLength);
        for (int i = 0; i < lineCount; ++i) {
            std::string line = dataProvider.ConsumeRandomLengthString(bufferSize - 1);
        }

        AotRuntimeInfoTest runtimeInfoTest;
        runtimeInfoTest.TestSetRuntimeInfo(realOutPath.c_str(), lines, maxLength);

        // Clean up the created file
        unlink(realOutPath.c_str());
    };

    void GetRuntimeInfoByPathFuzzTest(const uint8_t *data, size_t size)
    {
        FuzzedDataProvider dataProvider(data, size);

        // Generate a random output file path
        std::string realOutPath = dataProvider.ConsumeRandomLengthString(AotRuntimeInfo::MAX_LENGTH);

        // Generate a random Build ID
        std::string soBuildId = dataProvider.ConsumeRandomLengthString(AotRuntimeInfo::BUFFER_SIZE - 1);

        // prepare lines
        char lines[AotRuntimeInfo::MAX_LENGTH][AotRuntimeInfo::BUFFER_SIZE] = {{0}};

        AotRuntimeInfoTest runtimeInfoTest;
        runtimeInfoTest.TestGetRuntimeInfoByPath(lines, realOutPath.c_str(), soBuildId.c_str());
    };

    void GetRuntimeBuildIdFuzzTest(const uint8_t *data, size_t size)
    {
        FuzzedDataProvider dataProvider(data, size);

        // generate a random buildIdLength
        int buildIdLength = dataProvider.ConsumeIntegralInRange<int>(1, PATH_MAX);

        // Generate a random Build ID
        char buildId[PATH_MAX] = {'\0'};

        AotRuntimeInfoTest runtimeInfoTest;
        runtimeInfoTest.TestGetRuntimeBuildId(buildId, buildIdLength);
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Run your code on data.
    OHOS::SetRuntimeInfoFuzzTest(data, size);
    OHOS::GetRuntimeInfoByPathFuzzTest(data, size);
    OHOS::GetRuntimeBuildIdFuzzTest(data, size);
    return 0;
}