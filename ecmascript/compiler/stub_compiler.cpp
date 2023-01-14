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

#include "ecmascript/compiler/stub_compiler.h"

#include "ecmascript/base/config.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/file_generators.h"
#include "ecmascript/compiler/interpreter_stub-inl.h"
#include "ecmascript/compiler/llvm_codegen.h"
#include "ecmascript/compiler/pass.h"
#include "ecmascript/compiler/scheduler.h"
#include "ecmascript/compiler/stub.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/log.h"
#include "ecmascript/napi/include/jsnapi.h"

namespace panda::ecmascript::kungfu {
class StubPassData : public PassData {
public:
    explicit StubPassData(Stub *stub, LLVMModule *module, CompilerLog *log)
        : PassData(nullptr, nullptr, nullptr, log, "stubs"), module_(module), stub_(stub) {}
    ~StubPassData() = default;

    const CompilationConfig *GetCompilationConfig() const
    {
        return module_->GetCompilationConfig();
    }

    Circuit *GetCircuit() const
    {
        return stub_->GetEnvironment()->GetCircuit();
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
        bool enableLog =  data->GetLog()->GetEnableMethodLog() && data->GetLog()->OutputCIR();
        auto stubModule = data->GetStubModule();
        CreateCodeGen(stubModule, enableLog);
        CodeGenerator codegen(llvmImpl_, "stubs");
        codegen.RunForStub(data->GetCircuit(), data->GetConstScheduleResult(), index, data->GetCompilationConfig());
        return true;
    }
private:
    std::unique_ptr<CodeGeneratorImpl> llvmImpl_ {nullptr};
};

void StubCompiler::RunPipeline(LLVMModule *module) const
{
    auto callSigns = module->GetCSigns();
    CompilerLog *log = GetLog();
    auto logList = GetLogList();
    auto cconfig = module->GetCompilationConfig();
    NativeAreaAllocator allocator;

    bool enableMethodLog = !log->NoneMethod();
    for (size_t i = 0; i < callSigns.size(); i++) {
        Circuit circuit(&allocator, cconfig->Is64Bit());
        Stub stub(callSigns[i], &circuit);
        ASSERT(callSigns[i]->HasConstructor());
        void* env = reinterpret_cast<void*>(stub.GetEnvironment());
        StubBuilder* stubBuilder = static_cast<StubBuilder*>(callSigns[i]->GetConstructor()(env));
        stub.SetStubBuilder(stubBuilder);

        if (log->CertainMethod()) {
            enableMethodLog = logList->IncludesMethod(stub.GetMethodName());
        }
        log->SetEnableMethodLog(enableMethodLog);

        StubPassData data(&stub, module, log);
        PassRunner<StubPassData> pipeline(&data);
        pipeline.RunPass<StubBuildCircuitPass>();
        pipeline.RunPass<VerifierPass>();
        pipeline.RunPass<SchedulingPass>();
        pipeline.RunPass<StubLLVMIRGenPass>(i);
        delete stubBuilder;
    }
}

void StubCompiler::InitializeCS() const
{
    BytecodeStubCSigns::Initialize();
    CommonStubCSigns::Initialize();
    BuiltinsStubCSigns::Initialize();
    RuntimeStubCSigns::Initialize();
}

bool StubCompiler::BuildStubModuleAndSave() const
{
    InitializeCS();
    size_t res = 0;
    CompilerLog *log = GetLog();
    const MethodLogList *logList = GetLogList();
    StubFileGenerator generator(log, logList, triple_, enablePGOProfiler_);
    if (!filePath_.empty()) {
        LOG_COMPILER(INFO) << "compiling bytecode handler stubs";
        LLVMModule bcStubModule("bc_stub", triple_, enablePGOProfiler_);
        LLVMAssembler bcStubAssembler(bcStubModule.GetModule(), LOptions(optLevel_, false, relocMode_));
        bcStubModule.SetUpForBytecodeHandlerStubs();
        RunPipeline(&bcStubModule);
        generator.AddModule(&bcStubModule, &bcStubAssembler);
        res++;

        LOG_COMPILER(INFO) << "compiling common stubs";
        LLVMModule comStubModule("com_stub", triple_, enablePGOProfiler_);
        LLVMAssembler comStubAssembler(comStubModule.GetModule(), LOptions(optLevel_, true, relocMode_));
        comStubModule.SetUpForCommonStubs();
        RunPipeline(&comStubModule);
        generator.AddModule(&comStubModule, &comStubAssembler);
        res++;

        LOG_COMPILER(INFO) << "compiling builtins stubs";
        LLVMModule builtinsStubModule("builtins_stub", triple_, enablePGOProfiler_);
        LLVMAssembler builtinsStubAssembler(builtinsStubModule.GetModule(), LOptions(optLevel_, true, relocMode_));
        builtinsStubModule.SetUpForBuiltinsStubs();
        RunPipeline(&builtinsStubModule);
        generator.AddModule(&builtinsStubModule, &builtinsStubAssembler);
        res++;
        generator.SaveStubFile(filePath_);
    }
    return (res > 0);
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

    panda::ecmascript::Log::Initialize(runtimeOptions);
    std::string triple = runtimeOptions.GetTargetTriple();
    std::string stubFile = runtimeOptions.GetStubFile();
    size_t optLevel = runtimeOptions.GetOptLevel();
    size_t relocMode = runtimeOptions.GetRelocMode();
    std::string logOption = runtimeOptions.GetCompilerLogOption();
    std::string methodsList = runtimeOptions.GetMethodsListForLog();
    bool enablePGOProfiler = runtimeOptions.IsEnablePGOProfiler();
    panda::ecmascript::kungfu::CompilerLog logOpt(logOption);
    panda::ecmascript::kungfu::MethodLogList logList(methodsList);
    panda::ecmascript::kungfu::StubCompiler compiler(
        triple, stubFile, optLevel, relocMode, &logOpt, &logList, enablePGOProfiler);

    bool res = compiler.BuildStubModuleAndSave();
    LOG_COMPILER(INFO) << "stub compiler run finish, result condition(T/F):" << std::boolalpha << res;
    return res ? 0 : -1;
}
