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

#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/log.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/platform/file.h"

namespace panda::ecmascript::kungfu {
std::string GetHelper()
{
    std::string str;
    str.append(COMPILER_HELP_HEAD_MSG);
    str.append(HELP_OPTION_MSG);
    return str;
}

void AOTInitialize(EcmaVM *vm)
{
    BytecodeStubCSigns::Initialize();
    CommonStubCSigns::Initialize();
    RuntimeStubCSigns::Initialize();
    vm->GetJSThread()->GetCurrentEcmaContext()->GetTSManager()->Initialize();
}

JSPandaFile *CreateAndVerifyJSPandaFile(const JSRuntimeOptions &runtimeOptions, const std::string &fileName, EcmaVM *vm)
{
    JSPandaFileManager *jsPandaFileManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> jsPandaFile = nullptr;
    if (runtimeOptions.IsTargetCompilerMode()) {
        std::string hapPath = runtimeOptions.GetHapPath();
        uint32_t offset = runtimeOptions.GetHapAbcOffset();
        uint32_t size = runtimeOptions.GetHapAbcSize();
        if (size == 0) {
            LOG_ECMA(ERROR) << "buffer is empty in target compiler mode!";
            return nullptr;
        }
        std::string realPath;
        if (!RealPath(hapPath, realPath, false)) {
            LOG_ECMA(ERROR) << "realpath for hap path failed!";
            return nullptr;
        }
        MemMap fileMapMem = FileMap(realPath.c_str(), FILE_RDONLY, PAGE_PROT_READ);
        if (fileMapMem.GetOriginAddr() == nullptr) {
            LOG_ECMA(ERROR) << "File mmap failed";
            return nullptr;
        }
        uint8_t *buffer = reinterpret_cast<uint8_t *>(fileMapMem.GetOriginAddr()) + offset;
        jsPandaFile = jsPandaFileManager->OpenJSPandaFileFromBuffer(buffer, size, fileName.c_str());
        FileUnMap(fileMapMem);
        fileMapMem.Reset();
    } else {
        jsPandaFile = jsPandaFileManager->OpenJSPandaFile(fileName.c_str());
    }
    if (jsPandaFile == nullptr) {
        LOG_ECMA(ERROR) << "open file " << fileName << " error";
        return nullptr;
    }

    if (!jsPandaFile->IsNewVersion()) {
        LOG_COMPILER(ERROR) << "AOT only support panda file with new ISA, while the '" <<
            fileName << "' file is the old version";
        return nullptr;
    }

    jsPandaFileManager->AddJSPandaFileVm(vm, jsPandaFile);
    return jsPandaFile.get();
}

int Main(const int argc, const char **argv)
{
    auto startTime =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
                    .count();
    std::string entrypoint = "init::func_main_0";
    LOG_ECMA(DEBUG) << "Print ark_aot_compiler received args:";
    for (int i = 0; i < argc; i++) {
        LOG_ECMA(DEBUG) << argv[i];
    }

    int newArgc = argc;
    if (argc < 2) { // 2: at least have two arguments
        LOG_COMPILER(ERROR) << GetHelper();
        return -1;
    }

    std::string files = argv[argc - 1];
    if (!base::StringHelper::EndsWith(files, ".abc")) {
        LOG_COMPILER(ERROR) << "The last argument must be abc file" << std::endl;
        LOG_COMPILER(ERROR) << GetHelper();
        return 1;
    }

    newArgc--;
    JSRuntimeOptions runtimeOptions;
    bool retOpt = runtimeOptions.ParseCommand(newArgc, argv);
    if (!retOpt) {
        LOG_COMPILER(ERROR) << GetHelper();
        return 1;
    }

    if (runtimeOptions.IsStartupTime()) {
        LOG_COMPILER(DEBUG) << "Startup start time: " << startTime;
    }

    bool ret = true;
    // ark_aot_compiler running need disable asm interpreter to disable the loading of AOT files.
    runtimeOptions.SetEnableAsmInterpreter(false);
    runtimeOptions.DisableReportModuleResolvingFailure();
    runtimeOptions.SetOptionsForTargetCompilation();
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
        size_t maxAotMethodSize = runtimeOptions.GetMaxAotMethodSize();
        size_t maxMethodsInModule = runtimeOptions.GetCompilerModuleMethods();
        bool isEnableArrayBoundsCheckElimination = runtimeOptions.IsEnableArrayBoundsCheckElimination();
        bool isEnableTypeLowering = runtimeOptions.IsEnableTypeLowering();
        bool isEnableEarlyElimination = runtimeOptions.IsEnableEarlyElimination();
        bool isEnableLaterElimination = runtimeOptions.IsEnableLaterElimination();
        bool isEnableValueNumbering = runtimeOptions.IsEnableValueNumbering();
        bool isEnableOptInlining = runtimeOptions.IsEnableOptInlining();
        bool isEnableTypeInfer = isEnableTypeLowering ||
            vm->GetJSThread()->GetCurrentEcmaContext()->GetTSManager()->AssertTypes();
        bool isEnableOptPGOType = runtimeOptions.IsEnableOptPGOType();
        bool isEnableOptTrackField = runtimeOptions.IsEnableOptTrackField();
        bool isEnableOptLoopPeeling = runtimeOptions.IsEnableOptLoopPeeling();
        bool isEnableOptOnHeapCheck = runtimeOptions.IsEnableOptOnHeapCheck();

        if (runtimeOptions.IsTargetCompilerMode()) {
            JSRuntimeOptions &vmOpt = vm->GetJSOptions();
            // target need fast compiler mode
            vmOpt.SetFastAOTCompileMode(true);
            vmOpt.SetOptLevel(3); // 3: default opt level
            optLevel = 3;
            vmOpt.SetEnableOptOnHeapCheck(false);
            isEnableOptOnHeapCheck = false;
        }

        PassOptions passOptions(isEnableArrayBoundsCheckElimination, isEnableTypeLowering, isEnableEarlyElimination,
                                isEnableLaterElimination, isEnableValueNumbering, isEnableTypeInfer,
                                isEnableOptInlining, isEnableOptPGOType, isEnableOptTrackField, isEnableOptLoopPeeling,
                                isEnableOptOnHeapCheck);

        uint32_t hotnessThreshold = runtimeOptions.GetPGOHotnessThreshold();
        AOTInitialize(vm);

        CompilerLog log(logOption);
        log.SetEnableCompilerLogTime(compilerLogTime);
        AotMethodLogList logList(logMethodsList);
        AOTFileGenerator generator(&log, &logList, vm, triple);
        std::string profilerIn(runtimeOptions.GetPGOProfilerPath());

        if (runtimeOptions.WasSetEntryPoint()) {
            entrypoint = runtimeOptions.GetEntryPoint();
        }
        PassManager passManager(vm, entrypoint, triple, optLevel, relocMode, &log, &logList, maxAotMethodSize,
                                maxMethodsInModule, profilerIn, hotnessThreshold, &passOptions);
        for (const auto &fileName : pandaFileNames) {
            auto extendedFilePath = panda::os::file::File::GetExtendedFilePath(fileName);
            LOG_COMPILER(INFO) << "AOT compile: " << extendedFilePath;
            JSPandaFile *jsPandaFile = CreateAndVerifyJSPandaFile(runtimeOptions, extendedFilePath, vm);
            if (passManager.Compile(jsPandaFile, extendedFilePath, generator) == false) {
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
