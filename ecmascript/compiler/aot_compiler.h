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
#ifndef ECMASCRIPT_COMPILER_AOT_COMPILER_H
#define ECMASCRIPT_COMPILER_AOT_COMPILER_H

#include "ecmascript/ohos/ohos_pkg_args.h"
#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/ecma_vm.h"

namespace panda::ecmascript::kungfu {
namespace {
using PGOProfilerDecoder = pgo::PGOProfilerDecoder;

struct AbcFileInfo {
    explicit AbcFileInfo(std::string extendedFilePath, std::shared_ptr<JSPandaFile> jsPandaFile)
        : extendedFilePath_(extendedFilePath), jsPandaFile_(jsPandaFile) {}
    ~AbcFileInfo() = default;

    std::string extendedFilePath_;
    std::shared_ptr<JSPandaFile> jsPandaFile_;
};

struct CompilationOptions {
    explicit CompilationOptions(EcmaVM *vm, JSRuntimeOptions &runtimeOptions);

    std::string triple_;
    std::string outputFileName_;
    size_t optLevel_;
    size_t relocMode_;
    std::string logOption_;
    std::string logMethodsList_;
    bool compilerLogTime_;
    size_t maxAotMethodSize_;
    size_t maxMethodsInModule_;
    uint32_t hotnessThreshold_;
    std::string profilerIn_;
    bool isEnableArrayBoundsCheckElimination_;
    bool isEnableTypeLowering_;
    bool isEnableEarlyElimination_;
    bool isEnableLaterElimination_;
    bool isEnableValueNumbering_;
    bool isEnableOptInlining_;
    bool isEnableTypeInfer_;
    bool isEnableOptPGOType_;
    bool isEnableOptTrackField_;
    bool isEnableOptLoopPeeling_;
    bool isEnableOptOnHeapCheck_;
    bool isEnableOptLoopInvariantCodeMotion_;
    bool isEnableCollectLiteralInfo_;
    bool isEnableOptConstantFolding_;
};

class CompilationPreprocessor {
public:
    CompilationPreprocessor(EcmaVM *vm, JSRuntimeOptions &runtimeOptions, std::map<std::string, OhosPkgArgs> &pkgsArgs,
                            PGOProfilerDecoder &profilerDecoder, arg_list_t &pandaFileNames)
        : vm_(vm), runtimeOptions_(runtimeOptions), pkgsArgs_(pkgsArgs),
          profilerDecoder_(profilerDecoder), pandaFileNames_(pandaFileNames) {};
    ~CompilationPreprocessor() = default;

    bool HandleTargetCompilerMode(CompilationOptions &cOptions);

    bool HandlePandaFileNames(const int argc, const char **argv);

    void AOTInitialize();

    void SetShouldCollectLiteralInfo(CompilationOptions &cOptions, const CompilerLog *log);

    void GenerateAbcFileInfos();

    void GenerateGlobalTypes(const CompilationOptions &cOptions);

    void SnapshotInitialize();

    bool GetCompilerResult()
    {
        // The size of fileInfos is not equal to pandaFiles size, set compiler result to false
        return fileInfos_.size() == pandaFileNames_.size();
    }

    const CVector<AbcFileInfo>& GetAbcFileInfo() const
    {
        return fileInfos_;
    }

private:
    bool HandleOhosPkgArgs();

    void HandleTargetModeInfo(CompilationOptions &cOptions);

    std::shared_ptr<JSPandaFile> CreateAndVerifyJSPandaFile(const std::string &fileName);

    void ResolveModule(const JSPandaFile *jsPandaFile, const std::string &fileName);

    EcmaVM *vm_;
    JSRuntimeOptions &runtimeOptions_;
    std::map<std::string, OhosPkgArgs> &pkgsArgs_;
    PGOProfilerDecoder &profilerDecoder_;
    arg_list_t &pandaFileNames_;
    CVector<AbcFileInfo> fileInfos_;
};
}
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_AOT_COMPILER_H
