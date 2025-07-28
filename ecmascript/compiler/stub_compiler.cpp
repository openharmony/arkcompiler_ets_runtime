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

#include "ecmascript/compiler/stub_compiler.h"

#include "ecmascript/compiler/pass.h"
#include "ecmascript/compiler/stub.h"
#include "ecmascript/js_runtime_options.h"

namespace panda::ecmascript::kungfu {
class StubPassData : public PassData {
public:
    StubPassData(Stub *stub, LLVMModule *module, CompilerLog *log)
        : PassData(nullptr, nullptr, nullptr, log, "stubs"),
          cfg_(module->GetTripleStr()),
          module_(module),
          stub_(stub) {}
    ~StubPassData() = default;

    const CompilationConfig *GetCompilationConfig() const
    {
        return &cfg_;
    }

    Circuit *GetCircuit() const
    {
        return stub_->GetCircuit();
    }

    LLVMModule *GetStubModule() const
    {
        return module_;
    }

    Stub *GetStub() const
    {
        return stub_;
    }

private:
    CompilationConfig cfg_;
    LLVMModule *module_;
    Stub *stub_;
};

class StubBuildCircuitPass {
public:
    bool Run(StubPassData *data)
    {
        auto stub = data->GetStub();
        LOG_COMPILER(INFO) << "Stub Name: " << stub->GetMethodName();
        stub->GenerateCircuit(data->GetCompilationConfig());
        return true;
    }
};

class StubLLVMIRGenPass {
public:
    void CreateCodeGen(LLVMModule *module, bool enableLog)
    {
        llvmImpl_ = std::make_unique<LLVMIRGeneratorImpl>(module, enableLog);
    }

    bool Run(StubPassData *data, size_t index)
    {
        bool enableLog = data->GetLog()->EnableMethodCIRLog() || data->GetLog()->OutputASM();
        auto stubModule = data->GetStubModule();
        CreateCodeGen(stubModule, enableLog);
        CodeGenerator codegen(llvmImpl_, "stubs");
        codegen.RunForStub(data->GetCircuit(), data->GetConstScheduleResult(), index, data->GetCompilationConfig());
        return true;
    }
private:
    std::unique_ptr<CodeGeneratorImpl> llvmImpl_ {nullptr};
};

void StubCompiler::RunPipeline(LLVMModule *module, NativeAreaAllocator *allocator) const
{
    auto callSigns = module->GetCSigns();
    CompilerLog *log = GetLog();
    for (size_t i = 0; i < callSigns.size(); i++) {
        auto &cs = callSigns[i];
        Circuit circuit(allocator, module->GetDebugInfo(), cs->GetName().c_str(), module->Is64Bit());
        Stub stub(cs, &circuit);
        log->SetStubLog(stub.GetMethodName(), GetLogList());

        StubPassData data(&stub, module, log);
        PassRunner<StubPassData> pipeline(&data);
        pipeline.RunPass<StubBuildCircuitPass>();
        pipeline.RunPass<VerifierPass>();
        pipeline.RunPass<SchedulingPass>(cs->IsStwCopyStub());
        pipeline.RunPass<StubLLVMIRGenPass>(i);
    }
}

void StubCompiler::InitializeCS() const
{
    BytecodeStubCSigns::Initialize();
    CommonStubCSigns::Initialize();
    BaselineStubCSigns::Initialize();
    BuiltinsStubCSigns::Initialize();
    RuntimeStubCSigns::Initialize();
}

bool StubCompiler::BuildStubModuleAndSave() const
{
    if (filePath_.empty()) {
        return false;
    }

    InitializeCS();
    CompilerLog *log = GetLog();
    MethodLogList *logList = GetLogList();

    NativeAreaAllocator allocator;
    StubFileGenerator generator(log, logList, triple_, concurrentCompile_);

    LOG_COMPILER(INFO) << "=============== compiling bytecode handler stubs ===============";
    LOptions stubOp(optLevel_, FPFlag::ELIM_FP, relocMode_);
    Module* stubM = generator.AddModule(&allocator, "bc_stub", triple_, stubOp, log->OutputASM(), StubFileKind::BC);
    if (!stubM->IsLLVM()) {
        LOG_COMPILER(FATAL) << " Stub compiler is not supported for litecg ===============";
        return false;
    }
    RunPipeline(static_cast<LLVMModule*>(stubM->GetModule()), &allocator);

    LOG_COMPILER(INFO) << "=============== compiling bytecode stwcopy handler stubs ===============";
    LOptions stubStwCopyOp(optLevel_, FPFlag::ELIM_FP, relocMode_);
    Module* stubStwCopyM = generator.AddModule(&allocator, "bc_stub_stw_copy", triple_, stubStwCopyOp,
                                               log->OutputASM(), StubFileKind::BC_STW_COPY);
    if (!stubStwCopyM->IsLLVM()) {
        LOG_COMPILER(FATAL) << " Stub compiler is not supported for litecg ===============";
        return false;
    }
    RunPipeline(static_cast<LLVMModule*>(stubStwCopyM->GetModule()), &allocator);

    LOG_COMPILER(INFO) << "=============== compiling common stubs ===============";
    LOptions comOp(optLevel_, FPFlag::RESERVE_FP, relocMode_);
    Module* comM = generator.AddModule(&allocator, "com_stub", triple_, comOp, log->OutputASM(), StubFileKind::COM);
    if (!comM->IsLLVM()) {
        LOG_COMPILER(FATAL) << " Stub compiler is not supported for litecg ===============";
        return false;
    }
    RunPipeline(static_cast<LLVMModule*>(comM->GetModule()), &allocator);

    LOG_COMPILER(INFO) << "=============== compiling builtins stubs ===============";
    LOptions builtinOp(optLevel_, FPFlag::RESERVE_FP, relocMode_);
    Module* builtinM = generator.AddModule(&allocator, "builtin_stub", triple_, builtinOp, log->OutputASM(),
                                           StubFileKind::BUILTIN);
    if (!builtinM->IsLLVM()) {
        LOG_COMPILER(FATAL) << " Stub compiler is not supported for litecg ===============";
        return false;
    }
    RunPipeline(static_cast<LLVMModule*>(builtinM->GetModule()), &allocator);

    LOG_COMPILER(INFO) << "=============== compiling baseline stubs ===============";
    LOptions baselineOp(optLevel_, FPFlag::RESERVE_FP, relocMode_);
    Module* baselineM = generator.AddModule(&allocator, "baseline_stub", triple_, baselineOp, log->OutputASM(),
                                            StubFileKind::BASELINE);
    if (!baselineM->IsLLVM()) {
        LOG_COMPILER(FATAL) << " Stub compiler is not supported for litecg ===============";
        return false;
    }
    RunPipeline(static_cast<LLVMModule*>(baselineM->GetModule()), &allocator);

    generator.SaveStubFile(filePath_);
    return true;
}

std::string GetHelper()
{
    std::string str;
    str.append(STUB_HELP_HEAD_MSG);
    str.append(HELP_OPTION_MSG);
    return str;
}
}  // namespace panda::ecmascript::kungfu

int main(const int argc, const char **argv)
{
    panda::ecmascript::JSRuntimeOptions runtimeOptions;
    bool ret = runtimeOptions.ParseCommand(argc, argv);
    if (!ret) {
        std::cerr << panda::ecmascript::kungfu::GetHelper();
        return 1;
    }

    common::Log::Initialize(runtimeOptions.GetLogOptions());
    std::string triple = runtimeOptions.GetTargetTriple();
    std::string stubFile = runtimeOptions.GetStubFile();
    size_t optLevel = runtimeOptions.GetOptLevel();
    size_t relocMode = runtimeOptions.GetRelocMode();
    std::string logOption = runtimeOptions.GetCompilerLogOption();
    std::string methodsList = runtimeOptions.GetMethodsListForLog();
    bool concurrentCompile = runtimeOptions.IsConcurrentCompile();

    panda::ecmascript::kungfu::CompilerLog logOpt(logOption);
    panda::ecmascript::kungfu::MethodLogList logList(methodsList);
    panda::ecmascript::kungfu::StubCompiler compiler(triple, stubFile, optLevel, relocMode, &logOpt, &logList,
                                                     concurrentCompile);

    bool res = compiler.BuildStubModuleAndSave();
    LOG_COMPILER(INFO) << "stub compiler run finish, result condition(T/F):" << std::boolalpha << res;
    return res ? 0 : -1;
}
