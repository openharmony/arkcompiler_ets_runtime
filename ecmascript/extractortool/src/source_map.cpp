/**
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

#include "source_map.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <sstream>

#include "ecmascript/base/string_helper.h"
#include "ecmascript/extractortool/src/extractor.h"

namespace panda {
namespace ecmascript {
namespace {
static constexpr char DELIMITER_COMMA = ',';
static constexpr char DELIMITER_SEMICOLON = ';';
static constexpr char DOUBLE_SLASH = '\\';

static constexpr int32_t INDEX_ONE = 1;
static constexpr int32_t INDEX_TWO = 2;
static constexpr int32_t INDEX_THREE = 3;
static constexpr int32_t INDEX_FOUR = 4;
static constexpr int32_t ANS_MAP_SIZE = 5;
static constexpr int32_t DIGIT_NUM = 64;

const std::string MEGER_SOURCE_MAP_PATH = "ets/sourceMaps.map";
static constexpr std::string_view DOBULE_QUOTATION = "\"";
static constexpr std::string_view FLAG_URL = ": {";
static constexpr std::string_view FLAG_SOURCES = "    \"sources\":";
static constexpr std::string_view FLAG_MAPPINGS = "    \"mappings\": \"";
static constexpr std::string_view FLAG_ENTRY_PACKAGE_INFO = "    \"entry-package-info\": \"";
static constexpr std::string_view FLAG_PACKAGE_INFO = "    \"package-info\": \"";
static constexpr std::string_view FLAG_BLOCK_END = "  }";

// Constants from ability_runtime
const std::string NOT_FOUNDMAP = "Cannot get SourceMap info, dump raw stack:\n";
const std::string FLAG_CLOSE_BRACE = ")";
const std::string FLAG_OPEN_BRACE = "(";
const std::string FLAG_END = "  }";

static constexpr size_t SOURCEMAP_START_LEN = 2;
static constexpr size_t FLAG_SOURCES_LEN = 14;
static constexpr size_t FLAG_MAPPINGS_LEN = 17;
static constexpr size_t REAL_URL_INDEX = 3;
static constexpr size_t REAL_SOURCE_INDEX = 7;
static constexpr size_t FLAG_ENTRY_PACKAGE_INFO_SIZE = 27;
static constexpr size_t FLAG_PACKAGE_INFO_SIZE = 21;
static constexpr size_t FLAG_BLOCK_END_SIZE = 3;
static constexpr size_t FLAG_BLOCK_END_EXTRA_SIZE = 2;
static constexpr int32_t OFFSET_PREVIEW = 1;

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

bool StringStartWith(const std::string& str, const std::string& startStr)
{
    size_t startStrLen = startStr.length();
    return ((str.length() >= startStrLen) && (str.compare(0, startStrLen, startStr) == 0));
}
} // namespace

// Static member initialization
std::mutex SourceMap::sourceMapMutex_;

SourceMap& SourceMap::GetInstance()
{
    static SourceMap instance;
    return instance;
}

uint32_t SourceMap::Base64CharToInt(char charCode)
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
}

#if defined(PANDA_TARGET_OHOS)
bool SourceMap::ReadSourceMapData(const std::string& hapPath)
{
    if (hapPath.empty()) {
        return false;
    }
    bool newCreate = false;
    std::shared_ptr<Extractor> extractor = ExtractorUtil::GetExtractor(hapPath, newCreate);
    if (extractor == nullptr) {
        return false;
    }
    if (!extractor->ExtractToBufByName(MEGER_SOURCE_MAP_PATH, dataPtr_, dataLen_)) {
        return false;
    }
    return true;
}

void SourceMap::Init(const std::string& hapPath)
{
    auto start = Clock::now();
    std::string sourceMapData;
    if (ReadSourceMapData(hapPath)) {
        SplitSourceMap();
    }
    auto end = Clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    LOG_ECMA(INFO) << "Init sourcemap time: " << duration.count() << "ms";
}
#endif

// Zero-copy path: parse raw buffer into URL -> string_view blocks
void SourceMap::SplitSourceMap()
{
    std::string_view data = std::string_view(reinterpret_cast<char*>(dataPtr_.get()), dataLen_);

    size_t pos = SOURCEMAP_START_LEN;
    while (pos != std::string_view::npos) {
        size_t start = data.find(FLAG_URL, pos);
        if (start == std::string_view::npos) {
            break;
        }
        size_t end = data.find(FLAG_BLOCK_END, start + 3); // 3 means skig "：{"
        if (end == std::string_view::npos) {
            break;
        }
        auto url = data.substr(pos + REAL_URL_INDEX, start - pos - REAL_URL_INDEX - 1); // 1 means skip "
        auto block = data.substr(start + FLAG_BLOCK_END_SIZE, end - start - FLAG_BLOCK_END_SIZE);
        sourceMaps_[url] = block;
        pos = end + FLAG_BLOCK_END_SIZE + FLAG_BLOCK_END_EXTRA_SIZE;
    }
}

// Eager parsing path: parse string into URL -> shared_ptr<SourceMapData>
void SourceMap::SplitSourceMap(const std::string& sourceMapData)
{
    std::lock_guard<std::mutex> lock(sourceMapMutex_);
    std::stringstream ss(sourceMapData);
    std::string tmp;
    std::string url;

    std::getline(ss, tmp);
    bool isUrl = true;
    std::shared_ptr<SourceMapData> mapData;
    while (std::getline(ss, tmp)) {
        if (isUrl && tmp.size() > REAL_SOURCE_INDEX) { // url
            url = tmp.substr(REAL_URL_INDEX, tmp.size() - REAL_SOURCE_INDEX);
            isUrl = false;
            mapData = std::make_shared<SourceMapData>();
            continue;
        }
        if (StringStartWith(tmp, std::string(FLAG_SOURCES))) { // sources
            std::getline(ss, tmp);
            if (mapData) {
                mapData->sources_ = tmp;
                continue;
            }
        }
        if (StringStartWith(tmp, std::string(FLAG_MAPPINGS))) { // mapping
            ExtractSourceMapData(tmp.substr(FLAG_MAPPINGS_LEN, tmp.size() - FLAG_MAPPINGS_LEN - INDEX_TWO), mapData);
            continue;
        }
        if (StringStartWith(tmp, std::string(FLAG_ENTRY_PACKAGE_INFO))) { // entryPackageInfo
            if (mapData) {
                mapData->packageName_ = tmp;
                continue;
            }
        }
        if (StringStartWith(tmp, std::string(FLAG_PACKAGE_INFO))) { // packageInfo
            if (mapData) {
                mapData->packageName_ = tmp;
                mapData->isPackageInfo_ = true;
                continue;
            }
        }
        if (StringStartWith(tmp, FLAG_END)) {
            eagerSourceMaps_[url] = mapData;
            isUrl = true;
        }
    }
}

void SourceMap::ExtractSourceMapData(const std::string& allmappings, SourceMapData *curMapData)
{
    curMapData->mappings_ = HandleMappings(allmappings);

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

void SourceMap::ExtractSourceMapData(const std::string& allmappings, std::shared_ptr<SourceMapData>& curMapData)
{
    if (!curMapData) {
        LOG_ECMA(ERROR) << "curMapData is null";
        return;
    }
    curMapData->mappings_ = HandleMappings(allmappings);
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
            LOG_ECMA(ERROR) << "decode sourcemap fail, mapping: " << mapping;
            break;
        }
        if (ans.size() == 1) {
            curMapData->nowPos_.afterColumn += ans[0];
            continue;
        }
        // after decode, assgin each value to the position
        curMapData->nowPos_.afterColumn += ans[0];
        curMapData->nowPos_.beforeRow += ans[INDEX_TWO];
        curMapData->nowPos_.beforeColumn += ans[INDEX_THREE];
        curMapData->afterPos_.push_back({
            curMapData->nowPos_.beforeRow,
            curMapData->nowPos_.beforeColumn,
            curMapData->nowPos_.afterRow,
            curMapData->nowPos_.afterColumn,
        });
    }
    curMapData->mappings_.clear();
    curMapData->mappings_.shrink_to_fit();
}

// ets_runtime Find: returns MappingInfo without sources
MappingInfo SourceMap::Find(int32_t row, int32_t col, const SourceMapData& targetMap, bool& isReplaces)
{
    if (row < 1 || col < 1) {
        LOG_ECMA(ERROR) << "SourceMap find failed, line: " << row << ", column: " << col;
        return MappingInfo { 0, 0, {} };
    } else if (targetMap.afterPos_.empty()) {
        LOG_ECMA(ERROR) << "Target map can't find after pos.";
        return MappingInfo { 0, 0, {} };
    }
    row--;
    col--;
    // binary search
    int32_t left = 0;
    int32_t right = static_cast<int32_t>(targetMap.afterPos_.size()) - 1;
    int32_t res = 0;
    if (row > targetMap.afterPos_[targetMap.afterPos_.size() - 1].afterRow) {
        isReplaces = false;
        return MappingInfo { row + 1, col + 1, {}};
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
    return MappingInfo { targetMap.afterPos_[res].beforeRow + 1, targetMap.afterPos_[res].beforeColumn + 1, {} };
}

// ability_runtime Find: returns MappingInfo with sources
MappingInfo SourceMap::Find(int32_t row, int32_t col, const SourceMapData& targetMap, const std::string& key)
{
    if (row < 1 || col < 1 || targetMap.afterPos_.empty() || targetMap.sources_.empty()) {
        return MappingInfo {row, col, key};
    }
    size_t realSourceIndex = std::min(REAL_SOURCE_INDEX, targetMap.sources_.size());
    std::string sources = targetMap.sources_.substr(realSourceIndex,
                                                    targetMap.sources_.size() - realSourceIndex - 1);
    if (key.rfind(".js") == key.size() - INDEX_THREE) {
        return MappingInfo {
            .row = row,
            .col = col,
            .sources = sources,
        };
    }
    row--;
    col--;
    // binary search
    int32_t left = 0;
    int32_t right = static_cast<int32_t>(targetMap.afterPos_.size()) - 1;
    int32_t res = 0;
    if (row > targetMap.afterPos_[targetMap.afterPos_.size() - 1].afterRow) {
        return MappingInfo { row + 1, col + 1, key };
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

    if (res + 1 < static_cast<int32_t>(targetMap.afterPos_.size()) &&
        targetMap.afterPos_[res].afterRow != row &&
        targetMap.afterPos_[res + 1].afterRow == row) {
        res++;
    }

    return MappingInfo {
        .row = targetMap.afterPos_[res].beforeRow + 1,
        .col = targetMap.afterPos_[res].beforeColumn + 1,
        .sources = sources,
    };
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
}

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
}

std::string SourceMap::GetMappings(std::string_view sourcemap)
{
    auto start = sourcemap.find(FLAG_MAPPINGS);
    if (start == std::string_view::npos) {
        return std::string();
    }
    auto end = sourcemap.find(DOBULE_QUOTATION, start + FLAG_MAPPINGS_LEN);
    if (end == std::string_view::npos) {
        return std::string();
    }
    return std::string(sourcemap.substr(start + FLAG_MAPPINGS_LEN, end - start - FLAG_MAPPINGS_LEN));
}

std::string SourceMap::GetSources(std::string_view sourcemap)
{
    auto start = sourcemap.find(FLAG_SOURCES);
    if (start == std::string_view::npos) {
        return std::string();
    }

    start = sourcemap.find(DOBULE_QUOTATION, start + FLAG_SOURCES_LEN);
    if (start == std::string_view::npos) {
        return std::string();
    }

    auto end = sourcemap.find(DOBULE_QUOTATION, start + 1);
    if (end == std::string_view::npos) {
        return std::string();
    }

    return std::string(sourcemap.substr(start + 1, end - start - 1));
}

std::string SourceMap::GetEntryPackageInfo(std::string_view sourcemap)
{
    auto start = sourcemap.find(FLAG_ENTRY_PACKAGE_INFO);
    if (start == std::string_view::npos) {
        return std::string();
    }
    auto end = sourcemap.find(DOBULE_QUOTATION, start + FLAG_ENTRY_PACKAGE_INFO_SIZE);
    if (end == std::string_view::npos) {
        return std::string();
    }

    return std::string(sourcemap.substr(start + FLAG_ENTRY_PACKAGE_INFO_SIZE,
        end - start - FLAG_ENTRY_PACKAGE_INFO_SIZE));
}

std::string SourceMap::GetPackageInfo(std::string_view sourcemap)
{
    auto start = sourcemap.find(FLAG_PACKAGE_INFO);
    if (start == std::string_view::npos) {
        return std::string();
    }
    auto end = sourcemap.find(DOBULE_QUOTATION, start + FLAG_PACKAGE_INFO_SIZE);
    if (end == std::string_view::npos) {
        return std::string();
    }

    return std::string(sourcemap.substr(start + FLAG_PACKAGE_INFO_SIZE,
        end - start - FLAG_PACKAGE_INFO_SIZE));
}

std::string SourceMap::GetPackageName(std::string_view sourcemap)
{
    std::string packageName = GetPackageInfo(sourcemap);
    if (packageName.empty()) {
        packageName = GetEntryPackageInfo(sourcemap);
    }
    return packageName;
}

bool SourceMap::ParseSourceMapData(std::string_view url)
{
    auto iterData = sourceMapDatas_.find(url);
    if (iterData != sourceMapDatas_.end()) {
        return true;
    }

    auto iter = sourceMaps_.find(url);
    if (iter == sourceMaps_.end()) {
        LOG_ECMA(ERROR) << "SourceMaps find failed, url: " << url;
        return false;
    }

    std::shared_ptr<SourceMapData> modularMap = std::make_shared<SourceMapData>();
    if (modularMap == nullptr) {
        LOG_ECMA(ERROR) << "New SourceMapData failed";
        return false;
    }

    auto sources = GetSources(iter->second);
    if (sources.empty()) {
        LOG_ECMA(ERROR) << "GetSources failed, block: " << iter->second;
        return false;
    }

    auto packageName = GetPackageName(iter->second);
    if (!packageName.empty()) {
        auto last = packageName.rfind('|');
        if (last == std::string::npos) {
            LOG_ECMA(ERROR) << "packageName can't find |, packageName: " << packageName;
        }
        packageName = packageName.substr(0, last);
    }

    modularMap->sources_ = sources;
    modularMap->packageName_ = packageName;
    if (url.rfind(".js") != std::string_view::npos) {
        sourceMapDatas_.emplace(iter->first, modularMap);
        return true;
    }

    auto mappings = GetMappings(iter->second);

    ExtractSourceMapData(mappings, modularMap.get());
    sourceMapDatas_.emplace(iter->first, modularMap);
    return true;
}

bool SourceMap::TranslateUrlPositionBySourceMap(std::string& url, int& line, int& column, std::string& packageName)
{
    std::lock_guard<std::mutex> lock(sourceMapMutex_);

    // First try eager parsing path (ability_runtime)
    auto eagerIter = eagerSourceMaps_.find(url);
    if (eagerIter != eagerSourceMaps_.end()) {
        return GetLineAndColumnNumbers(line, column, *(eagerIter->second), url, packageName);
    }

    // Then try zero-copy path (ets_runtime)
    std::string_view urlView(url);
    if (!ParseSourceMapData(urlView)) {
        LOG_ECMA(ERROR) << "stageMode sourceMaps find fail";
        return false;
    }

    auto sourceMapData = sourceMapDatas_[urlView];
    if (!sourceMapData) {
        return false;
    }
    packageName = sourceMapData->packageName_;
    if (url.rfind(".js") != std::string::npos) {
        url = sourceMapData->sources_;
        return true;
    }
    bool isReplaces = true;
    auto ret = GetLineAndColumnNumbers(line, column, *sourceMapData, isReplaces);
    if (isReplaces) {
        url = sourceMapData->sources_;
    }
    return ret;
}

// ets_runtime GetLineAndColumnNumbers
bool SourceMap::GetLineAndColumnNumbers(int& line, int& column, SourceMapData& targetMap, bool& isReplaces)
{
    int32_t offSet = 0;
    MappingInfo mapInfo;
#if defined(WINDOWS_PLATFORM) || defined(MAC_PLATFORM)
    mapInfo = Find(line - offSet + OFFSET_PREVIEW, column, targetMap, isReplaces);
#else
    mapInfo = Find(line - offSet, column, targetMap, isReplaces);
#endif
    if (mapInfo.row == 0 || mapInfo.col == 0) {
        return false;
    } else {
        line = mapInfo.row;
        column = mapInfo.col;
        return true;
    }
}

// ability_runtime GetLineAndColumnNumbers
bool SourceMap::GetLineAndColumnNumbers(int& line, int& column, SourceMapData& targetMap,
    std::string& url, std::string& packageName)
{
    // Note: this is a public method but typically called from TranslateUrlPositionBySourceMap
    // which already holds the lock. If called directly, the caller should ensure thread safety.
    int32_t offSet = 0;
    MappingInfo mapInfo;
#if defined(WINDOWS_PLATFORM) || defined(MAC_PLATFORM)
        mapInfo = Find(line - offSet + OFFSET_PREVIEW, column, targetMap, url);
#else
        mapInfo = Find(line - offSet, column, targetMap, url);
#endif
    if (mapInfo.row == 0 || mapInfo.col == 0) {
        return false;
    } else {
        line = mapInfo.row;
        column = mapInfo.col;
        url = mapInfo.sources;
        GetPackageName(targetMap, packageName);
        return true;
    }
}

std::string SourceMap::TranslateBySourceMap(const std::string& stackStr)
{
    std::lock_guard<std::mutex> lock(sourceMapMutex_);
    std::string ans = "";

    // find per line of stack
    std::vector<std::string> res;
    ExtractStackInfo(stackStr, res);

    // collect error info first
    for (uint32_t i = 0; i < res.size(); i++) {
        std::string temp = res[i];
        std::string key = ExtractFileName(temp);
        auto closeBracePos = static_cast<int32_t>(temp.find(FLAG_CLOSE_BRACE));
        auto openBracePos = static_cast<int32_t>(temp.find(FLAG_OPEN_BRACE));
        if (closeBracePos < 0 || openBracePos < 0) {
            ans = ans + temp + "\n";
            continue;
        }
        std::string line;
        std::string column;
        GetPosInfo(temp, closeBracePos, line, column);
        if (line.empty() || column.empty()) {
            LOG_ECMA(ERROR) << "the stack without line info";
            continue;
        }
        std::string sourceInfo;
        auto iter = eagerSourceMaps_.find(key);
        if (iter != eagerSourceMaps_.end()) {
            sourceInfo = GetSourceInfo(line, column, *(iter->second), key);
        } else {
            ans = ans + temp + "\n";
            continue;
        }
        if (sourceInfo.empty()) {
            continue;
        }
        temp.replace(openBracePos, closeBracePos - openBracePos + 1, sourceInfo);
        replace(temp.begin(), temp.end(), '\\', '/');
        ans = ans + temp + "\n";
    }
    if (ans.empty()) {
        return (NOT_FOUNDMAP + stackStr);
    }
    return ans;
}

std::string SourceMap::GetSourceInfo(const std::string& line, const std::string& column,
    const SourceMapData& targetMap, const std::string& key)
{
    int32_t offSet = 0;
    std::string sourceInfo;
    MappingInfo mapInfo;
#if defined(WINDOWS_PLATFORM) || defined(MAC_PLATFORM)
        mapInfo = Find(StringToInt(line) - offSet + OFFSET_PREVIEW, StringToInt(column), targetMap, key);
#else
        mapInfo = Find(StringToInt(line) - offSet, StringToInt(column), targetMap, key);
#endif
    std::string sources = mapInfo.sources;
    std::string packageName = targetMap.packageName_;
    if (!packageName.empty()) {
        auto last = packageName.rfind('|');
        if (last != std::string::npos) {
            auto packageNameSize = targetMap.isPackageInfo_ ? FLAG_PACKAGE_INFO_SIZE : FLAG_ENTRY_PACKAGE_INFO_SIZE;
            sourceInfo = packageName.substr(packageNameSize, last - packageNameSize);
            return sourceInfo.append(" (" + sources + ":" + std::to_string(mapInfo.row) + ":" +
                std::to_string(mapInfo.col) + ")");
        }
    }
    sourceInfo = "(" + sources + ":" + std::to_string(mapInfo.row) + ":" + std::to_string(mapInfo.col) + ")";
    return sourceInfo;
}

std::string SourceMap::ExtractFileName(const std::string& str)
{
    // at funcName (@param:version|url:line:column)
    // Find the position of the last colon in the character string.
    size_t lastColon = str.rfind(':');
    if (lastColon != std::string::npos) {
        // Find the position of the last but one colon in the character string.
        size_t prevColon = str.rfind(':', lastColon - 1);
        if (prevColon != std::string::npos) {
            // Find the position of the first brace in the character string.
            size_t openBrace = str.find(FLAG_OPEN_BRACE);
            if (openBrace != std::string::npos) {
                // Extract the character string between colons and braces as the file name.
                return str.substr(openBrace + 1, prevColon - openBrace - 1);
            }
        }
    }
    return str;
}

void SourceMap::ExtractStackInfo(const std::string& stackStr, std::vector<std::string>& res)
{
    std::stringstream ss(stackStr);
    std::string tempStr;
    while (std::getline(ss, tempStr)) {
        res.push_back(tempStr);
    }
}

void SourceMap::GetPackageName(const SourceMapData& targetMap, std::string& packageName)
{
    std::string packageInfo = targetMap.packageName_;
    if (!packageInfo.empty()) {
        auto last = packageInfo.rfind('|');
        if (last != std::string::npos) {
            auto packageNameSize = targetMap.isPackageInfo_ ? FLAG_PACKAGE_INFO_SIZE : FLAG_ENTRY_PACKAGE_INFO_SIZE;
            packageName = packageInfo.substr(packageNameSize, last - packageNameSize);
        }
    }
}

void SourceMap::SetInitStatus(InitStatus status)
{
    initStatus_.store(status, std::memory_order_release);
}

InitStatus SourceMap::GetInitStatus() const
{
    return initStatus_.load(std::memory_order_acquire);
}
}   // namespace panda
}   // namespace ecmascript
