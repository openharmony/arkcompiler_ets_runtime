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

#ifndef ECMASCRIPT_COMPILER_PASS_H
#define ECMASCRIPT_COMPILER_PASS_H

#include "ecmascript/compiler/async_function_lowering.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/compiler/early_elimination.h"
#include "ecmascript/compiler/graph_linearizer.h"
#include "ecmascript/compiler/later_elimination.h"
#include "ecmascript/compiler/lcr_lowering.h"
#include "ecmascript/compiler/llvm_codegen.h"
#include "ecmascript/compiler/loop_analysis.h"
#include "ecmascript/compiler/ntype_hcr_lowering.h"
#include "ecmascript/compiler/ntype_mcr_lowering.h"
#include "ecmascript/compiler/number_speculative_runner.h"
#include "ecmascript/compiler/scheduler.h"
#include "ecmascript/compiler/slowpath_lowering.h"
#include "ecmascript/compiler/state_split_linearizer.h"
#include "ecmascript/compiler/ts_class_analysis.h"
#include "ecmascript/compiler/ts_inline_lowering.h"
#include "ecmascript/compiler/ts_hcr_lowering.h"
#include "ecmascript/compiler/type_inference/global_type_infer.h"
#include "ecmascript/compiler/type_inference/initialization_analysis.h"
#include "ecmascript/compiler/type_inference/pgo_type_infer.h"
#include "ecmascript/compiler/type_mcr_lowering.h"
#include "ecmascript/compiler/value_numbering.h"
#include "ecmascript/compiler/verifier.h"

namespace panda::ecmascript::kungfu {
class PassContext;

class PassData {
public:
    PassData(BytecodeCircuitBuilder *builder, Circuit *circuit, PassContext *ctx, CompilerLog *log,
             std::string methodName, MethodInfo *methodInfo = nullptr, bool hasTypes = false,
             const CString &recordName = "", MethodLiteral *methodLiteral = nullptr,
             uint32_t methodOffset = 0, NativeAreaAllocator *allocator = nullptr,
             PGOProfilerDecoder *decoder = nullptr, PassOptions *passOptions = nullptr)
        : builder_(builder), circuit_(circuit), ctx_(ctx), log_(log), methodName_(methodName),
          methodInfo_(methodInfo), hasTypes_(hasTypes), recordName_(recordName), methodLiteral_(methodLiteral),
          methodOffset_(methodOffset), allocator_(allocator), decoder_(decoder), passOptions_(passOptions)
    {
    }

    virtual ~PassData() = default;

    const ControlFlowGraph &GetConstScheduleResult() const
    {
        return cfg_;
    }

    ControlFlowGraph &GetCfg()
    {
        return cfg_;
    }

    virtual Circuit* GetCircuit() const
    {
        return circuit_;
    }

    BytecodeCircuitBuilder* GetBuilder() const
    {
        return builder_;
    }

    PassContext* GetPassContext() const
    {
        return ctx_;
    }

    CompilationConfig* GetCompilerConfig() const
    {
        return ctx_->GetCompilerConfig();
    }

    TSManager* GetTSManager() const
    {
        return ctx_->GetTSManager();
    }

    const JSPandaFile *GetJSPandaFile() const
    {
        return ctx_->GetJSPandaFile();
    }

    LLVMModule* GetAotModule() const
    {
        return ctx_->GetAOTModule();
    }

    CompilerLog* GetLog() const
    {
        return log_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    const MethodLiteral* GetMethodLiteral() const
    {
        return methodLiteral_;
    }

    uint32_t GetMethodOffset() const
    {
        return methodOffset_;
    }

    MethodInfo* GetMethodInfo() const
    {
        return methodInfo_;
    }

    size_t GetMethodInfoIndex() const
    {
        return methodInfo_->GetMethodInfoIndex();
    }

    bool HasTypes() const
    {
        return hasTypes_;
    }

    const CString &GetRecordName() const
    {
        return recordName_;
    }

    NativeAreaAllocator* GetNativeAreaAllocator() const
    {
        return allocator_;
    }

    PGOProfilerDecoder *GetPGOProfilerDecoder() const
    {
        return decoder_;
    }

    PassOptions *GetPassOptions() const
    {
        return passOptions_;
    }

    bool IsTypeAbort() const
    {
        if (hasTypes_) {
            // A ts method which has low type percent and not marked as a resolved method
            // should be skipped from full compilation.
            if (methodInfo_->IsTypeInferAbort() && !methodInfo_->IsResolvedMethod()) {
                return true;
            }
        } else {
            // For js method, type infer pass will be skipped and it don't have a type percent.
            // If we set an non zero type threshold, js method will be skipped from full compilation.
            // The default Type threshold is -1.
            if (ctx_->GetTSManager()->GetTypeThreshold() >= 0) {
                return true;
            }
        }
        // when a method will be full compiled, we should confirm its TypeInferAbortBit to be false
        // maybe it used to be true in the first round of compilation.
        methodInfo_->SetTypeInferAbort(false);
        log_->AddCompiledMethod(methodName_, recordName_);
        return false;
    }

    void AbortCompilation()
    {
        ctx_->GetBytecodeInfo().AddSkippedMethod(methodOffset_);
        methodInfo_->SetIsCompiled(false);
        log_->RemoveCompiledMethod(methodName_, recordName_);
    }

    void MarkAsTypeAbort()
    {
        methodInfo_->SetTypeInferAbort(true);
    }

private:
    BytecodeCircuitBuilder *builder_ {nullptr};
    Circuit *circuit_ {nullptr};
    ControlFlowGraph cfg_;
    PassContext *ctx_ {nullptr};
    CompilerLog *log_ {nullptr};
    std::string methodName_;
    MethodInfo *methodInfo_ {nullptr};
    bool hasTypes_;
    const CString &recordName_;
    MethodLiteral *methodLiteral_ {nullptr};
    uint32_t methodOffset_;
    NativeAreaAllocator *allocator_ {nullptr};
    PGOProfilerDecoder *decoder_ {nullptr};
    PassOptions *passOptions_ {nullptr};
};

template<typename T1>
class PassRunner {
public:
    explicit PassRunner(T1* data) : data_(data) {}
    virtual ~PassRunner() = default;
    template<typename T2, typename... Args>
    bool RunPass(Args... args)
    {
        T2 pass;
        return pass.Run(data_, std::forward<Args>(args)...);
    }

private:
    T1* data_;
};

class TypeInferPass {
public:
    bool Run(PassData* data)
    {
        PassOptions *passOptions = data->GetPassOptions();
        if (passOptions != nullptr && !passOptions->EnableTypeInfer()) {
            return false;
        }
        TimeScope timescope("TypeInferPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        if (data->HasTypes()) {
            bool enableLog = data->GetLog()->GetEnableMethodLog() && data->GetLog()->OutputType();
            GlobalTypeInfer globalTypeInfer(data->GetPassContext(), data->GetMethodOffset(), data->GetRecordName(),
                                            data->GetPGOProfilerDecoder(), passOptions->EnableOptTrackField(),
                                            enableLog);
            globalTypeInfer.ProcessTypeInference(data->GetBuilder(), data->GetCircuit());
            if (data->GetMethodLiteral()->IsClassConstructor()) {
                InitializationAnalysis initAnalysis(data->GetCircuit(), data->GetTSManager(), data->GetRecordName(),
                                                    data->GetMethodName(), enableLog);
                initAnalysis.Run();
            }
        }
        return true;
    }
};

class PGOTypeInferPass {
public:
    bool Run(PassData* data)
    {
        TimeScope timescope("PGOTypeInferPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->GetEnableMethodLog() && data->GetLog()->OutputType();
        Chunk chunk(data->GetNativeAreaAllocator());
        PGOTypeInfer pgoTypeInfer(data->GetCircuit(), data->GetTSManager(), data->GetBuilder(),
                                  data->GetMethodName(), &chunk, enableLog);
        pgoTypeInfer.Run();
        return true;
    }
};

class TSClassAnalysisPass {
public:
    bool Run(PassData *data)
    {
        TimeScope timescope("TSClassAnalysisPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        TSClassAnalysis analyzer(data->GetPassContext()->GetTSManager());
        analyzer.Run();
        return true;
    }
};

class TSHCRLoweringPass {
public:
    bool Run(PassData* data)
    {
        PassOptions *passOptions = data->GetPassOptions();
        if (!passOptions->EnableTypeLowering()) {
            return false;
        }
        TimeScope timescope("TSHCRLoweringPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        bool enableTypeLog = data->GetLog()->GetEnableMethodLog() && data->GetLog()->OutputType();
        TSHCRLowering lowering(data->GetCircuit(), data->GetPassContext(),
            enableLog, enableTypeLog, passOptions->EnableOptStaticMethod(), data->GetMethodName());
        bool success = lowering.RunTSHCRLowering();
        if (!success) {
            data->MarkAsTypeAbort();
        }
        return true;
    }
};

class NTypeHCRLoweringPass {
public:
    bool Run(PassData* data)
    {
        PassOptions *passOptions = data->GetPassOptions();
        if (!passOptions->EnableTypeLowering()) {
            return false;
        }
        TimeScope timescope("NTypeHCRLoweringPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        NTypeHCRLowering lowering(data->GetCircuit(), data->GetPassContext(),
            data->GetTSManager(), enableLog, data->GetMethodName());
        lowering.RunNTypeHCRLowering();
        return true;
    }
};

class TypeMCRLoweringPass {
public:
    bool Run(PassData* data)
    {
        PassOptions *passOptions = data->GetPassOptions();
        if (!passOptions->EnableTypeLowering()) {
            return false;
        }
        TimeScope timescope("TypeMCRLoweringPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        TypeMCRLowering lowering(data->GetCircuit(), data->GetCompilerConfig(), data->GetTSManager(),
                                 enableLog, data->GetMethodName());
        lowering.RunTypeMCRLowering();
        return true;
    }
};

class NTypeMCRLoweringPass {
public:
    bool Run(PassData* data)
    {
        PassOptions *passOptions = data->GetPassOptions();
        if (!passOptions->EnableTypeLowering()) {
            return false;
        }
        TimeScope timescope("NTypeMCRLoweringPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        NTypeMCRLowering lowering(data->GetCircuit(), data->GetPassContext(), data->GetTSManager(),
                                  enableLog, data->GetMethodName());
        lowering.RunNTypeMCRLowering();
        return true;
    }
};

class LCRLoweringPass {
public:
    bool Run(PassData* data)
    {
        PassOptions *passOptions = data->GetPassOptions();
        if (!passOptions->EnableTypeLowering()) {
            return false;
        }
        TimeScope timescope("LCRLoweringPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        LCRLowering lowering(data->GetCircuit(), data->GetCompilerConfig(), enableLog, data->GetMethodName());
        lowering.Run();
        return true;
    }
};

class TSInlineLoweringPass {
public:
    bool Run(PassData* data)
    {
        PassOptions *passOptions = data->GetPassOptions();
        if (!passOptions->EnableOptInlining() || !passOptions->EnableTypeLowering()) {
            return false;
        }
        TimeScope timescope("TSInlineLoweringPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        TSInlineLowering inlining(data->GetCircuit(), data->GetPassContext(), enableLog, data->GetMethodName(),
                                  data->GetNativeAreaAllocator(), passOptions);
        inlining.RunTSInlineLowering();
        return true;
    }
};

class SlowPathLoweringPass {
public:
    bool Run(PassData* data)
    {
        PassOptions *passOptions = data->GetPassOptions();
        TimeScope timescope("SlowPathLoweringPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        SlowPathLowering lowering(data->GetCircuit(), data->GetCompilerConfig(), data->GetTSManager(),
                                  data->GetMethodLiteral(), enableLog,
                                  passOptions->EnableOptStaticMethod(), data->GetMethodName());
        lowering.CallRuntimeLowering();
        return true;
    }
};

class VerifierPass {
public:
    bool Run(PassData* data)
    {
        TimeScope timescope("VerifierPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        bool isQualified = Verifier::Run(data->GetCircuit(), data->GetMethodName(), enableLog);
        if (!isQualified) {
            LOG_FULL(FATAL) << "VerifierPass fail";
            UNREACHABLE();
        }
        return isQualified;
    }
};

class NumberSpeculativePass {
public:
    bool Run(PassData* data)
    {
        PassOptions *passOptions = data->GetPassOptions();
        if (!passOptions->EnableTypeLowering()) {
            return false;
        }
        TimeScope timescope("NumberSpeculativePass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        Chunk chunk(data->GetNativeAreaAllocator());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        NumberSpeculativeRunner(data->GetCircuit(), data->GetTSManager(),
                                enableLog, data->GetMethodName(), &chunk, data->GetPassContext()).Run();
        return true;
    }
};

class LoopOptimizationPass {
public:
    bool Run(PassData* data)
    {
        TimeScope timescope("LoopOptimizationPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        Chunk chunk(data->GetNativeAreaAllocator());
        const auto& headList = data->GetBuilder()->GetLoopHeads();
        LoopAnalysis loopAnalysis_(data->GetCircuit(), &chunk);
        for (auto head : headList) {
            auto bb = data->GetBuilder()->GetBasicBlockById(head.second);
            auto loopInfo = new LoopInfo(&chunk, bb.stateCurrent);
            loopAnalysis_.CollectLoopBody(loopInfo);
        }
        loopAnalysis_.LoopExitElimination();
        return true;
    }
};

class EarlyEliminationPass {
public:
    bool Run(PassData* data)
    {
        PassOptions *passOptions = data->GetPassOptions();
        if (!passOptions->EnableTypeLowering() || !passOptions->EnableEarlyElimination()) {
            return false;
        }
        TimeScope timescope("EarlyEliminationPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        Chunk chunk(data->GetNativeAreaAllocator());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        EarlyElimination(data->GetCircuit(), enableLog, data->GetMethodName(), &chunk).Run();
        return true;
    }
};

class LaterEliminationPass {
public:
    bool Run(PassData* data)
    {
        PassOptions *passOptions = data->GetPassOptions();
        if (!passOptions->EnableTypeLowering() || !passOptions->EnableLaterElimination()) {
            return false;
        }
        TimeScope timescope("LaterEliminationPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        Chunk chunk(data->GetNativeAreaAllocator());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        LaterElimination(data->GetCircuit(), enableLog, data->GetMethodName(), &chunk).Run();
        return true;
    }
};

class ValueNumberingPass {
public:
    bool Run(PassData* data)
    {
        PassOptions *passOptions = data->GetPassOptions();
        if (!passOptions->EnableTypeLowering() || !passOptions->EnableValueNumbering()) {
            return false;
        }
        TimeScope timescope("ValueNumberingPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        Chunk chunk(data->GetNativeAreaAllocator());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        ValueNumbering(data->GetCircuit(), enableLog, data->GetMethodName(), &chunk).Run();
        return true;
    }
};

class SchedulingPass {
public:
    bool Run(PassData* data)
    {
        TimeScope timescope("SchedulingPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        Scheduler::Run(data->GetCircuit(), data->GetCfg(), data->GetMethodName(), enableLog);
        return true;
    }
};

class StateSplitLinearizerPass {
public:
    bool Run(PassData* data)
    {
        PassOptions *passOptions = data->GetPassOptions();
        if (!passOptions->EnableTypeLowering()) {
            return false;
        }
        TimeScope timescope("StateSplitLinearizerPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        Chunk chunk(data->GetNativeAreaAllocator());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        StateSplitLinearizer(data->GetCircuit(), data->GetCompilerConfig(),
            enableLog, data->GetMethodName(), &chunk).Run();
        return true;
    }
};

class GraphLinearizerPass {
public:
    bool Run(PassData* data)
    {
        TimeScope timescope("GraphLinearizerPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        Chunk chunk(data->GetNativeAreaAllocator());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        GraphLinearizer(data->GetCircuit(), enableLog, data->GetMethodName(), &chunk).Run(data->GetCfg());
        return true;
    }
};

class LLVMIRGenPass {
public:
    void CreateCodeGen(LLVMModule *module, bool enableLog)
    {
        llvmImpl_ = std::make_unique<LLVMIRGeneratorImpl>(module, enableLog);
    }

    bool Run(PassData *data)
    {
        auto module = data->GetAotModule();
        TimeScope timescope("LLVMIRGenPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog() || data->GetLog()->OutputASM();
        CreateCodeGen(module, enableLog);
        CodeGenerator codegen(llvmImpl_, data->GetMethodName());
        codegen.Run(data->GetCircuit(), data->GetConstScheduleResult(), data->GetCompilerConfig(),
                    data->GetMethodLiteral(), data->GetJSPandaFile());
        return true;
    }
private:
    std::unique_ptr<CodeGeneratorImpl> llvmImpl_ {nullptr};
};

class AsyncFunctionLoweringPass {
public:
    bool Run(PassData* data)
    {
        TimeScope timescope("AsyncFunctionLoweringPass", data->GetMethodName(),
                            data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog() || data->GetLog()->OutputASM();
        AsyncFunctionLowering lowering(data->GetBuilder(), data->GetCircuit(), data->GetCompilerConfig(),
                                       enableLog, data->GetMethodName());
        if (lowering.IsAsyncRelated()) {
            lowering.ProcessAll();
        }
        return true;
    }
};
} // namespace panda::ecmascript::kungfu
#endif
