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
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/file_generators.h"
#include "ecmascript/compiler/interpreter_stub-inl.h"
#include "ecmascript/compiler/llvm_codegen.h"
#include "ecmascript/compiler/pass.h"
#include "ecmascript/compiler/scheduler.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/napi/include/jsnapi.h"

#include "generated/base_options.h"
#include "libpandabase/utils/pandargs.h"
#include "libpandabase/utils/span.h"

namespace panda::ecmascript::kungfu {
class StubPassData : public PassData {
public:
    explicit StubPassData(StubBuilder *stubBuilder, LLVMModule *module)
        : PassData(nullptr), module_(module), stubBuilder_(stubBuilder) {}
    ~StubPassData() = default;

    const CompilationConfig *GetCompilationConfig() const
    {
        return module_->GetCompilationConfig();
    }

    Circuit *GetCircuit() const
    {
        return stubBuilder_->GetEnvironment()->GetCircuit();
    }

    LLVMModule *GetStubModule() const
    {
        return module_;
    }

    StubBuilder *GetStubBuilder() const
    {
        return stubBuilder_;
    }

private:
    LLVMModule *module_;
    StubBuilder *stubBuilder_;
};

class StubBuildCircuitPass {
public:
    bool Run(StubPassData *data, [[maybe_unused]] bool enableLog)
    {
        auto stub = data->GetStubBuilder();
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
    bool Run(StubPassData *data, bool enableLog, size_t index)
    {
        auto stubModule = data->GetStubModule();
        CreateCodeGen(stubModule, enableLog);
        CodeGenerator codegen(llvmImpl_);
        codegen.RunForStub(data->GetCircuit(), data->GetScheduleResult(), index, data->GetCompilationConfig());
        return true;
    }
private:
    std::unique_ptr<CodeGeneratorImpl> llvmImpl_ {nullptr};
};

void StubCompiler::RunPipeline(LLVMModule *module) const
{
    auto callSigns = module->GetCSigns();
    const CompilerLog *log = GetLog();
    bool enableLog = log->IsAlwaysEnabled();

    for (size_t i = 0; i < callSigns.size(); i++) {
        Circuit circuit;
        ASSERT(callSigns[i]->HasConstructor());
        StubBuilder* stubBuilder = static_cast<StubBuilder*>(callSigns[i]->GetConstructor()(reinterpret_cast<void*>(&circuit)));

        if (!log->IsAlwaysEnabled() && !log->IsAlwaysDisabled()) {  // neither "all" nor "none"
            enableLog = log->IncludesMethod(stubBuilder->GetMethodName());
        }

        StubPassData data(stubBuilder, module);
        PassRunner<StubPassData> pipeline(&data, enableLog);
        pipeline.RunPass<StubBuildCircuitPass>();
        pipeline.RunPass<VerifierPass>();
        pipeline.RunPass<SchedulingPass>();
        pipeline.RunPass<StubLLVMIRGenPass>(i);
        delete stubBuilder;
    }
}

bool StubCompiler::BuildStubModuleAndSave() const
{
    BytecodeStubCSigns::Initialize();
    CommonStubCSigns::Initialize();
    RuntimeStubCSigns::Initialize();
    size_t res = 0;
    const CompilerLog *log = GetLog();
    StubFileGenerator generator(log, triple_);
    if (!filePath_.empty()) {
        LOG_COMPILER(INFO) << "compiling bytecode handler stubs";
        LLVMModule bcStubModule("bc_stub", triple_);
        LLVMAssembler bcStubAssembler(bcStubModule.GetModule(), LOptions(optLevel_, false, relocMode_));
        bcStubModule.SetUpForBytecodeHandlerStubs();
        RunPipeline(&bcStubModule);
        generator.AddModule(&bcStubModule, &bcStubAssembler);
        res++;
        LOG_COMPILER(INFO) << "compiling common stubs";
        LLVMModule comStubModule("com_stub", triple_);
        LLVMAssembler comStubAssembler(comStubModule.GetModule(), LOptions(optLevel_, true, relocMode_));
        comStubModule.SetUpForCommonStubs();
        RunPipeline(&comStubModule);
        generator.AddModule(&comStubModule, &comStubAssembler);
        res++;
        generator.SaveStubFile(filePath_);
    }
    return (res > 0);
}
}  // namespace panda::ecmascript::kungfu

int main(const int argc, const char **argv)
{
    panda::Span<const char *> sp(argv, argc);
    panda::ecmascript::JSRuntimeOptions runtimeOptions;
    panda::base_options::Options baseOptions(sp[0]);
    panda::PandArg<bool> help("help", false, "Print this message and exit");
    panda::PandArg<bool> options("options", false, "Print options");
    panda::PandArgParser paParser;

    runtimeOptions.AddOptions(&paParser);
    baseOptions.AddOptions(&paParser);

    paParser.Add(&help);
    paParser.Add(&options);

    if (!paParser.Parse(argc, argv) || help.GetValue()) {
        std::cerr << paParser.GetErrorString() << std::endl;
        std::cerr << "Usage: " << "ark_stub_compiler" << " [OPTIONS]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "optional arguments:" << std::endl;

        std::cerr << paParser.GetHelpString() << std::endl;
        return 1;
    }

    panda::Logger::Initialize(baseOptions);
    panda::Logger::SetLevel(panda::Logger::Level::INFO);
    panda::Logger::ResetComponentMask();  // disable all Component
    panda::Logger::EnableComponent(panda::Logger::Component::ECMASCRIPT);  // enable ECMASCRIPT

    std::string triple = runtimeOptions.GetTargetTriple();
    std::string stubFile = runtimeOptions.GetStubFile();
    size_t optLevel = runtimeOptions.GetOptLevel();
    size_t relocMode = runtimeOptions.GetRelocMode();
    std::string logMethods = runtimeOptions.GetlogCompiledMethods();
    panda::ecmascript::kungfu::CompilerLog log(logMethods);
    panda::ecmascript::kungfu::StubCompiler compiler(triple, stubFile, optLevel, relocMode, &log);

    bool res = compiler.BuildStubModuleAndSave();
    LOG_COMPILER(INFO) << "stub compiler run finish, result condition(T/F):" << std::boolalpha << res;
    return res ? 0 : -1;
}
