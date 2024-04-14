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
    deviceIsScreenOff_ = runtimeOptions.GetDeviceState();
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
    optBCRange_ = runtimeOptions.GetOptCodeRange();
    isEnableEscapeAnalysis_ = runtimeOptions.IsEnableEscapeAnalysis();
    isEnableInductionVariableAnalysis_ = runtimeOptions.IsEnableInductionVariableAnalysis();

    std::string optionSelectMethods = runtimeOptions.GetCompilerSelectMethods();
    std::string optionSkipMethods = runtimeOptions.GetCompilerSkipMethods();
    if (!optionSelectMethods.empty() && !optionSkipMethods.empty()) {
        LOG_COMPILER(FATAL) <<
            "--compiler-select-methods and --compiler-skip-methods should not be set at the same time";
    }
    if (!optionSelectMethods.empty()) {
        ParseOption(optionSelectMethods, optionSelectMethods_);
    }
    if (!optionSkipMethods.empty()) {
        ParseOption(optionSkipMethods, optionSkipMethods_);
    }
}

std::vector<std::string> CompilationOptions::SplitString(const std::string &str, const char ch) const
{
    std::vector<std::string> vec {};
    std::istringstream sstr(str.c_str());
    std::string split;
    while (getline(sstr, split, ch)) {
        vec.emplace_back(split);
    }
    return vec;
}

void CompilationOptions::ParseOption(const std::string &option,
    std::map<std::string, std::vector<std::string>> &optionMap) const
{
    const char colon = ':';
    const char comma = ',';
    std::string str = option;
    size_t posColon = 0;
    size_t posComma = 0;
    do {
        posColon = str.find_last_of(colon);
        std::string methodNameList = str.substr(posColon + 1, str.size());
        std::vector<std::string> methodNameVec = SplitString(methodNameList, comma);
        str = str.substr(0, posColon);
        posComma = str.find_last_of(comma);
        std::string recordName = str.substr(posComma + 1, str.size());
        str = str.substr(0, posComma);
        optionMap[recordName] = methodNameVec;
    } while (posComma != std::string::npos);
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
    BuiltinsStubCSigns::Initialize();
    RuntimeStubCSigns::Initialize();
    vm_->GetJSThread()->GetCurrentEcmaContext()->GetTSManager()->Initialize();
}

void AotCompilerPreprocessor::SetShouldCollectLiteralInfo(CompilationOptions &cOptions, const CompilerLog *log)
{
    TSManager *tsManager = vm_->GetJSThread()->GetCurrentEcmaContext()->GetTSManager();
    cOptions.isEnableCollectLiteralInfo_ = cOptions.isEnableTypeInfer_ &&
        (profilerDecoder_.IsLoaded() || tsManager->AssertTypes() || log->OutputType());
}

uint32_t AotCompilerPreprocessor::GenerateAbcFileInfos()
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
    return checksum;
}

bool AotCompilerPreprocessor::HandleMergedPgoFile(uint32_t checksum)
{
    return PGOProfilerManager::MergeApFiles(checksum, profilerDecoder_);
}

std::shared_ptr<JSPandaFile> AotCompilerPreprocessor::CreateAndVerifyJSPandaFile(const std::string &fileName)
{
    JSPandaFileManager *jsPandaFileManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> jsPandaFile = nullptr;
    if (runtimeOptions_.IsTargetCompilerMode()) {
        auto pkgArgsIter = pkgsArgs_.find(fileName);
        if (pkgArgsIter == pkgsArgs_.end()) {
            jsPandaFile = jsPandaFileManager->OpenJSPandaFile(fileName.c_str());
        } else if (!(pkgArgsIter->second->GetJSPandaFile(runtimeOptions_, jsPandaFile))) {
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

    jsPandaFileManager->AddJSPandaFile(jsPandaFile);
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
            moduleManager->HostResolveImportedModuleWithMerge(fileName.c_str(), recordName);
        }
    }
}

void AotCompilerPreprocessor::RecordArrayElement(const CompilationOptions &cOptions)
{
    for (const AbcFileInfo &fileInfo : fileInfos_) {
        JSPandaFile *jsPandaFile = fileInfo.jsPandaFile_.get();
        PGOTypeManager *ptManager = vm_->GetJSThread()->GetCurrentEcmaContext()->GetPTManager();
        ptManager->SetCurCompilationFile(jsPandaFile);
        BytecodeInfoCollector collector(vm_, jsPandaFile, profilerDecoder_, cOptions.maxAotMethodSize_,
                                        cOptions.isEnableCollectLiteralInfo_);
        BCInfo &bytecodeInfo = collector.GetBytecodeInfo();
        const PGOBCInfo *bcInfo = collector.GetPGOBCInfo();
        auto &methodList = bytecodeInfo.GetMethodList();
        for (const auto &method : methodList) {
            uint32_t methodOffset = method.first;
            CString recordName = MethodLiteral::GetRecordName(jsPandaFile, EntityId(methodOffset));
            auto callback = [this, ptManager, methodOffset, &recordName]
                (const uint32_t, const uint32_t, const uint32_t cpIdx) {
                    JSThread *thread = vm_->GetJSThread();
                    JSTaggedValue cp = ptManager->GetConstantPoolByMethodOffset(methodOffset);
                    JSHandle<ConstantPool> constpoolHandle(thread, cp);
                    JSTaggedValue unsharedCp = thread->GetCurrentEcmaContext()
                        ->FindOrCreateUnsharedConstpool(constpoolHandle.GetTaggedValue());
                    ASSERT(ConstantPool::CheckUnsharedConstpool(unsharedCp));
                    JSTaggedValue arr = ConstantPool::GetLiteralFromCache<ConstPoolType::ARRAY_LITERAL>(
                            thread, unsharedCp, cpIdx, recordName);
                    JSHandle<JSArray> arrayHandle(thread, arr);
                    panda_file::File::EntityId id =
                        ConstantPool::GetIdFromCache(constpoolHandle.GetTaggedValue(), cpIdx);
                    ptManager->RecordElements(id, arrayHandle->GetElements());
                };

            bcInfo->IterateInfoByType(methodOffset, PGOBCInfo::Type::ARRAY_LITERAL, callback);
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
    RecordArrayElement(cOptions);
}

void AotCompilerPreprocessor::SnapshotInitialize()
{
    PGOTypeManager *ptManager = vm_->GetJSThread()->GetCurrentEcmaContext()->GetPTManager();
    ptManager->InitAOTSnapshot(fileInfos_.size());
}

void CallMethodFlagMap::SetIsFastCall(CString fileDesc, uint32_t methodOffset, bool isFastCall)
{
    abcIdMethodIdToIsFastCall_[std::pair<CString, uint32_t>(fileDesc, methodOffset)] = isFastCall;
}

bool CallMethodFlagMap::IsFastCall(CString fileDesc, uint32_t methodOffset) const
{
    if (!abcIdMethodIdToIsFastCall_.count(std::pair<CString, uint32_t>(fileDesc, methodOffset))) {
        return false;
    }
    return abcIdMethodIdToIsFastCall_.at(std::pair<CString, uint32_t>(fileDesc, methodOffset));
}

void CallMethodFlagMap::SetIsAotCompile(CString fileDesc, uint32_t methodOffset, bool isAotCompile)
{
    abcIdMethodIdToIsAotCompile_[std::pair<CString, uint32_t>(fileDesc, methodOffset)] = isAotCompile;
}

bool CallMethodFlagMap::IsAotCompile(CString fileDesc, uint32_t methodOffset) const
{
    if (!abcIdMethodIdToIsAotCompile_.count(std::pair<CString, uint32_t>(fileDesc, methodOffset))) {
        return false;
    }
    return abcIdMethodIdToIsAotCompile_.at(std::pair<CString, uint32_t>(fileDesc, methodOffset));
}


bool AotCompilerPreprocessor::FilterOption(const std::map<std::string, std::vector<std::string>> &optionMap,
    const std::string &recordName, const std::string &methodName) const
{
    if (optionMap.empty()) {
        return false;
    }

    auto it = optionMap.find(recordName);
    if (it == optionMap.end()) {
        return false;
    }

    std::vector<std::string> vec = it->second;
    return find(vec.begin(), vec.end(), methodName) != vec.end();
}

bool AotCompilerPreprocessor::IsSkipMethod(const JSPandaFile *jsPandaFile, const BCInfo &bytecodeInfo,
                                           const CString &recordName, const MethodLiteral *methodLiteral,
                                           const MethodPcInfo &methodPCInfo, const std::string &methodName,
                                           CompilationOptions &cOptions) const
{
    if (methodPCInfo.methodsSize > bytecodeInfo.GetMaxMethodSize() ||
        !profilerDecoder_.Match(jsPandaFile, recordName, methodLiteral->GetMethodId())) {
        return true;
    }

    if (!cOptions.optionSelectMethods_.empty()) {
        return !FilterOption(cOptions.optionSelectMethods_, ConvertToStdString(recordName), methodName);
    } else if (!cOptions.optionSkipMethods_.empty()) {
        return FilterOption(cOptions.optionSkipMethods_, ConvertToStdString(recordName), methodName);
    }

    return false;
}

void AotCompilerPreprocessor::GenerateMethodMap(CompilationOptions &cOptions)
{
    JSPandaFileManager *jsPandaFileManager = JSPandaFileManager::GetInstance();
    jsPandaFileManager->EnumerateNonVirtualJSPandaFiles(
        [this, &cOptions] (std::shared_ptr<JSPandaFile> jsPandaFilePtr) {
        JSPandaFile *jsPandaFile = jsPandaFilePtr.get();
        BytecodeInfoCollector collector(vm_, jsPandaFile, profilerDecoder_, cOptions.maxAotMethodSize_,
                                        cOptions.isEnableCollectLiteralInfo_);
        BCInfo &bytecodeInfo = collector.GetBytecodeInfo();
        const auto &methodPcInfos = bytecodeInfo.GetMethodPcInfos();
        auto &methodList = bytecodeInfo.GetMethodList();
        for (auto it = methodList.begin(); it != methodList.end(); it++) {
            uint32_t index = it->first;
            auto &methodInfo = it->second;
            auto &methodPcInfo = methodPcInfos[methodInfo.GetMethodPcInfoIndex()];
            auto methodLiteral = jsPandaFile->FindMethodLiteral(index);
            auto methodId = methodLiteral->GetMethodId();
            const std::string methodName(MethodLiteral::GetMethodName(jsPandaFile, methodId));
            bool isAotcompile = !IsSkipMethod(jsPandaFile, bytecodeInfo, MethodLiteral::GetRecordName(
                jsPandaFile, EntityId(index)), methodLiteral, methodPcInfo, methodName, cOptions);
            bool isFastCall = methodLiteral->IsFastCall();
            CString fileDesc = jsPandaFile->GetNormalizedFileDesc();
            uint32_t offset = methodId.GetOffset();
            callMethodFlagMap_.SetIsAotCompile(fileDesc, offset, isAotcompile);
            callMethodFlagMap_.SetIsFastCall(fileDesc, offset, isFastCall);
            LOG_COMPILER(INFO) <<"!!!"<< fileDesc <<" "<< offset << " " << isAotcompile << " " << isFastCall;
        }
        return true;
    });
}

std::string AotCompilerPreprocessor::GetMainPkgArgsAppSignature() const
{
    return GetMainPkgArgs() == nullptr ? "" : GetMainPkgArgs()->GetAppSignature();
}
} // namespace panda::ecmascript::kungfu
