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

#ifndef ECMASCRIPT_COMPILER_COMPILATION_DRIVER_H
#define ECMASCRIPT_COMPILER_COMPILATION_DRIVER_H

#include "ecmascript/compiler/aot_compiler_preprocessor.h"
#include "ecmascript/compiler/bytecode_info_collector.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript::kungfu {
class AOTFileGenerator;
class CompilerLog;
struct LOptions;
class Module;
class CompilationDriver {
public:
    CompilationDriver(PGOProfilerDecoder &profilerDecoder,
                      BytecodeInfoCollector* collector,
                      AOTFileGenerator *fileGenerator,
                      const std::string &fileName,
                      const std::string &triple,
                      LOptions *lOptions,
                      CompilerLog *log,
                      bool outputAsm,
                      size_t maxMethodsInModule);
    ~CompilationDriver() = default;

    NO_COPY_SEMANTIC(CompilationDriver);
    NO_MOVE_SEMANTIC(CompilationDriver);

    bool IsPGOLoaded() const
    {
        return pfDecoder_.IsLoaded();
    }

    template <class Callback>
    void CompileMethod(const Callback &cb,
                       const CString &recordName,
                       const std::string &methodName,
                       MethodLiteral *methodLiteral,
                       uint32_t methodOffset,
                       const MethodPcInfo &methodPcInfo,
                       MethodInfo &methodInfo)
    {
        Module *module = GetModule();
        cb(recordName, methodName, methodLiteral, methodOffset,
            methodPcInfo, methodInfo, module);
        IncCompiledMethod();
        CompileModuleThenDestroyIfNeeded(false);
    }

    template <class Callback>
    void Run(const CallMethodFlagMap &callMethonFlagMap, const Callback &cb)
    {
        SetCurrentCompilationFile();
        const auto &methodPcInfos = bytecodeInfo_.GetMethodPcInfos();
        for (auto &[methodId, methodInfo] : bytecodeInfo_.GetMethodList()) {
            bytecodeInfo_.AddMethodOffsetToRecordName(methodId, methodInfo.GetRecordName());
            auto &methodPcInfo = methodPcInfos[methodInfo.GetMethodPcInfoIndex()];
            auto methodLiteral = jsPandaFile_->FindMethodLiteral(methodId);
            if (methodLiteral == nullptr) {
                continue;
            }
            const std::string methodName(MethodLiteral::GetMethodName(jsPandaFile_, methodLiteral->GetMethodId()));
            if (!callMethonFlagMap.IsAotCompile(jsPandaFile_->GetNormalizedFileDesc(),
                                                methodLiteral->GetMethodId().GetOffset())) {
                bytecodeInfo_.AddSkippedMethod(methodId);
            } else {
                if (!methodInfo.IsCompiled()) {
                    methodInfo.SetIsCompiled(true);
                    CompileMethod(cb, methodInfo.GetRecordName(), methodName, methodLiteral,
                        methodId, methodPcInfo, methodInfo);
                }
            }
        }
        CompileLastModuleThenDestroyIfNeeded();
        StoreConstantPoolInfo();
    }

    void FetchPGOMismatchResult();

    uint32_t GetCompilerMethodCount() const
    {
        return compiledMethodCnt_;
    }
protected:
    // add maxMethodsInModule_ functions in a module and when a module is
    // full(maxMethodsInModule_ functions have been put into) or the module is the last module,
    // compile it and the destroy it.
    Module *GetModule();

    void IncCompiledMethod();

    bool IsCurModuleFull() const;

    void CompileModuleThenDestroyIfNeeded(bool isJit = false);

    void CompileLastModuleThenDestroyIfNeeded();

    std::vector<std::string> SplitString(const std::string &str, const char ch) const;

    void SetCurrentCompilationFile() const;

    void StoreConstantPoolInfo() const;

    CompilationEnv *compilationEnv_ {nullptr};
    const JSPandaFile *jsPandaFile_ {nullptr};
    PGOProfilerDecoder &pfDecoder_;
    BytecodeInfoCollector* collector_;
    BCInfo &bytecodeInfo_;
    uint32_t compiledMethodCnt_ {0};
    AOTFileGenerator *fileGenerator_ {nullptr};
    std::string fileName_ {};
    std::string triple_ {};
    LOptions *lOptions_ {nullptr};
    CompilerLog *log_ {nullptr};
    bool outputAsm_ {false};
    size_t maxMethodsInModule_ {0};
};

class JitCompilationDriver : public CompilationDriver {
public:
    JitCompilationDriver(PGOProfilerDecoder &profilerDecoder,
                         BytecodeInfoCollector* collector,
                         AOTFileGenerator *fileGenerator,
                         const std::string &fileName,
                         const std::string &triple,
                         LOptions *lOptions,
                         CompilerLog *log,
                         bool outputAsm,
                         size_t maxMethodsInModule)
        : CompilationDriver(profilerDecoder, collector, fileGenerator, fileName, triple, lOptions,
                            log, outputAsm, maxMethodsInModule) { };
    ~JitCompilationDriver() = default;
    bool RunCg();
    Module *GetModule();

    template <class Callback>
    bool CompileMethod(const JSPandaFile *jsPandaFile, MethodLiteral *methodLiteral,
                       JSHandle<ProfileTypeInfo> &profileTypeInfo, const uint8_t *pcStart,
                       const panda_file::File::Header *header, ApEntityId abcId, const Callback &cb)
    {
        SetCurrentCompilationFile();
        if (methodLiteral == nullptr || methodLiteral->IsShared()) {
            return false;
        }
        const std::string methodName(MethodLiteral::GetMethodName(jsPandaFile, methodLiteral->GetMethodId()));

        auto &methodList = bytecodeInfo_.GetMethodList();
        const auto &methodPcInfos = bytecodeInfo_.GetMethodPcInfos();
        auto &methodInfo = methodList.at(methodLiteral->GetMethodId().GetOffset());

        auto &methodPcInfo = methodPcInfos[methodInfo.GetMethodPcInfoIndex()];
        auto methodOffset = methodLiteral->GetMethodId().GetOffset();
        bytecodeInfo_.EraseSkippedMethod(methodOffset);

        Module *module = GetModule();
        return cb(bytecodeInfo_.GetRecordNameWithIndex(0), methodName, methodLiteral, profileTypeInfo,
            methodOffset, methodPcInfo, methodInfo, module, pcStart, header, abcId);
    }
};
} // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_COMPILATION_DRIVER_H
