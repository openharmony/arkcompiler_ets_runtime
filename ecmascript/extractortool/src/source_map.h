/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#ifndef PANDA_ECMASCRIPT_EXTRACTORTOOL_SOURCE_MAP_H
#define PANDA_ECMASCRIPT_EXTRACTORTOOL_SOURCE_MAP_H

#include <atomic>
#include <cstring>
#include <fstream>
#include <functional>
#include <limits.h>
#include <memory>
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
    std::string sources;
};

class SourceMapData final {
public:
    SourceMapData() = default;
    ~SourceMapData() = default;

    std::string sources_;
    std::string packageName_; // packageInfo or entryPackageInfo, preferentially use packageInfo
    bool isPackageInfo_ = false;
    SourceMapInfo nowPos_;
    std::vector<std::string> mappings_;
    std::vector<SourceMapInfo> afterPos_;

    inline SourceMapData GetSourceMapData() const
    {
        return *this;
    }
};

enum class InitStatus { NOT_EXECUTED, IN_EXECUTED, EXECUTED_SUCCESSFULLY };

class SourceMap final {
public:
    SourceMap() = default;
    ~SourceMap() = default;
    static SourceMap& GetInstance();
    SourceMap(const SourceMap&) = delete;
    SourceMap& operator=(const SourceMap&) = delete;

#if defined(PANDA_TARGET_OHOS)
    void Init(const std::string& hapPath);
#endif
    void SplitSourceMap(const std::string& sourceMapData);
    bool TranslateUrlPositionBySourceMap(std::string& url, int& line, int& column, std::string& packageName);
    std::string TranslateBySourceMap(const std::string& stackStr);
    static std::string ExtractFileName(const std::string& str);
    static void ExtractStackInfo(const std::string& stackStr, std::vector<std::string>& res);
    void SetInitStatus(InitStatus status);
    InitStatus GetInitStatus() const;

private:
    void SplitSourceMap();
    void ExtractSourceMapData(const std::string& allmappings, SourceMapData *curMapData);
    void ExtractSourceMapData(const std::string& allmappings, std::shared_ptr<SourceMapData>& curMapData);
    std::vector<std::string> HandleMappings(const std::string& mapping);
    uint32_t Base64CharToInt(char charCode);
    bool VlqRevCode(const std::string& vStr, std::vector<int32_t>& ans);
    MappingInfo Find(int32_t row, int32_t col, const SourceMapData& targetMap, bool& isReplaces);
    MappingInfo Find(int32_t row, int32_t col, const SourceMapData& targetMap, const std::string& key);
    void GetPosInfo(const std::string& temp, int32_t start, std::string& line, std::string& column);
    bool GetLineAndColumnNumbers(int& line, int& column, SourceMapData& targetMap, bool& isReplaces);
    bool GetLineAndColumnNumbers(int& line, int& column, SourceMapData& targetMap,
                                std::string& url, std::string& packageName);
    std::string GetSourceInfo(const std::string& line, const std::string& column,
                              const SourceMapData& targetMap, const std::string& key);
    static void GetPackageName(const SourceMapData& targetMap, std::string& packageName);
    friend class SourceMapFriend;
#if defined(PANDA_TARGET_OHOS)
    bool ReadSourceMapData(const std::string& hapPath);
#endif
    bool ParseSourceMapData(std::string_view url);
    std::string GetMappings(std::string_view sourcemap);
    std::string GetSources(std::string_view sourcemap);
    std::string GetEntryPackageInfo(std::string_view sourcemap);
    std::string GetPackageInfo(std::string_view sourcemap);
    std::string GetPackageName(std::string_view sourcemap);

private:
    static std::mutex sourceMapMutex_;

    // Zero-copy path (ets_runtime): raw buffer + lazy parsing
    std::unique_ptr<uint8_t[]> dataPtr_ {nullptr};
    size_t dataLen_ = 0;
    std::unordered_map<std::string_view, std::string_view> sourceMaps_;
    std::unordered_map<std::string_view, std::shared_ptr<SourceMapData>> sourceMapDatas_;

    // Eager parsing path (from ability_runtime's SplitSourceMap(string))
    std::unordered_map<std::string, std::shared_ptr<SourceMapData>> eagerSourceMaps_;

    std::atomic<InitStatus> initStatus_ = InitStatus::NOT_EXECUTED;
};
} // namespace panda
} // namespace ecmascript

#endif // PANDA_ECMASCRIPT_EXTRACTORTOOL_SOURCE_MAP_H
