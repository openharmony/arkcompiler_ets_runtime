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
    JSPandaFile *jsPandaFile = CreateJSPandaFile(fileName.c_str());
    if (jsPandaFile == nullptr) {
        LOG_COMPILER(ERROR) << "Cannot execute panda file '" << fileName << "'";
        return false;
    }

    auto bcInfoCollector = BytecodeInfoCollector(jsPandaFile);
    auto constantPool = ResolveModuleAndConstPool(jsPandaFile, fileName);

    // ts type system
    TSManager *tsManager = vm_->GetTSManager();
    tsManager->DecodeTSTypes(jsPandaFile);
    tsManager->GenerateSnapshotConstantPool(constantPool.GetTaggedValue());

    auto aotModule = new LLVMModule(fileName, triple_);
    auto aotModuleAssembler = new LLVMAssembler(aotModule->GetModule(),
                                                LOptions(optLevel_, true, relocMode_));
    CompilationConfig cmpCfg(triple_, false, log_->IsEnableByteCodeTrace());
    Bytecodes bytecodes;

    auto &bytecodeInfo = bcInfoCollector.GetBytecodeInfo();
    auto lexEnvManager = LexEnvManager(bytecodeInfo);
    bool enableMethodLog = !log_->NoneMethod();
    uint32_t skippedMethodNum = 0;
    profilerLoader_.LoadProfiler(profilerIn, hotnessThreshold_);

    bytecodeInfo.EnumerateBCInfo([this, &fileName, &enableMethodLog, aotModule, jsPandaFile, constantPool,
        &cmpCfg, tsManager, &bytecodes, &lexEnvManager, &skippedMethodNum]
        (const CString &recordName, uint32_t methodOffset, MethodPcInfo &methodPCInfo, size_t methodInfoId) {
        auto method = jsPandaFile->FindMethodLiteral(methodOffset);
        const std::string methodName(MethodLiteral::GetMethodName(jsPandaFile, method->GetMethodId()));
        if (FilterMethod(jsPandaFile, recordName, method, methodOffset, methodPCInfo)) {
            ++skippedMethodNum;
            tsManager->AddIndexOrSkippedMethodID(TSManager::SnapshotInfoType::SKIPPED_METHOD,
                                                 method->GetMethodId().GetOffset());
            LOG_COMPILER(INFO) << " method " << methodName << " has been skipped";
            return;
        }

        if (log_->CertainMethod()) {
            enableMethodLog = logList_->IncludesMethod(fileName, methodName);
        }

        std::string fullName = methodName + "@" + fileName;
        if (enableMethodLog) {
            LOG_COMPILER(INFO) << "\033[34m" << "aot method [" << fullName << "] log:" << "\033[0m";
        }

        bool hasTyps = jsPandaFile->HasTSTypes(recordName);
        BytecodeCircuitBuilder builder(jsPandaFile, method, methodPCInfo, tsManager, &bytecodes,
                                       &cmpCfg, hasTyps, enableMethodLog && log_->OutputCIR(),
                                       EnableTypeLowering(), fullName, recordName);
        builder.BytecodeToCircuit();
        PassData data(builder.GetCircuit(), log_, enableMethodLog, fullName);
        PassRunner<PassData> pipeline(&data);
        pipeline.RunPass<TypeInferPass>(&builder, constantPool, tsManager, &lexEnvManager, methodInfoId);
        pipeline.RunPass<AsyncFunctionLoweringPass>(&builder, &cmpCfg);
        if (EnableTypeLowering()) {
            pipeline.RunPass<TSTypeLoweringPass>(&builder, &cmpCfg, tsManager, constantPool);
            pipeline.RunPass<GuardEliminatingPass>(&builder, &cmpCfg);
            pipeline.RunPass<GuardLoweringPass>(&builder, &cmpCfg);
            pipeline.RunPass<TypeLoweringPass>(&builder, &cmpCfg, tsManager);
        }
        pipeline.RunPass<SlowPathLoweringPass>(&builder, &cmpCfg, tsManager);
        pipeline.RunPass<VerifierPass>();
        pipeline.RunPass<SchedulingPass>();
        pipeline.RunPass<LLVMIRGenPass>(aotModule, method, jsPandaFile);
    });
    LOG_COMPILER(INFO) << " ";
    LOG_COMPILER(INFO) << skippedMethodNum << " large methods in " << fileName << " have been skipped";
    generator.AddModule(aotModule, aotModuleAssembler);
    return true;
}

JSPandaFile *PassManager::CreateJSPandaFile(const CString &fileName)
{
    JSPandaFileManager *jsPandaFileManager = JSPandaFileManager::GetInstance();
    JSPandaFile *jsPandaFile = jsPandaFileManager->OpenJSPandaFile(fileName);
    if (jsPandaFile == nullptr) {
        LOG_ECMA(ERROR) << "open file " << fileName << " error";
        return nullptr;
    }

    JSPandaFileManager::GetInstance()->InsertJSPandaFile(jsPandaFile);
    return jsPandaFile;
}

JSHandle<JSTaggedValue> PassManager::ResolveModuleAndConstPool(const JSPandaFile *jsPandaFile,
                                                               const std::string &fileName)
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
    return constPool;
}

bool PassManager::FilterMethod(const JSPandaFile *jsPandaFile, const CString &recordName, MethodLiteral *method,
    uint32_t methodOffset, MethodPcInfo &methodPCInfo)
{
    if (methodOffset != jsPandaFile->GetMainMethodIndex(recordName)) {
        if (methodPCInfo.methodsSize > maxAotMethodSize_ ||
            !profilerLoader_.Match(recordName, method->GetMethodId())) {
            return true;
        }
    }
    return false;
}
} // namespace panda::ecmascript::kungfu
