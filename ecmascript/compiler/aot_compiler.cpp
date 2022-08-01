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

#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/napi/include/jsnapi.h"

#include "generated/base_options.h"
#include "libpandabase/utils/pandargs.h"

namespace panda::ecmascript::kungfu {
int Main(const int argc, const char **argv)
{
    auto startTime =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
                    .count();
    Span<const char *> sp(argv, argc);
    JSRuntimeOptions runtimeOptions;
    base_options::Options baseOptions(sp[0]);

    panda::PandArg<bool> help("help", false, "Print this message and exit");
    panda::PandArg<bool> options("options", false, "Print options");
    // tail arguments
    panda::PandArg<arg_list_t> files("files", {""}, "path to pandafiles", ":");
    panda::PandArg<std::string> entrypoint("entrypoint", "init::func_main_0",
                                           "full name of entrypoint function or method");
    panda::PandArgParser paParser;

    runtimeOptions.AddOptions(&paParser);
    baseOptions.AddOptions(&paParser);

    paParser.Add(&help);
    paParser.Add(&options);
    paParser.PushBackTail(&files);
    paParser.PushBackTail(&entrypoint);
    paParser.EnableTail();
    paParser.EnableRemainder();

    if (!paParser.Parse(argc, argv) || files.GetValue().empty() || entrypoint.GetValue().empty() || help.GetValue()) {
        std::cerr << paParser.GetErrorString() << std::endl;
        std::cerr << "Usage: "
                  << "panda"
                  << " [OPTIONS] [file1:file2:file3] [entrypoint] -- [arguments]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "optional arguments:" << std::endl;
        std::cerr << paParser.GetHelpString() << std::endl;
        return 1;
    }

    Logger::Initialize(baseOptions);
    Logger::SetLevel(Logger::Level::INFO);
    Logger::ResetComponentMask();  // disable all Component
    Logger::EnableComponent(Logger::Component::ECMASCRIPT);  // enable ECMASCRIPT

    arg_list_t arguments = paParser.GetRemainder();

    if (runtimeOptions.IsStartupTime()) {
        LOG_COMPILER(DEBUG) << "Startup start time: " << startTime;
    }

    bool ret = true;
    // ark_aot_compiler running need disable asm interpreter
    runtimeOptions.SetEnableAsmInterpreter(false);
    EcmaVM *vm = JSNApi::CreateEcmaVM(runtimeOptions);
    if (vm == nullptr) {
        LOG_COMPILER(ERROR) << "Cannot Create vm";
        return -1;
    }

    LocalScope scope(vm);
    std::string entry = entrypoint.GetValue();
    arg_list_t pandaFileNames = files.GetValue();
    std::string triple = runtimeOptions.GetTargetTriple();
    std::string outputFileName = runtimeOptions.GetAOTOutputFile();
    size_t optLevel = runtimeOptions.GetOptLevel();
    size_t relocMode = runtimeOptions.GetRelocMode();
    std::string logOption = runtimeOptions.GetCompilerLogOption();
    std::string logMethodsList = runtimeOptions.GetMethodsListForLog();
    bool isEnableBcTrace = runtimeOptions.IsEnableByteCodeTrace();
    BytecodeStubCSigns::Initialize();
    CommonStubCSigns::Initialize();
    RuntimeStubCSigns::Initialize();

    CompilerLog log(logOption, isEnableBcTrace);
    AotMethodLogList logList(logMethodsList);
    AOTFileGenerator generator(&log, &logList, vm);
    PassManager passManager(vm, entry, triple, optLevel, relocMode, &log, &logList);
    for (const auto &fileName : pandaFileNames) {
        LOG_COMPILER(INFO) << "AOT start to execute ark file: " << fileName;
        if (passManager.Compile(fileName, generator) == false) {
            ret = false;
            break;
        }
    }
    generator.SaveAOTFile(outputFileName);
    generator.SaveSnapshotFile();

    JSNApi::DestroyJSVM(vm);
    paParser.DisableTail();
    return ret ? 0 : -1;
}
} // namespace panda::ecmascript::kungfu

int main(const int argc, const char **argv)
{
    auto result = panda::ecmascript::kungfu::Main(argc, argv);
    LOG_COMPILER(INFO) << (result == 0 ? "ts aot compile success" : "ts aot compile failed");
    return result;
}
