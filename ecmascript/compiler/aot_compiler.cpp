/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include <signal.h>  // NOLINTNEXTLINE(modernize-deprecated-headers)
#include <vector>

#include "ecmascript/aot_file_manager.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/log.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/platform/file.h"

namespace panda::ecmascript::kungfu {
std::string GetHelper()
{
    std::string str;
    str.append(COMMON_HELP_HEAD_MSG);
    str.append(HELP_OPTION_MSG);
    str.append(HELP_TAIL_MSG);
    return str;
}

void AOTInitialize(EcmaVM *vm)
{
    BytecodeStubCSigns::Initialize();
    CommonStubCSigns::Initialize();
    RuntimeStubCSigns::Initialize();
    vm->GetTSManager()->Initialize();
}

int Main(const int argc, const char **argv)
{
    auto startTime =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
                    .count();
    std::string entrypoint = "init::func_main_0";

    int newArgc = argc;
    if (argc < 2) { // 2: at least have two arguments
        std::cerr << GetHelper();
        return -1;
    }

    std::string files = argv[argc - 1];
    if (!base::StringHelper::EndsWith(files, ".abc")) {
        std::cerr << "The last argument must be abc file" << std::endl;
        std::cerr << GetHelper();
        return 1;
    }

    newArgc--;
    JSRuntimeOptions runtimeOptions;
    bool retOpt = runtimeOptions.ParseCommand(newArgc, argv);
    if (!retOpt) {
        std::cerr << GetHelper();
        return 1;
    }

    if (runtimeOptions.IsStartupTime()) {
        LOG_COMPILER(DEBUG) << "Startup start time: " << startTime;
    }

    bool ret = true;
    // ark_aot_compiler running need disable asm interpreter to disable the loading of AOT files.
    runtimeOptions.SetEnableAsmInterpreter(false);
    EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);
    if (vm == nullptr) {
        LOG_COMPILER(ERROR) << "Cannot Create vm";
        return -1;
    }

    {
        LocalScope scope(vm);
        std::string delimiter = GetFileDelimiter();
        arg_list_t pandaFileNames = base::StringHelper::SplitString(files, delimiter);
        std::string triple = runtimeOptions.GetTargetTriple();
        if (runtimeOptions.GetAOTOutputFile().empty()) {
            runtimeOptions.SetAOTOutputFile("aot_file");
        }
        std::string outputFileName = runtimeOptions.GetAOTOutputFile();
        size_t optLevel = runtimeOptions.GetOptLevel();
        size_t relocMode = runtimeOptions.GetRelocMode();
        std::string logOption = runtimeOptions.GetCompilerLogOption();
        std::string logMethodsList = runtimeOptions.GetMethodsListForLog();
        bool compilerLogTime = runtimeOptions.IsEnableCompilerLogTime();
        bool isTraceBC = runtimeOptions.IsTraceBC();
        size_t maxAotMethodSize = runtimeOptions.GetMaxAotMethodSize();
        bool isEnableTypeLowering = runtimeOptions.IsEnableTypeLowering();
        uint32_t hotnessThreshold = runtimeOptions.GetPGOHotnessThreshold();
        AOTInitialize(vm);

        CompilerLog log(logOption, isTraceBC);
        log.SetEnableCompilerLogTime(compilerLogTime);
        AotMethodLogList logList(logMethodsList);
        AOTFileGenerator generator(&log, &logList, vm, triple);
        std::string profilerIn(runtimeOptions.GetPGOProfilerPath());
        if (runtimeOptions.WasSetEntryPoint()) {
            entrypoint = runtimeOptions.GetEntryPoint();
        }
        PassManager passManager(vm, entrypoint, triple, optLevel, relocMode, &log, &logList, maxAotMethodSize,
                                isEnableTypeLowering, profilerIn, hotnessThreshold);
        for (const auto &fileName : pandaFileNames) {
            auto extendedFilePath = panda::os::file::File::GetExtendedFilePath(fileName);
            LOG_COMPILER(INFO) << "AOT compile: " << extendedFilePath;
            if (passManager.Compile(extendedFilePath, generator) == false) {
                ret = false;
                continue;
            }
        }
        generator.SaveAOTFile(outputFileName + AOTFileManager::FILE_EXTENSION_AN);
        generator.SaveSnapshotFile();
        log.Print();
    }

    LOG_COMPILER(INFO) << (ret ? "ts aot compile success" : "ts aot compile failed");
    JSNApi::DestroyJSVM(vm);
    return ret ? 0 : -1;
}
} // namespace panda::ecmascript::kungfu

int main(const int argc, const char **argv)
{
    auto result = panda::ecmascript::kungfu::Main(argc, argv);
    return result;
}
