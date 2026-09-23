/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/arksteed/arksteed_compiler.h"

#include <limits>
#include <utility>
#include <vector>

#include "ecmascript/arksteed/arksteed_assembler.h"
#include "ecmascript/arksteed/arksteed_deopt_helper.h"
#include "ecmascript/arksteed/arksteed_graph_builder.h"
#include "ecmascript/arksteed/arksteed_codegen.h"
#include "ecmascript/arksteed/arksteed_graph_printer.h"
#include "ecmascript/arksteed/arksteed_graph_processor.h"
#include "ecmascript/arksteed/arksteed_graph_verifier.h"
#include "ecmascript/arksteed/arksteed_regalloc.h"
#include "ecmascript/arksteed/arksteed_regalloc_processors.h"
#include "ecmascript/arksteed/arksteed_write_barrier_value_kind_pass.h"
#include "ecmascript/arksteed/arksteed_safepoint_table.h"
#include "ecmascript/arksteed/arksteed_task.h"
#include "ecmascript/compiler/jit_compiler.h"
#include "ecmascript/jit/jit.h"
#include "ecmascript/jit/jit_profiler.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/mem/machine_code.h"

#ifdef JIT_ENABLE_CODE_SIGN
#include "ecmascript/compiler/jit_signcode.h"
#include "ecmascript/mem/jit_fort.h"
#include "jit_buffer_integrity.h"
#endif

#include <sstream>

namespace panda::ecmascript::arksteed {

static void VerifyGraph(Graph *graph)
{
#ifndef NDEBUG
    GraphProcessor<ArkSteedGraphVerifier> verifier(graph->GetChunk());
    verifier.Run(graph);
#endif
}

static void LogAsm(ArkSteedAssembler *assembler)
{
    if (assembler == nullptr) {
        return;
    }

    CommentList &comments = assembler->GetCommentList();
    uint8_t *commentsData = comments.EmitToBuffer();
    uint32_t commentsSize = comments.GetCommentsDataSize();
    std::ostringstream asmOut;
    assembler->Disassemble(asmOut, commentsData, commentsSize);
    delete[] commentsData;

    LOG_COMPILER(INFO) << "========== ArkSteed After Codegen ASM ==========";
    std::istringstream asmStream(asmOut.str());
    std::string line;
    while (std::getline(asmStream, line)) {
        LOG_COMPILER(INFO) << line;
    }
    LOG_COMPILER(INFO) << "================================================";
}

ArkSteedCompilationOptions::ArkSteedCompilationOptions(JSRuntimeOptions runtimeOptions)
{
#if defined(PANDA_TARGET_AMD64)
    triple = "x86_64-unknown-linux-gnu";
#elif defined(PANDA_TARGET_ARM64)
    triple = "aarch64-unknown-linux-gnu";
#else
    LOG_JIT(FATAL) << "arksteed jit unsupport arch";
    UNREACHABLE();
#endif
    deviceIsScreenOff = runtimeOptions.GetDeviceState();
    deviceThermalLevel = runtimeOptions.GetThermalLevel();
    hotnessThreshold = runtimeOptions.GetPGOHotnessThreshold();
    profilerIn = std::string(runtimeOptions.GetPGOProfilerPath());
    enableLog = runtimeOptions.WasSetCompilerLogOption();
    printMethodName = runtimeOptions.GetCompilerArkSteedPrintMethodName();
    enableCodeComment = runtimeOptions.GetCompilerArkSteedEnableCodeComment();
}

ArkSteedCompilerTask::~ArkSteedCompilerTask()
{
    if (assembler_ != nullptr) {
        delete assembler_;
        assembler_ = nullptr;
    }
    if (safepointTableBuilder_ != nullptr) {
        delete safepointTableBuilder_;
        safepointTableBuilder_ = nullptr;
    }
    if (translationBuilder_ != nullptr) {
        delete translationBuilder_;
        translationBuilder_ = nullptr;
    }
    if (deoptLiteralTableBuilder_ != nullptr) {
        delete deoptLiteralTableBuilder_;
        deoptLiteralTableBuilder_ = nullptr;
    }
}

ArkSteedCompilerTask *ArkSteedCompilerTask::CreateJitCompilerTask(ArkSteedTask *arkSteedTask)
{
    return new (std::nothrow) ArkSteedCompilerTask(arkSteedTask);
}

void ArkSteedCompilerTask::DebugLogOnCompilationStart()
{
    if (!common::Log::LogIsLoggable(Level::INFO, Component::COMPILER)) {
        return;
    }
    JitCompilationEnv *env = jitCompilationEnv_.get();
    MethodLiteral *method = env->GetMethodLiteral();
    LOG_COMPILER(INFO) << "======== ArkSteedCompilerTask: Starts compiling: "
                       << MethodLiteral::ParseFunctionName(env->GetJSPandaFile(), method->GetMethodId()) << " ========";
}

void ArkSteedCompilerTask::DebugLogOnCompilationDone()
{
    if (!common::Log::LogIsLoggable(Level::INFO, Component::COMPILER)) {
        return;
    }
    JitCompilationEnv *env = jitCompilationEnv_.get();
    MethodLiteral *method = env->GetMethodLiteral();
    LOG_COMPILER(INFO) << "======== ArkSteedCompilerTask: Finished compiling: "
                       << MethodLiteral::ParseFunctionName(env->GetJSPandaFile(), method->GetMethodId()) << " ========";
}

bool ArkSteedCompilerTask::BuildGraph(JSThread *compilerThread, uintptr_t hostGlueAddr)
{
    JitCompilationEnv *env = jitCompilationEnv_.get();
    if (env->GetJSOptions().IsEnableJITPGO()) {
        auto jitProfiler = env->GetPGOProfiler()->GetJITProfile();
        if (jitProfiler != nullptr) {
            MethodLiteral *method = env->GetMethodLiteral();
            const JSPandaFile *jsPandaFile = env->GetJSPandaFile();
            jitProfiler->SetCompilationEnv(env);
            jitProfiler->InitChunk(chunk_.get());
            jitProfiler->ProfileBytecode(env->GetJSThread(), profileTypeInfo_, method->GetMethodId(),
                                         env->GetMethodAbcId(), env->GetMethodPcStart(),
                                         method->GetCodeSize(jsPandaFile, method->GetMethodId()),
                                         jsPandaFile->GetPandaFile()->GetHeader(), jsFunction_, env->GetGlobalEnv());
        }
    }

    BytecodePreprocessor preproc(env, chunk_.get());
    if (!preproc.Run()) {
        LOG_COMPILER(WARN) << "JIT compilation halts due to bytecode preprocessing error.";
        return false;
    }
    BytecodeAnalysis analysis(&preproc);
    if (!analysis.Run()) {
        LOG_COMPILER(WARN) << "JIT compilation halts due to bytecode analysis error.";
        return false;
    }
    GraphBuilder graphBuilder(compilerThread, graph_, hostGlueAddr, &preproc, &analysis);
    if (!graphBuilder.Run()) {
        LOG_COMPILER(WARN) << "JIT compilation halts due to graph building error.";
        return false;
    }
    return true;
}

void ArkSteedCompilerTask::RunPreRegallocProcessors()
{
    // VertexInfoAllocateProcessor - allocate regalloc info for all vertices
    GraphProcessor<VertexInfoAllocateProcessor> processor;
    processor.Run(graph_);
    // ValueLocationConstraintProcessor - set input/output location constraints
    GraphProcessor<ValueLocationConstraintProcessor> constraintProcessor;
    constraintProcessor.Run(graph_);

    // MaxCallDepthProcessor - compute max call stack args
    GraphProcessor<MaxCallDepthProcessor> callDepthProcessor;
    callDepthProcessor.Run(graph_);

    // LivenessProcessor - track live ranges and next use
    GraphProcessor<LivenessProcessor> liveRangeProcessor;
    liveRangeProcessor.Run(graph_);
}

bool ArkSteedCompilerTask::Compile()
{
    DebugLogOnCompilationStart();

    ArkSteedCompileTimeScope totalScope(arkSteedTask_);
    NativeAreaAllocator *allocator = arkSteedTask_->GetCompilerVM()->GetNativeAreaAllocator();
    chunk_ = std::make_unique<Chunk>(allocator);
    graph_ = Graph::New(chunk_.get());

    // Allocate FuncEntryDes from chunk (persists until ArkSteedCompilerTask destroyed)
    funcEntryDes_ = chunk_->New<FuncEntryDes>();

    // Debug vertex labels are scoped for the entire compilation
    VertexLabelScope vertexLabelScope;

    // Graph building phase
    auto *compilerThread = arkSteedTask_->GetCompilerThread();
    auto *hostThread = arkSteedTask_->GetHostThread();
    uintptr_t hostGlueAddr = hostThread->GetGlueAddr();

    ArkSteedPGOContext pgoContext(compilerThread, jitCompilationEnv_.get());

    if (!BuildGraph(compilerThread, hostGlueAddr)) {
        return false;
    }

    WriteBarrierValueKindPass writeBarrierValueKindPass(graph_);
    writeBarrierValueKindPass.Run();

    // Verify graph integrity
    VerifyGraph(graph_);
    RunPreRegallocProcessors();

    // Print graph with labeller if option is enabled
    if (arkSteedTask_->GetHostVM()->GetJSOptions().GetCompilerArkSteedPrintGraph()) {
        LOG_COMPILER(INFO) << "===== Starts Graph Dump =====";
        bool withColors = arkSteedTask_->GetHostVM()->GetJSOptions().GetCompilerArkSteedPrintWithColors();
        GraphProcessor<GraphPrinter> graphPrinterProcessor(chunk_.get(), withColors);
        graphPrinterProcessor.Run(graph_);
        LOG_COMPILER(INFO) << "===== Finishes Graph Dump =====";
    }

    // Register allocation
    graph_->SetReuseStackSlots(arkSteedTask_->GetHostVM()->GetJSOptions().GetCompilerArkSteedReuseStackSlots());
    ArkSteedRegisterAllocator registerAllocator(graph_);

    // Code generation
    assembler_ = new ArkSteedAssembler(chunk_.get(), compilerThread, hostThread);
    if (arkSteedTask_->GetHostVM()->GetJSOptions().GetCompilerArkSteedEnableCodeComment()) {
        assembler_->EnableComments(true);
    }
#ifdef JIT_ENABLE_CODE_SIGN
    EnableCodeSign();
#endif
    safepointTableBuilder_ = new ArkSteedSafepointTableBuilder(chunk_.get());
    translationBuilder_ = new DeoptTranslationBuilder();
    deoptLiteralTableBuilder_ = new DeoptLiteralTableBuilder();
    bool withColors = arkSteedTask_->GetHostVM()->GetJSOptions().GetCompilerArkSteedPrintWithColors();
    ArkSteedCodeGenerator codegen(assembler_, graph_, safepointTableBuilder_, translationBuilder_,
                                  deoptLiteralTableBuilder_, withColors);
    codegen.Generate();
    if (arkSteedTask_->GetHostVM()->GetJSOptions().GetCompilerArkSteedPrintCode()) {
        LogAsm(assembler_);
    }

    DebugLogOnCompilationDone();
    return true;
}

#ifdef JIT_ENABLE_CODE_SIGN
void ArkSteedCompilerTask::EnableCodeSign()
{
    kungfu::JitSignCode *singleton = kungfu::JitSignCode::GetInstance();
    singleton->Reset();
    if (Jit::GetInstance()->IsEnableJitFort() && !Jit::GetInstance()->IsDisableCodeSign() &&
        JitFort::IsResourceAvailable()) {
        OHOS::Security::CodeSign::JitCodeSigner *jitSigner = CreateJitCodeSigner();
        singleton->SetCodeSigner(jitSigner);
        assembler_->EnableCodeSign();
    }
}
#endif

void ArkSteedCompilerTask::FillCodeDesc(MachineCodeDesc &codeDesc)
{
    codeDesc.codeAddr = reinterpret_cast<uintptr_t>(assembler_->GetCodeBuffer());
    codeDesc.codeSize = assembler_->GetCodeSize();

    codeDesc.codeType = MachineCodeType::ARKSTEED_CODE;

    const auto &heapConstantHandles = jitCompilationEnv_->GetHeapConstantTable();
    std::vector<JSHandle<JSTaggedValue>> deoptLiteralHandles;
    const auto &deoptLiteralHandleIndices = deoptLiteralTableBuilder_->GetHandleIndices();
    deoptLiteralHandles.reserve(deoptLiteralHandleIndices.size());
    for (uint32_t handleIndex : deoptLiteralHandleIndices) {
        CHECK(handleIndex < heapConstantHandles.size());
        deoptLiteralHandles.push_back(heapConstantHandles[handleIndex]);
    }
    CHECK(deoptLiteralHandles.size() <= std::numeric_limits<uint32_t>::max());
    safepointTableBuilder_->SetDeoptLiteralCount(static_cast<uint32_t>(deoptLiteralHandles.size()));
    arkSteedTask_->SetDeoptLiteralData(std::move(deoptLiteralHandles));

    // Safepoint table
    safepointTableBuilder_->SetFrameSlots(graph_->GetTaggedStackSlots(), graph_->GetUntaggedStackSlots());
    size_t safepointSize = safepointTableBuilder_->GetTableSize();
    if (safepointSize > 0) {
        uint8_t *safepointBuffer = safepointTableBuilder_->EmitToNewBuffer();
        codeDesc.stackMapOrOffsetTableAddr = reinterpret_cast<uintptr_t>(safepointBuffer);
        codeDesc.stackMapOrOffsetTableSize = safepointSize;
    } else {
        codeDesc.stackMapOrOffsetTableAddr = 0;
        codeDesc.stackMapOrOffsetTableSize = 0;
    }
    LOG_COMPILER(DEBUG) << safepointTableBuilder_->DumpMemoryUsage();

#if ECMASCRIPT_ENABLE_ARK_STEED
    arkSteedTask_->SetDeoptTranslationData(translationBuilder_->Encode());
#endif

    arkSteedTask_->SetEmbeddedRefData(assembler_->GetEmbeddedRefRelocations(), heapConstantHandles);

    // Frame info - fill FuncEntryDes
    // Set funcEntry to point to heap-allocated FuncEntryDes
    codeDesc.funcEntryDesAddr = reinterpret_cast<uintptr_t>(funcEntryDes_);

    // fpDelta = frame size + saved FP + return address
    funcEntryDes_->fpDeltaPrevFrameSp_ = 2 * static_cast<int>(sizeof(uint64_t));  // 2: saved FP + return address
    funcEntryDes_->funcSize_ = static_cast<uint32_t>(assembler_->GetCodeSize());
    funcEntryDes_->isFastCall_ = false;
    funcEntryDes_->calleeRegisterNum_ = 0;
    // ArkSteed is single function, no need for CalleeReg2Offset currently

    // Code comments
    CommentList &comments = assembler_->GetCommentList();
    if (comments.Size() > 0) {
        uint8_t *commentsData = comments.EmitToBuffer();
        codeDesc.codeCommentsAddr = reinterpret_cast<uintptr_t>(commentsData);
        codeDesc.codeCommentsSize = comments.GetCommentsDataSize();
    } else {
        codeDesc.codeCommentsAddr = 0;
        codeDesc.codeCommentsSize = 0;
    }

#ifdef JIT_ENABLE_CODE_SIGN
    codeDesc.codeSigner = reinterpret_cast<uintptr_t>(kungfu::JitSignCode::GetInstance()->GetCodeSigner());
#endif
}

extern "C" {
PUBLIC_API void InitArkSteedCompiler(JSRuntimeOptions options)
{
    kungfu::BytecodeStubCSigns::Initialize();
    kungfu::CommonStubCSigns::Initialize();
    kungfu::BuiltinsStubCSigns::Initialize();
    kungfu::RuntimeStubCSigns::Initialize();
}

PUBLIC_API void *CreateArkSteedCompilerTask(ArkSteedTask *arkSteedTask)
{
    if (arkSteedTask == nullptr) {
        return nullptr;
    }
    return ArkSteedCompilerTask::CreateJitCompilerTask(arkSteedTask);
}

PUBLIC_API bool ArkSteedCompile(void *compilerTask, ArkSteedTask *arkSteedTask)
{
    if (arkSteedTask == nullptr || compilerTask == nullptr) {
        return false;
    }
    auto jitCompilerTask = reinterpret_cast<ArkSteedCompilerTask *>(compilerTask);
    return jitCompilerTask->Compile();
}

PUBLIC_API bool ArkSteedFinalize(void *compilerTask, ArkSteedTask *arkSteedTask)
{
    if (arkSteedTask == nullptr || compilerTask == nullptr) {
        return false;
    }
    auto task = reinterpret_cast<ArkSteedCompilerTask *>(compilerTask);

    // Fill MachineCodeDesc from compiler output
    MachineCodeDesc &machineDesc = arkSteedTask->GetMachineCodeDesc();
    task->FillCodeDesc(machineDesc);

    // JitFort async copy (if enabled)
    if (Jit::GetInstance()->IsEnableJitFort() && Jit::GetInstance()->IsEnableAsyncCopyToFort() &&
        machineDesc.codeSize > 0) {
        if (kungfu::JitCompiler::AllocFromFortAndCopy(*task->GetCompilationEnv(), machineDesc) == false) {
            return false;
        }
    }

    return true;
}

PUBLIC_API void DeleteArkSteedCompilerTask(void *compilerTask)
{
    if (compilerTask == nullptr) {
        return;
    }
    delete reinterpret_cast<ArkSteedCompilerTask *>(compilerTask);
}

}  // extern "C"

}  // namespace panda::ecmascript::arksteed
