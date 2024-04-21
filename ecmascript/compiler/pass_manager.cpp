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
#include "ecmascript/compiler/pass_manager.h"

#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/compilation_driver.h"
#include "ecmascript/compiler/pass.h"
#include "ecmascript/ecma_handle_scope.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/jspandafile/panda_file_translator.h"
#include "ecmascript/log.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/pgo_profiler/pgo_profiler.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/jit/jit.h"
#include "jsnapi_expo.h"

namespace panda::ecmascript::kungfu {
using PGOProfilerManager = pgo::PGOProfilerManager;
bool JitPassManager::Compile(JSHandle<ProfileTypeInfo> &profileTypeInfo,
                             AOTFileGenerator &gen, int32_t osrOffset)
{
    const JSPandaFile *jsPandaFile = compilationEnv_->GetJSPandaFile();
    MethodLiteral *methodLiteral = compilationEnv_->GetMethodLiteral();
    const uint8_t *pcStart = compilationEnv_->GetMethodPcStart();
    const panda_file::File::Header *header = jsPandaFile->GetPandaFile()->GetHeader();
    ApEntityId abcId = compilationEnv_->GetMethodAbcId();
    std::string fileName = jsPandaFile->GetFileName();

    collector_ = new BytecodeInfoCollector(compilationEnv_, const_cast<JSPandaFile*>(jsPandaFile),
        profilerDecoder_);

    gen.SetCurrentCompileFileName(jsPandaFile->GetNormalizedFileDesc());
    lOptions_ = new LOptions(optLevel_, FPFlag::RESERVE_FP, relocMode_);
    cmpDriver_ = new JitCompilationDriver(profilerDecoder_,
                                          collector_,
                                          compilationEnv_->GetJSOptions().GetCompilerSelectMethods(),
                                          compilationEnv_->GetJSOptions().GetCompilerSkipMethods(),
                                          &gen,
                                          fileName,
                                          triple_,
                                          lOptions_,
                                          log_,
                                          log_->OutputASM(),
                                          maxMethodsInModule_,
                                          compilationEnv_->GetJSOptions().GetCompilerMethodsRange());
    cmpDriver_->CompileMethod(jsPandaFile, methodLiteral, profileTypeInfo, pcStart, header, abcId,
                              [this, &fileName, &osrOffset] (
                                const CString recordName,
                                const std::string &methodName,
                                MethodLiteral *methodLiteral,
                                JSHandle<ProfileTypeInfo> &profileTypeInfo,
                                uint32_t methodOffset,
                                const MethodPcInfo &methodPCInfo,
                                MethodInfo &methodInfo,
                                Module *m,
                                const uint8_t *pcStart,
                                const panda_file::File::Header *header,
                                ApEntityId abcId) {
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
        bool hasTypes = jsPandaFile->HasTSTypes(recordName);
        if (UNLIKELY(!hasTypes)) {
            LOG_COMPILER(INFO) << "record: " << recordName << " has no types";
        }

        if (compilationEnv_->GetJSOptions().IsEnableJITPGO()) {
            Jit::JitLockHolder lock(compilationEnv_, "PGO ProfileBytecode");
            jitProfiler_ = compilationEnv_->GetPGOProfiler()->GetJITProfile();
            LOG_COMPILER(INFO) << "GetPGOProfiler(): " << static_cast<void *>(compilationEnv_->GetPGOProfiler().get());
            jitProfiler_->ProfileBytecode(profileTypeInfo, methodLiteral->GetMethodId(), abcId, pcStart,
                                          methodLiteral->GetCodeSize(jsPandaFile, methodLiteral->GetMethodId()),
                                          header);
        } else {
            jitProfiler_ = nullptr;
        }
        circuit_ = new Circuit(compilationEnv_->GetNativeAreaAllocator(), ctx_->GetAOTModule()->GetDebugInfo(),
            fullName.c_str(), cmpCfg->Is64Bit(), FrameType::OPTIMIZED_JS_FUNCTION_FRAME);
        PGOProfilerDecoder *decoder = passOptions_->EnableOptPGOType() ? &profilerDecoder_ : nullptr;

        builder_ = new BytecodeCircuitBuilder(jsPandaFile, methodLiteral, methodPCInfo,
            circuit_, ctx_->GetByteCodes(), enableMethodLog && log_->OutputCIR(),
            passOptions_->EnableTypeLowering(), fullName, recordName, decoder, false, jitProfiler_);
        builder_->SetOsrOffset(osrOffset);
        {
            TimeScope timeScope("BytecodeToCircuit", methodName, methodOffset, log_);
            builder_->BytecodeToCircuit();
        }

        data_ = new PassData(builder_, circuit_, ctx_, log_, fullName, &methodInfo, hasTypes, recordName,
            methodLiteral, methodOffset, nullptr, CVector<AbcFileInfo> {},
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
        pipeline.RunPass<InductionVariableAnalysisPass>();
        pipeline.RunPass<RedundantPhiEliminationPass>();
        pipeline.RunPass<NTypeBytecodeLoweringPass>();
        pipeline.RunPass<EarlyEliminationPass>();
        pipeline.RunPass<NumberSpeculativePass>();
        pipeline.RunPass<LaterEliminationPass>();
        pipeline.RunPass<ValueNumberingPass>();
        pipeline.RunPass<StateSplitLinearizerPass>();
        pipeline.RunPass<EscapeAnalysisPass>();
        pipeline.RunPass<StringOptimizationPass>();
        pipeline.RunPass<NTypeHCRLoweringPass>();
        pipeline.RunPass<TypeHCRLoweringPass>();
        pipeline.RunPass<LaterEliminationPass>();
        pipeline.RunPass<EarlyEliminationPass>();
        pipeline.RunPass<LCRLoweringPass>();
        pipeline.RunPass<ConstantFoldingPass>();
        pipeline.RunPass<ValueNumberingPass>();
        pipeline.RunPass<SlowPathLoweringPass>();
        pipeline.RunPass<ValueNumberingPass>();
        pipeline.RunPass<InstructionCombinePass>();
        pipeline.RunPass<EarlyEliminationPass>();
        pipeline.RunPass<VerifierPass>();
        pipeline.RunPass<GraphLinearizerPass>();
    });
    return true;
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

bool PassManager::Compile(JSPandaFile *jsPandaFile, const std::string &fileName, AOTFileGenerator &gen)
{
    [[maybe_unused]] EcmaHandleScope handleScope(compilationEnv_->GetJSThread());

    BytecodeInfoCollector collector(compilationEnv_, jsPandaFile, profilerDecoder_, maxAotMethodSize_);
    // Checking released/debuggable pandafile uses method literals, which are initialized in BytecodeInfoCollector,
    // should after it.
    if (!IsReleasedPandaFile(jsPandaFile)) {
        LOG_COMPILER(ERROR) << "The input panda file [" << fileName
                            << "] of AOT Compiler is debuggable version, do not use for performance test!";
    }

    LOptions lOptions(optLevel_, FPFlag::RESERVE_FP, relocMode_);
    CompilationDriver cmpDriver(profilerDecoder_,
                                &collector,
                                compilationEnv_->GetJSOptions().GetCompilerSelectMethods(),
                                compilationEnv_->GetJSOptions().GetCompilerSkipMethods(),
                                &gen,
                                fileName,
                                triple_,
                                &lOptions,
                                log_,
                                log_->OutputASM(),
                                maxMethodsInModule_,
                                compilationEnv_->GetJSOptions().GetCompilerMethodsRange());

    cmpDriver.Run([this, &fileName, &collector](const CString recordName,
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
        bool hasTypes = jsPandaFile->HasTSTypes(recordName);
        if (UNLIKELY(!hasTypes)) {
            LOG_COMPILER(INFO) << "record: " << recordName << " has no types";
        }

        Circuit circuit(compilationEnv_->GetNativeAreaAllocator(), ctx.GetAOTModule()->GetDebugInfo(),
                        fullName.c_str(), cmpCfg->Is64Bit(), FrameType::OPTIMIZED_JS_FUNCTION_FRAME);

        PGOProfilerDecoder *decoder = passOptions_->EnableOptPGOType() ? &profilerDecoder_ : nullptr;

        BytecodeCircuitBuilder builder(jsPandaFile, methodLiteral, methodPCInfo, &circuit,
                                       ctx.GetByteCodes(), enableMethodLog && log_->OutputCIR(),
                                       passOptions_->EnableTypeLowering(), fullName, recordName, decoder, false);
        {
            TimeScope timeScope("BytecodeToCircuit", methodName, methodOffset, log_);
            builder.BytecodeToCircuit();
        }

        PassData data(&builder, &circuit, &ctx, log_, fullName, &methodInfo, hasTypes, recordName,
                      methodLiteral, methodOffset, callMethodFlagMap_, fileInfos_,
                      compilationEnv_->GetNativeAreaAllocator(), decoder, passOptions_,
                      optBCRange_);
        PassRunner<PassData> pipeline(&data);
        if (data.GetMethodLiteral()->HasDebuggerStmt()) {
            data.AbortCompilation();
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
        pipeline.RunPass<InductionVariableAnalysisPass>();
        pipeline.RunPass<RedundantPhiEliminationPass>();
        pipeline.RunPass<NTypeBytecodeLoweringPass>();
        pipeline.RunPass<EarlyEliminationPass>();
        pipeline.RunPass<NumberSpeculativePass>();
        pipeline.RunPass<LaterEliminationPass>();
        pipeline.RunPass<ValueNumberingPass>();
        pipeline.RunPass<StateSplitLinearizerPass>();
        pipeline.RunPass<EscapeAnalysisPass>();
        pipeline.RunPass<StringOptimizationPass>();
        pipeline.RunPass<NTypeHCRLoweringPass>();
        pipeline.RunPass<TypeHCRLoweringPass>();
        pipeline.RunPass<LaterEliminationPass>();
        pipeline.RunPass<EarlyEliminationPass>();
        pipeline.RunPass<LCRLoweringPass>();
        pipeline.RunPass<ConstantFoldingPass>();
        pipeline.RunPass<ValueNumberingPass>();
        pipeline.RunPass<SlowPathLoweringPass>();
        pipeline.RunPass<ValueNumberingPass>();
        pipeline.RunPass<InstructionCombinePass>();
        pipeline.RunPass<EarlyEliminationPass>();
        pipeline.RunPass<VerifierPass>();
        pipeline.RunPass<GraphLinearizerPass>();
        pipeline.RunPass<CGIRGenPass>();
    });

    LOG_COMPILER(INFO) << collector.GetBytecodeInfo().GetSkippedMethodSize()
                       << " methods have been skipped";
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
