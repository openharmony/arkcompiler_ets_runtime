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

#include "ecmascript/base/file_path_helper.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/method.h"

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
    if (!base::FilePathHelper::RealPath(inPath, realPath)) {
        LOG_ECMA(ERROR) << "The file path(" << inPath << ") real path failure!";
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
    size_t index = profilerString.find_first_of(":");
    if (index == std::string::npos) {
        LOG_ECMA(ERROR) << "Profiler format error";
        return;
    }
    CString recordName = ConvertToString(profilerString.substr(0, index));
    std::string content = profilerString.substr(index + 1);
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
    if (methodCountString.size() != METHOD_INFO_COUNT) {
        LOG_ECMA(ERROR) << "method info format error";
        return;
    }
    int count = atoi(methodCountString[1].c_str());
    if (count >= static_cast<int>(hotnessThreshold_)) {
        int methodId = atoi(methodCountString[0].c_str());
        methodIds.emplace(methodId);
    }
}

bool PGOProfilerLoader::Match([[maybe_unused]] const CString &recordName, EntityId methodId)
{
    if (!isLoaded_) {
        return true;
    }
    for (auto hotnessMethodSet : hotnessMethods_) {
        if (hotnessMethodSet.second.find(methodId) != hotnessMethodSet.second.end()) {
            return true;
        }
    }
    return false;
}
} // namespace panda::ecmascript
