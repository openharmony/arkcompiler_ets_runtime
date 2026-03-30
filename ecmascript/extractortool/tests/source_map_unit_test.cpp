/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/extractortool/tests/source_map_test.h"

#include <cstdio>

#include "ecmascript/extractortool/src/source_map.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class SourceMapUnitTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
        instance->SetEnableForceGC(false);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance{nullptr};
    EcmaHandleScope *scope{nullptr};
    JSThread *thread{nullptr};
};

std::string CreateSourceMapData(size_t size)
{
    std::string sourceMapData = "{\n";
    int blockIndex = 1;

    for (;;) {
        std::string blockKey = "\"entry/src/main/ets/pages/Index" + std::to_string(blockIndex) + ".ts\"";
        std::string currentBlock =
            "  " + blockKey + ": {\n"
            "    \"version\": 3,\n"
            "    \"file\": \"Index.ets\",\n"
            "    \"sources\": [\n"
            "      \"entry/src/main/ets/pages/Index.ets\"\n"
            "    ],\n"
            "    \"names\": [],\n"
            "    \"mappings\": \";MAEO,CAAK,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,"
            "CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,"
            "CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,"
            "CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,"
            "CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA;"
            "AAFZ,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,"
            "CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,"
            "CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,"
            "CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA;\"\n"
            "    \"entry-package-info\": \"entry|1.0.0\"\n"
            "  }";

        sourceMapData += currentBlock;
        if (sourceMapData.size() > size) {
            sourceMapData += "\n}";
            break;
        }
        blockIndex++;
        sourceMapData += ",\n";
    }

    return sourceMapData;
}

double AverageWithoutMinMax(const std::vector<int>& vec)
{
    if (vec.size() <= 2) { // 2 means max & min
        return 0.0;
    }

    int sum = std::accumulate(vec.begin(), vec.end(), 0.0);

    int max = *std::max_element(vec.begin(), vec.end());
    int min = *std::min_element(vec.begin(), vec.end());

    return (sum - max - min) / (vec.size() - 2); // 2 means max & min
}

HWTEST_F_L0(SourceMapUnitTest, CreateSourceMapData100MBTest)
{
    constexpr double TIME_100MB = 120.0;
    constexpr size_t TARGET_SIZE_100MB = 100 * 1024 * 1024; // 100MB
    std::string largeSourceMapData = CreateSourceMapData(TARGET_SIZE_100MB);

    EXPECT_GE(largeSourceMapData.size(), TARGET_SIZE_100MB);

    std::vector<int> durations;
    durations.reserve(10);
    for (int i = 0; i < 10; i++) {
        int duration = 0;
        SourceMapFriend sourceMapFriend(largeSourceMapData, duration);
        durations.push_back(duration);
        std::string url = "entry/src/main/ets/pages/Index1.ts";
        int line = 10, column = 5;
        std::string packageName;
        EXPECT_TRUE(sourceMapFriend.TranslateUrlPositionBySourceMap(url, line, column, packageName));
        EXPECT_EQ(packageName, "entry");
    }
    double average = AverageWithoutMinMax(durations);
    GTEST_LOG_(INFO) << "CreateSourceMapData100MBTest expected value: " << TIME_100MB
                     << ", actual value: " << average;
    EXPECT_LE(average, TIME_100MB * 1.1);
}

HWTEST_F_L0(SourceMapUnitTest, CreateSourceMapData500MBTest)
{
    constexpr double TIME_500MB = 700.0;
    constexpr size_t TARGET_SIZE_500MB = 500 * 1024 * 1024; // 500MB
    std::string largeSourceMapData = CreateSourceMapData(TARGET_SIZE_500MB);

    EXPECT_GE(largeSourceMapData.size(), TARGET_SIZE_500MB);

    std::vector<int> durations;
    durations.reserve(10);
    for (int i = 0; i < 10; i++) {
        int duration = 0;
        SourceMapFriend sourceMapFriend(largeSourceMapData, duration);
        durations.push_back(duration);
        std::string url = "entry/src/main/ets/pages/Index1.ts";
        int line = 10, column = 5;
        std::string packageName;
        EXPECT_TRUE(sourceMapFriend.TranslateUrlPositionBySourceMap(url, line, column, packageName));
        EXPECT_EQ(packageName, "entry");
    }
    double average = AverageWithoutMinMax(durations);
    GTEST_LOG_(INFO) << "CreateSourceMapData100MBTest expected value: " << TIME_500MB
                     << ", actual value: " << average;
    EXPECT_LE(average, TIME_500MB * 1.1);
}
}