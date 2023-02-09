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

#ifndef ECMASCRIPT_COMPILER_PASS_MANAGER_H
#define ECMASCRIPT_COMPILER_PASS_MANAGER_H

#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/compiler/file_generators.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/pgo_profiler/pgo_profiler_loader.h"

namespace panda::ecmascript::kungfu {
class Bytecodes;
class LexEnvManager;
class CompilationConfig;
class BytecodeInfoCollector;

class PassInfo {
public:
    explicit PassInfo(TSManager *tsManager, Bytecodes *bytecodes, LexEnvManager *lexEnvManager,
                             CompilationConfig *cmpCfg, CompilerLog *log, const JSPandaFile *jsPandaFile,
                             BytecodeInfoCollector* bcInfoCollector, LLVMModule *aotModule)
        : tsManager_(tsManager), bytecodes_(bytecodes), lexEnvManager_(lexEnvManager), cmpCfg_(cmpCfg),
          log_(log), jsPandaFile_(jsPandaFile), bcInfoCollector_(bcInfoCollector), aotModule_(aotModule)
    {
    }

    TSManager* GetTSManager() const
    {
        return tsManager_;
    }

    Bytecodes* GetByteCodes() const
    {
        return bytecodes_;
    }

    LexEnvManager* GetLexEnvManager() const
    {
        return lexEnvManager_;
    }

    CompilationConfig* GetCompilerConfig() const
    {
        return cmpCfg_;
    }

    CompilerLog* GetCompilerLog() const
    {
        return log_;
    }

    const JSPandaFile* GetJSPandaFile() const
    {
        return jsPandaFile_;
    }

    BytecodeInfoCollector* GetBytecodeInfoCollector() const
    {
        return bcInfoCollector_;
    }

    LLVMModule* GetAOTModule() const
    {
        return aotModule_;
    }

    bool IsSkippedMethod(uint32_t methodOffset) const
    {
        return bcInfoCollector_->IsSkippedMethod(methodOffset);
    }

    BCInfo& GetBytecodeInfo()
    {
        return bcInfoCollector_->GetBytecodeInfo();
    }

private:
    TSManager *tsManager_ {nullptr};
    Bytecodes *bytecodes_ {nullptr};
    LexEnvManager *lexEnvManager_ {nullptr};
    CompilationConfig *cmpCfg_ {nullptr};
    CompilerLog *log_ {nullptr};
    const JSPandaFile *jsPandaFile_ {nullptr};
    BytecodeInfoCollector *bcInfoCollector_ {nullptr};
    LLVMModule *aotModule_ {nullptr};
};

class PassManager {
public:
    PassManager(EcmaVM* vm, std::string entry, std::string &triple, size_t optLevel, size_t relocMode,
                CompilerLog *log, AotMethodLogList *logList, size_t maxAotMethodSize, bool enableTypeLowering,
                const std::string &profIn, uint32_t hotnessThreshold)
        : vm_(vm), entry_(entry), triple_(triple), optLevel_(optLevel), relocMode_(relocMode), log_(log),
          logList_(logList), maxAotMethodSize_(maxAotMethodSize), enableTypeLowering_(enableTypeLowering),
          enableTypeInfer_(enableTypeLowering || vm_->GetTSManager()->AssertTypes()),
          profilerLoader_(profIn, hotnessThreshold) {};
    PassManager() = default;
    ~PassManager() = default;

    bool Compile(const std::string &fileName, AOTFileGenerator &generator);

private:
    JSPandaFile *CreateAndVerifyJSPandaFile(const CString &fileName);
    bool IsReleasedPandaFile(const JSPandaFile *jsPandaFile) const;
    void ResolveModule(const JSPandaFile *jsPandaFile, const std::string &fileName);

    bool EnableTypeLowering() const
    {
        return enableTypeLowering_;
    }

    bool EnableTypeInfer() const
    {
        return enableTypeInfer_;
    }

    EcmaVM *vm_ {nullptr};
    std::string entry_ {};
    std::string triple_ {};
    size_t optLevel_ {3}; // 3 : default backend optimization level
    size_t relocMode_ {2}; // 2 : default relocation mode-- PIC
    CompilerLog *log_ {nullptr};
    AotMethodLogList *logList_ {nullptr};
    size_t maxAotMethodSize_ {0};
    bool enableTypeLowering_ {true};
    bool enableTypeInfer_ {true};
    PGOProfilerLoader profilerLoader_;
};
}
#endif // ECMASCRIPT_COMPILER_PASS_MANAGER_H
