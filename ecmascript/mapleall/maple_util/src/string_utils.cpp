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

#include "string_utils.h"
#include <cstddef>
#include <iostream>

namespace maple {
std::string StringUtils::Trim(const std::string &src)
{
    // remove space
    return std::regex_replace(src, std::regex("\\s+"), "");
}

std::string StringUtils::Replace(const std::string &src, const std::string &target, const std::string &replacement)
{
    std::string::size_type replaceLen = replacement.size();
    std::string::size_type targetLen = target.size();
    std::string temp = src;
    std::string::size_type index = 0;
    index = temp.find(target, index);
    while (index != std::string::npos) {
        temp.replace(index, targetLen, replacement);
        index += replaceLen;
        index = temp.find(target, index);
    }
    return temp;
}

std::string StringUtils::Append(const std::string &src, const std::string &target, const std::string &spliter)
{
    return src + spliter + target;
}

std::string StringUtils::GetStrAfterLast(const std::string &src, const std::string &target, bool isReturnEmpty)
{
    size_t pos = src.find_last_of(target);
    if (pos == std::string::npos) {
        return isReturnEmpty ? "" : src;
    }
    return src.substr(pos + 1);
}

std::string StringUtils::GetStrBeforeLast(const std::string &src, const std::string &target, bool isReturnEmpty)
{
    size_t pos = src.find_last_of(target);
    if (pos == std::string::npos) {
        return isReturnEmpty ? "" : src;
    }
    return src.substr(0, pos);
}

void StringUtils::Split(const std::string &src, std::unordered_set<std::string> &container, char delim)
{
    if (Trim(src).empty()) {
        return;
    }
    std::stringstream strStream(src + delim);
    std::string item;
    while (std::getline(strStream, item, delim)) {
        container.insert(item);
    }
}

void StringUtils::Split(const std::string &src, std::queue<std::string> &container, char delim)
{
    if (Trim(src).empty()) {
        return;
    }
    std::stringstream strStream(src + delim);
    std::string item;
    while (std::getline(strStream, item, delim)) {
        container.push(item);
    }
}

std::regex StringUtils::kCommandInjectionRegex("[;\\|\\&\\$\\>\\<`]");
}  // namespace maple
