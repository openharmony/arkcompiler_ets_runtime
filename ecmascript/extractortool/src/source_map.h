/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_ABILITY_JS_ENVIRONMENT_SOURCE_MAP_H
#define OHOS_ABILITY_JS_ENVIRONMENT_SOURCE_MAP_H

#include <cstring>
#include <fstream>
#include <limits.h>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <thread>
#include <vector>

#include "ecmascript/log_wrapper.h"

namespace panda {
namespace ecmascript {
using Clock = std::chrono::high_resolution_clock;

struct SourceMapInfo {
    int32_t beforeRow = 0;
    int32_t beforeColumn = 0;
    int32_t afterRow = 0;
    int32_t afterColumn = 0;
    int32_t sourcesVal = 0;
    int32_t namesVal = 0;
};

struct MappingInfo {
    int32_t row = 0;
    int32_t col = 0;
};

class SourceMapData final {
public:
    SourceMapData() = default;
    ~SourceMapData() = default;

    std::string url_;
    SourceMapInfo nowPos_;
    std::vector<std::string> files_;
    std::vector<std::string> sources_;
    std::vector<std::string> names_;
    std::vector<std::string> mappings_;
    std::vector<SourceMapInfo> afterPos_;

    inline SourceMapData GetSourceMapData() const
    {
        return *this;
    }
};

class SourceMap final {
public:
    SourceMap() = default;
    ~SourceMap() = default;

#if defined(PANDA_TARGET_OHOS)
    void Init(const std::string& hapPath);
#endif
    void Init(uint8_t *data, size_t dataSize);
    bool TranslateUrlPositionBySourceMap(std::string& url, int& line, int& column);

private:
    void SplitSourceMap(const std::string& sourceMapData);
    void ExtractSourceMapData(const std::string& sourceMapData, std::shared_ptr<SourceMapData>& curMapData);
    void ExtractKeyInfo(const std::string& sourceMap, std::vector<std::string>& sourceKeyInfo);
    std::vector<std::string> HandleMappings(const std::string& mapping);
    bool VlqRevCode(const std::string& vStr, std::vector<int32_t>& ans);
    MappingInfo Find(int32_t row, int32_t col, const SourceMapData& targetMap, bool& isReplaces);
    void GetPosInfo(const std::string& temp, int32_t start, std::string& line, std::string& column);
    bool GetLineAndColumnNumbers(int& line, int& column, SourceMapData& targetMap, bool& isReplaces);
    uint32_t Base64CharToInt(char charCode);
    friend class SourceMapFriend;
#if defined(PANDA_TARGET_OHOS)
    bool ReadSourceMapData(const std::string& hapPath, std::string& content);
#endif

private:
    std::unordered_map<std::string, std::string> sources_;
    std::unordered_map<std::string, std::string> mappings_;
    std::unordered_map<std::string, std::shared_ptr<SourceMapData>> sourceMaps_;
};
} // namespace panda
} // namespace ecmascript

#endif // OHOS_ABILITY_JS_ENVIRONMENT_SOURCE_MAP_H
