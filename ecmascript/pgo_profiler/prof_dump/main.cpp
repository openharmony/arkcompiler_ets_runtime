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

#include <getopt.h>

#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/platform/file.h"

namespace panda::ecmascript::pgo {
static const std::string VERSION = "0.0.0.1";
static const int MIN_PARAM_COUNT = 3;
using ApGenMode = PGOProfilerEncoder::ApGenMode;

class Option {
public:
    enum class Mode : uint8_t {
        VERSION_QUERY,
        TO_BINARY,
        TO_TEXT,
        MERGE,
    };

    std::string GetProfInPath() const
    {
        return profInPath_;
    }

    std::string GetProfOutPath() const
    {
        return profOutPath_;
    }

    uint32_t GetHotnessThreshold() const
    {
        return hotnessThreshold_;
    }

    Mode GetMode() const
    {
        return mode_;
    }

    bool ParseCommand(const int argc, const char **argv)
    {
        if (argc <= 1) {
            return false;
        }

        const struct option longOptions[] = {
            {"text", required_argument, nullptr, 't'},
            {"binary", required_argument, nullptr, 'b'},
            {"hotness-threshold", required_argument, nullptr, 's'},
            {"merge", no_argument, nullptr, 'm'},
            {"help", no_argument, nullptr, 'h'},
            {"version", no_argument, nullptr, 'v'},
            {nullptr, 0, nullptr, 0},
        };

        const char *optstr = "tbs:hv";
        int opt;
        while ((opt = getopt_long_only(argc, const_cast<char **>(argv), optstr, longOptions, nullptr)) != -1) {
            switch (opt) {
                case 't':
                    mode_ = Mode::TO_TEXT;
                    break;
                case 'b':
                    mode_ = Mode::TO_BINARY;
                    break;
                case 's':
                    if (!base::StringHelper::StrToUInt32(optarg, &hotnessThreshold_)) {
                        LOG_NO_TAG(ERROR) << "hotness-threshold parse failure";
                        return false;
                    }
                    break;
                case 'm':
                    mode_ = Mode::MERGE;
                    break;
                case 'h':
                    return false;
                case 'v':
                    mode_ = Mode::VERSION_QUERY;
                    return true;
                default:
                    LOG_NO_TAG(ERROR) << "Invalid option";
                    return false;
            }
        }
        if (optind != argc - MIN_PARAM_COUNT + 1) {
            return false;
        }
        profInPath_ = argv[optind];
        profOutPath_ = argv[optind + 1];

        return true;
    }

    std::string GetHelper() const
    {
        const std::string PROF_DUMP_HELP_HEAD_MSG =
            "Usage: profdump... SOURCE... DEST... [OPTIONS]\n"
            "\n"
            "optional arguments:\n";
        const std::string PROF_DUMP_HELP_OPTION_MSG =
            "-t, --text                binary to text.\n"
            "-b, --binary              text to binary.\n"
            "-s, --hotness-threshold   set minimum number of calls to filter method. default: 2\n"
            "-m, --merge               merge multi binary ap files into one.\n"
            "-h, --help                display this help and exit\n"
            "-v, --version             output version information and exit\n";
        return PROF_DUMP_HELP_HEAD_MSG + PROF_DUMP_HELP_OPTION_MSG;
    }

    const std::string GetVersion() const
    {
        return VERSION;
    }

private:
    Mode mode_ { Mode::TO_TEXT };
    uint32_t hotnessThreshold_ { 1 };
    std::string profInPath_;
    std::string profOutPath_;
};

int Main(const int argc, const char **argv)
{
    Option option;
    if (!option.ParseCommand(argc, argv)) {
        LOG_NO_TAG(ERROR) << option.GetHelper();
        return  -1;
    }
    switch (option.GetMode()) {
        case Option::Mode::VERSION_QUERY:
            LOG_NO_TAG(ERROR) << "Ver: " << VERSION;
            break;
        case Option::Mode::TO_TEXT: {
            if (PGOProfilerManager::GetInstance()->BinaryToText(option.GetProfInPath(),
                option.GetProfOutPath(), option.GetHotnessThreshold())) {
                LOG_NO_TAG(ERROR) << "profiler dump to text success!";
            } else {
                LOG_NO_TAG(ERROR) << "profiler dump to text failed!";
            }
            break;
        }
        case Option::Mode::TO_BINARY: {
            if (PGOProfilerManager::GetInstance()->TextToBinary(option.GetProfInPath(), option.GetProfOutPath(),
                                                                option.GetHotnessThreshold(), ApGenMode::OVERWRITE)) {
                LOG_NO_TAG(ERROR) << "profiler dump to binary success!";
            } else {
                LOG_NO_TAG(ERROR) << "profiler dump to binary failed!";
            }
            break;
        }
        case Option::Mode::MERGE: {
            // For CLI tools, do not merge with existed output file
            if (PGOProfilerManager::MergeApFiles(option.GetProfInPath(), option.GetProfOutPath(),
                                                 option.GetHotnessThreshold(), ApGenMode::OVERWRITE)) {
                LOG_NO_TAG(ERROR) << "profiler merge success!";
            }
            break;
        }
        default:
            break;
    }
    return 0;
}
} // namespace panda::ecmascript::pgo

int main(int argc, const char **argv)
{
    return panda::ecmascript::pgo::Main(argc, argv);
}
