/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/dfx/pgo_profiler/pgo_profiler_loader.h"

#include <string>

#include "ecmascript/base/string_helper.h"
#include "ecmascript/dfx/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/method.h"
#include "ecmascript/platform/file.h"

namespace panda::ecmascript {
void PGOProfilerLoader::LoadProfiler(const std::string &inPath, uint32_t hotnessThreshold)
{
    hotnessThreshold_ = hotnessThreshold;
    isLoaded_ = false;
    hotnessMethods_.clear();
    if (inPath.empty()) {
        return;
    }
    std::string realPath;
    if (!RealPath(inPath, realPath)) {
        return;
    }

    static const std::string endString = ".aprof";
    if (realPath.compare(realPath.length() - endString.length(), endString.length(), endString)) {
        LOG_ECMA(ERROR) << "The file path( " << realPath << ") does not end with .aprof";
        return;
    }
    LOG_ECMA(INFO) << "Load profiler from file:" << realPath;

    std::ifstream file(realPath.c_str());
    if (!file.is_open()) {
        LOG_ECMA(ERROR) << "File path(" << realPath << ") open failure!";
        return;
    }
    std::string lineString;
    while (std::getline(file, lineString)) {
        if (!lineString.empty()) {
            ParseProfiler(lineString);
        }
    }

    file.close();
    isLoaded_ = true;
}

void PGOProfilerLoader::ParseProfiler(const std::string &profilerString)
{
    size_t index = profilerString.find_first_of("[");
    size_t end = profilerString.find_last_of("]");
    if (index == std::string::npos || end == std::string::npos || index > end) {
        LOG_ECMA(ERROR) << "Profiler format error" << profilerString;
        return;
    }
    CString recordName = ConvertToString(profilerString.substr(0, index - 1));
    std::string content = profilerString.substr(index + 1, end - index - 1);
    std::vector<std::string> hotnessMethodIdStrings = base::StringHelper::SplitString(content, ",");
    if (hotnessMethodIdStrings.empty()) {
        LOG_ECMA(INFO) << "hotness method is none";
        return;
    }
    auto hotnessMethodSet = hotnessMethods_.find(recordName);
    if (hotnessMethodSet != hotnessMethods_.end()) {
        auto &methodIds = hotnessMethodSet->second;
        for (auto &method : hotnessMethodIdStrings) {
            ParseHotMethodInfo(method, methodIds);
        }
    } else {
        std::unordered_set<EntityId> methodIds;
        for (auto &method : hotnessMethodIdStrings) {
            ParseHotMethodInfo(method, methodIds);
        }
        if (!methodIds.empty()) {
            hotnessMethods_.emplace(recordName, methodIds);
        }
    }
}

void PGOProfilerLoader::ParseHotMethodInfo(const std::string &methodInfo, std::unordered_set<EntityId> &methodIds)
{
    std::vector<std::string> methodCountString = base::StringHelper::SplitString(methodInfo, "/");
    if (methodCountString.size() < METHOD_INFO_COUNT) {
        LOG_ECMA(ERROR) << "method info format error" << methodInfo;
        return;
    }
    char *endPtr = nullptr;
    static constexpr int NUMBER_BASE = 10;
    SampleMode mode =
        static_cast<SampleMode>(strtol(methodCountString[METHOD_MODE_INDEX].c_str(), &endPtr, NUMBER_BASE));
    if (endPtr == methodCountString[METHOD_MODE_INDEX].c_str() || *endPtr != '\0') {
        LOG_ECMA(ERROR) << "method mode strtol error" << methodCountString[METHOD_MODE_INDEX];
        return;
    }
    bool isHotness = true;
    if (mode == SampleMode::CALL_MODE) {
        uint32_t count =
            static_cast<uint32_t>(strtol(methodCountString[METHOD_COUNT_INDEX].c_str(), &endPtr, NUMBER_BASE));
        if (endPtr == methodCountString[METHOD_COUNT_INDEX].c_str() || *endPtr != '\0') {
            LOG_ECMA(ERROR) << "method count strtol error" << methodCountString[METHOD_COUNT_INDEX];
            return;
        }
        if (count < hotnessThreshold_) {
            isHotness = false;
        }
    }
    if (isHotness) {
        endPtr = nullptr;
        uint32_t methodId =
            static_cast<uint32_t>(strtol(methodCountString[METHOD_ID_INDEX].c_str(), &endPtr, NUMBER_BASE));
        if (endPtr == methodCountString[METHOD_ID_INDEX].c_str() || *endPtr != '\0') {
            LOG_ECMA(ERROR) << "method id strtol error" << methodCountString[METHOD_ID_INDEX];
            return;
        }
        methodIds.emplace(methodId);
    }
}

bool PGOProfilerLoader::Match(const CString &recordName, EntityId methodId)
{
    if (!isLoaded_) {
        return true;
    }
    auto hotnessMethodSet = hotnessMethods_.find(recordName);
    if (hotnessMethodSet == hotnessMethods_.end()) {
        return false;
    }
    return hotnessMethodSet->second.find(methodId) != hotnessMethodSet->second.end();
}
} // namespace panda::ecmascript
