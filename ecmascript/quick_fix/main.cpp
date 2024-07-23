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
#include <ostream>
#include <signal.h>  // NOLINTNEXTLINE(modernize-deprecated-headers)
#include <vector>

#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/log.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/napi/include/jsnapi.h"

namespace panda::ecmascript {
using PatchErrorCode = panda::JSNApi::PatchErrorCode;

const std::string TEST_ENTRY_POINT = "test";
const std::string RETEST_ENTRY_POINT = "retest";

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

std::string GetHelper()
{
    std::string str;
    str.append(COMMON_HELP_HEAD_MSG);
    str.append(HELP_OPTION_MSG);
    return str;
}

bool QuickFixQueryFuncColdPatch(std::string baseFileName, std::string &patchFileName,
                                uint8_t ** patchBuffer, size_t &patchBufferSize)
{
    return true;
}

int Main(const int argc, const char **argv)
{
    auto startTime =
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();

    BlockSignals();

    if (argc < 2) { // 2: at least have two arguments
        std::cout << GetHelper();
        return -1;
    }

    int newArgc = argc;
    std::string files = argv[argc - 1];
    if (!base::StringHelper::EndsWith(files, ".abc")) {
        std::cout << "The last argument must be abc file" << std::endl;
        std::cout << GetHelper();
        return 1;
    }

    newArgc--;
    JSRuntimeOptions runtimeOptions;
    bool retOpt = runtimeOptions.ParseCommand(newArgc, argv);
    if (!retOpt) {
        std::cout << GetHelper();
        return 1;
    }

    if (runtimeOptions.IsStartupTime()) {
        std::cout << "\n"
                  << "Startup start time: " << startTime << std::endl;
    }
    EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);
    if (vm == nullptr) {
        std::cout << "Cannot Create vm" << std::endl;
        return -1;
    }

    {
        std::cout << "QuickFix Execute start" << std::endl;
        LocalScope scope(vm);
        std::string entry = runtimeOptions.GetEntryPoint();

#if defined(PANDA_TARGET_WINDOWS)
        arg_list_t fileNames = base::StringHelper::SplitString(files, ";");
#else
        arg_list_t fileNames = base::StringHelper::SplitString(files, ":");
#endif
        uint32_t len = fileNames.size();
        if (len < 4) {  // 4: four abc file
            std::cout << "Must include base.abc, patch.abc, test.abc, retest.abc absolute path" << std::endl;
            JSNApi::DestroyJSVM(vm);
            return -1;
        }
        std::string baseFileName = fileNames[0];
        bool isMergeAbc = runtimeOptions.GetMergeAbc();
        JSNApi::SetBundle(vm, !isMergeAbc);

        arg_list_t entryList = base::StringHelper::SplitString(entry, ":");
        uint32_t size = entryList.size();
        uint32_t entryNum = 1; // 1: one entry, excluding coldpatch testcases.
        if (size < entryNum) { // 1: one entry
            std::cout << "Must include 1 entry at least" << std::endl;
            JSNApi::DestroyJSVM(vm);
            return -1;
        }

        bool res = false;
        if (size == entryNum) {
            res = JSNApi::Execute(vm, baseFileName, entryList[0]);
            if (!res) {
                std::cout << "Cannot execute panda file '" << baseFileName <<
                    "' with entry '" << entry << "'" << std::endl;
                JSNApi::DestroyJSVM(vm);
                return -1;
            }
        }
        JSNApi::EnableUserUncaughtErrorHandler(vm);

        std::string testLoadFileName = fileNames[1];
        std::string testUnloadFileName = fileNames[2]; // 2: third file, test unloadpatch abc file.
        for (uint32_t i = 3; i < len; i++) { // 3: patch file, test unloadpatch abc file.
            std::string patchFileName = fileNames[i];
            std::cout << "QuickFix start load patch" << std::endl;
            auto result = JSNApi::LoadPatch(vm, patchFileName, baseFileName);
            if (result != PatchErrorCode::SUCCESS) {
                std::cout << "LoadPatch failed"<< std::endl;
                break;
            }
            std::cout << "QuickFix load patch success" << std::endl;

            if (size > entryNum) {
                JSNApi::RegisterQuickFixQueryFunc(vm, QuickFixQueryFuncColdPatch);
                res = JSNApi::Execute(vm, baseFileName, entryList[0]);
            }

            res = JSNApi::Execute(vm, testLoadFileName, TEST_ENTRY_POINT);

            if (size == entryNum) {
                std::cout << "QuickFix start check exception" << std::endl;
                Local<ObjectRef> exception = JSNApi::GetAndClearUncaughtException(vm);
                res = JSNApi::IsQuickFixCausedException(vm, exception, patchFileName);
                if (res) {
                    std::cout << "QuickFix have exception" << std::endl;
                } else {
                    std::cout << "QuickFix have no exception" << std::endl;
                }

                std::cout << "QuickFix start unload patch" << std::endl;
                result = JSNApi::UnloadPatch(vm, patchFileName);
                if (result != PatchErrorCode::SUCCESS) {
                    std::cout << "UnloadPatch failed!" << std::endl;
                    break;
                }
                std::cout << "QuickFix unload patch success" << std::endl;

                res = JSNApi::Execute(vm, testUnloadFileName, RETEST_ENTRY_POINT);
                if (!res) {
                    std::cout << "Cannot execute panda file '" << testUnloadFileName
                        << "' with entry '" << entry << "'" << std::endl;
                    break;
                }
            }
        }
        std::cout << "QuickFix Execute end" << std::endl;
    }

    JSNApi::DestroyJSVM(vm);
    return 0;
}
}  // namespace panda::ecmascript

int main(int argc, const char **argv)
{
    return panda::ecmascript::Main(argc, argv);
}
