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

#include <chrono>
#include <iostream>
#include <iterator>
#include <limits>
#include <ostream>
#include <signal.h>  // NOLINTNEXTLINE(modernize-deprecated-headers)
#include <vector>

#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/log.cpp"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "libpandabase/utils/pandargs.h"
#include "libpandabase/utils/span.h"

namespace panda::ecmascript {
void BlockSignals()
{
#if defined(PANDA_TARGET_UNIX)
    sigset_t set;
    if (sigemptyset(&set) == -1) {
        LOG_ECMA(ERROR) << "sigemptyset failed";
        return;
    }
#endif  // PANDA_TARGET_UNIX
}

int Main(const int argc, const char **argv)
{
    auto startTime =
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();

    BlockSignals();
    Span<const char *> sp(argv, argc);
    JSRuntimeOptions runtimeOptions;

    panda::PandArg<bool> help("help", false, "Print this message and exit");
    panda::PandArg<bool> options("options", false, "Print compiler and runtime options");
    // tail arguments
#if defined(PANDA_TARGET_WINDOWS)
    panda::PandArg<arg_list_t> files("files", {""}, "path to pandafiles", ";");
#else
    panda::PandArg<arg_list_t> files("files", {""}, "path to pandafiles", ":");
#endif
    panda::PandArg<std::string> entrypoint("entrypoint", "_GLOBAL::func_main_0",
                                           "full name of entrypoint function or method");
    panda::PandArg<bool> mergeAbc("merge-abc", false, "abc file is merge abc. Default: false");
    panda::PandArgParser paParser;

    runtimeOptions.AddOptions(&paParser);

    paParser.Add(&help);
    paParser.Add(&options);
    paParser.Add(&mergeAbc);
    paParser.PushBackTail(&files);
    paParser.PushBackTail(&entrypoint);
    paParser.EnableTail();
    paParser.EnableRemainder();

    if (!paParser.Parse(argc, argv) || files.GetValue().empty() || entrypoint.GetValue().empty() || help.GetValue()) {
        std::cerr << paParser.GetErrorString() << std::endl;
        std::cerr << "Usage: "
                  << "quick_fix"
                  << " [OPTIONS] [file1:file2:file3] [entrypoint] -- [arguments]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "optional arguments:" << std::endl;
        std::cerr << paParser.GetHelpString() << std::endl;
        return 1;
    }

    arg_list_t arguments = paParser.GetRemainder();

    if (runtimeOptions.IsStartupTime()) {
        std::cout << "\n"
                  << "Startup start time: " << startTime << std::endl;
    }
    EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);
    if (vm == nullptr) {
        std::cerr << "Cannot Create vm" << std::endl;
        return -1;
    }

    {
        std::cout << "QuickFix Test start!" << std::endl;
        LocalScope scope(vm);
        std::string entry = entrypoint.GetValue();

        arg_list_t fileNames = files.GetValue();
        uint32_t len = fileNames.size();
        if (len < 4) {  // 4: four abc file
            std::cerr << "Must include base.abc, patch.abc, test1.abc, test2.abc absolute path" << std::endl;
            return -1;
        }
        std::string baseFileName = fileNames[0];
        std::string patchFileName = fileNames[1];  // 1: second file, patch.abc
        std::string testLoadFileName = fileNames[2];  // 2: third file, test loadpatch abc file.
        std::string testUnloadFileName = fileNames[3]; // 3: fourth file, test unloadpatch abc file.

        JSNApi::EnableUserUncaughtErrorHandler(vm);

        bool isMergeAbc = mergeAbc.GetValue();
        JSNApi::SetBundle(vm, !isMergeAbc);
        auto res = JSNApi::Execute(vm, baseFileName, entry);
        if (!res) {
            std::cerr << "Cannot execute panda file '" << baseFileName << "' with entry '" << entry << "'" << std::endl;
            return -1;
        }

        res = JSNApi::LoadPatch(vm, patchFileName, baseFileName);
        if (!res) {
            std::cerr << "LoadPatch failed!"<< std::endl;
            return -1;
        }

        res = JSNApi::Execute(vm, testLoadFileName, entry);
        if (!res) {
            std::cerr << "Cannot execute panda file '" << testLoadFileName
                      << "' with entry '" << entry << "'" << std::endl;
            return -1;
        }

        Local<ObjectRef> exception = JSNApi::GetUncaughtException(vm);
        res = JSNApi::IsQuickFixCausedException(vm, exception, patchFileName);
        if (res) {
            std::cout << "Patch have exception." << std::endl;
        } else {
            std::cout << "Patch have no exception." << std::endl;
        }

        res = JSNApi::UnloadPatch(vm, patchFileName);
        if (!res) {
            std::cerr << "UnloadPatch failed!" << std::endl;
            return -1;
        }

        res = JSNApi::Execute(vm, testUnloadFileName, entry);
        if (!res) {
            std::cerr << "Cannot execute panda file '" << testUnloadFileName
                      << "' with entry '" << entry << "'" << std::endl;
            return -1;
        }

        std::cout << "QuickFix Test end!" << std::endl;
    }

    JSNApi::DestroyJSVM(vm);
    paParser.DisableTail();
    return 0;
}
}  // namespace panda::ecmascript

int main(int argc, const char **argv)
{
    return panda::ecmascript::Main(argc, argv);
}
