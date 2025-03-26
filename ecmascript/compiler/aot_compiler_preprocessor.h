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
#ifndef ECMASCRIPT_COMPILER_AOT_COMPILER_PREPROCESSOR_H
#define ECMASCRIPT_COMPILER_AOT_COMPILER_PREPROCESSOR_H

#include "ecmascript/compiler/bytecode_info_collector.h"
#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"
#include "ecmascript/ecma_vm.h"
#include "macros.h"
#include "ecmascript/compiler/aot_compilation_env.h"
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/ohos/ohos_preload_app_info.h"

namespace panda::ecmascript::kungfu {
class OhosPkgArgs;
using PGOProfilerDecoder = pgo::PGOProfilerDecoder;

struct AbcFileInfo {
    explicit AbcFileInfo(std::string extendedFilePath, std::shared_ptr<JSPandaFile> jsPandaFile)
        : extendedFilePath_(extendedFilePath), jsPandaFile_(jsPandaFile) {}
    ~AbcFileInfo() = default;

    std::string extendedFilePath_;
    std::shared_ptr<JSPandaFile> jsPandaFile_;
};

class CallMethodFlagMap {
public:
    CallMethodFlagMap() {}
    void SetIsFastCall(CString fileDesc, uint32_t methodOffset, bool isFastCall);
    bool IsFastCall(CString fileDesc, uint32_t methodOffset) const;
    void SetIsAotCompile(CString fileDesc, uint32_t methodOffset, bool isAotCompile);
    bool IsAotCompile(CString fileDesc, uint32_t methodOffset) const;
    void SetIsJitCompile(CString fileDesc, uint32_t methodOffset, bool isAotCompile);
    bool IsJitCompile(CString fileDesc, uint32_t methodOffset) const;
private:
    std::map<std::pair<CString, uint32_t>, bool> abcIdMethodIdToIsFastCall_ {};
    std::map<std::pair<CString, uint32_t>, bool> abcIdMethodIdToIsAotCompile_ {};
    std::map<std::pair<CString, uint32_t>, bool> abcIdMethodIdToIsJitCompile_ {};
};

struct CompilationOptions {
    explicit PUBLIC_API CompilationOptions(JSRuntimeOptions &runtimeOptions);
    void ParseOption(const std::string &option, std::map<std::string, std::vector<std::string>> &optionMap) const;
    std::vector<std::string> SplitString(const std::string &str, const char ch) const;
    std::string triple_;
    std::string outputFileName_;
    size_t optLevel_;
    size_t relocMode_;
    std::string logOption_;
    std::string logMethodsList_;
    bool compilerLogTime_;
    bool deviceIsScreenOff_;
    size_t maxAotMethodSize_;
    size_t maxMethodsInModule_;
    uint32_t hotnessThreshold_;
    int32_t deviceThermalLevel_ {0};
    std::string profilerIn_;
    std::string optBCRange_;
    bool needMerge_ {false};
    bool isEnableArrayBoundsCheckElimination_ {true};
    bool isEnableTypeLowering_ {true};
    bool isEnableEarlyElimination_ {true};
    bool isEnableLaterElimination_ {true};
    bool isEnableValueNumbering_ {true};
    bool isEnableOptInlining_ {true};
    bool isEnableOptString_ {true};
    bool isEnableOptPGOType_ {true};
    bool isEnableOptTrackField_ {true};
    bool isEnableOptLoopPeeling_ {true};
    bool isEnableOptLoopInvariantCodeMotion_ {false};
    bool isEnableOptConstantFolding_ {true};
    bool isEnableLexenvSpecialization_ {false};
    bool isEnableNativeInline_ {true};
    bool isEnablePGOHCRLowering_ {false};
    bool isEnableLoweringBuiltin_ {true};
    bool isEnableOptBranchProfiling_ {true};
    bool isEnableEscapeAnalysis_ {false};
    bool isEnableInductionVariableAnalysis_ {false};
    bool isEnableVerifierPass_ {true};
    bool isEnableBaselinePgo_ {false};
    bool isEnableMergePoly_ {true};
    bool enableAotCodeComment_ {false};
    std::map<std::string, std::vector<std::string>> optionSelectMethods_;
    std::map<std::string, std::vector<std::string>> optionSkipMethods_;
    size_t anFileMaxByteSize_ {0_MB};
};

class AotCompilerPreprocessor {
public:
    AotCompilerPreprocessor(EcmaVM *vm, JSRuntimeOptions &runtimeOptions,
                            std::map<std::string, std::shared_ptr<OhosPkgArgs>> &pkgsArgs,
                            PGOProfilerDecoder &profilerDecoder, arg_list_t &pandaFileNames)
        : vm_(vm),
          runtimeOptions_(runtimeOptions),
          pkgsArgs_(pkgsArgs),
          profilerDecoder_(profilerDecoder),
          pandaFileNames_(pandaFileNames),
          aotCompilationEnv_(vm) {};

    ~AotCompilerPreprocessor() = default;

    bool PUBLIC_API HandleTargetCompilerMode(CompilationOptions &cOptions);

    bool PUBLIC_API HandlePandaFileNames(const int argc, const char **argv);

    void PUBLIC_API AOTInitialize();

    void DoPreAnalysis(CompilationOptions &cOptions);

    void AnalyzeGraphs(JSPandaFile *jsPandaFile, BytecodeInfoCollector &collector, CompilationOptions &cOptions);

    void AnalyzeGraph(BCInfo &bytecodeInfo, CompilationOptions &cOptions, BytecodeInfoCollector &collector,
                      MethodLiteral *methodLiteral, MethodPcInfo &methodPCInfo);

    void PUBLIC_API Process(CompilationOptions &cOptions);

    void PUBLIC_API GenerateAbcFileInfos(std::unordered_map<CString, uint32_t> &fileNameToChecksumMap);

    void GenerateBytecodeInfoCollectors(const CompilationOptions &cOptions);

    bool PUBLIC_API HandleMergedPgoFile(std::unordered_map<CString, uint32_t> &fileNameToChecksumMap);

    void GeneratePGOTypes();

    void SnapshotInitialize();

    bool FilterOption(const std::map<std::string, std::vector<std::string>> &optionMap,
                      const std::string &recordName, const std::string &methodName) const;

    bool IsSkipMethod(const JSPandaFile *jsPandaFile, const BCInfo &bytecodeInfo,
                      const CString &recordName, const MethodLiteral *methodLiteral,
                      const MethodPcInfo &methodPCInfo, const std::string &methodName,
                      CompilationOptions &cOptions) const;

    void GenerateMethodMap(CompilationOptions &cOptions);

    bool MethodHasTryCatch(const JSPandaFile *jsPandaFile, const MethodLiteral *methodLiteral) const;

    bool HasSkipMethod(const CVector<std::string> &methodList, const std::string &methodName) const;

    bool ForbidenRebuildAOT(std::string &fileName) const;

    bool PUBLIC_API HasPreloadAotFile() const;

    bool PUBLIC_API HasExistsAOTFiles(CompilationOptions &cOptions) const;

    void SetIsFastCall(CString fileDesc, uint32_t methodOffset, bool isFastCall)
    {
        callMethodFlagMap_.SetIsFastCall(fileDesc, methodOffset, isFastCall);
    }

    bool IsFastCall(CString fileDesc, uint32_t methodOffset)
    {
        return callMethodFlagMap_.IsFastCall(fileDesc, methodOffset);
    }

    void SetIsAotCompile(CString fileDesc, uint32_t methodOffset, bool isAotCompile)
    {
        callMethodFlagMap_.SetIsAotCompile(fileDesc, methodOffset, isAotCompile);
    }

    bool GetIsAotCompile(CString fileDesc, uint32_t methodOffset)
    {
        return callMethodFlagMap_.IsAotCompile(fileDesc, methodOffset);
    }

    std::string PUBLIC_API GetMainPkgArgsAppSignature() const;

    bool GetCompilerResult()
    {
        // The size of fileInfos is not equal to pandaFiles size, set compiler result to false
        return fileInfos_.size() == pandaFileNames_.size();
    }

    const CVector<AbcFileInfo>& GetAbcFileInfo() const
    {
        return fileInfos_;
    }

    const CVector<std::unique_ptr<BytecodeInfoCollector>>& GetBcInfoCollectors() const
    {
        return bcInfoCollectors_;
    }

    std::shared_ptr<OhosPkgArgs> GetMainPkgArgs() const
    {
        if (pkgsArgs_.empty()) {
            return nullptr;
        }
        return pkgsArgs_.at(mainPkgName_);
    }

    std::string GetMainPkgName() const
    {
        return mainPkgName_;
    }

    const std::map<std::string, std::shared_ptr<OhosPkgArgs>> &GetPkgsArgs() const
    {
        return pkgsArgs_;
    }
    CallMethodFlagMap *GetCallMethodFlagMap()
    {
        return &callMethodFlagMap_;
    }

    static std::string GetHelper()
    {
        std::string str;
        str.append(COMPILER_HELP_HEAD_MSG);
        str.append(HELP_OPTION_MSG);
        return str;
    }

private:
    NO_COPY_SEMANTIC(AotCompilerPreprocessor);
    NO_MOVE_SEMANTIC(AotCompilerPreprocessor);
    void HandleTargetModeInfo(CompilationOptions &cOptions);

    std::shared_ptr<JSPandaFile> CreateAndVerifyJSPandaFile(const std::string &fileName);

    void ResolveModule(const JSPandaFile *jsPandaFile, const std::string &fileName);

    bool OutCompiledMethodsRange() const
    {
        static uint32_t compiledMethodsCount = 0;
        ++compiledMethodsCount;
        return compiledMethodsCount < runtimeOptions_.GetCompilerMethodsRange().first ||
            runtimeOptions_.GetCompilerMethodsRange().second <= compiledMethodsCount;
    }

    EcmaVM *vm_;
    JSRuntimeOptions &runtimeOptions_;
    std::map<std::string, std::shared_ptr<OhosPkgArgs>> &pkgsArgs_;
    std::string mainPkgName_;
    PGOProfilerDecoder &profilerDecoder_;
    arg_list_t &pandaFileNames_;
    CVector<AbcFileInfo> fileInfos_;
    CVector<std::unique_ptr<BytecodeInfoCollector>> bcInfoCollectors_;
    CallMethodFlagMap callMethodFlagMap_;
    AOTCompilationEnv aotCompilationEnv_;
    CVector<std::string> irreducibleMethods_;
    friend class OhosPkgArgs;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_AOT_COMPILER_PREPROCESSOR_H
