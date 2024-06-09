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
    explicit CompilationOptions(JSRuntimeOptions &runtimeOptions);
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
    std::string profilerIn_;
    std::string optBCRange_;
    bool needMerge_ {false};
    bool isEnableArrayBoundsCheckElimination_;
    bool isEnableTypeLowering_;
    bool isEnableEarlyElimination_;
    bool isEnableLaterElimination_;
    bool isEnableValueNumbering_;
    bool isEnableOptInlining_;
    bool isEnableOptString_;
    bool isEnableOptPGOType_;
    bool isEnableOptTrackField_;
    bool isEnableOptLoopPeeling_;
    bool isEnableOptLoopInvariantCodeMotion_;
    bool isEnableOptConstantFolding_;
    bool isEnableLexenvSpecialization_;
    bool isEnableNativeInline_;
    bool isEnablePGOHCRLowering_ {false};
    bool isEnableLoweringBuiltin_;
    bool isEnableOptBranchProfiling_;
    bool isEnableEscapeAnalysis_;
    bool isEnableInductionVariableAnalysis_;
    bool isEnableVerifierPass_;
    std::map<std::string, std::vector<std::string>> optionSelectMethods_;
    std::map<std::string, std::vector<std::string>> optionSkipMethods_;
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

    bool HandleTargetCompilerMode(CompilationOptions &cOptions);

    bool HandlePandaFileNames(const int argc, const char **argv);

    void AOTInitialize();

    uint32_t GenerateAbcFileInfos();

    bool HandleMergedPgoFile(uint32_t checksum);

    void GeneratePGOTypes(const CompilationOptions &cOptions);

    void SnapshotInitialize();

    bool FilterOption(const std::map<std::string, std::vector<std::string>> &optionMap,
                      const std::string &recordName, const std::string &methodName) const;

    bool IsSkipMethod(const JSPandaFile *jsPandaFile, const BCInfo &bytecodeInfo,
                      const CString &recordName, const MethodLiteral *methodLiteral,
                      const MethodPcInfo &methodPCInfo, const std::string &methodName,
                      CompilationOptions &cOptions) const;

    void GenerateMethodMap(CompilationOptions &cOptions);

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

    std::string GetMainPkgArgsAppSignature() const;

    bool GetCompilerResult()
    {
        // The size of fileInfos is not equal to pandaFiles size, set compiler result to false
        return fileInfos_.size() == pandaFileNames_.size();
    }

    const CVector<AbcFileInfo>& GetAbcFileInfo() const
    {
        return fileInfos_;
    }

    std::shared_ptr<OhosPkgArgs> GetMainPkgArgs() const
    {
        if (pkgsArgs_.empty()) {
            return nullptr;
        }
        return pkgsArgs_.at(mainPkgName_);
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

    void RecordArrayElement(const CompilationOptions &cOptions);

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
    CallMethodFlagMap callMethodFlagMap_;
    AOTCompilationEnv aotCompilationEnv_;
    friend class OhosPkgArgs;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_AOT_COMPILER_PREPROCESSOR_H
