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

#include <iostream>
#include <list>
#include <map>
#include <vector>

#include "ecmascript/compiler/binary_section.h"
#include "ecmascript/compiler/code_generator.h"
#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_thread.h"

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

#include "ecmascript/mem/machine_code.h"
#include "ecmascript/mem/region.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Target.h"
#include "llvm-c/Transforms/PassManagerBuilder.h"
#include "llvm-c/Transforms/Scalar.h"
#include "llvm-c/Types.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/IR/Instructions.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/Support/Host.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace panda::ecmascript::kungfu {
struct CodeInfo {
    using sectionInfo = std::pair<uint8_t *, size_t>;
    CodeInfo()
    {
        ASSERT(REQUIRED_SECS_LIMIT == AlignUp(REQUIRED_SECS_LIMIT, PageSize()));
#ifdef PANDA_TARGET_MACOS
        reqSecs_ = static_cast<uint8_t *>(PageMap(REQUIRED_SECS_LIMIT, PAGE_PROT_READWRITE).GetMem());
#else
        reqSecs_ = static_cast<uint8_t *>(PageMap(REQUIRED_SECS_LIMIT, PAGE_PROT_EXEC_READWRITE).GetMem());
#endif
        if (reqSecs_ == reinterpret_cast<uint8_t *>(-1)) {
            reqSecs_ = nullptr;
        }
        ASSERT(UNREQUIRED_SECS_LIMIT == AlignUp(UNREQUIRED_SECS_LIMIT, PageSize()));
#ifdef PANDA_TARGET_MACOS
        unreqSecs_ = static_cast<uint8_t *>(PageMap(UNREQUIRED_SECS_LIMIT, PAGE_PROT_READWRITE).GetMem());
#else
        unreqSecs_ = static_cast<uint8_t *>(PageMap(UNREQUIRED_SECS_LIMIT, PAGE_PROT_EXEC_READWRITE).GetMem());
#endif
        if (unreqSecs_ == reinterpret_cast<uint8_t *>(-1)) {
            unreqSecs_ = nullptr;
        }
        secInfos_.fill(std::make_pair(nullptr, 0));
    }
    ~CodeInfo()
    {
        Reset();
        if (reqSecs_ != nullptr) {
            PageUnmap(MemMap(reqSecs_, REQUIRED_SECS_LIMIT));
        }
        reqSecs_ = nullptr;
        if (unreqSecs_ != nullptr) {
            PageUnmap(MemMap(unreqSecs_, UNREQUIRED_SECS_LIMIT));
        }
        unreqSecs_ = nullptr;
    }

    uint8_t *AllocaInReqSecBuffer(uintptr_t size, bool alignFlag = true)
    {
        return Alloca(size, reqSecs_, reqBufPos_, alignFlag);
    }

    uint8_t *AllocaInNotReqSecBuffer(uintptr_t size)
    {
        return Alloca(size, unreqSecs_, unreqBufPos_);
    }

    uint8_t *AllocaCodeSection(uintptr_t size, const char *sectionName)
    {
        // if have got section, don't use align.
        uint8_t *addr = AllocaInReqSecBuffer(size, false);
        auto curSec = ElfSection(sectionName);
        codeInfo_.push_back({addr, size});
        if (curSec.isValidAOTSec()) {
            secInfos_[curSec.GetIntIndex()] = std::make_pair(addr, size);
        }
        return addr;
    }

    uint8_t *AllocaDataSection(uintptr_t size, const char *sectionName)
    {
        uint8_t *addr = nullptr;
        auto curSec = ElfSection(sectionName);
        // rodata section needs 16 bytes alignment
        if (curSec.InRodataSection()) {
            size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_REGION));
        }
        addr = curSec.isSequentialAOTSec() ? AllocaInReqSecBuffer(size) : AllocaInNotReqSecBuffer(size);
        if (curSec.isValidAOTSec()) {
            secInfos_[curSec.GetIntIndex()] = std::make_pair(addr, size);
        }
        return addr;
    }

    void Reset()
    {
        codeInfo_.clear();
        reqBufPos_ = 0;
        unreqBufPos_ = 0;
    }

    uint8_t *GetSectionAddr(ElfSecName sec) const
    {
        auto curSection = ElfSection(sec);
        auto idx = curSection.GetIntIndex();
        return const_cast<uint8_t *>(secInfos_[idx].first);
    }

    size_t GetSectionSize(ElfSecName sec) const
    {
        auto curSection = ElfSection(sec);
        auto idx = curSection.GetIntIndex();
        return secInfos_[idx].second;
    }

    std::vector<std::pair<uint8_t *, uintptr_t>> GetCodeInfo() const
    {
        return codeInfo_;
    }

    template <class Callback>
    void IterateSecInfos(const Callback &cb) const
    {
        for (size_t i = 0; i < secInfos_.size(); i++) {
            if (secInfos_[i].second == 0) {
                continue;
            }
            cb(i, secInfos_[i]);
        }
    }

private:
    static constexpr size_t REQUIRED_SECS_LIMIT = (1 << 29);  // 512M
    static constexpr size_t UNREQUIRED_SECS_LIMIT = (1 << 28);  // 256M
    // start point of the buffer reserved for sections required in executing phase
    uint8_t *reqSecs_ {nullptr};
    size_t reqBufPos_ {0};
    // start point of the buffer reserved for sections not required in executing phase
    uint8_t *unreqSecs_ {nullptr};
    size_t unreqBufPos_ {0};
    std::array<sectionInfo, static_cast<int>(ElfSecName::SIZE)> secInfos_;
    std::vector<std::pair<uint8_t *, uintptr_t>> codeInfo_ {}; // info for disasssembler, planed to be deprecated

    uint8_t *Alloca(uintptr_t size, uint8_t *bufBegin, size_t &curPos, bool alignFlag = true)
    {
        // align up for rodata section
        if (alignFlag) {
            size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_REGION));
        }
        uint8_t *addr = nullptr;
        size_t limit = (bufBegin == reqSecs_) ? REQUIRED_SECS_LIMIT : UNREQUIRED_SECS_LIMIT;
        if (curPos + size > limit) {
            LOG_COMPILER(ERROR) << std::hex << "Alloca Section failed. Current curPos:" << curPos
                      << " plus size:" << size << "exceed limit:" << limit;
            return nullptr;
        }
        addr = bufBegin + curPos;
        curPos += size;
        return addr;
    }
};

struct LOptions {
    uint32_t optLevel : 2; // 2 bits for optimized level 0-4
    uint32_t genFp : 1; // 1 bit for whether to generated frame pointer or not
    uint32_t relocMode : 3; // 3 bits for relocation mode
    // 3: default optLevel, 1: generating fp, 2: PIC mode
    LOptions() : optLevel(3), genFp(1), relocMode(2) {};
    LOptions(size_t level, bool genFp, size_t relocMode) : optLevel(level), genFp(genFp), relocMode(relocMode) {};
};

class LLVMAssembler {
public:
    explicit LLVMAssembler(LLVMModuleRef module, LOptions option = LOptions());
    virtual ~LLVMAssembler();
    void Run(const CompilerLog &log);
    const LLVMExecutionEngineRef &GetEngine()
    {
        return engine_;
    }
    void Disassemble(const std::map<uintptr_t, std::string> &addr2name,
                     const CompilerLog &log, const MethodLogList &logList) const;
    static void Disassemble(uint8_t *buf, size_t size);
    static int GetFpDeltaPrevFramSp(LLVMValueRef fn, const CompilerLog &log);
    static kungfu::CalleeRegAndOffsetVec GetCalleeReg2Offset(LLVMValueRef fn, const CompilerLog &log);

    uintptr_t GetSectionAddr(ElfSecName sec) const
    {
        return reinterpret_cast<uintptr_t>(codeInfo_.GetSectionAddr(sec));
    }

    uint32_t GetSectionSize(ElfSecName sec) const
    {
        return static_cast<uint32_t>(codeInfo_.GetSectionSize(sec));
    }

    void *GetFuncPtrFromCompiledModule(LLVMValueRef function)
    {
        return LLVMGetPointerToGlobal(engine_, function);
    }

    template <class Callback>
    void IterateSecInfos(const Callback &cb) const
    {
        codeInfo_.IterateSecInfos(cb);
    }

private:
    void UseRoundTripSectionMemoryManager();
    bool BuildMCJITEngine();
    void BuildAndRunPasses();
    void Initialize(LOptions option);
    static void PrintInstAndStep(unsigned &pc, uint8_t **byteSp, uintptr_t &numBytes, size_t instSize, char *outString,
        bool logFlag = true);

    LLVMMCJITCompilerOptions options_ {};
    LLVMModuleRef module_;
    LLVMExecutionEngineRef engine_ {nullptr};
    char *error_ {nullptr};
    struct CodeInfo codeInfo_ {};
};

class LLVMIRGeneratorImpl : public CodeGeneratorImpl {
public:
    explicit LLVMIRGeneratorImpl(LLVMModule *module, bool enableLog)
        : module_(module), enableLog_(enableLog) {}
    ~LLVMIRGeneratorImpl() = default;
    void GenerateCodeForStub(Circuit *circuit, const ControlFlowGraph &graph, size_t index,
                             const CompilationConfig *cfg) override;
    void GenerateCode(Circuit *circuit, const ControlFlowGraph &graph, const CompilationConfig *cfg,
        const MethodLiteral *methodLiteral, const JSPandaFile *jsPandaFile) override;

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
