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
#include "ecmascript/compiler/ir_module.h"
#include "ecmascript/compiler/pass_options.h"
#include "ecmascript/compiler/ir_module.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/ts_types/ts_manager.h"

namespace panda::ecmascript::kungfu {
class Bytecodes;
class LexEnvManager;
class CompilationConfig;
class PassData;
class PassContext {
public:
    PassContext(const std::string &triple, CompilerLog *log, BytecodeInfoCollector* collector, IRModule *aotModule,
        PGOProfilerDecoder *decoder)
        : vm_(collector->GetVM()),
          bcInfoCollector_(collector),
          tsManager_(vm_->GetJSThread()->GetCurrentEcmaContext()->GetTSManager()),
          bytecodes_(collector->GetByteCodes()),
          lexEnvManager_(bcInfoCollector_->GetEnvManager()),
          cmpCfg_(triple, &vm_->GetJSOptions()),
          log_(log),
          jsPandaFile_(collector->GetJSPandaFile()),
          aotModule_(aotModule),
          decoder_(decoder)
    {
    }

    TSManager* GetTSManager() const
    {
        return tsManager_;
    }

    PGOTypeManager* GetPTManager() const
    {
        return vm_->GetJSThread()->GetCurrentEcmaContext()->GetPTManager();
    }

    Bytecodes* GetByteCodes()
    {
        return bytecodes_;
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

    IRModule* GetAOTModule() const
    {
        return aotModule_;
    }

    bool FilterMethod(const MethodLiteral *methodLiteral, const MethodPcInfo &methodPCInfo) const
    {
        return bcInfoCollector_->FilterMethod(methodLiteral, methodPCInfo);
    }

    bool IsSkippedMethod(uint32_t methodOffset) const
    {
        return bcInfoCollector_->IsSkippedMethod(methodOffset);
    }

    BCInfo& GetBytecodeInfo()
    {
        return bcInfoCollector_->GetBytecodeInfo();
    }

    NativeAreaAllocator *GetNativeAreaAllocator() const
    {
        return vm_->GetNativeAreaAllocator();
    }

    EcmaVM *GetEcmaVM() const
    {
        return vm_;
    }

    PGOProfilerDecoder *GetPfDecoder() const
    {
        return decoder_;
    }

private:
    EcmaVM *vm_ {nullptr};
    BytecodeInfoCollector *bcInfoCollector_ {nullptr};
    TSManager *tsManager_ {nullptr};
    Bytecodes *bytecodes_ {nullptr};
    LexEnvManager *lexEnvManager_ {nullptr};
    CompilationConfig cmpCfg_;
    CompilerLog *log_ {nullptr};
    const JSPandaFile *jsPandaFile_ {nullptr};
    IRModule *aotModule_ {nullptr};
    PGOProfilerDecoder *decoder_ {nullptr};
};

class PassManager {
public:
    explicit PassManager(EcmaVM* vm, std::string &triple, size_t optLevel, size_t relocMode,
        CompilerLog *log, AotMethodLogList *logList, size_t maxAotMethodSize, size_t maxMethodsInModule,
        PGOProfilerDecoder &profilerDecoder, PassOptions *passOptions, std::string optBCRange)
        : vm_(vm), triple_(triple), optLevel_(optLevel), relocMode_(relocMode), log_(log),
          logList_(logList), maxAotMethodSize_(maxAotMethodSize), maxMethodsInModule_(maxMethodsInModule),
          profilerDecoder_(profilerDecoder), passOptions_(passOptions), optBCRange_(optBCRange) {
                enableJITLog_ =  vm_->GetJSOptions().GetTraceJIT();
            };

    virtual ~PassManager() = default;
    bool Compile(JSPandaFile *jsPandaFile, const std::string &fileName, AOTFileGenerator &generator);

protected:
    bool IsReleasedPandaFile(const JSPandaFile *jsPandaFile) const;

    EcmaVM *vm_ {nullptr};
    std::string triple_ {};
    size_t optLevel_ {3}; // 3 : default backend optimization level
    size_t relocMode_ {2}; // 2 : default relocation mode-- PIC
    CompilerLog *log_ {nullptr};
    AotMethodLogList *logList_ {nullptr};
    size_t maxAotMethodSize_ {0};
    size_t maxMethodsInModule_ {0};
    PGOProfilerDecoder &profilerDecoder_;
    PassOptions *passOptions_ {nullptr};
    std::string optBCRange_ {};
    bool enableJITLog_ {false};
};

class JitPassManager : public PassManager {
public:
    JitPassManager(EcmaVM* vm, std::string &triple, size_t optLevel, size_t relocMode,
        CompilerLog *log, AotMethodLogList *logList,
        PGOProfilerDecoder &profilerDecoder, PassOptions *passOptions)
        : PassManager(vm, triple, optLevel, relocMode, log, logList, 1, 1, profilerDecoder, passOptions, "") { };

    bool Compile(JSHandle<JSFunction> &jsFunction, AOTFileGenerator &gen);
    bool RunCg();
    virtual ~JitPassManager();

private:
    BytecodeInfoCollector *collector_ {nullptr};
    LOptions *lOptions_ {nullptr};
    JitCompilationDriver *cmpDriver_ {nullptr};

    PassContext *ctx_ {nullptr};
    Circuit *circuit_ {nullptr};
    BytecodeCircuitBuilder *builder_ {nullptr};
    PassData *data_ {nullptr};
};
}
#endif // ECMASCRIPT_COMPILER_PASS_MANAGER_H
