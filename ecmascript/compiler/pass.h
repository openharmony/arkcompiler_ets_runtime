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
#include "ecmascript/compiler/generic_type_lowering.h"
#include "ecmascript/compiler/later_elimination.h"
#include "ecmascript/compiler/llvm_codegen.h"
#include "ecmascript/compiler/number_speculative_runner.h"
#include "ecmascript/compiler/scheduler.h"
#include "ecmascript/compiler/slowpath_lowering.h"
#include "ecmascript/compiler/ts_inline_lowering.h"
#include "ecmascript/compiler/ts_type_lowering.h"
#include "ecmascript/compiler/type_inference/type_infer.h"
#include "ecmascript/compiler/type_lowering.h"
#include "ecmascript/compiler/verifier.h"

namespace panda::ecmascript::kungfu {
class PassContext;

class PassData {
public:
    PassData(BytecodeCircuitBuilder *builder, Circuit *circuit, PassContext *ctx, CompilerLog *log,
             std::string methodName, MethodInfo *methodInfo = nullptr, bool hasTypes = false,
             const CString &recordName = "", MethodLiteral *methodLiteral = nullptr,
             uint32_t methodOffset = 0, NativeAreaAllocator *allocator = nullptr)
        : builder_(builder), circuit_(circuit), ctx_(ctx), log_(log), methodName_(methodName),
          methodInfo_(methodInfo), hasTypes_(hasTypes), recordName_(recordName), methodLiteral_(methodLiteral),
          methodOffset_(methodOffset), allocator_(allocator)
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

    bool IsTypeAbort() const
    {
        if (hasTypes_) {
            // A ts method which has low type percent and not marked as a resolved method
            // should be skipped from full compilation.
            if (methodInfo_->IsTypeInferAbort() && !methodInfo_->IsResolvedMethod()) {
                ctx_->GetBytecodeInfo().AddSkippedMethod(methodOffset_);
                return true;
            }
        } else {
            // For js method, type infer pass will be skipped and it don't have a type percent.
            // If we set an non zero type threshold, js method will be skipped from full compilation.
            // The default Type threshold is -1.
            if (ctx_->GetTSManager()->GetTypeThreshold() >= 0) {
                ctx_->GetBytecodeInfo().AddSkippedMethod(methodOffset_);
                return true;
            }
        }
        // when a method will be full compiled, we should confirm its TypeInferAbortBit to be false
        // maybe it used to be true in the first round of compilation.
        methodInfo_->SetTypeInferAbort(false);
        log_->AddCompiledMethod(methodName_, recordName_);
        return false;
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
        TimeScope timescope("TypeInferPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        if (data->HasTypes()) {
            bool enableLog = data->GetLog()->GetEnableMethodLog() && data->GetLog()->OutputType();
            TypeInfer typeInfer(data->GetBuilder(), data->GetCircuit(), data->GetPassContext(),
                                data->GetMethodInfoIndex(), enableLog, data->GetMethodName(),
                                data->GetRecordName(), data->GetMethodInfo());
            typeInfer.TraverseCircuit();
        }
        return true;
    }
};

class TSTypeLoweringPass {
public:
    bool Run(PassData *data)
    {
        TimeScope timescope("TSTypeLoweringPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        TSTypeLowering lowering(data->GetCircuit(), data->GetPassContext(), enableLog, data->GetMethodName());
        lowering.RunTSTypeLowering();
        return true;
    }
};

class TypeLoweringPass {
public:
    bool Run(PassData *data)
    {
        TimeScope timescope("TypeLoweringPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        TypeLowering lowering(data->GetCircuit(), data->GetCompilerConfig(), data->GetTSManager(),
                              enableLog, data->GetMethodName());
        lowering.RunTypeLowering();
        return true;
    }
};

class GenericTypeLoweringPass {
public:
    bool Run(PassData *data)
    {
        TimeScope timescope("GenericTypeLoweringPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        GenericTypeLowering lowering(data->GetCircuit(), data->GetCompilerConfig(),
                                     enableLog, data->GetMethodName());
        lowering.Run();
        return true;
    }
};

class TSInlineLoweringPass {
public:
    bool Run(PassData *data)
    {
        TimeScope timescope("TSInlineLoweringPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        TSInlineLowering inlining(data->GetCircuit(), data->GetPassContext(), enableLog, data->GetMethodName());
        inlining.RunTSInlineLowering();
        return true;
    }
};

class SlowPathLoweringPass {
public:
    bool Run(PassData* data)
    {
        TimeScope timescope("SlowPathLoweringPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        SlowPathLowering lowering(data->GetCircuit(), data->GetCompilerConfig(), data->GetTSManager(),
                                  data->GetMethodLiteral(), enableLog, data->GetMethodName());
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
        TimeScope timescope("NumberSpeculativePass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        Chunk chunk(data->GetNativeAreaAllocator());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        NumberSpeculativeRunner(data->GetCircuit(), data->GetTSManager(),
                                enableLog, data->GetMethodName(), &chunk).Run();
        return true;
    }
};

class EarlyEliminationPass {
public:
    bool Run(PassData* data)
    {
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
        TimeScope timescope("LaterEliminationPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        Chunk chunk(data->GetNativeAreaAllocator());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        LaterElimination(data->GetCircuit(), enableLog, data->GetMethodName(), &chunk).Run();
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
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        CreateCodeGen(module, enableLog);
        CodeGenerator codegen(llvmImpl_, data->GetMethodName());
        codegen.Run(data->GetCircuit(), data->GetConstScheduleResult(), module->GetCompilationConfig(),
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
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
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
