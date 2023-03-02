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
#include "ecmascript/compiler/compilation_driver.h"
#include "ecmascript/ecma_handle_scope.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/panda_file_translator.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/ts_types/ts_manager.h"

namespace panda::ecmascript::kungfu {

bool PassManager::Compile(const std::string &fileName, AOTFileGenerator &generator)
{
    [[maybe_unused]] EcmaHandleScope handleScope(vm_->GetJSThread());
    JSPandaFile *jsPandaFile = CreateAndVerifyJSPandaFile(fileName.c_str());
    if (jsPandaFile == nullptr) {
        LOG_COMPILER(ERROR) << "Cannot execute panda file '" << fileName << "'";
        return false;
    }

    if (!profilerLoader_.LoadAndVerify(jsPandaFile->GetChecksum())) {
        LOG_COMPILER(ERROR) << "Load and verify profiler failure";
        return false;
    }
    bool enableCollectLiteralInfo = EnableTypeInfer() &&
        (profilerLoader_.IsLoaded() || vm_->GetTSManager()->AssertTypes());
    BytecodeInfoCollector bcInfoCollector(vm_, jsPandaFile, maxAotMethodSize_,
        enableCollectLiteralInfo);

    if (!IsReleasedPandaFile(jsPandaFile)) {
        LOG_COMPILER(ERROR) << "The input panda file [" << fileName
                            << "] of AOT Compiler is debuggable version, do not use for performance test!";
    }

    ResolveModule(jsPandaFile, fileName);
    auto aotModule = new LLVMModule(fileName, triple_);
    auto aotModuleAssembler = new LLVMAssembler(aotModule->GetModule(),
                                                LOptions(optLevel_, true, relocMode_));

    CompilationConfig cmpCfg(triple_, false, log_->IsTraceBC(), vm_->GetJSOptions().GetOptCodeProfiler());
    Bytecodes bytecodes;
    auto &bytecodeInfo = bcInfoCollector.GetBytecodeInfo();
    auto lexEnvManager = LexEnvManager(bytecodeInfo);

    CompilationDriver cmpDriver(jsPandaFile, profilerLoader_, bytecodeInfo);
    // ts type system
    TSManager *tsManager = vm_->GetTSManager();
    tsManager->SetCompilationDriver(&cmpDriver);
    PassInfo info(tsManager, &bytecodes, &lexEnvManager, &cmpCfg, log_,
        jsPandaFile, &bcInfoCollector, aotModule);

    cmpDriver.Run([this, &fileName, &info]
        (const CString recordName, const std::string &methodName, MethodLiteral *methodLiteral,
         uint32_t methodOffset, const MethodPcInfo &methodPCInfo, size_t methodInfoIndex) {
        auto jsPandaFile = info.GetJSPandaFile();
        auto cmpCfg = info.GetCompilerConfig();
        auto tsManager = info.GetTSManager();
        // note: TSManager need to set current constantpool before all pass
        tsManager->SetCurConstantPool(jsPandaFile, methodOffset);

        log_->SetMethodLog(fileName, recordName, methodName, logList_);

        std::string fullName = methodName + "@" + fileName;
        bool enableMethodLog = log_->GetEnableMethodLog();
        if (enableMethodLog) {
            LOG_COMPILER(INFO) << "\033[34m" << "aot method [" << fullName << "] log:" << "\033[0m";
        }

        bool hasTypes = jsPandaFile->HasTSTypes(recordName);
        if (UNLIKELY(!hasTypes)) {
            LOG_COMPILER(INFO) << "record: " << recordName << " has no types";
        }

        Circuit circuit(vm_->GetNativeAreaAllocator(), cmpCfg->Is64Bit());
        BytecodeCircuitBuilder builder(jsPandaFile, methodLiteral, methodPCInfo, tsManager, &circuit,
                                       info.GetByteCodes(), hasTypes, enableMethodLog && log_->OutputCIR(),
                                       EnableTypeLowering(), fullName, recordName);
        {
            TimeScope timeScope("BytecodeToCircuit", methodName, methodOffset, log_);
            builder.BytecodeToCircuit();
        }

        PassData data(&builder, &circuit, &info, log_, fullName,
                      methodInfoIndex, hasTypes, recordName,
                      methodLiteral, methodOffset, vm_->GetNativeAreaAllocator());
        PassRunner<PassData> pipeline(&data);
        if (EnableTypeInfer()) {
            pipeline.RunPass<TypeInferPass>();
        }
        pipeline.RunPass<AsyncFunctionLoweringPass>();
        if (EnableTypeLowering()) {
            pipeline.RunPass<TSTypeLoweringPass>();
            pipeline.RunPass<EarlyEliminationPass>();
            pipeline.RunPass<TypeLoweringPass>();
        }
        pipeline.RunPass<SlowPathLoweringPass>();
        pipeline.RunPass<VerifierPass>();
        pipeline.RunPass<SchedulingPass>();
        pipeline.RunPass<LLVMIRGenPass>();
    });
    LOG_COMPILER(INFO) << bytecodeInfo.GetSkippedMethodSize() << " methods in "
                       << fileName << " have been skipped";
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

bool PassManager::IsReleasedPandaFile(const JSPandaFile *jsPandaFile) const
{
    MethodLiteral* methodLiteral = jsPandaFile->GetMethodLiterals();
    if (methodLiteral == nullptr) {
        LOG_COMPILER(ERROR) << "There is no mehtod literal in " << jsPandaFile->GetJSPandaFileDesc();
        return false;
    }

    panda_file::File::EntityId methodId = methodLiteral->GetMethodId();
    DebugInfoExtractor *debugInfoExtractor = JSPandaFileManager::GetInstance()->GetJSPtExtractor(jsPandaFile);
    LocalVariableTable lvt = debugInfoExtractor->GetLocalVariableTable(methodId);
    return lvt.empty();
}

void PassManager::ResolveModule(const JSPandaFile *jsPandaFile, const std::string &fileName)
{
    const auto &recordInfo = jsPandaFile->GetJSRecordInfo();
    ModuleManager *moduleManager = vm_->GetModuleManager();
    JSThread *thread = vm_->GetJSThread();
    for (auto info: recordInfo) {
        auto recordName = info.first;
        if (jsPandaFile->IsModule(thread, recordName)) {
            ASSERT(!thread->HasPendingException());
            moduleManager->HostResolveImportedModuleWithMerge(fileName.c_str(), recordName);
        }
    }
}
} // namespace panda::ecmascript::kungfu
