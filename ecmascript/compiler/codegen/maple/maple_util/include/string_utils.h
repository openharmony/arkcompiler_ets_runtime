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

#ifndef MAPLE_UTIL_INCLUDE_STRING_UTILS_H
#define MAPLE_UTIL_INCLUDE_STRING_UTILS_H
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_set>
#include <regex>
#include <queue>

namespace maple {
class StringUtils {
public:
    template <class Container>
    static void Split(const std::string &src, Container &container, char delim)
    {
        if (Trim(src).empty()) {
            return;
        }

        std::stringstream strStream(src + delim);
        std::string item;
        while (std::getline(strStream, item, delim)) {
            container.emplace_back(item);
        }
    }

    template <typename Container>
    static void SplitSV(const std::string_view &src, Container &container, char delim)
    {
        size_t startInd = 0;
        while (startInd < src.size()) {
            if (src[startInd] == delim) {
                ++startInd;
                continue;
            }

            size_t endInd = src.find_first_of(delim, startInd);
            if (startInd != endInd) {
                container.emplace_back(src.substr(startInd, endInd - startInd));
            }

            if (endInd == std::string_view::npos) {
                break;
            }
            startInd = endInd + 1;
        }
    }

    static void Split(const std::string &src, std::unordered_set<std::string> &container, char delim)
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

    static void Split(const std::string &src, std::queue<std::string> &container, char delim)
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

    static std::string Trim(const std::string &s)
    {
        // remove space
        return std::regex_replace(s, std::regex("\\s+"), "");
    }

    static bool HasCommandInjectionChar(const std::string &s)
    {
        return std::regex_search(s, kCommandInjectionRegex);
    }

    static bool EndsWith(const std::string &str, const std::string &end)
    {
        if (end.size() > str.size()) {
            return false;
        }

        return std::equal(end.rbegin(), end.rend(), str.rbegin());
    }

    static bool StartsWith(const std::string &str, const std::string &start)
    {
        if (start.size() > str.size()) {
            return false;
        }

        return std::equal(start.cbegin(), start.cend(), str.cbegin());
    }

private:
    static std::regex kCommandInjectionRegex;
};
}  // namespace maple
#endif  // MAPLE_UTIL_INCLUDE_STRING_UTILS_H
