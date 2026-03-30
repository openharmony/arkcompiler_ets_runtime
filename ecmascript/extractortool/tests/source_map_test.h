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

#ifndef ECMASCRIPT_EXTRACTORTOOL_TESTS_SOURCE_MAP_TEST_H
#define ECMASCRIPT_EXTRACTORTOOL_TESTS_SOURCE_MAP_TEST_H

#include <cstdio>

#include "ecmascript/extractortool/src/source_map.h"

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
        "    \"entry-package-info\": \"entry|1.0.0\"\n"
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
        "    \"entry-package-info\": \"entry|1.0.0\"\n"
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
        "    \"entry-package-info\": \"entry|1.0.0\"\n"
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
        "    \"entry-package-info\": \"entry|1.0.0\"\n"
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
        "    \"entry-package-info\": \"entry|1.0.0\"\n"
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
        "    \"entry-package-info\": \"entry|1.0.0\"\n"
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
        "    \"entry-package-info\": \"entry|1.0.0\"\n"
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
        "    \"entry-package-info\": \"entry|1.0.0\"\n"
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
        "    \"entry-package-info\": \"entry|1.0.0\"\n"
        "  }\n"
        "}";
const std::string sourceMapData1 =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.ts\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [\n"
        "      \"entry/src/main/ets/pages/Index.ets\"\n"
        "    ],\n"
        "    \"names\": [],\n"
        "    \"mappings\": ;\n"
        "    \"entry-package-info\": \"entry|1.0.0\n"
        "    \"package-info\": \"library|1.0.0\"\n"
        "  }\n"
        "}";
const std::string sourceMapData2 =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.ts\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [\n"
        "      \"entry/src/main/ets/pages/Index.ets\"\n"
        "    ],\n"
        "    \"names\": [],\n"
        "    \"mappings\": ;\n"
        "    \"entry-package-info\": \"entry|1.0.0\"\n"
        "  }\n"
        "}";
const std::string sourceMapData3 =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.ts\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [\n"
        "      \"entry/src/main/ets/pages/Index.ets\"\n"
        "    ],\n"
        "    \"names\": [],\n"
        "    \"mappings\": ;\n"
        "    \"package-info\": \"library|1.0.0\"\n"
        "  }\n"
        "}";
const std::string sourceMapData4 =
        "{\n"
        "  \"entry/src/main/ets/pages/Index.ts\": {\n"
        "    \"version\": 3,\n"
        "    \"file\": \"Index.ets\",\n"
        "    \"sources\": [\n"
        "      \"entry/src/main/ets/pages/Index.ets\"\n"
        "    ],\n"
        "    \"names\": [],\n"
        "    \"mappings\": \";MAEO,CAAK,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,"
        "CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA,CAAA;\"\n"
        "  }\n"
        "}";
namespace panda::ecmascript {
class SourceMapFriend {
public:
    explicit SourceMapFriend(const std::string &str)
    {
        sourceMap_ = std::make_shared<SourceMap>();
        StringToUint8(str, sourceMap_->dataPtr_, sourceMap_->dataLen_);
        SplitSourceMap();
    }

    explicit SourceMapFriend(const std::string &str, int &durationCount)
    {
        sourceMap_ = std::make_shared<SourceMap>();
        StringToUint8(str, sourceMap_->dataPtr_, sourceMap_->dataLen_);
        auto start = Clock::now();
        SplitSourceMap();
        auto end = Clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        durationCount = duration.count();
    }

    uint32_t Base64CharToInt(char charCode)
    {
        return sourceMap_->Base64CharToInt(charCode);
    }

    void SplitSourceMap()
    {
        sourceMap_->SplitSourceMap();
    }

    void ExtractSourceMapData(const std::string &allmappings, std::shared_ptr<SourceMapData> &curMapData)
    {
        sourceMap_->ExtractSourceMapData(allmappings, curMapData.get());
    }

    MappingInfo Find(int32_t row, int32_t col, const SourceMapData &targetMap, bool& isReplaces)
    {
        return sourceMap_->Find(row, col, targetMap, isReplaces);
    }

    void GetPosInfo(const std::string &temp, int32_t start, std::string &line, std::string &column)
    {
        sourceMap_->GetPosInfo(temp, start, line, column);
    }

    std::vector<std::string> HandleMappings(const std::string &mapping)
    {
        return sourceMap_->HandleMappings(mapping);
    }

    bool VlqRevCode(const std::string &vStr, std::vector<int32_t> &ans)
    {
        return sourceMap_->VlqRevCode(vStr, ans);
    }

    bool GetLineAndColumnNumbers(int &line, int &column, SourceMapData &targetMap, bool& isReplaces)
    {
        return sourceMap_->GetLineAndColumnNumbers(line, column, targetMap, isReplaces);
    }

    void StringToUint8(const std::string &str, std::unique_ptr<uint8_t[]> &dataPtr, size_t &len)
    {
        len = str.size();
        auto dataTmp = std::make_unique<uint8_t[]>(len);
        std::copy(str.begin(), str.end(), dataTmp.get());
        dataPtr = std::move(dataTmp);
    }

    bool TranslateUrlPositionBySourceMap(std::string& url, int& line, int& column, std::string& packageName)
    {
        return sourceMap_->TranslateUrlPositionBySourceMap(url, line, column, packageName);
    }

private:
    std::shared_ptr<SourceMap> sourceMap_ {nullptr};
};
}
#endif  // ECMASCRIPT_DFX_STACKINFO_JS_STACKINFO_H
