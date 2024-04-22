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

#include "ecmascript/extractortool/src/extractor.h"
#include "source_map.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <unistd.h>

namespace panda {
namespace ecmascript {
namespace {
constexpr char SOURCES[] = "sources";
constexpr char MAPPINGS[] = "mappings";
constexpr char DELIMITER_COMMA = ',';
constexpr char DELIMITER_SEMICOLON = ';';
constexpr char DOUBLE_SLASH = '\\';
constexpr char WEBPACK[] = "webpack:///";
constexpr int32_t INDEX_ONE = 1;
constexpr int32_t INDEX_TWO = 2;
constexpr int32_t INDEX_THREE = 3;
constexpr int32_t INDEX_FOUR = 4;
constexpr int32_t ANS_MAP_SIZE = 5;
constexpr int32_t DIGIT_NUM = 64;
const std::string MEGER_SOURCE_MAP_PATH = "ets/sourceMaps.map";
} // namespace

#if defined(PANDA_TARGET_OHOS)
bool SourceMap::ReadSourceMapData(const std::string& hapPath, std::string& content)
{
    if (hapPath.empty()) {
        return false;
    }
    bool newCreate = false;
    std::shared_ptr<Extractor> extractor = ExtractorUtil::GetExtractor(
        ExtractorUtil::GetLoadFilePath(hapPath), newCreate);
    if (extractor == nullptr) {
        return false;
    }
    std::unique_ptr<uint8_t[]> dataPtr = nullptr;
    size_t len = 0;
    if (!extractor->ExtractToBufByName(MEGER_SOURCE_MAP_PATH, dataPtr, len)) {
        return false;
    }
    content.assign(dataPtr.get(), dataPtr.get() + len);
    return true;
}
#endif

int32_t StringToInt(const std::string& value)
{
    errno = 0;
    char* pEnd = nullptr;
    int64_t result = std::strtol(value.c_str(), &pEnd, 10);
    if (pEnd == value.c_str() || (result < INT_MIN || result > INT_MAX) || errno == ERANGE) {
        return 0;
    } else {
        return result;
    }
}

uint32_t Base64CharToInt(char charCode)
{
    if ('A' <= charCode && charCode <= 'Z') {
        // 0 - 25: ABCDEFGHIJKLMNOPQRSTUVWXYZ
        return charCode - 'A';
    } else if ('a' <= charCode && charCode <= 'z') {
        // 26 - 51: abcdefghijklmnopqrstuvwxyz
        return charCode - 'a' + 26;
    } else if ('0' <= charCode && charCode <= '9') {
        // 52 - 61: 0123456789
        return charCode - '0' + 52;
    } else if (charCode == '+') {
        // 62: +
        return 62;
    } else if (charCode == '/') {
        // 63: /
        return 63;
    }
    return DIGIT_NUM;
};

#if defined(PANDA_TARGET_OHOS)
void SourceMap::Init(const std::string& hapPath)
{
    std::string sourceMapData;
    if (ReadSourceMapData(hapPath, sourceMapData)) {
        SplitSourceMap(sourceMapData);
    }
}
#endif

void SourceMap::Init(uint8_t *data, size_t dataSize)
{
    std::string content;
    content.assign(data, data + dataSize);
    SplitSourceMap(content);
}

void SourceMap::SplitSourceMap(const std::string& sourceMapData)
{
    size_t urlLeft = 0;
    size_t urlRight = 0;
    size_t leftBracket = 0;
    size_t rightBracket = 0;
    while ((leftBracket = sourceMapData.find(": {", rightBracket)) != std::string::npos) {
        urlLeft = leftBracket;
        urlRight = sourceMapData.find("  \"", rightBracket) + INDEX_THREE;
        if (urlRight == std::string::npos) {
            continue;
        }
        std::string url = sourceMapData.substr(urlRight, urlLeft - urlRight - INDEX_ONE);
        rightBracket = sourceMapData.find("},", leftBracket);
        std::string value = sourceMapData.substr(leftBracket, rightBracket);
        sourceMapsData_.emplace(url, value);
    }
}

void SourceMap::ExtractSourceMapData(const std::string& sourceMapData, std::shared_ptr<SourceMapData>& curMapData)
{
    std::vector<std::string> sourceKey;
    ExtractKeyInfo(sourceMapData, sourceKey);

    std::string mark = "";
    for (auto sourceKeyInfo : sourceKey) {
        if (sourceKeyInfo == SOURCES || sourceKeyInfo == MAPPINGS) {
            mark = sourceKeyInfo;
        } else if (mark == SOURCES) {
            curMapData->sources_.push_back(sourceKeyInfo);
        } else if (mark == MAPPINGS) {
            curMapData->mappings_.push_back(sourceKeyInfo);
        }
    }

    if (curMapData->mappings_.empty()) {
        return;
    }

    // transform to vector for mapping easily
    curMapData->mappings_ = HandleMappings(curMapData->mappings_[0]);

    // the first bit: the column after transferring.
    // the second bit: the source file.
    // the third bit: the row before transferring.
    // the fourth bit: the column before transferring.
    // the fifth bit: the variable name.
    for (const auto& mapping : curMapData->mappings_) {
        if (mapping == ";") {
            // plus a line for each semicolon
            curMapData->nowPos_.afterRow++,
            curMapData->nowPos_.afterColumn = 0;
            continue;
        }
        std::vector<int32_t> ans;

        if (!VlqRevCode(mapping, ans)) {
            return;
        }
        if (ans.empty()) {
            break;
        }
        if (ans.size() == 1) {
            curMapData->nowPos_.afterColumn += ans[0];
            continue;
        }
        // after decode, assgin each value to the position
        curMapData->nowPos_.afterColumn += ans[0];
        curMapData->nowPos_.sourcesVal += ans[INDEX_ONE];
        curMapData->nowPos_.beforeRow += ans[INDEX_TWO];
        curMapData->nowPos_.beforeColumn += ans[INDEX_THREE];
        if (ans.size() == ANS_MAP_SIZE) {
            curMapData->nowPos_.namesVal += ans[INDEX_FOUR];
        }
        curMapData->afterPos_.push_back({
            curMapData->nowPos_.beforeRow,
            curMapData->nowPos_.beforeColumn,
            curMapData->nowPos_.afterRow,
            curMapData->nowPos_.afterColumn,
            curMapData->nowPos_.sourcesVal,
            curMapData->nowPos_.namesVal
        });
    }
    curMapData->mappings_.clear();
    curMapData->mappings_.shrink_to_fit();
}

MappingInfo SourceMap::Find(int32_t row, int32_t col, const SourceMapData& targetMap)
{
    if (row < 1 || col < 1 || targetMap.afterPos_.empty()) {
        return MappingInfo {0, 0, ""};
    }
    row--;
    col--;
    // binary search
    int32_t left = 0;
    int32_t right = static_cast<int32_t>(targetMap.afterPos_.size()) - 1;
    int32_t res = 0;
    if (row > targetMap.afterPos_[targetMap.afterPos_.size() - 1].afterRow) {
        return MappingInfo { row + 1, col + 1, targetMap.files_[0] };
    }
    while (right - left >= 0) {
        int32_t mid = (right + left) / 2;
        if ((targetMap.afterPos_[mid].afterRow == row && targetMap.afterPos_[mid].afterColumn > col) ||
             targetMap.afterPos_[mid].afterRow > row) {
            right = mid - 1;
        } else {
            res = mid;
            left = mid + 1;
        }
    }
    std::string sources = targetMap.url_;
    auto pos = sources.find(WEBPACK);
    if (pos != std::string::npos) {
        sources.replace(pos, sizeof(WEBPACK) - 1, "");
    }

    MappingInfo mappingInfo;
    mappingInfo.row = targetMap.afterPos_[res].beforeRow + 1;
    mappingInfo.col = targetMap.afterPos_[res].beforeColumn + 1;
    mappingInfo.sources = sources;
    return mappingInfo;
}

void SourceMap::ExtractKeyInfo(const std::string& sourceMap, std::vector<std::string>& sourceKeyInfo)
{
    uint32_t cnt = 0;
    std::string tempStr;
    for (uint32_t i = 0; i < sourceMap.size(); i++) {
        // reslove json file
        if (sourceMap[i] == DOUBLE_SLASH) {
            i++;
            tempStr += sourceMap[i];
            continue;
        }
        // cnt is used to represent a pair of double quotation marks: ""
        if (sourceMap[i] == '"') {
            cnt++;
        }
        if (cnt == INDEX_TWO) {
            sourceKeyInfo.push_back(tempStr);
            tempStr = "";
            cnt = 0;
        } else if (cnt == 1) {
            if (sourceMap[i] != '"') {
                tempStr += sourceMap[i];
            }
        }
    }
}

void SourceMap::GetPosInfo(const std::string& temp, int32_t start, std::string& line, std::string& column)
{
    // 0 for colum, 1 for row
    int32_t flag = 0;
    // find line, column
    for (int32_t i = start - 1; i > 0; i--) {
        if (temp[i] == ':') {
            flag += 1;
            continue;
        }
        if (flag == 0) {
            column = temp[i] + column;
        } else if (flag == 1) {
            line = temp[i] + line;
        } else {
            break;
        }
    }
}

std::vector<std::string> SourceMap::HandleMappings(const std::string& mapping)
{
    std::vector<std::string> keyInfo;
    std::string tempStr;
    for (uint32_t i = 0; i < mapping.size(); i++) {
        if (mapping[i] == DELIMITER_COMMA) {
            keyInfo.push_back(tempStr);
            tempStr = "";
        } else if (mapping[i] == DELIMITER_SEMICOLON) {
            if (tempStr != "") {
                keyInfo.push_back(tempStr);
            }
            tempStr = "";
            keyInfo.push_back(";");
        } else {
            tempStr += mapping[i];
        }
    }
    if (tempStr != "") {
        keyInfo.push_back(tempStr);
    }
    return keyInfo;
};

bool SourceMap::VlqRevCode(const std::string& vStr, std::vector<int32_t>& ans)
{
    const int32_t VLQ_BASE_SHIFT = 5;
    // binary: 100000
    uint32_t VLQ_BASE = 1 << VLQ_BASE_SHIFT;
    // binary: 011111
    uint32_t VLQ_BASE_MASK = VLQ_BASE - 1;
    // binary: 100000
    uint32_t VLQ_CONTINUATION_BIT = VLQ_BASE;
    uint32_t result = 0;
    uint32_t shift = 0;
    bool continuation = 0;
    for (uint32_t i = 0; i < vStr.size(); i++) {
        uint32_t digit = Base64CharToInt(vStr[i]);
        if (digit == DIGIT_NUM) {
            return false;
        }
        continuation = digit & VLQ_CONTINUATION_BIT;
        digit &= VLQ_BASE_MASK;
        result += digit << shift;
        if (continuation) {
            shift += VLQ_BASE_SHIFT;
        } else {
            bool isNegate = result & 1;
            result >>= 1;
            ans.push_back(isNegate ? -result : result);
            result = 0;
            shift = 0;
        }
    }
    if (continuation) {
        return false;
    }
    return true;
};

bool SourceMap::TranslateUrlPositionBySourceMap(std::string& url, int& line, int& column)
{
    if (url.rfind(".js") != std::string::npos) {
        return true;
    }
    auto iterData = sourceMaps_.find(url);
    if (iterData != sourceMaps_.end()) {
        return GetLineAndColumnNumbers(line, column, *(iterData->second));
    }
    auto iter = sourceMapsData_.find(url);
    if (iter != sourceMapsData_.end()) {
        std::shared_ptr<SourceMapData> modularMap = std::make_shared<SourceMapData>();
        ExtractSourceMapData(iter->second, modularMap);
        url = modularMap->sources_[0];
        sourceMaps_.emplace(url, modularMap);
        return GetLineAndColumnNumbers(line, column, *(modularMap));
    }
    return false;
}

bool SourceMap::GetLineAndColumnNumbers(int& line, int& column, SourceMapData& targetMap)
{
    int32_t offSet = 0;
    MappingInfo mapInfo;
#if defined(WINDOWS_PLATFORM) || defined(MAC_PLATFORM)
        mapInfo = Find(line - offSet + OFFSET_PREVIEW, column, targetMap);
#else
        mapInfo = Find(line - offSet, column, targetMap);
#endif
    if (mapInfo.row == 0 || mapInfo.col == 0) {
        return false;
    } else {
        line = mapInfo.row;
        column = mapInfo.col;
        return true;
    }
}
}   // namespace JsEnv
}   // namespace OHOS
