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

#include "ecmascript/compiler/bytecode_info_collector.h"
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/pass.h"
#include "ecmascript/ecma_handle_scope.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/panda_file_translator.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/ts_types/ts_manager.h"

namespace panda::ecmascript::kungfu {

bool PassManager::Compile(const std::string &fileName, AOTFileGenerator &generator, const std::string &profilerIn)
{
    [[maybe_unused]] EcmaHandleScope handleScope(vm_->GetJSThread());
    JSPandaFile *jsPandaFile = CreateAndVerifyJSPandaFile(fileName.c_str());
    if (jsPandaFile == nullptr) {
        LOG_COMPILER(ERROR) << "Cannot execute panda file '" << fileName << "'";
        return false;
    }

    profilerLoader_.LoadProfiler(profilerIn, hotnessThreshold_);
    BytecodeInfoCollector bcInfoCollector(jsPandaFile, profilerLoader_, maxAotMethodSize_);
    ResolveModuleAndConstPool(jsPandaFile, fileName);
    auto aotModule = new LLVMModule(fileName, triple_);
    auto aotModuleAssembler = new LLVMAssembler(aotModule->GetModule(),
                                                LOptions(optLevel_, true, relocMode_));

    CompilationConfig cmpCfg(triple_, false, log_->IsEnableByteCodeTrace());
    Bytecodes bytecodes;
    auto &bytecodeInfo = bcInfoCollector.GetBytecodeInfo();
    auto lexEnvManager = LexEnvManager(bytecodeInfo);
    bool enableMethodLog = !log_->NoneMethod();

    // ts type system
    TSManager *tsManager = vm_->GetTSManager();
    CompilationInfo info(tsManager, &bytecodes, &lexEnvManager, &cmpCfg, log_,
        jsPandaFile, &bcInfoCollector, aotModule);

    bytecodeInfo.EnumerateBCInfo(jsPandaFile, [this, &fileName, &enableMethodLog, &info]
        (const CString &recordName, const std::string &methodName, MethodLiteral *methodLiteral,
         uint32_t methodOffset, MethodPcInfo &methodPCInfo, size_t methodInfoIndex) {
        auto jsPandaFile = info.GetJSPandaFile();
        auto cmpCfg = info.GetCompilerConfig();
        auto tsManager = info.GetTSManager();

        if (log_->CertainMethod()) {
            enableMethodLog = logList_->IncludesMethod(fileName, methodName);
        }
        log_->SetEnableMethodLog(enableMethodLog);

        std::string fullName = methodName + "@" + fileName;
        if (enableMethodLog) {
            LOG_COMPILER(INFO) << "\033[34m" << "aot method [" << fullName << "] log:" << "\033[0m";
        }

        bool hasTypes = jsPandaFile->HasTSTypes(recordName);
        Circuit circuit(cmpCfg->Is64Bit());
        BytecodeCircuitBuilder builder(jsPandaFile, methodLiteral, methodPCInfo, tsManager, &circuit,
                                       info.GetByteCodes(), hasTypes, enableMethodLog && log_->OutputCIR(),
                                       EnableTypeLowering(), fullName, recordName);
        {
            TimeScope timeScope("BytecodeToCircuit", methodName, methodOffset, log_);
            builder.BytecodeToCircuit();
        }

        PassData data(&builder, &circuit, &info, log_, fullName,
                      methodInfoIndex, hasTypes, recordName,
                      methodLiteral, methodOffset);
        PassRunner<PassData> pipeline(&data);
        if (EnableTypeInfer()) {
            pipeline.RunPass<TypeInferPass>();
        }
        pipeline.RunPass<AsyncFunctionLoweringPass>();
        if (EnableTypeLowering()) {
            pipeline.RunPass<TSTypeLoweringPass>();
            pipeline.RunPass<GuardEliminatingPass>();
            pipeline.RunPass<GuardLoweringPass>();
            pipeline.RunPass<TypeLoweringPass>();
        }
        pipeline.RunPass<SlowPathLoweringPass>();
        pipeline.RunPass<VerifierPass>();
        pipeline.RunPass<SchedulingPass>();
        pipeline.RunPass<LLVMIRGenPass>();
    });
    LOG_COMPILER(INFO) << bytecodeInfo.GetSkippedMethodSize() << " methods in "
                       << fileName << " have been skipped";
    if (log_->GetEnableCompilerLogTime()) {
        log_->PrintTime();
    }
    generator.AddModule(aotModule, aotModuleAssembler, &bcInfoCollector);
    return true;
}

JSPandaFile *PassManager::CreateAndVerifyJSPandaFile(const CString &fileName)
{
    JSPandaFileManager *jsPandaFileManager = JSPandaFileManager::GetInstance();
    JSPandaFile *jsPandaFile = jsPandaFileManager->OpenJSPandaFile(fileName);
    if (jsPandaFile == nullptr) {
        LOG_ECMA(ERROR) << "open file " << fileName << " error";
        return nullptr;
    }

    if (!jsPandaFile->IsNewVersion()) {
        LOG_COMPILER(ERROR) << "AOT only support panda file with new ISA, while the '" <<
            fileName << "' file is the old version";
        return nullptr;
    }

    JSPandaFileManager::GetInstance()->InsertJSPandaFile(jsPandaFile);
    return jsPandaFile;
}

void PassManager::ResolveModuleAndConstPool(const JSPandaFile *jsPandaFile, const std::string &fileName)
{
    JSThread *thread = vm_->GetJSThread();

    JSHandle<Program> program;
    const auto &recordInfo = jsPandaFile->GetJSRecordInfo();
    ModuleManager *moduleManager = vm_->GetModuleManager();
    for (auto info: recordInfo) {
        auto recordName = info.first;
        if (jsPandaFile->IsModule(recordName)) {
            moduleManager->HostResolveImportedModuleWithMerge(fileName.c_str(), recordName);
        }
        program = PandaFileTranslator::GenerateProgram(vm_, jsPandaFile, recordName);
    }

    JSHandle<JSFunction> mainFunc(thread, program->GetMainFunction());
    JSHandle<Method> method(thread, mainFunc->GetMethod());
    JSHandle<JSTaggedValue> constPool(thread, method->GetConstantPool());
    TSManager *tsManager = vm_->GetTSManager();
    tsManager->InitSnapshotConstantPool(constPool.GetTaggedValue());
}
} // namespace panda::ecmascript::kungfu
