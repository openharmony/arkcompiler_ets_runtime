/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "metadata_parse.h"
#include "rawheap_translate.h"
#include "serializer.h"
#include "utils.h"

#include <cstdlib>

namespace rawheap_translate {
// Argument index constants for argv positional access.
static constexpr int ARG_INDEX_INPUT = 1;            // argv[1]: first .rawheap (single or dynamic)
static constexpr int ARG_INDEX_SINGLE_OUTPUT = 2;    // argv[2]: single-file optional .heapsnapshot
static constexpr int ARG_INDEX_STATIC = 2;           // argv[2]: static .rawheap (two-file mode)
static constexpr int ARG_INDEX_TWO_FILE_OUTPUT = 3;  // argv[3]: two-file optional .heapsnapshot

// Minimum argc thresholds for each mode.
static constexpr int MIN_ARGC_SINGLE = 2;            // single-file: <rawheap>
static constexpr int MIN_ARGC_SINGLE_OUTPUT = 3;     // single-file + output: <rawheap> <heapsnapshot>
static constexpr int MIN_ARGC_TWO_FILE = 3;          // two-file: <dynamic> <static>
static constexpr int MIN_ARGC_TWO_FILE_OUTPUT = 4;   // two-file + output: <dynamic> <static> <heapsnapshot>

std::string RAWHEAP_TRANSLATE_HELPER =
    "Usage:\n"
    "  Single-file mode:\n"
    "    rawheap_translator <filename.rawheap> [filename.heapsnapshot]\n"
    "  Two-file hybrid mode:\n"
    "    rawheap_translator <dynamic.rawheap> <static.rawheap> [filename.heapsnapshot]\n"
    "\n"
    "In single-file mode, the input .rawheap is translated to a .heapsnapshot.\n"
    "In two-file mode, a dynamic (V1/V2) rawheap and a static binary snapshot\n"
    "are merged into a single .heapsnapshot.\n"
    "If the output file name is not provided, an automatic one will be generated.";

// Parse single-file arguments: 1 .rawheap + optional .heapsnapshot.
bool ParseArgsSingle(const int argc, const char **argv, std::string &input, std::string &output)
{
    std::string rawheapPath = argv[ARG_INDEX_INPUT];
    std::string userOutput = (argc >= MIN_ARGC_SINGLE_OUTPUT) ? argv[ARG_INDEX_SINGLE_OUTPUT] : "";
    if (!GenerateOutputNameFromInput(userOutput, output)) {
        std::cout << "Generate dump file name failed!\n";
        return false;
    }
    input = rawheapPath;
    return true;
}

// Parse two-file arguments: 2 .rawheap + optional .heapsnapshot.
bool ParseArgsTwoFile(const int argc, const char **argv, std::string &dynamicInput,
                      std::string &staticInput, std::string &output)
{
    if (argc < MIN_ARGC_TWO_FILE) {
        return false;
    }
    std::string dynamicPath = argv[ARG_INDEX_INPUT];
    std::string staticPath = argv[ARG_INDEX_STATIC];
    if (!EndsWith(dynamicPath, ".rawheap") || !EndsWith(staticPath, ".rawheap")) {
        return false;
    }
    std::string userOutput = (argc >= MIN_ARGC_TWO_FILE_OUTPUT) ? argv[ARG_INDEX_TWO_FILE_OUTPUT] : "";
    if (!GenerateOutputNameFromInput(userOutput, output)) {
        std::cout << "Generate dump file name failed!\n";
        return false;
    }
    dynamicInput = dynamicPath;
    staticInput = staticPath;
    return true;
}

int Main(const int argc, const char **argv)
{
    if (argc < MIN_ARGC_SINGLE) {
        std::cout << "Input error!\n" << RAWHEAP_TRANSLATE_HELPER << std::endl;
        return 0;
    }

    std::string firstArg = argv[ARG_INDEX_INPUT];
    if (firstArg == "--version" || firstArg == "-v") {
        std::cout << VERSION.ToString() << std::endl;
        return 0;
    }
    if (firstArg == "--help" || firstArg == "-h") {
        std::cout << RAWHEAP_TRANSLATE_HELPER << std::endl;
        return 0;
    }

    // Try two-file mode first (two .rawheap inputs).
    std::string dynamicInput;
    std::string staticInput;
    std::string outputPath;
    if (ParseArgsTwoFile(argc, argv, dynamicInput, staticInput, outputPath)) {
        return RawHeap::TranslateRawheap(dynamicInput, staticInput, outputPath) ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    // Fall back to single-file mode.
    if (!EndsWith(firstArg, ".rawheap")) {
        std::cout << "Input error!\n" << RAWHEAP_TRANSLATE_HELPER << std::endl;
        return 0;
    }
    std::string singleInput;
    if (!ParseArgsSingle(argc, argv, singleInput, outputPath)) {
        std::cout << "Input error!\n" << RAWHEAP_TRANSLATE_HELPER << std::endl;
        return 0;
    }
    RawHeap::TranslateRawheap(singleInput, outputPath);
    return 0;
}

}  // namespace rawheap_translate

#ifndef RAWHEAP_TRANSLATOR_UNITTEST
int main(int argc, const char **argv)
{
    return rawheap_translate::Main(argc, argv);
}
#endif
