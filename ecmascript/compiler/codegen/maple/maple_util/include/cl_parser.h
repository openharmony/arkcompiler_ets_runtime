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

#ifndef MAPLE_UTIL_INCLUDE_PARSER_H
#define MAPLE_UTIL_INCLUDE_PARSER_H

#include <algorithm>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <deque>

namespace maplecl {

enum class RetCode {
    noError,
    notRegistered,
    incorrectValue,
    unnecessaryValue,
    outOfRange,
    notSupportedOptionType,
    valueEmpty,
    parsingErr,
};

class OptionInterface;

using OptionsMapType = std::unordered_map<std::string, OptionInterface *>;

/* This structure is used to aggregate option information during option parsing.
 * It's a string view from original input command line string. */
struct KeyArg {
    explicit KeyArg(std::string_view arg) : rawArg(arg) {}
    const std::string_view rawArg; /* full option, like "--key=value" */
    std::string_view key;          /* Extracted key, like "--key" */
    std::string_view val;          /* Extracted value, like "value" */
    bool isEqualOpt = false;       /* indicates whether the parsed option contained "=" symbol.
                                      For options like --key=value, it's true.
                                      For options like --key value, it's false */
    bool isJoinedOpt = false;      /* indicates whether the parsed option was a joined option, like: --optValue */
};

struct OptionCategory {
    OptionsMapType options;
    OptionsMapType joinedOptions;
    std::vector<OptionInterface *> registredOptions;

    void AddEnabledOption(OptionInterface *opt)
    {
        if (enabledOptionsSet.find(opt) == enabledOptionsSet.end()) {
            enabledOptionsSet.insert(opt);
            enabledOptions.push_back(opt);
        }
    }

    const std::vector<OptionInterface *> &GetEnabledOptions()
    {
        return enabledOptions;
    }

    void Remove(OptionInterface *opt)
    {
        enabledOptionsSet.erase(opt);
        auto it = std::find(enabledOptions.begin(), enabledOptions.end(), opt);
        if (it != enabledOptions.end()) {
            enabledOptions.erase(it);
        }
    }

private:
    std::unordered_set<OptionInterface *> enabledOptionsSet;
    std::vector<OptionInterface *> enabledOptions;
};

class CommandLine {
public:
    /* singleton */
    static CommandLine &GetCommandLine();

    // CommandLine must not be copyable
    CommandLine(const CommandLine &) = delete;
    CommandLine &operator=(const CommandLine &) = delete;

    RetCode Parse(std::vector<std::string> argvs, OptionCategory &optCategory);
    RetCode Parse(std::vector<std::string> argvs)
    {
        return Parse(argvs, defaultCategory);
    }
    RetCode HandleInputArgs(const std::deque<std::string_view> &args, OptionCategory &optCategory);
    void Register(const std::vector<std::string> &optNames, OptionInterface &opt, OptionCategory &optCategory);

    std::vector<std::pair<std::string, RetCode>> badCLArgs;
    OptionCategory defaultCategory;

    /* NOTE: categories must be constructed before options.
     * It's the reason why they are inside CommandLine.
     * Looks like ugly architecture, but we need it */
    OptionCategory clangCategory;
    OptionCategory hir2mplCategory;
    OptionCategory mpl2mplCategory;
    OptionCategory meCategory;
    OptionCategory cgCategory;
    OptionCategory asCategory;
    OptionCategory ldCategory;

    OptionCategory jbc2mplCategory;
    OptionCategory ipaCategory;

private:
    CommandLine() = default;

    OptionInterface *CheckJoinedOption(KeyArg &keyArg, OptionCategory &optCategory);
    RetCode ParseJoinedOption(size_t &argsIndex, const std::deque<std::string_view> &args, KeyArg &keyArg,
                              OptionCategory &optCategory);
    RetCode ParseOption(size_t &argsIndex, const std::deque<std::string_view> &args, KeyArg &keyArg,
                        OptionCategory &optCategory, OptionInterface *opt);
    RetCode ParseEqualOption(size_t &argsIndex, const std::deque<std::string_view> &args, KeyArg &keyArg,
                             OptionCategory &optCategory, const OptionsMapType &optMap, size_t pos);
    RetCode ParseSimpleOption(size_t &argsIndex, const std::deque<std::string_view> &args, KeyArg &keyArg,
                              OptionCategory &optCategory, const OptionsMapType &optMap);
};

}  // namespace maplecl

#endif /* MAPLE_UTIL_INCLUDE_PARSER_H */
