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

#ifndef ECMASCRIPT_COMPILER_LLVM_CODEGEN_H
#define ECMASCRIPT_COMPILER_LLVM_CODEGEN_H

#include "ecmascript/compiler/binary_section.h"
#include "ecmascript/compiler/code_generator.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITEventListener.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace panda::ecmascript::kungfu {
class CompilerLog;
class MethodLogList;
class LLVMModule;

enum class FPFlag : uint32_t {
    ELIM_FP = 0,
    RESERVE_FP = 1
};

struct LOptions {
    uint32_t optLevel : 2; // 2 bits for optimized level 0-4
    uint32_t genFp : 1; // 1 bit for whether to generated frame pointer or not
    uint32_t relocMode : 3; // 3 bits for relocation mode
    // 3: default optLevel, 1: generating fp, 2: PIC mode
    LOptions() : optLevel(3), genFp(static_cast<uint32_t>(FPFlag::RESERVE_FP)), relocMode(2) {};
    LOptions(size_t level, FPFlag flag, size_t relocMode)
        : optLevel(level), genFp(static_cast<uint32_t>(flag)), relocMode(relocMode) {};
};

class LLVMAssembler : public Assembler {
public:
    explicit LLVMAssembler(LLVMModule *lm, CodeInfo::CodeSpaceOnDemand &codeSpaceOnDemand,
                           LOptions option = LOptions());
    virtual ~LLVMAssembler();
    void Run(const CompilerLog &log, bool fastCompileMode, bool isJit = false) override;
    const LLVMExecutionEngineRef &GetEngine()
    {
        return engine_;
    }
    void Disassemble(const std::map<uintptr_t, std::string> &addr2name, uint64_t textOffset,
                     const CompilerLog &log, const MethodLogList &logList, std::ostringstream &codeStream) const;
    static void Disassemble(const std::map<uintptr_t, std::string> *addr2name,
                            const std::string& triple, uint8_t *buf, size_t size);
    static int GetFpDeltaPrevFramSp(LLVMValueRef fn, const CompilerLog &log);
    static kungfu::CalleeRegAndOffsetVec GetCalleeReg2Offset(LLVMValueRef fn, const CompilerLog &log);

    void *GetFuncPtrFromCompiledModule(LLVMValueRef function)
    {
        return LLVMGetPointerToGlobal(engine_, function);
    }

    void SetObjFile(const llvm::object::ObjectFile *obj)
    {
        objFile_ = obj;
    }
private:
    class AOTEventListener : public llvm::JITEventListener {
      public:
        AOTEventListener(LLVMAssembler* as) : as_(as)
        {
        }
        void notifyObjectLoaded([[maybe_unused]] ObjectKey key, const llvm::object::ObjectFile &objFile,
                                [[maybe_unused]] const llvm::RuntimeDyld::LoadedObjectInfo &objInfo)
        {
            as_->SetObjFile(&objFile);
        }
      private:
        LLVMAssembler* GetAssembler() const
        {
            return as_;
        }

        LLVMAssembler* as_ {nullptr};
    };

    void UseRoundTripSectionMemoryManager(bool isJit);
    bool BuildMCJITEngine();
    void BuildAndRunPasses();
    void BuildAndRunPassesFastMode();
    void Initialize(LOptions option);
    static void PrintInstAndStep(uint64_t &pc, uint8_t **byteSp, uintptr_t &numBytes, size_t instSize,
                                 uint64_t textOffset, char *outString, std::ostringstream &codeStream,
                                 bool logFlag = true);
    uint64_t GetTextSectionIndex() const;

    LLVMMCJITCompilerOptions options_ {};
    LLVMModule *llvmModule_ {nullptr};
    LLVMModuleRef module_ {nullptr};
    const llvm::object::ObjectFile* objFile_ {nullptr};
    LLVMExecutionEngineRef engine_ {nullptr};
    AOTEventListener listener_;
    char *error_ {nullptr};
};

class LLVMIRGeneratorImpl : public CodeGeneratorImpl {
public:
    LLVMIRGeneratorImpl(LLVMModule *module, bool enableLog)
        : module_(module), enableLog_(enableLog) {}
    ~LLVMIRGeneratorImpl() override = default;
    void GenerateCodeForStub(Circuit *circuit, const ControlFlowGraph &graph, size_t index,
                             const CompilationConfig *cfg) override;
    void GenerateCode(Circuit *circuit, const ControlFlowGraph &graph, const CompilationConfig *cfg,
        const MethodLiteral *methodLiteral, const JSPandaFile *jsPandaFile, const std::string &methodName,
        const FrameType frameType, bool enableOptInlining, bool enableBranchProfiling) override;

    bool IsLogEnabled() const
    {
        return enableLog_;
    }

private:
    LLVMModule *module_;
    bool enableLog_ {false};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_LLVM_CODEGEN_H
