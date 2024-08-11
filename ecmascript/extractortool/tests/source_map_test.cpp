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

#include <cstdio>

#include "ecmascript/extractortool/src/source_map.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
const std::string sourceMapData =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.ts\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [\n"
        "      \"entry/src/main/ets/pages/Index.ets\"\n"
        "    ],\n"
        "    \"names\": [],\n"
        "    \"mappings\": \"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\"\n"
        "  }\n"
        "}";
const std::string sourceMapDataWithoutSources =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.ts\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [],\n"
        "    \"names\": [],\n"
        "    \"mappings\": \";MAEO,CAAK,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,"
        "CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA;\"\n"
        "  }\n"
        "}";
const std::string sourceMapDataWithShortMappings =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.ts\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [\n"
        "      \"entry/src/main/ets/pages/Index.ets\"\n"
        "    ],\n"
        "    \"names\": [],\n"
        "    \"mappings\": \"\n"
        "  }\n"
        "}";
const std::string sourceMapDataWithInvalidMappings1 =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.ts\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [\n"
        "      \"entry/src/main/ets/pages/Index.ets\"\n"
        "    ],\n"
        "    \"names\": [],\n"
        "    \"mappings\": pA;\n"
        "  }\n"
        "}";
const std::string sourceMapDataWithInvalidMappings2 =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.ts\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [\n"
        "      \"entry/src/main/ets/pages/Index.ets\"\n"
        "    ],\n"
        "    \"names\": [],\n"
        "    \"mappings\": pAq;ABCD;abcd;0123;0+/-;\n"
        "  }\n"
        "}";
const std::string sourceMapDataWithInvalidMappings3 =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.ts\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [\n"
        "      \"entry/src/main/ets/pages/Index.ets\"\n"
        "    ],\n"
        "    \"names\": [],\n"
        "    \"mappings\": X;\n"
        "  }\n"
        "}";
const std::string sourceMapDataWithInvalidMappings4 =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.ts\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [\n"
        "      \"entry/src/main/ets/pages/Index.ets\"\n"
        "    ],\n"
        "    \"names\": [],\n"
        "    \"mappings\": A;\n"
        "  }\n"
        "}";
const std::string sourceMapDataWithInvalidMappings5 =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.ts\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [\n"
        "      \"entry/src/main/ets/pages/Index.ets\"\n"
        "    ],\n"
        "    \"names\": [],\n"
        "    \"mappings\": ;\n"
        "  }\n"
        "}";
const std::string sourceMapDataWithJsSources =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.js\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [\n"
        "      \"entry/src/main/ets/pages/Index.js\"\n"
        "    ],\n"
        "    \"names\": [],\n"
        "    \"mappings\": ;\n"
        "  }\n"
        "}";

namespace panda::test {
class SourceMapTest : public testing::Test {
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

uint8_t *StringToUint8(const std::string &str)
{
    const auto *data = reinterpret_cast<const uint8_t *>(str.c_str());
    return const_cast<uint8_t *>(data);
}

SourceMap TestInit(const std::string &sourceMapData)
{
    uint8_t *data = StringToUint8(sourceMapData);
    size_t dataSize = sourceMapData.size();
    SourceMap sourceMap;
    sourceMap.Init(const_cast<uint8_t *>(data), dataSize);
    return sourceMap;
}

HWTEST_F_L0(SourceMapTest, TranslateUrlPositionBySourceMapTest)
{
    SourceMap sourceMap;
    int line = 10, column = 5;
    std::string url = "test.js";
    // if url contains ".js" returns true
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    // if it can't find sources which match url after init, returns false and throw Translate failed
    // e.g. 1. sourceMapData is valid, but url is not valid;
    sourceMap = TestInit(sourceMapData);
    url = "entry/src/main/ets/pages/Index1.ts";
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    // e.g. 2. sourceMapData is not valid
    sourceMap = TestInit("testInvalidSourceMapData");
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    sourceMap = TestInit(sourceMapDataWithoutSources);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_TRUE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    // if mappings is too short(<18), it will throw translate failed
    sourceMap = TestInit(sourceMapDataWithShortMappings);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    sourceMap = TestInit(sourceMapData);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_TRUE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    GTEST_LOG_(INFO) << "EXPECT111";

    // if sourceMapData was used twice, sourceMap should cache url/modularMap
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));
    GTEST_LOG_(INFO) << "EXPECT111";

    sourceMap = TestInit(sourceMapDataWithInvalidMappings1);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    sourceMap = TestInit(sourceMapDataWithInvalidMappings2);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    sourceMap = TestInit(sourceMapDataWithInvalidMappings3);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    sourceMap = TestInit(sourceMapDataWithInvalidMappings4);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    sourceMap = TestInit(sourceMapDataWithInvalidMappings5);
    url = "entry/src/main/ets/pages/Index.ts";
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    sourceMap = TestInit(sourceMapData);
    url = "entry/src/main/ets/pages/Index.ts";
    line = 0, column = 5;
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    sourceMap = TestInit(sourceMapData);
    url = "entry/src/main/ets/pages/Index.ts";
    line = 5, column = 0;
    EXPECT_FALSE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));

    // if sourceMapData is valid and url is end by ".js", it should return true
    url = "entry/src/main/ets/pages/Index.js";
    sourceMap = TestInit(sourceMapDataWithJsSources);
    EXPECT_TRUE(sourceMap.TranslateUrlPositionBySourceMap(url, line, column));
}
}