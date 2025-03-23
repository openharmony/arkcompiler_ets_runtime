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

#include "cl_option.h"
#include "cl_parser.h"

using namespace maplecl;

CommandLine &CommandLine::GetCommandLine()
{
    static CommandLine cl;
    return cl;
}

#ifdef ARK_LITECG_DEBUG
OptionInterface *CommandLine::CheckJoinedOption(KeyArg &keyArg, OptionCategory &optCategory)
{
    auto &str = keyArg.rawArg;

    for (auto joinedOption : optCategory.joinedOptions) {
        /* Joined Option (like -DMACRO) can be detected as substring (-D) in the option string */
        if (str.find(joinedOption.first) == 0) {
            size_t keySize = joinedOption.first.size();

            keyArg.val = str.substr(keySize);
            keyArg.key = str.substr(0, keySize);
            keyArg.isJoinedOpt = true;
            return joinedOption.second;
        }
    }

    return nullptr;
}

RetCode CommandLine::ParseJoinedOption(size_t &argsIndex, const std::deque<std::string_view> &args, KeyArg &keyArg,
                                       OptionCategory &optCategory)
{
    OptionInterface *option = CheckJoinedOption(keyArg, optCategory);
    if (option != nullptr) {
        RetCode err = option->Parse(argsIndex, args, keyArg);
        if (err != RetCode::noError) {
            return err;
        }

        /* Set Option in all categories registering for this option */
        for (auto &category : option->optCategories) {
            category->AddEnabledOption(option);
        }
    } else {
        return RetCode::notRegistered;
    }

    return RetCode::noError;
}

RetCode CommandLine::ParseOption(size_t &argsIndex, const std::deque<std::string_view> &args, KeyArg &keyArg,
                                 OptionCategory &optCategory, OptionInterface *opt)
{
    RetCode err = opt->Parse(argsIndex, args, keyArg);
    if (err != RetCode::noError) {
        return err;
    }

    /* Set Option in all categories registering for this option */
    for (auto &category : opt->optCategories) {
        category->AddEnabledOption(opt);
    }

    return RetCode::noError;
}

RetCode CommandLine::ParseEqualOption(size_t &argsIndex, const std::deque<std::string_view> &args, KeyArg &keyArg,
                                      OptionCategory &optCategory, const OptionsMapType &optMap, size_t pos)
{
    keyArg.isEqualOpt = true;
    auto &arg = args[argsIndex];

    /* To handle joined option, we must have full (not splitted key),
     * because joined option splitting is different:
     * As example for -Dkey=value: default splitting key="Dkey" value="value",
     * Joined option splitting key="D" value="key=value"
     */
    auto item = optMap.find(std::string(arg.substr(0, pos)));
    if (item != optMap.end()) {
        /* equal option, like --key=value */
        keyArg.key = arg.substr(0, pos);
        keyArg.val = arg.substr(pos + 1);
        return ParseOption(argsIndex, args, keyArg, optCategory, item->second);
    } else {
        /* It can be joined option, like: -DMACRO=VALUE */
        return ParseJoinedOption(argsIndex, args, keyArg, optCategory);
    }
}

RetCode CommandLine::ParseSimpleOption(size_t &argsIndex, const std::deque<std::string_view> &args, KeyArg &keyArg,
                                       OptionCategory &optCategory, const OptionsMapType &optMap)
{
    keyArg.isEqualOpt = false;
    auto &arg = args[argsIndex];

    auto item = optMap.find(std::string(arg));
    if (item != optMap.end()) {
        /* --key or --key value */
        return ParseOption(argsIndex, args, keyArg, optCategory, item->second);
    } else {
        /* It can be joined option, like: -DMACRO */
        return ParseJoinedOption(argsIndex, args, keyArg, optCategory);
    }
}
#endif

RetCode CommandLine::HandleInputArgs(const std::deque<std::string_view> &args, OptionCategory &optCategory)
{
    RetCode err = RetCode::noError;

    /* badCLArgs contains option parsing errors for each incorrect option.
     * We should clear old badCLArgs results. */
    badCLArgs.clear();

#ifdef ARK_LITECG_DEBUG
    bool wasError = false;
#endif
    for (size_t argsIndex = 0; argsIndex < args.size();) {
        auto &arg = args[argsIndex];
        if (arg == "") {
            ++argsIndex;
            continue;
        }
#ifdef ARK_LITECG_DEBUG
        KeyArg keyArg(arg);

        auto pos = arg.find('=');
        /* option like --key=value */
        if (pos != std::string::npos) {
            DEBUG_ASSERT(pos > 0, "CG internal error, composite unit with less than 2 unit elements.");
            err = ParseEqualOption(argsIndex, args, keyArg, optCategory, optCategory.options, pos);
            if (err != RetCode::noError) {
                badCLArgs.emplace_back(args[argsIndex], err);
                ++argsIndex;
                wasError = true;
            }
            continue;
        } else {
            /* option like "--key value" or "--key" */
            err = ParseSimpleOption(argsIndex, args, keyArg, optCategory, optCategory.options);
            if (err != RetCode::noError) {
                badCLArgs.emplace_back(args[argsIndex], err);
                ++argsIndex;
                wasError = true;
            }
            continue;
        }
#endif
        ++argsIndex;
        continue;
    }

#ifdef ARK_LITECG_DEBUG
    if (wasError == true) {
        return RetCode::parsingErr;
    }
#endif

    return err;
}

RetCode CommandLine::Parse(std::vector<std::string> argvs, OptionCategory &optCategory)
{
    std::deque<std::string_view> args;
    for (size_t i = 0; i < argvs.size(); i++) {
        args.emplace_back(argvs[i]);
    }
    return HandleInputArgs(args, optCategory);
}

void CommandLine::Register(const std::vector<std::string> &optNames, OptionInterface &opt, OptionCategory &optCategory)
{
    for (auto &optName : optNames) {
        if (optName.empty()) {
            continue;
        }

        DEBUG_ASSERT(optCategory.options.count(optName) == 0, "Duplicated options name %s", optName.data());
        optCategory.options.emplace(optName, &opt);

        if (opt.IsJoinedValPermitted()) {
            optCategory.joinedOptions.emplace(optName, &opt);
        }
    }

    auto &disabledWith = opt.GetDisabledName();
    if (!disabledWith.empty()) {
        for (auto &disabledName : disabledWith) {
            DEBUG_ASSERT(optCategory.options.count(disabledName) == 0, "Duplicated options name %s",
                         disabledName.data());
            optCategory.options.emplace(disabledName, &opt);
        }
    }

    optCategory.registredOptions.push_back(&opt);
    opt.optCategories.push_back(&optCategory);
}