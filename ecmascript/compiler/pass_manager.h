/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/aot_compiler_preprocessor.h"
#include "ecmascript/compiler/jit_compilation_env.h"
#include "ecmascript/compiler/bytecode_info_collector.h"
#include "ecmascript/compiler/compilation_driver.h"
#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/compiler/file_generators.h"
#include "ecmascript/compiler/ir_module.h"
#include "ecmascript/compiler/ir_module.h"
#include "ecmascript/compiler/pass_options.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/jit/jit_profiler.h"
#include "ecmascript/jspandafile/method_literal.h"

#include "ecmascript/compiler/aot_compiler_stats.h"

namespace panda::ecmascript::kungfu {
class Bytecodes;
class CompilationConfig;
class PassData;
class CallMethodFlagMap;
struct AbcFileInfo;
class PassContext {
public:
    PassContext(const std::string &triple, CompilerLog *log, BytecodeInfoCollector* collector, IRModule *aotModule,
        PGOProfilerDecoder *decoder);

    PGOTypeManager* GetPTManager() const
    {
        return compilationEnv_->GetPTManager();
    }

    Bytecodes* GetByteCodes()
    {
        return bytecodes_;
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
        return compilationEnv_->GetNativeAreaAllocator();
    }

    CompilationEnv *GetCompilationEnv() const
    {
        return compilationEnv_;
    }

    PGOProfilerDecoder *GetPfDecoder() const
    {
        return decoder_;
    }

private:
    CompilationEnv *compilationEnv_ {nullptr};
    BytecodeInfoCollector *bcInfoCollector_ {nullptr};
    Bytecodes *bytecodes_ {nullptr};
    CompilationConfig cmpCfg_;
    CompilerLog *log_ {nullptr};
    const JSPandaFile *jsPandaFile_ {nullptr};
    IRModule *aotModule_ {nullptr};
    PGOProfilerDecoder *decoder_ {nullptr};
};

class PassManager {
public:
    explicit PassManager(CompilationEnv *env, std::string &triple, size_t optLevel, size_t relocMode,
        CompilerLog *log, AotMethodLogList *logList, size_t maxAotMethodSize, size_t maxMethodsInModule,
        PGOProfilerDecoder &profilerDecoder, PassOptions *passOptions, CallMethodFlagMap *callMethodFlagMap,
        const CVector<AbcFileInfo> &fileInfos, const CVector<std::unique_ptr<BytecodeInfoCollector>> &bcInfoCollectors,
        std::string optBCRange)
        : compilationEnv_(env), triple_(triple), optLevel_(optLevel), relocMode_(relocMode), log_(log),
          logList_(logList), maxAotMethodSize_(maxAotMethodSize), maxMethodsInModule_(maxMethodsInModule),
          profilerDecoder_(profilerDecoder), passOptions_(passOptions), callMethodFlagMap_(callMethodFlagMap),
          fileInfos_(fileInfos), bcInfoCollectors_(bcInfoCollectors), optBCRange_(optBCRange) {
                enableJITLog_ = compilationEnv_->GetJSOptions().GetTraceJIT();
            };

    virtual ~PassManager() = default;

    void CompileValidFiles(AOTFileGenerator &generator, bool &ret, AotCompilerStats &compilerStats);

    bool Compile(JSPandaFile *jsPandaFile, const std::string &fileName, AOTFileGenerator &generator,
                 AotCompilerStats &compilerStats, BytecodeInfoCollector &collector);

protected:
    bool IsReleasedPandaFile(const JSPandaFile *jsPandaFile) const;

    CompilationEnv *compilationEnv_ {nullptr};
    std::string triple_ {};
    size_t optLevel_ {3}; // 3 : default backend optimization level
    size_t relocMode_ {2}; // 2 : default relocation mode-- PIC
    CompilerLog *log_ {nullptr};
    AotMethodLogList *logList_ {nullptr};
    size_t maxAotMethodSize_ {0};
    size_t maxMethodsInModule_ {0};
    PGOProfilerDecoder &profilerDecoder_;
    PassOptions *passOptions_ {nullptr};
    CallMethodFlagMap *callMethodFlagMap_ {nullptr};
    const CVector<AbcFileInfo> &fileInfos_;
    const CVector<std::unique_ptr<BytecodeInfoCollector>> &bcInfoCollectors_;
    std::string optBCRange_ {};
    bool enableJITLog_ {false};
};

class JitPassManager : public PassManager {
public:
    JitPassManager(JitCompilationEnv *env, std::string &triple, size_t optLevel, size_t relocMode,
        CompilerLog *log, AotMethodLogList *logList,
        PGOProfilerDecoder &profilerDecoder, PassOptions *passOptions)
        : PassManager(env, triple, optLevel, relocMode, log, logList, 1, 1, profilerDecoder, passOptions,
                      nullptr, CVector<AbcFileInfo> {}, CVector<std::unique_ptr<BytecodeInfoCollector>> {}, "") { };

    bool Compile(JSHandle<ProfileTypeInfo> &profileTypeInfo, AOTFileGenerator &gen, int32_t osrOffset = -1);
    bool RunCg();
    virtual ~JitPassManager();

private:
    BytecodeInfoCollector *collector_ {nullptr};
    LOptions *lOptions_ {nullptr};
    JitCompilationDriver *cmpDriver_ {nullptr};

    PassContext *ctx_ {nullptr};
    Circuit *circuit_ {nullptr};
    BytecodeCircuitBuilder *builder_ {nullptr};
    JITProfiler *jitProfiler_ {nullptr};
    PassData *data_ {nullptr};
};
}
#endif // ECMASCRIPT_COMPILER_PASS_MANAGER_H
