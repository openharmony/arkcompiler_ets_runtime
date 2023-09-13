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

#include "ecmascript/compiler/aot_compiler.h"

#include "ecmascript/base/string_helper.h"
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/log.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/platform/file.h"

namespace panda::ecmascript::kungfu {
namespace {
using PGOProfilerManager = pgo::PGOProfilerManager;

std::string GetHelper()
{
    std::string str;
    str.append(COMPILER_HELP_HEAD_MSG);
    str.append(HELP_OPTION_MSG);
    return str;
}

CompilationOptions::CompilationOptions(EcmaVM *vm, JSRuntimeOptions &runtimeOptions)
{
    triple_ = runtimeOptions.GetTargetTriple();
    if (runtimeOptions.GetAOTOutputFile().empty()) {
        runtimeOptions.SetAOTOutputFile("aot_file");
    }
    outputFileName_ = runtimeOptions.GetAOTOutputFile();
    optLevel_ = runtimeOptions.GetOptLevel();
    relocMode_ = runtimeOptions.GetRelocMode();
    logOption_ = runtimeOptions.GetCompilerLogOption();
    logMethodsList_ = runtimeOptions.GetMethodsListForLog();
    compilerLogTime_ = runtimeOptions.IsEnableCompilerLogTime();
    maxAotMethodSize_ = runtimeOptions.GetMaxAotMethodSize();
    maxMethodsInModule_ = runtimeOptions.GetCompilerModuleMethods();
    hotnessThreshold_ = runtimeOptions.GetPGOHotnessThreshold();
    profilerIn_ = std::string(runtimeOptions.GetPGOProfilerPath());
    isEnableArrayBoundsCheckElimination_ = runtimeOptions.IsEnableArrayBoundsCheckElimination();
    isEnableTypeLowering_ = runtimeOptions.IsEnableTypeLowering();
    isEnableEarlyElimination_ = runtimeOptions.IsEnableEarlyElimination();
    isEnableLaterElimination_ = runtimeOptions.IsEnableLaterElimination();
    isEnableValueNumbering_ = runtimeOptions.IsEnableValueNumbering();
    isEnableOptInlining_ = runtimeOptions.IsEnableOptInlining();
    isEnableTypeInfer_ = isEnableTypeLowering_ ||
        vm->GetJSThread()->GetCurrentEcmaContext()->GetTSManager()->AssertTypes();
    isEnableOptPGOType_ = runtimeOptions.IsEnableOptPGOType();
    isEnableOptTrackField_ = runtimeOptions.IsEnableOptTrackField();
    isEnableOptLoopPeeling_ = runtimeOptions.IsEnableOptLoopPeeling();
    isEnableOptOnHeapCheck_ = runtimeOptions.IsEnableOptOnHeapCheck();
    isEnableOptLoopInvariantCodeMotion_ = runtimeOptions.IsEnableOptLoopInvariantCodeMotion();
    isEnableOptConstantFolding_ = runtimeOptions.IsEnableOptConstantFolding();
    isEnableCollectLiteralInfo_ = false;
}

bool CompilationPreprocessor::HandleTargetCompilerMode(CompilationOptions &cOptions)
{
    if (runtimeOptions_.IsTargetCompilerMode()) {
        if (!HandleOhosPkgArgs()) {
            LOG_COMPILER(ERROR) << GetHelper();
            LOG_COMPILER(ERROR) << "Parse pkg info failed, exit.";
            return false;
        }
        HandleTargetModeInfo(cOptions);
    }
    return true;
}

bool CompilationPreprocessor::HandleOhosPkgArgs()
{
    ASSERT(runtimeOptions_.IsTargetCompilerMode());
    OhosPkgArgs pkgArgs;
    if (!runtimeOptions_.GetCompilerPkgJsonInfo().empty()) {
        if (pkgArgs.ParseFromJson(vm_, runtimeOptions_.GetCompilerPkgJsonInfo())) {
            LOG_COMPILER(INFO) << "Parse main pkg info success.";
            pkgsArgs_[pkgArgs.GetFullName()] = pkgArgs;
        } else {
            return false;
        }
    }
    if (runtimeOptions_.GetCompilerEnableExternalPkg() && !runtimeOptions_.GetCompilerExternalPkgJsonInfo().empty()) {
        OhosPkgArgs::ParseListFromJson(vm_, runtimeOptions_.GetCompilerExternalPkgJsonInfo(), pkgsArgs_);
    }
    for (const auto &pkgInfo : pkgsArgs_) {
        pandaFileNames_.emplace_back(pkgInfo.first);
        pkgInfo.second.Dump();
    }
    return true;
}

void CompilationPreprocessor::HandleTargetModeInfo(CompilationOptions &cOptions)
{
    JSRuntimeOptions &vmOpt = vm_->GetJSOptions();
    ASSERT(vmOpt.IsTargetCompilerMode());
    // target need fast compiler mode
    vmOpt.SetFastAOTCompileMode(true);
    vmOpt.SetOptLevel(3);  // 3: default opt level
    cOptions.optLevel_ = 3;
    vmOpt.SetEnableOptOnHeapCheck(false);
    cOptions.isEnableOptOnHeapCheck_ = false;
}

bool CompilationPreprocessor::HandlePandaFileNames(const int argc, const char **argv)
{
    if (runtimeOptions_.GetCompilerPkgJsonInfo().empty() || pkgsArgs_.empty()) {
        // if no pkgArgs, last param must be abc file
        std::string files = argv[argc - 1];
        if (!base::StringHelper::EndsWith(files, ".abc")) {
            LOG_COMPILER(ERROR) << "The last argument must be abc file" << std::endl;
            LOG_COMPILER(ERROR) << GetHelper();
            return false;
        }
        std::string delimiter = GetFileDelimiter();
        pandaFileNames_ = base::StringHelper::SplitString(files, delimiter);
    }
    return true;
}

void CompilationPreprocessor::AOTInitialize()
{
    BytecodeStubCSigns::Initialize();
    CommonStubCSigns::Initialize();
    RuntimeStubCSigns::Initialize();
    vm_->GetJSThread()->GetCurrentEcmaContext()->GetTSManager()->Initialize();
}

void CompilationPreprocessor::SetShouldCollectLiteralInfo(CompilationOptions &cOptions, const CompilerLog *log)
{
    TSManager *tsManager = vm_->GetJSThread()->GetCurrentEcmaContext()->GetTSManager();
    cOptions.isEnableCollectLiteralInfo_ = cOptions.isEnableTypeInfer_ &&
        (profilerDecoder_.IsLoaded() || tsManager->AssertTypes() || log->OutputType());
}

void CompilationPreprocessor::GenerateAbcFileInfos()
{
    size_t size = pandaFileNames_.size();
    for (size_t i = 0; i < size; ++i) {
        const auto &fileName = pandaFileNames_.at(i);
        auto extendedFilePath = panda::os::file::File::GetExtendedFilePath(fileName);
        std::shared_ptr<JSPandaFile> jsPandaFile = CreateAndVerifyJSPandaFile(extendedFilePath);
        AbcFileInfo fileInfo(extendedFilePath, jsPandaFile);
        if (jsPandaFile == nullptr) {
            LOG_COMPILER(ERROR) << "Cannot execute panda file '" << extendedFilePath << "'";
            continue;
        }

        if (!PGOProfilerManager::MergeApFiles(jsPandaFile->GetChecksum(), profilerDecoder_)) {
            LOG_COMPILER(ERROR) << "File '" << extendedFilePath << "' load and verify profiler failure";
            continue;
        }

        ResolveModule(jsPandaFile.get(), extendedFilePath);
        fileInfos_.emplace_back(fileInfo);
    }
}

std::shared_ptr<JSPandaFile> CompilationPreprocessor::CreateAndVerifyJSPandaFile(const std::string &fileName)
{
    JSPandaFileManager *jsPandaFileManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> jsPandaFile = nullptr;
    if (runtimeOptions_.IsTargetCompilerMode()) {
        auto pkgArgsIter = pkgsArgs_.find(fileName);
        if (pkgArgsIter == pkgsArgs_.end()) {
            LOG_COMPILER(ERROR) << "Can not find file in ohos pkgs args. file name: " << fileName;
            return nullptr;
        }
        if (!(pkgArgsIter->second.GetJSPandaFile(runtimeOptions_, jsPandaFile))) {
            return nullptr;
        }
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

    jsPandaFileManager->AddJSPandaFileVm(vm_, jsPandaFile);
    return jsPandaFile;
}

void CompilationPreprocessor::ResolveModule(const JSPandaFile *jsPandaFile, const std::string &fileName)
{
    const auto &recordInfo = jsPandaFile->GetJSRecordInfo();
    JSThread *thread = vm_->GetJSThread();
    ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    [[maybe_unused]] EcmaHandleScope scope(thread);
    for (auto info: recordInfo) {
        if (jsPandaFile->IsModule(info.second)) {
            auto recordName = info.first;
            JSHandle<JSTaggedValue> moduleRecord = moduleManager->HostResolveImportedModuleWithMerge(fileName.c_str(),
                recordName);
            SourceTextModule::Instantiate(thread, moduleRecord);
        }
    }
}

void CompilationPreprocessor::GenerateGlobalTypes(const CompilationOptions &cOptions)
{
    for (const AbcFileInfo &fileInfo : fileInfos_) {
        JSPandaFile *jsPandaFile = fileInfo.jsPandaFile_.get();
        TSManager *tsManager = vm_->GetJSThread()->GetCurrentEcmaContext()->GetTSManager();
        BytecodeInfoCollector collector(vm_, jsPandaFile, profilerDecoder_, cOptions.maxAotMethodSize_,
                                        cOptions.isEnableCollectLiteralInfo_);
        BCInfo &bytecodeInfo = collector.GetBytecodeInfo();
        const auto &methodPcInfos = bytecodeInfo.GetMethodPcInfos();
        auto &methodList = bytecodeInfo.GetMethodList();
        for (const auto &method : methodList) {
            uint32_t methodOffset = method.first;
            tsManager->SetCurConstantPool(jsPandaFile, methodOffset);
            CString recordName = MethodLiteral::GetRecordName(jsPandaFile, EntityId(methodOffset));
            auto methodLiteral = jsPandaFile->FindMethodLiteral(methodOffset);
            auto &methodInfo = methodList.at(methodOffset);
            auto &methodPcInfo = methodPcInfos[methodInfo.GetMethodPcInfoIndex()];
            TypeRecorder typeRecorder(jsPandaFile, methodLiteral, tsManager, recordName, &profilerDecoder_,
                                      methodPcInfo, collector.GetByteCodes(), cOptions.isEnableOptTrackField_);
            typeRecorder.BindPgoTypeToGateType(jsPandaFile, tsManager, methodLiteral);
        }
    }
}

void CompilationPreprocessor::SnapshotInitialize()
{
    TSManager *tsManager = vm_->GetJSThread()->GetCurrentEcmaContext()->GetTSManager();
    tsManager->SnapshotInit(fileInfos_.size());
}

std::string GetEntryPoint(const JSRuntimeOptions &runtimeOptions)
{
    std::string entrypoint = "init::func_main_0";
    if (runtimeOptions.WasSetEntryPoint()) {
        entrypoint = runtimeOptions.GetEntryPoint();
    }
    return entrypoint;
}

void CompileValidFiles(PassManager &passManager, AOTFileGenerator &generator, bool &ret,
                       const CVector<AbcFileInfo> &fileInfos)
{
    for (const AbcFileInfo &fileInfo : fileInfos) {
        JSPandaFile *jsPandaFile = fileInfo.jsPandaFile_.get();
        const std::string &extendedFilePath = fileInfo.extendedFilePath_;
        LOG_COMPILER(INFO) << "AOT compile: " << extendedFilePath;
        generator.SetCurrentCompileFileName(jsPandaFile->GetNormalizedFileDesc());
        if (passManager.Compile(jsPandaFile, extendedFilePath, generator) == false) {
            ret = false;
            continue;
        }
    }
}
} // namespace

int Main(const int argc, const char **argv)
{
    auto startTime =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch())
                    .count();
    LOG_ECMA(DEBUG) << "Print ark_aot_compiler received args:";
    for (int i = 0; i < argc; i++) {
        LOG_ECMA(DEBUG) << argv[i];
    }

    if (argc < 2) { // 2: at least have two arguments
        LOG_COMPILER(ERROR) << GetHelper();
        return -1;
    }

    JSRuntimeOptions runtimeOptions;
    bool retOpt = runtimeOptions.ParseCommand(argc, argv);
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
        arg_list_t pandaFileNames {};
        std::map<std::string, OhosPkgArgs> pkgArgsMap;
        CompilationOptions cOptions(vm, runtimeOptions);

        CompilerLog log(cOptions.logOption_);
        log.SetEnableCompilerLogTime(cOptions.compilerLogTime_);
        AotMethodLogList logList(cOptions.logMethodsList_);
        PGOProfilerDecoder profilerDecoder(cOptions.profilerIn_, cOptions.hotnessThreshold_);

        CompilationPreprocessor cPreprocessor(vm, runtimeOptions, pkgArgsMap, profilerDecoder, pandaFileNames);
        if (!cPreprocessor.HandleTargetCompilerMode(cOptions) ||
            !cPreprocessor.HandlePandaFileNames(argc, argv)) {
            return 1;
        }
        cPreprocessor.AOTInitialize();
        cPreprocessor.SetShouldCollectLiteralInfo(cOptions, &log);
        cPreprocessor.GenerateAbcFileInfos();
        cPreprocessor.GenerateGlobalTypes(cOptions);
        cPreprocessor.SnapshotInitialize();
        ret = cPreprocessor.GetCompilerResult();

        PassOptions passOptions(cOptions.isEnableArrayBoundsCheckElimination_,
                                cOptions.isEnableTypeLowering_,
                                cOptions.isEnableEarlyElimination_,
                                cOptions.isEnableLaterElimination_,
                                cOptions.isEnableValueNumbering_,
                                cOptions.isEnableTypeInfer_,
                                cOptions.isEnableOptInlining_,
                                cOptions.isEnableOptPGOType_,
                                cOptions.isEnableOptTrackField_,
                                cOptions.isEnableOptLoopPeeling_,
                                cOptions.isEnableOptOnHeapCheck_,
                                cOptions.isEnableOptLoopInvariantCodeMotion_,
                                cOptions.isEnableCollectLiteralInfo_,
                                cOptions.isEnableOptConstantFolding_);
        std::string entrypoint = GetEntryPoint(runtimeOptions);
        PassManager passManager(vm,
                                entrypoint,
                                cOptions.triple_,
                                cOptions.optLevel_,
                                cOptions.relocMode_,
                                &log,
                                &logList,
                                cOptions.maxAotMethodSize_,
                                cOptions.maxMethodsInModule_,
                                profilerDecoder,
                                &passOptions);
        AOTFileGenerator generator(&log, &logList, vm, cOptions.triple_);
        const auto &fileInfos = cPreprocessor.GetAbcFileInfo();
        CompileValidFiles(passManager, generator, ret, fileInfos);
        generator.SaveAOTFile(cOptions.outputFileName_ + AOTFileManager::FILE_EXTENSION_AN);
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
