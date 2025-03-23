/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/compilation_driver.h"

#include "ecmascript/compiler/file_generators.h"
#include "ecmascript/compiler/pgo_type/pgo_type_manager.h"

namespace panda::ecmascript::kungfu {
CompilationDriver::CompilationDriver(PGOProfilerDecoder &profilerDecoder,
                                     BytecodeInfoCollector *collector,
                                     AOTFileGenerator *fileGenerator,
                                     const std::string &fileName,
                                     const std::string &triple,
                                     LOptions *lOptions,
                                     CompilerLog *log,
                                     bool outputAsm,
                                     size_t maxMethodsInModule)
    : compilationEnv_(collector->GetCompilationEnv()),
      jsPandaFile_(collector->GetJSPandaFile()),
      pfDecoder_(profilerDecoder),
      collector_(collector),
      bytecodeInfo_(collector->GetBytecodeInfo()),
      fileGenerator_(fileGenerator),
      fileName_(fileName),
      triple_(triple),
      lOptions_(lOptions),
      log_(log),
      outputAsm_(outputAsm),
      maxMethodsInModule_(maxMethodsInModule)
{
}

Module *CompilationDriver::GetModule()
{
    return IsCurModuleFull() ? fileGenerator_->AddModule(fileName_, triple_, *lOptions_, outputAsm_)
                                : fileGenerator_->GetLatestModule();
}

void CompilationDriver::IncCompiledMethod()
{
    compiledMethodCnt_++;
}

bool CompilationDriver::IsCurModuleFull() const
{
    return (compiledMethodCnt_ % maxMethodsInModule_ == 0);
}

void CompilationDriver::CompileModuleThenDestroyIfNeeded(bool isJitAndCodeSign)
{
    if (IsCurModuleFull()) {
        fileGenerator_->CompileLatestModuleThenDestroy(isJitAndCodeSign);
    }
}

void CompilationDriver::CompileLastModuleThenDestroyIfNeeded()
{
    if (!IsCurModuleFull()) {
        fileGenerator_->CompileLatestModuleThenDestroy();
    }
}

std::vector<std::string> CompilationDriver::SplitString(const std::string &str, const char ch) const
{
    std::vector<std::string> vec {};
    std::istringstream sstr(str.c_str());
    std::string split;
    while (getline(sstr, split, ch)) {
        vec.emplace_back(split);
    }
    return vec;
}

void CompilationDriver::SetCurrentCompilationFile() const
{
    PGOTypeManager *ptManager = compilationEnv_->GetPTManager();
    ptManager->SetCurCompilationFile(jsPandaFile_);
}

void CompilationDriver::StoreConstantPoolInfo() const
{
    auto &methodList = bytecodeInfo_.GetMethodList();
    for (auto &x : methodList) {
        if (!bytecodeInfo_.FindMethodOffsetToRecordName(x.first)) {
            bytecodeInfo_.AddSkippedMethod(x.first);
        }
    }
    PGOTypeManager *ptManager = compilationEnv_->GetPTManager();
    ptManager->GetAOTSnapshot().StoreConstantPoolInfo(collector_);
}

bool JitCompilationDriver::RunCg()
{
    IncCompiledMethod();
    bool enableCodeSign = !compilationEnv_->GetJSOptions().GetDisableCodeSign();
    CompileModuleThenDestroyIfNeeded(enableCodeSign);  // isJit AND !DisabelCodeSign
    return true;
}

Module *JitCompilationDriver::GetModule()
{
    return IsCurModuleFull() ? fileGenerator_->AddModule(fileName_, triple_, *lOptions_, outputAsm_, true) :
        fileGenerator_->GetLatestModule();
}
} // namespace panda::ecmascript::kungfu
