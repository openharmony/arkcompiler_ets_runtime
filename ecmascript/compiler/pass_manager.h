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

#include "ecmascript/compiler/bytecode_info_collector.h"
#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/compiler/file_generators.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/pgo_profiler/pgo_profiler_loader.h"
#include "ecmascript/ts_types/ts_manager.h"

namespace panda::ecmascript::kungfu {
class Bytecodes;
class LexEnvManager;
class CompilationConfig;

class PassContext {
public:
    PassContext(const std::string &triple, CompilerLog *log, BytecodeInfoCollector* collector, LLVMModule *aotModule)
        : vm_(collector->GetVM()),
          bcInfoCollector_(collector),
          tsManager_(vm_->GetTSManager()),
          lexEnvManager_(bcInfoCollector_->GetEnvManager()),
          cmpCfg_(triple, log->IsTraceBC(), vm_->GetJSOptions().GetOptCodeProfiler()),
          log_(log),
          jsPandaFile_(collector->GetJSPandaFile()),
          aotModule_(aotModule)
    {
    }

    TSManager* GetTSManager() const
    {
        return tsManager_;
    }

    Bytecodes* GetByteCodes()
    {
        return &bytecodes_;
    }

    LexEnvManager* GetLexEnvManager() const
    {
        return lexEnvManager_;
    }

    CompilationConfig* GetCompilerConfig()
    {
        return &cmpCfg_;
    }

    CompilerLog* GetCompilerLog() const
    {
        return log_;
    }

    const JSPandaFile *GetJSPandaFile() const
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
    EcmaVM *vm_ {nullptr};
    BytecodeInfoCollector *bcInfoCollector_ {nullptr};
    TSManager *tsManager_ {nullptr};
    Bytecodes bytecodes_;
    LexEnvManager *lexEnvManager_ {nullptr};
    CompilationConfig cmpCfg_;
    CompilerLog *log_ {nullptr};
    const JSPandaFile *jsPandaFile_ {nullptr};
    LLVMModule *aotModule_ {nullptr};
};

class PassOptions {
public:
    PassOptions(bool enableTypeLowering, bool enableTypeInfer, bool enableOptInlining, bool enableOptPGOType)
        : enableTypeLowering_(enableTypeLowering),
          enableTypeInfer_(enableTypeInfer),
          enableOptInlining_(enableOptInlining),
          enableOptPGOType_(enableOptPGOType)
        {
        }

    bool EnableTypeLowering() const
    {
        return enableTypeLowering_;
    }

    bool EnableTypeInfer() const
    {
        return enableTypeInfer_;
    }

    bool EnableOptInlining() const
    {
        return enableOptInlining_;
    }

    bool EnableOptPGOType() const
    {
        return enableOptPGOType_;
    }
private:
    bool enableTypeLowering_ {false};
    bool enableTypeInfer_ {false};
    bool enableOptInlining_ {false};
    bool enableOptPGOType_ {false};
};

class PassManager {
public:
    PassManager(EcmaVM* vm, std::string entry, std::string &triple, size_t optLevel, size_t relocMode,
                CompilerLog *log, AotMethodLogList *logList, size_t maxAotMethodSize, const std::string &profIn,
                uint32_t hotnessThreshold, PassOptions *passOptions)
        : vm_(vm), entry_(entry), triple_(triple), optLevel_(optLevel), relocMode_(relocMode), log_(log),
          logList_(logList), maxAotMethodSize_(maxAotMethodSize),
          profilerLoader_(profIn, hotnessThreshold), passOptions_(passOptions) {};
    PassManager() = default;
    ~PassManager() = default;

    bool Compile(const std::string &fileName, AOTFileGenerator &generator);

private:
    JSPandaFile *CreateAndVerifyJSPandaFile(const CString &fileName);
    void ProcessConstantPool(BytecodeInfoCollector *collector);
    bool IsReleasedPandaFile(const JSPandaFile *jsPandaFile) const;
    void ResolveModule(const JSPandaFile *jsPandaFile, const std::string &fileName);
    bool ShouldCollect() const;

    EcmaVM *vm_ {nullptr};
    std::string entry_ {};
    std::string triple_ {};
    size_t optLevel_ {3}; // 3 : default backend optimization level
    size_t relocMode_ {2}; // 2 : default relocation mode-- PIC
    CompilerLog *log_ {nullptr};
    AotMethodLogList *logList_ {nullptr};
    size_t maxAotMethodSize_ {0};
    PGOProfilerLoader profilerLoader_;
    PassOptions *passOptions_ {nullptr};
};
}
#endif // ECMASCRIPT_COMPILER_PASS_MANAGER_H
