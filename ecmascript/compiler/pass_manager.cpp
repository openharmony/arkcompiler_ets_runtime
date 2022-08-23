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

#include "ecmascript/compiler/file_generators.h"
#include "ecmascript/compiler/pass.h"
#include "ecmascript/compiler/bytecode_info_collector.h"
#include "ecmascript/ecma_handle_scope.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/panda_file_translator.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/ts_types/ts_manager.h"

namespace panda::ecmascript::kungfu {
bool PassManager::Compile(const std::string &fileName, AOTFileGenerator &generator)
{
    BytecodeInfoCollector::BCInfo bytecodeInfo;
    [[maybe_unused]] EcmaHandleScope handleScope(vm_->GetJSThread());
    bool res = CollectBCInfo(fileName, &bytecodeInfo);
    if (!res) {
        LOG_COMPILER(ERROR) << "Cannot execute panda file '" << fileName << "'";
        return false;
    }
    auto aotModule = new LLVMModule("aot_" + fileName, triple_);
    auto aotModuleAssembler = new LLVMAssembler(aotModule->GetModule(),
        LOptions(optLevel_, true, relocMode_));
    CompilationConfig cmpCfg(triple_, log_->IsEnableByteCodeTrace());
    TSManager *tsManager = vm_->GetTSManager();

    bool enableLog = !log_->NoneMethod();

    bytecodeInfo.EnumerateBCInfo([this, &fileName, &enableLog, aotModule, &cmpCfg, tsManager]
        (const JSPandaFile *jsPandaFile, JSHandle<JSTaggedValue> &constantPool,
        BytecodeInfoCollector::MethodPcInfo &methodPCInfo) {
        for (auto method : methodPCInfo.methods) {
            const std::string methodName(MethodLiteral::GetMethodName(jsPandaFile, method->GetMethodId()));
            if (log_->CertainMethod()) {
                enableLog = logList_->IncludesMethod(fileName, methodName);
            }

            if (enableLog) {
                LOG_COMPILER(INFO) << "\033[34m" << "aot method [" << fileName << ":"
                                << methodName << "] log:" << "\033[0m";
            }

            BytecodeCircuitBuilder builder(jsPandaFile, constantPool, method, methodPCInfo, tsManager,
                                        enableLog && log_->OutputCIR());
            builder.BytecodeToCircuit();
            PassData data(builder.GetCircuit());
            PassRunner<PassData> pipeline(&data, enableLog && log_->OutputCIR());
            pipeline.RunPass<AsyncFunctionLoweringPass>(&builder, &cmpCfg);
            pipeline.RunPass<TypeInferPass>(&builder, tsManager);
            pipeline.RunPass<TypeLoweringPass>(&builder, &cmpCfg, tsManager);
            pipeline.RunPass<SlowPathLoweringPass>(&builder, &cmpCfg);
            pipeline.RunPass<VerifierPass>();
            pipeline.RunPass<SchedulingPass>();
            pipeline.RunPass<LLVMIRGenPass>(aotModule, method, jsPandaFile);
        }
    });

    generator.AddModule(aotModule, aotModuleAssembler, bytecodeInfo.jsPandaFile);
    return true;
}

bool PassManager::CollectBCInfo(const std::string &fileName, BytecodeInfoCollector::BCInfo *bytecodeInfo)
{
    if (bytecodeInfo == nullptr) {
        return false;
    }
    const JSPandaFile *jsPandaFile = BytecodeInfoCollector::LoadInfoFromPf(fileName.c_str(), entry_,
                                                                           bytecodeInfo->methodPcInfos);

    if (jsPandaFile == nullptr) {
        return false;
    }
    bytecodeInfo->jsPandaFile = jsPandaFile;

    if (jsPandaFile->HasTSTypes()) {
        TSManager *tsManager = vm_->GetTSManager();
        tsManager->DecodeTSTypes(jsPandaFile);
    } else {
        LOG_COMPILER(INFO) << fileName << " has no type info";
    }

    if (jsPandaFile->IsModule()) {
        ModuleManager *moduleManager = vm_->GetModuleManager();
        moduleManager->HostResolveImportedModule(fileName.c_str());
    }

    auto program = PandaFileTranslator::GenerateProgram(vm_, jsPandaFile);
    JSThread *thread = vm_->GetJSThread();
    JSHandle<JSFunction> mainFunc(thread, program->GetMainFunction());
    JSHandle<Method> method(thread, mainFunc->GetMethod());
    JSHandle<JSTaggedValue> constPool(thread, method->GetConstantPool());
    bytecodeInfo->constantPool = constPool;
    return true;
}
} // namespace panda::ecmascript::kungfu
