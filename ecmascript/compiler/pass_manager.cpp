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

#include "ecmascript/compiler/pass_manager.h"
#include "ecmascript/compiler/pass.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/jit/jit.h"

namespace panda::ecmascript::kungfu {
using PGOProfilerManager = pgo::PGOProfilerManager;

PassContext::PassContext(const std::string &triple, CompilerLog *log, BytecodeInfoCollector* collector,
                         IRModule *aotModule, PGOProfilerDecoder *decoder)
    : compilationEnv_(collector->GetCompilationEnv()),
      bcInfoCollector_(collector),
      bytecodes_(collector->GetByteCodes()),
      cmpCfg_(triple, &compilationEnv_->GetJSOptions()),
      log_(log),
      jsPandaFile_(collector->GetJSPandaFile()),
      aotModule_(aotModule),
      decoder_(decoder)
{
}


bool JitPassManager::Compile(JSHandle<ProfileTypeInfo> &profileTypeInfo,
                             AOTFileGenerator &gen, int32_t osrOffset)
{
    const JSPandaFile *jsPandaFile = compilationEnv_->GetJSPandaFile();
    ASSERT(jsPandaFile != nullptr);
    MethodLiteral *methodLiteral = compilationEnv_->GetMethodLiteral();
    const uint8_t *pcStart = compilationEnv_->GetMethodPcStart();
    const panda_file::File::Header *header = jsPandaFile->GetPandaFile()->GetHeader();
    ApEntityId abcId = compilationEnv_->GetMethodAbcId();
    std::string fileName(jsPandaFile->GetJSPandaFileDesc());

    collector_ = new BytecodeInfoCollector(compilationEnv_, const_cast<JSPandaFile*>(jsPandaFile),
        profilerDecoder_);

    gen.SetCurrentCompileFileName(jsPandaFile->GetNormalizedFileDesc());
    lOptions_ = new LOptions(optLevel_, FPFlag::RESERVE_FP, relocMode_);
    cmpDriver_ = new JitCompilationDriver(profilerDecoder_,
                                          collector_,
                                          &gen,
                                          fileName,
                                          triple_,
                                          lOptions_,
                                          log_,
                                          log_->OutputASM(),
                                          maxMethodsInModule_);
    return cmpDriver_->CompileMethod(jsPandaFile, methodLiteral, profileTypeInfo, pcStart, header, abcId,
                                     [this, &fileName, &osrOffset] (
                                       const CString &recordName,
                                       const std::string &methodName,
                                       MethodLiteral *methodLiteral,
                                       JSHandle<ProfileTypeInfo> &profileTypeInfo,
                                       uint32_t methodOffset,
                                       const MethodPcInfo &methodPCInfo,
                                       MethodInfo &methodInfo,
                                       Module *m,
                                       const uint8_t *pcStart,
                                       const panda_file::File::Header *header,
                                       ApEntityId abcId) -> bool {
        if (compilationEnv_->GetJSOptions().GetTraceJIT()) {
            LOG_COMPILER(INFO) << "JIT Compile Method Start: " << methodName << ", " << methodOffset << "\n";
        }
        ctx_ = new PassContext(triple_, log_, collector_, m->GetModule(), &profilerDecoder_);

        auto jsPandaFile = ctx_->GetJSPandaFile();
        auto cmpCfg = ctx_->GetCompilerConfig();
        auto module = m->GetModule();
        log_->SetMethodLog(fileName, methodName, logList_);

        std::string fullName = module->GetFuncName(methodLiteral, jsPandaFile);
        bool enableMethodLog = log_->GetEnableMethodLog();
        if (enableMethodLog) {
            LOG_COMPILER(INFO) << "\033[34m" << "aot method [" << fullName
                               << "] recordName [" << recordName << "] log:" << "\033[0m";
        }
        Chunk chunk(compilationEnv_->GetNativeAreaAllocator());
        if (compilationEnv_->GetJSOptions().IsEnableJITPGO()) {
            jitProfiler_ = compilationEnv_->GetPGOProfiler()->GetJITProfile();
            static_cast<JitCompilationEnv*>(compilationEnv_)->SetProfileTypeInfo(profileTypeInfo);
            jitProfiler_->SetCompilationEnv(compilationEnv_);
            jitProfiler_->InitChunk(&chunk);
            jitProfiler_->ProfileBytecode(compilationEnv_->GetJSThread(), profileTypeInfo, nullptr,
                                          methodLiteral->GetMethodId(), abcId, pcStart,
                                          methodLiteral->GetCodeSize(jsPandaFile, methodLiteral->GetMethodId()),
                                          header, static_cast<JitCompilationEnv*>(compilationEnv_)->GetJsFunction());
        } else {
            jitProfiler_ = nullptr;
        }

        if (compilationEnv_->GetJSOptions().IsEnableJitFrame()) {
            circuit_ = new Circuit(compilationEnv_->GetNativeAreaAllocator(), ctx_->GetAOTModule()->GetDebugInfo(),
                fullName.c_str(), cmpCfg->Is64Bit(), FrameType::FASTJIT_FUNCTION_FRAME);
        } else {
            circuit_ = new Circuit(compilationEnv_->GetNativeAreaAllocator(), ctx_->GetAOTModule()->GetDebugInfo(),
                fullName.c_str(), cmpCfg->Is64Bit(), FrameType::OPTIMIZED_JS_FUNCTION_FRAME);
        }

        PGOProfilerDecoder defDecoder;
        PGOProfilerDecoder *decoder = passOptions_->EnableOptPGOType() ? &profilerDecoder_ : &defDecoder;

        builder_ = new BytecodeCircuitBuilder(jsPandaFile, methodLiteral, methodPCInfo,
            circuit_, ctx_->GetByteCodes(), enableMethodLog && log_->OutputCIR(),
            passOptions_->EnableTypeLowering(), fullName, recordName, decoder, false, jitProfiler_);
        builder_->SetOsrOffset(osrOffset);
        {
            TimeScope timeScope("BytecodeToCircuit", methodName, methodOffset, log_);
            builder_->SetJitCompile();
            builder_->BytecodeToCircuit();
            if (builder_->HasIrreducibleLoop()) {
                LOG_JIT(DEBUG) << "compile fail as has irreducible loop:" << methodName;
                return false;
            }
        }

        CallMethodFlagMap methodFlagMap;
        data_ = new PassData(builder_, circuit_, ctx_, log_, fullName, &methodInfo, recordName,
            methodLiteral, methodOffset, &methodFlagMap, CVector<AbcFileInfo> {},
            compilationEnv_->GetNativeAreaAllocator(), decoder, passOptions_);
        PassRunner<PassData> pipeline(data_);

        pipeline.RunPass<RunFlowCyclesVerifierPass>();
        pipeline.RunPass<RedundantPhiEliminationPass>();
        if (builder_->EnableLoopOptimization()) {
            pipeline.RunPass<LoopOptimizationPass>();
            pipeline.RunPass<RedundantPhiEliminationPass>();
        }
        if (passOptions_->EnableTypeLowering()) {
            pipeline.RunPass<PGOTypeInferPass>();
        }
        {
            Jit::JitLockHolder lock(compilationEnv_, "TSInlineLoweringPass");
            pipeline.RunPass<TSInlineLoweringPass>();
        }

        pipeline.RunPass<RedundantPhiEliminationPass>();
        pipeline.RunPass<AsyncFunctionLoweringPass>();
        pipeline.RunPass<TypeBytecodeLoweringPass>();
        pipeline.RunPass<UselessGateEliminationPass>();
        pipeline.RunPass<InductionVariableAnalysisPass>();
        pipeline.RunPass<RedundantPhiEliminationPass>();
        pipeline.RunPass<NTypeBytecodeLoweringPass>();
        pipeline.RunPass<UselessGateEliminationPass>();
        pipeline.RunPass<EarlyEliminationPass>();
        pipeline.RunPass<NumberSpeculativePass>();
        pipeline.RunPass<UselessGateEliminationPass>();
        pipeline.RunPass<LaterEliminationPass>();
        if (!compilationEnv_->GetJSOptions().IsEnableJitFastCompile()) {
            pipeline.RunPass<ValueNumberingPass>();
        }
        pipeline.RunPass<StateSplitLinearizerPass>();
        pipeline.RunPass<EscapeAnalysisPass>();
        pipeline.RunPass<StringOptimizationPass>();
        pipeline.RunPass<NTypeHCRLoweringPass>();
        pipeline.RunPass<TypeHCRLoweringPass>();
        pipeline.RunPass<UselessGateEliminationPass>();
        pipeline.RunPass<LaterEliminationPass>();
        pipeline.RunPass<EarlyEliminationPass>();
        pipeline.RunPass<LCRLoweringPass>();
        pipeline.RunPass<UselessGateEliminationPass>();
        pipeline.RunPass<ConstantFoldingPass>();
        if (!compilationEnv_->GetJSOptions().IsEnableJitFastCompile()) {
            pipeline.RunPass<ValueNumberingPass>();
        }
        pipeline.RunPass<SlowPathLoweringPass>();
        if (!compilationEnv_->GetJSOptions().IsEnableJitFastCompile()) {
            pipeline.RunPass<ValueNumberingPass>();
        }
        pipeline.RunPass<InstructionCombinePass>();
        pipeline.RunPass<EarlyEliminationPass>();
        pipeline.RunPass<UselessGateEliminationPass>();
        if (!compilationEnv_->GetJSOptions().IsEnableJitFastCompile()
            && compilationEnv_->GetJSOptions().IsEnableJitVerifyPass()) {
            pipeline.RunPass<VerifierPass>();
        }
        pipeline.RunPass<GraphLinearizerPass>();
        return true;
    });
}

bool JitPassManager::RunCg()
{
    PassRunner<PassData> pipeline(data_);
    pipeline.RunPass<CGIRGenPass>();
    return cmpDriver_->RunCg();
}

JitPassManager::~JitPassManager()
{
    if (data_ != nullptr) {
        delete data_;
        data_ = nullptr;
    }
    if (builder_ != nullptr) {
        delete builder_;
        builder_ = nullptr;
    }
    if (circuit_ != nullptr) {
        delete circuit_;
        circuit_ = nullptr;
    }
    if (ctx_ != nullptr) {
        delete ctx_;
        ctx_ = nullptr;
    }
    if (cmpDriver_ != nullptr) {
        delete cmpDriver_;
        cmpDriver_ = nullptr;
    }
    if (lOptions_ != nullptr) {
        delete lOptions_;
        lOptions_ = nullptr;
    }
    if (collector_ != nullptr) {
        delete collector_;
        collector_ = nullptr;
    }
}

void PassManager::CompileValidFiles(AOTFileGenerator &generator, bool &ret, AotCompilerStats &compilerStats)
{
    for (uint32_t i = 0 ; i < fileInfos_.size(); ++i) {
        JSPandaFile *jsPandaFile = fileInfos_[i].jsPandaFile_.get();
        auto &collector = *bcInfoCollectors_[i];
        const std::string &extendedFilePath = fileInfos_[i].extendedFilePath_;
        LOG_COMPILER(INFO) << "AOT compile: " << extendedFilePath;
        generator.SetCurrentCompileFileName(jsPandaFile->GetNormalizedFileDesc());
        if (!Compile(jsPandaFile, extendedFilePath, generator, compilerStats, collector)) {
            ret = false;
            continue;
        }
    }
}

bool PassManager::Compile(JSPandaFile *jsPandaFile, const std::string &fileName, AOTFileGenerator &gen,
                          AotCompilerStats &compilerStats, BytecodeInfoCollector &collector)
{
    [[maybe_unused]] EcmaHandleScope handleScope(compilationEnv_->GetJSThread());

    // Checking released/debuggable pandafile uses method literals, which are initialized in BytecodeInfoCollector,
    // should after it.
    if (!IsReleasedPandaFile(jsPandaFile)) {
        LOG_COMPILER(ERROR) << "The input panda file [" << fileName
                            << "] of AOT Compiler is debuggable version, do not use for performance test!";
    }
    LOptions lOptions(optLevel_, FPFlag::RESERVE_FP, relocMode_);
    CompilationDriver cmpDriver(profilerDecoder_,
                                &collector,
                                &gen,
                                fileName,
                                triple_,
                                &lOptions,
                                log_,
                                log_->OutputASM(),
                                maxMethodsInModule_);

    cmpDriver.Run(*callMethodFlagMap_, [this, &fileName, &collector](const CString recordName,
                                                const std::string &methodName,
                                                MethodLiteral *methodLiteral,
                                                uint32_t methodOffset,
                                                const MethodPcInfo &methodPCInfo,
                                                MethodInfo &methodInfo,
                                                Module *m) {
        PassContext ctx(triple_, log_, &collector, m->GetModule(), &profilerDecoder_);
        auto jsPandaFile = ctx.GetJSPandaFile();
        auto cmpCfg = ctx.GetCompilerConfig();
        auto module = m->GetModule();
        log_->SetMethodLog(fileName, methodName, logList_);

        std::string fullName = module->GetFuncName(methodLiteral, jsPandaFile);
        bool enableMethodLog = log_->GetEnableMethodLog();
        if (enableMethodLog) {
            LOG_COMPILER(INFO) << "\033[34m" << "aot method [" << fullName
                               << "] recordName [" << recordName << "] log:" << "\033[0m";
        }

        Circuit circuit(compilationEnv_->GetNativeAreaAllocator(), ctx.GetAOTModule()->GetDebugInfo(),
                        fullName.c_str(), cmpCfg->Is64Bit(), FrameType::OPTIMIZED_JS_FUNCTION_FRAME);

        PGOProfilerDecoder defDecoder;
        PGOProfilerDecoder *decoder = passOptions_->EnableOptPGOType() ? &profilerDecoder_ : &defDecoder;

        BytecodeCircuitBuilder builder(jsPandaFile, methodLiteral, methodPCInfo, &circuit,
                                       ctx.GetByteCodes(), enableMethodLog && log_->OutputCIR(),
                                       passOptions_->EnableTypeLowering(), fullName, recordName, decoder, false);
        {
            TimeScope timeScope("BytecodeToCircuit", methodName, methodOffset, log_);
            builder.BytecodeToCircuit();
        }

        PassData data(&builder, &circuit, &ctx, log_, fullName, &methodInfo, recordName,
                      methodLiteral, methodOffset, callMethodFlagMap_, fileInfos_,
                      compilationEnv_->GetNativeAreaAllocator(), decoder, passOptions_,
                      optBCRange_);
        PassRunner<PassData> pipeline(&data);
        if (!pipeline.RunPass<PreCompileCheckPass>()) {
            return;
        }
        pipeline.RunPass<RunFlowCyclesVerifierPass>();
        pipeline.RunPass<RedundantPhiEliminationPass>();
        if (builder.EnableLoopOptimization()) {
            pipeline.RunPass<LoopOptimizationPass>();
            pipeline.RunPass<RedundantPhiEliminationPass>();
        }
        pipeline.RunPass<PGOTypeInferPass>();
        pipeline.RunPass<TSInlineLoweringPass>();
        pipeline.RunPass<RedundantPhiEliminationPass>();
        pipeline.RunPass<AsyncFunctionLoweringPass>();
        pipeline.RunPass<TypeBytecodeLoweringPass>();
        pipeline.RunPass<UselessGateEliminationPass>();
        pipeline.RunPass<InductionVariableAnalysisPass>();
        pipeline.RunPass<RedundantPhiEliminationPass>();
        pipeline.RunPass<NTypeBytecodeLoweringPass>();
        pipeline.RunPass<UselessGateEliminationPass>();
        pipeline.RunPass<EarlyEliminationPass>();
        pipeline.RunPass<NumberSpeculativePass>();
        pipeline.RunPass<UselessGateEliminationPass>();
        pipeline.RunPass<LaterEliminationPass>();
        pipeline.RunPass<ValueNumberingPass>();
        pipeline.RunPass<StateSplitLinearizerPass>();
        pipeline.RunPass<EscapeAnalysisPass>();
        pipeline.RunPass<StringOptimizationPass>();
        pipeline.RunPass<NTypeHCRLoweringPass>();
        pipeline.RunPass<TypeHCRLoweringPass>();
        pipeline.RunPass<UselessGateEliminationPass>();
        pipeline.RunPass<LaterEliminationPass>();
        pipeline.RunPass<EarlyEliminationPass>();
        pipeline.RunPass<LCRLoweringPass>();
        pipeline.RunPass<UselessGateEliminationPass>();
        pipeline.RunPass<ConstantFoldingPass>();
        pipeline.RunPass<ValueNumberingPass>();
        pipeline.RunPass<SlowPathLoweringPass>();
        pipeline.RunPass<ValueNumberingPass>();
        pipeline.RunPass<InstructionCombinePass>();
        pipeline.RunPass<EarlyEliminationPass>();
        pipeline.RunPass<UselessGateEliminationPass>();
        if (passOptions_->EnableVerifierPass()) {
            pipeline.RunPass<VerifierPass>();
        }
        pipeline.RunPass<GraphLinearizerPass>();
        pipeline.RunPass<CGIRGenPass>();
    });

    compilerStats.SetCompilerMethodCount(cmpDriver.GetCompilerMethodCount());
    LOG_COMPILER(INFO) << collector.GetBytecodeInfo().GetSkippedMethodSize()
                       << " methods have been skipped";
    LOG_COMPILER(INFO) << compilerStats.GetCompilerMethodCount() << " methods have been compiled";
    return true;
}

bool PassManager::IsReleasedPandaFile(const JSPandaFile *jsPandaFile) const
{
    MethodLiteral* methodLiteral = jsPandaFile->GetMethodLiterals();
    if (methodLiteral == nullptr) {
        LOG_COMPILER(ERROR) << "There is no mehtod literal in " << jsPandaFile->GetJSPandaFileDesc();
        return false;
    }

    panda_file::File::EntityId methodId = methodLiteral->GetMethodId();
    ASSERT(methodId.IsValid());
    DebugInfoExtractor *debugInfoExtractor = JSPandaFileManager::GetInstance()->GetJSPtExtractor(jsPandaFile);
    LocalVariableTable lvt = debugInfoExtractor->GetLocalVariableTable(methodId);
    return lvt.empty();
}
} // namespace panda::ecmascript::kungfu
