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
#include "ecmascript/compiler/aot_compiler_preprocessor.h"
#include "ecmascript/compiler/pgo_type/pgo_type_parser.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/ohos/ohos_pgo_processor.h"
#include "ecmascript/ohos/ohos_pkg_args.h"

namespace panda::ecmascript::kungfu {
namespace {
constexpr int32_t DEFAULT_OPT_LEVEL = 3;  // 3: default opt level
}  // namespace
using PGOProfilerManager = pgo::PGOProfilerManager;

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
    needMerge_ = false;
    isEnableArrayBoundsCheckElimination_ = runtimeOptions.IsEnableArrayBoundsCheckElimination();
    isEnableTypeLowering_ = runtimeOptions.IsEnableTypeLowering();
    isEnableEarlyElimination_ = runtimeOptions.IsEnableEarlyElimination();
    isEnableLaterElimination_ = runtimeOptions.IsEnableLaterElimination();
    isEnableValueNumbering_ = runtimeOptions.IsEnableValueNumbering();
    isEnableOptInlining_ = runtimeOptions.IsEnableOptInlining();
    isEnableOptString_ = runtimeOptions.IsEnableOptString();
    isEnableTypeInfer_ = isEnableTypeLowering_ ||
        vm->GetJSThread()->GetCurrentEcmaContext()->GetTSManager()->AssertTypes();
    isEnableOptPGOType_ = runtimeOptions.IsEnableOptPGOType();
    isEnableOptTrackField_ = runtimeOptions.IsEnableOptTrackField();
    isEnableOptLoopPeeling_ = runtimeOptions.IsEnableOptLoopPeeling();
    isEnableOptLoopInvariantCodeMotion_ = runtimeOptions.IsEnableOptLoopInvariantCodeMotion();
    isEnableOptConstantFolding_ = runtimeOptions.IsEnableOptConstantFolding();
    isEnableCollectLiteralInfo_ = false;
    isEnableLexenvSpecialization_ = runtimeOptions.IsEnableLexenvSpecialization();
    isEnableNativeInline_ = runtimeOptions.IsEnableNativeInline();
    isEnableLoweringBuiltin_ = runtimeOptions.IsEnableLoweringBuiltin();
    isEnableOptBranchProfiling_ = runtimeOptions.IsEnableBranchProfiling();
}

bool AotCompilerPreprocessor::HandleTargetCompilerMode(CompilationOptions &cOptions)
{
    if (runtimeOptions_.IsTargetCompilerMode()) {
        if (!OhosPkgArgs::ParseArgs(*this, cOptions)) {
            LOG_COMPILER(ERROR) << GetHelper();
            LOG_COMPILER(ERROR) << "Parse pkg info failed, exit.";
            return false;
        }
        const auto& mainPkgArgs = GetMainPkgArgs();
        if (!mainPkgArgs) {
            LOG_COMPILER(ERROR) << "No main pkg args found, exit";
            return false;
        }
        if (!OhosPgoProcessor::MergeAndRemoveRuntimeAp(cOptions, mainPkgArgs)) {
            LOG_COMPILER(ERROR) << "Fusion runtime ap failed, exit";
            return false;
        }
        HandleTargetModeInfo(cOptions);
    }
    return true;
}

void AotCompilerPreprocessor::HandleTargetModeInfo(CompilationOptions &cOptions)
{
    JSRuntimeOptions &vmOpt = vm_->GetJSOptions();
    ASSERT(vmOpt.IsTargetCompilerMode());
    // target need fast compiler mode
    vmOpt.SetFastAOTCompileMode(true);
    vmOpt.SetOptLevel(DEFAULT_OPT_LEVEL);
    cOptions.optLevel_ = DEFAULT_OPT_LEVEL;
}

bool AotCompilerPreprocessor::HandlePandaFileNames(const int argc, const char **argv)
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

void AotCompilerPreprocessor::AOTInitialize()
{
    BytecodeStubCSigns::Initialize();
    CommonStubCSigns::Initialize();
    RuntimeStubCSigns::Initialize();
    vm_->GetJSThread()->GetCurrentEcmaContext()->GetTSManager()->Initialize();
}

void AotCompilerPreprocessor::SetShouldCollectLiteralInfo(CompilationOptions &cOptions, const CompilerLog *log)
{
    TSManager *tsManager = vm_->GetJSThread()->GetCurrentEcmaContext()->GetTSManager();
    cOptions.isEnableCollectLiteralInfo_ = cOptions.isEnableTypeInfer_ &&
        (profilerDecoder_.IsLoaded() || tsManager->AssertTypes() || log->OutputType());
}

bool AotCompilerPreprocessor::GenerateAbcFileInfos()
{
    size_t size = pandaFileNames_.size();
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; ++i) {
        const auto &fileName = pandaFileNames_.at(i);
        auto extendedFilePath = panda::os::file::File::GetExtendedFilePath(fileName);
        std::shared_ptr<JSPandaFile> jsPandaFile = CreateAndVerifyJSPandaFile(extendedFilePath);
        AbcFileInfo fileInfo(extendedFilePath, jsPandaFile);
        if (jsPandaFile == nullptr) {
            LOG_COMPILER(ERROR) << "Cannot execute panda file '" << extendedFilePath << "'";
            continue;
        }
        checksum = jsPandaFile->GetChecksum();
        ResolveModule(jsPandaFile.get(), extendedFilePath);
        fileInfos_.emplace_back(fileInfo);
    }

    return PGOProfilerManager::MergeApFiles(checksum, profilerDecoder_);
}

std::shared_ptr<JSPandaFile> AotCompilerPreprocessor::CreateAndVerifyJSPandaFile(const std::string &fileName)
{
    JSPandaFileManager *jsPandaFileManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> jsPandaFile = nullptr;
    if (runtimeOptions_.IsTargetCompilerMode()) {
        auto pkgArgsIter = pkgsArgs_.find(fileName);
        if (pkgArgsIter == pkgsArgs_.end()) {
            LOG_COMPILER(ERROR) << "Can not find file in ohos pkgs args. file name: " << fileName;
            return nullptr;
        }
        if (!(pkgArgsIter->second->GetJSPandaFile(runtimeOptions_, jsPandaFile))) {
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

void AotCompilerPreprocessor::ResolveModule(const JSPandaFile *jsPandaFile, const std::string &fileName)
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
            RETURN_IF_ABRUPT_COMPLETION(thread);
            SourceTextModule::Instantiate(thread, moduleRecord);
        }
    }
}

void AotCompilerPreprocessor::GenerateGlobalTypes(const CompilationOptions &cOptions)
{
    for (const AbcFileInfo &fileInfo : fileInfos_) {
        JSPandaFile *jsPandaFile = fileInfo.jsPandaFile_.get();
        TSManager *tsManager = vm_->GetJSThread()->GetCurrentEcmaContext()->GetTSManager();
        PGOTypeManager *ptManager = vm_->GetJSThread()->GetCurrentEcmaContext()->GetPTManager();
        BytecodeInfoCollector collector(vm_, jsPandaFile, profilerDecoder_, cOptions.maxAotMethodSize_,
                                        cOptions.isEnableCollectLiteralInfo_);
        BCInfo &bytecodeInfo = collector.GetBytecodeInfo();
        const PGOBCInfo *bcInfo = collector.GetPGOBCInfo();
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

            bcInfo->IterateInfoByType(methodOffset, PGOBCInfo::Type::ARRAY_LITERAL,
                                      [this, tsManager, ptManager, &recordName]([[maybe_unused]] const uint32_t bcIdx,
                                            [[maybe_unused]] const uint32_t bcOffset, const uint32_t cpIdx) {
                                            JSHandle<ConstantPool> constpoolHandle(tsManager->GetConstantPool());
                                            JSThread *thread = vm_->GetJSThread();
                                            JSTaggedValue unsharedCp = thread->GetCurrentEcmaContext()
                                                ->FindUnsharedConstpool(constpoolHandle.GetTaggedValue());
                                            ASSERT(ConstantPool::CheckUnsharedConstpool(unsharedCp));
                                            JSTaggedValue arr = ConstantPool::GetLiteralFromCache<ConstPoolType::ARRAY_LITERAL>(
                                                thread, unsharedCp, cpIdx, recordName);
                                            JSHandle<JSArray> arrayHandle(thread, arr);
                                            panda_file::File::EntityId id =
                                                ConstantPool::GetIdFromCache(constpoolHandle.GetTaggedValue(), cpIdx);
                                            ptManager->RecordElements(id, arrayHandle->GetElements());
                                        });
        }
    }
}

void AotCompilerPreprocessor::GeneratePGOTypes(const CompilationOptions &cOptions)
{
    PGOTypeManager *ptManager = vm_->GetJSThread()->GetCurrentEcmaContext()->GetPTManager();
    for (const AbcFileInfo &fileInfo : fileInfos_) {
        JSPandaFile *jsPandaFile = fileInfo.jsPandaFile_.get();
        BytecodeInfoCollector collector(vm_, jsPandaFile, profilerDecoder_, cOptions.maxAotMethodSize_,
                                        cOptions.isEnableCollectLiteralInfo_);
        PGOTypeParser parser(profilerDecoder_, ptManager);
        parser.CreatePGOType(collector);
    }
}

void AotCompilerPreprocessor::SnapshotInitialize()
{
    PGOTypeManager *ptManager = vm_->GetJSThread()->GetCurrentEcmaContext()->GetPTManager();
    ptManager->InitAOTSnapshot(fileInfos_.size());
}

std::string AotCompilerPreprocessor::GetMainPkgArgsAppSignature() const
{
    return GetMainPkgArgs() == nullptr ? "" : GetMainPkgArgs()->GetAppSignature();
}
} // namespace panda::ecmascript::kungfu