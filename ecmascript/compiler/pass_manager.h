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
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/ts_types/ts_manager.h"

namespace panda::ecmascript::kungfu {
class Bytecodes;
class LexEnvManager;
class CompilationConfig;

class PassContext {
public:
    PassContext(const std::string &triple, CompilerLog *log, BytecodeInfoCollector* collector, LLVMModule *aotModule,
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
    LLVMModule *aotModule_ {nullptr};
    PGOProfilerDecoder *decoder_ {nullptr};
};

class PassOptions {
public:
    PassOptions(bool enableArrayBoundsCheckElimination, bool enableTypeLowering, bool enableEarlyElimination,
                bool enableLaterElimination, bool enableValueNumbering, bool enableTypeInfer,
                bool enableOptInlining, bool enableOptPGOType, bool enableOptTrackField, bool enableOptLoopPeeling,
                bool enableOptOnHeapCheck)
        : enableArrayBoundsCheckElimination_(enableArrayBoundsCheckElimination),
          enableTypeLowering_(enableTypeLowering),
          enableEarlyElimination_(enableEarlyElimination),
          enableLaterElimination_(enableLaterElimination),
          enableValueNumbering_(enableValueNumbering),
          enableTypeInfer_(enableTypeInfer),
          enableOptInlining_(enableOptInlining),
          enableOptPGOType_(enableOptPGOType),
          enableOptTrackField_(enableOptTrackField),
          enableOptLoopPeeling_(enableOptLoopPeeling),
          enableOptOnHeapCheck_(enableOptOnHeapCheck)
        {
        }

#define OPTION_LIST(V)           \
    V(ArrayBoundsCheckElimination, true) \
    V(TypeLowering, true)        \
    V(EarlyElimination, true)    \
    V(LaterElimination, true)    \
    V(ValueNumbering, false)     \
    V(TypeInfer, false)          \
    V(OptInlining, false)        \
    V(OptNoGCCall, false)        \
    V(OptPGOType, false)         \
    V(NoCheck, false)            \
    V(OptTrackField, false)      \
    V(OptLoopPeeling, false)     \
    V(DeadCodeElimination, false) \
    V(OptOnHeapCheck, false)

#define DECL_OPTION(NAME, DEFAULT)    \
public:                               \
    bool Enable##NAME() const         \
    {                                 \
        return enable##NAME##_;       \
    }                                 \
                                      \
private:                              \
    bool enable##NAME##_ {DEFAULT};

    OPTION_LIST(DECL_OPTION)
#undef ENABLE_OPTION
#undef OPTION_LIST
};

class PassManager {
public:
    PassManager(EcmaVM* vm, std::string &entry, std::string &triple, size_t optLevel, size_t relocMode,
                CompilerLog *log, AotMethodLogList *logList, size_t maxAotMethodSize, size_t maxMethodsInModule,
                const std::string &profIn, uint32_t hotnessThreshold, PassOptions *passOptions)
        : vm_(vm), entry_(entry), triple_(triple), optLevel_(optLevel), relocMode_(relocMode), log_(log),
          logList_(logList), maxAotMethodSize_(maxAotMethodSize), maxMethodsInModule_(maxMethodsInModule),
          profilerDecoder_(profIn, hotnessThreshold), passOptions_(passOptions) {};
    PassManager() = default;
    ~PassManager() = default;

    bool Compile(JSPandaFile *jsPandaFile, const std::string &fileName, AOTFileGenerator &generator);

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
    size_t maxMethodsInModule_ {0};
    PGOProfilerDecoder profilerDecoder_;
    PassOptions *passOptions_ {nullptr};
};
}
#endif // ECMASCRIPT_COMPILER_PASS_MANAGER_H
