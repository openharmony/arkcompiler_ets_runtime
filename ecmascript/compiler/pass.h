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
#include "ecmascript/compiler/guard_eliminating.h"
#include "ecmascript/compiler/guard_lowering.h"
#include "ecmascript/compiler/llvm_codegen.h"
#include "ecmascript/compiler/scheduler.h"
#include "ecmascript/compiler/slowpath_lowering.h"
#include "ecmascript/compiler/ts_inline_lowering.h"
#include "ecmascript/compiler/ts_type_lowering.h"
#include "ecmascript/compiler/type_inference/type_infer.h"
#include "ecmascript/compiler/type_lowering.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/compiler/compiler_log.h"

namespace panda::ecmascript::kungfu {
struct CompilationInfo;
class PassData {
public:
    explicit PassData(Circuit *circuit, CompilerLog *log, std::string methodName, uint32_t methodOffset = 0)
        : circuit_(circuit), log_(log), methodName_(methodName), methodOffset_(methodOffset)
    {
    }

    virtual ~PassData() = default;
    const ControlFlowGraph &GetScheduleResult() const
    {
        return cfg_;
    }

    void SetScheduleResult(const ControlFlowGraph &result)
    {
        cfg_ = result;
    }

    virtual Circuit* GetCircuit() const
    {
        return circuit_;
    }

    CompilerLog *GetLog() const
    {
        return log_;
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    uint32_t GetMethodOffset() const
    {
        return methodOffset_;
    }

private:
    Circuit *circuit_ {nullptr};
    ControlFlowGraph cfg_;
    CompilerLog *log_ {nullptr};
    std::string methodName_;
    uint32_t methodOffset_;
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
    bool Run(PassData* data, BytecodeCircuitBuilder *builder, CompilationInfo *info, size_t methodId, bool hasTypes)
    {
        TimeScope timescope("TypeInferPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        if (hasTypes) {
            bool enableLog = data->GetLog()->GetEnableMethodLog() && data->GetLog()->OutputType();
            TypeInfer typeInfer(builder, data->GetCircuit(), info, methodId, enableLog, data->GetMethodName());
            typeInfer.TraverseCircuit();
        }
        return true;
    }
};

class TSTypeLoweringPass {
public:
    bool Run(PassData *data, CompilationInfo *info)
    {
        TimeScope timescope("TSTypeLoweringPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        TSTypeLowering lowering(data->GetCircuit(), info, enableLog,
                                data->GetMethodName());
        lowering.RunTSTypeLowering();
        return true;
    }
};

class TypeLoweringPass {
public:
    bool Run(PassData *data, CompilationConfig *cmpCfg, TSManager *tsManager)
    {
        TimeScope timescope("TypeLoweringPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        TypeLowering lowering(data->GetCircuit(), cmpCfg, tsManager, enableLog, data->GetMethodName());
        lowering.RunTypeLowering();
        return true;
    }
};

class TSInlineLoweringPass {
public:
    bool Run(PassData *data, CompilationInfo *info)
    {
        TimeScope timescope("TSInlineLoweringPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        TSInlineLowering inlining(data->GetCircuit(), info, enableLog, data->GetMethodName());
        inlining.RunTSInlineLowering();
        return true;
    }
};

class SlowPathLoweringPass {
public:
    bool Run(PassData* data, CompilationConfig *cmpCfg, TSManager *tsManager, const MethodLiteral *methodLiteral)
    {
        TimeScope timescope("SlowPathLoweringPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        SlowPathLowering lowering(data->GetCircuit(), cmpCfg, tsManager, methodLiteral,
                                  enableLog, data->GetMethodName());
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

class GuardEliminatingPass {
public:
    bool Run(PassData* data, CompilationConfig *cmpCfg)
    {
        TimeScope timescope("GuardEliminatingPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        GuardEliminating(data->GetCircuit(), cmpCfg, enableLog, data->GetMethodName()).Run();
        return true;
    }
};

class SchedulingPass {
public:
    bool Run(PassData* data)
    {
        TimeScope timescope("SchedulingPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        data->SetScheduleResult(Scheduler::Run(data->GetCircuit(), data->GetMethodName(), enableLog));
        return true;
    }
};

class LLVMIRGenPass {
public:
    void CreateCodeGen(LLVMModule *module, bool enableLog)
    {
        llvmImpl_ = std::make_unique<LLVMIRGeneratorImpl>(module, enableLog);
    }

    bool Run(PassData *data, LLVMModule *module, const MethodLiteral *methodLiteral,
             const JSPandaFile *jsPandaFile)
    {
        TimeScope timescope("LLVMIRGenPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        CreateCodeGen(module, enableLog);
        CodeGenerator codegen(llvmImpl_, data->GetMethodName());
        codegen.Run(data->GetCircuit(), data->GetScheduleResult(), module->GetCompilationConfig(),
                    methodLiteral, jsPandaFile);
        return true;
    }
private:
    std::unique_ptr<CodeGeneratorImpl> llvmImpl_ {nullptr};
};

class AsyncFunctionLoweringPass {
public:
    bool Run(PassData* data, BytecodeCircuitBuilder *builder, CompilationConfig *cmpCfg)
    {
        TimeScope timescope("AsyncFunctionLoweringPass", data->GetMethodName(),
                            data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        AsyncFunctionLowering lowering(builder, data->GetCircuit(), cmpCfg, enableLog, data->GetMethodName());
        if (lowering.IsAsyncRelated()) {
            lowering.ProcessAll();
        }
        return true;
    }
};

class GuardLoweringPass {
public:
    bool Run(PassData* data, CompilationConfig *cmpCfg)
    {
        TimeScope timescope("GuardLoweringPass", data->GetMethodName(), data->GetMethodOffset(), data->GetLog());
        bool enableLog = data->GetLog()->EnableMethodCIRLog();
        GuardLowering(cmpCfg, data->GetCircuit(), data->GetMethodName(), enableLog).Run();
        return true;
    }
};
} // namespace panda::ecmascript::kungfu
#endif
