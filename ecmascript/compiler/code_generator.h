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

#ifndef ECMASCRIPT_COMPILER_CODE_GENERATOR_H
#define ECMASCRIPT_COMPILER_CODE_GENERATOR_H

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/binary_section.h"
#include "ecmascript/jspandafile/method_literal.h"

namespace panda::ecmascript::kungfu {
using ControlFlowGraph = std::vector<std::vector<GateRef>>;
class CompilationConfig;
class CompilerLog;

struct CodeInfo {
    using sectionInfo = std::pair<uint8_t *, size_t>;
    typedef uint8_t *(CodeInfo::*AllocaSectionCallback)(uintptr_t size, size_t alignSize);

    class CodeSpace {
    public:
        static CodeSpace *GetInstance();

        uint8_t *Alloca(uintptr_t size, bool isReq, size_t alignSize);

    private:
        CodeSpace();
        ~CodeSpace();

        static constexpr size_t REQUIRED_SECS_LIMIT = (1 << 29);  // 512M
        static constexpr size_t UNREQUIRED_SECS_LIMIT = (1 << 28);  // 256M

        // start point of the buffer reserved for sections required in executing phase
        uint8_t *reqSecs_ {nullptr};
        size_t reqBufPos_ {0};
        // start point of the buffer reserved for sections not required in executing phase
        uint8_t *unreqSecs_ {nullptr};
        size_t unreqBufPos_ {0};
    };

    class CodeSpaceOnDemand {
    public:
        CodeSpaceOnDemand() = default;

        uint8_t *Alloca(uintptr_t size, bool isReq, size_t alignSize);

        ~CodeSpaceOnDemand();

    private:
        static constexpr size_t SECTION_LIMIT = (1 << 29);  // 512M

        // record all memory blocks requested.
        std::vector<std::pair<uint8_t *, uintptr_t>> sections_;
    };

    struct FuncInfo {
        uint32_t addr = 0;
        int32_t fp2PrevFrameSpDelta = 0;
        kungfu::CalleeRegAndOffsetVec calleeRegInfo;
    };

    CodeInfo(CodeSpaceOnDemand &codeSpaceOnDemand);

    ~CodeInfo();

    uint8_t *AllocaOnDemand(uintptr_t size, size_t alignSize = 0);

    uint8_t *AllocaInReqSecBuffer(uintptr_t size, size_t alignSize = 0);

    uint8_t *AllocaInNotReqSecBuffer(uintptr_t size, size_t alignSize = 0);

    uint8_t *AllocaCodeSectionImp(uintptr_t size, const char *sectionName, AllocaSectionCallback allocaInReqSecBuffer);

    uint8_t *AllocaCodeSection(uintptr_t size, const char *sectionName);

    uint8_t *AllocaCodeSectionOnDemand(uintptr_t size, const char *sectionName);

    uint8_t *AllocaDataSectionImp(uintptr_t size, const char *sectionName, AllocaSectionCallback allocaInReqSecBuffer,
                                  AllocaSectionCallback allocaInNotReqSecBuffer);

    uint8_t *AllocaDataSection(uintptr_t size, const char *sectionName);

    uint8_t *AllocaDataSectionOnDemand(uintptr_t size, const char *sectionName);

    void SaveFunc2Addr(std::string funcName, uint32_t address);

    void SaveFunc2FPtoPrevSPDelta(std::string funcName, int32_t fp2PrevSpDelta);

    void SaveFunc2CalleeOffsetInfo(std::string funcName, kungfu::CalleeRegAndOffsetVec calleeRegInfo);

    void SavePC2DeoptInfo(uint64_t pc, std::vector<uint8_t> pc2DeoptInfo);

    void SavePC2CallSiteInfo(uint64_t pc, std::vector<uint8_t> callSiteInfo);

    const std::map<std::string, FuncInfo> &GetFuncInfos() const;

    const std::map<uint64_t, std::vector<uint8_t>> &GetPC2DeoptInfo() const;

    const std::unordered_map<uint64_t, std::vector<uint8_t>> &GetPC2CallsiteInfo() const;

    void Reset();

    uint8_t *GetSectionAddr(ElfSecName sec) const;

    size_t GetSectionSize(ElfSecName sec) const;

    std::vector<std::pair<uint8_t *, uintptr_t>> GetCodeInfo() const;

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
    std::array<sectionInfo, static_cast<int>(ElfSecName::SIZE)> secInfos_;
    std::vector<std::pair<uint8_t *, uintptr_t>> codeInfo_ {}; // info for disasssembler, planed to be deprecated
    std::map<std::string, FuncInfo> func2FuncInfo;
    std::map<uint64_t, std::vector<uint8_t>> pc2DeoptInfo;
    std::unordered_map<uint64_t, std::vector<uint8_t>> pc2CallsiteInfo;
    bool alreadyPageAlign_ {false};
    CodeSpaceOnDemand &codeSpaceOnDemand_;
};

class Assembler {
public:
    explicit Assembler(CodeInfo::CodeSpaceOnDemand &codeSpaceOnDemand) : codeInfo_(codeSpaceOnDemand)
    {}
    virtual ~Assembler() = default;
    virtual void Run(const CompilerLog &log, bool fastCompileMode, bool isJit = false) = 0;

    uintptr_t GetSectionAddr(ElfSecName sec) const
    {
        return reinterpret_cast<uintptr_t>(codeInfo_.GetSectionAddr(sec));
    }

    uint32_t GetSectionSize(ElfSecName sec) const
    {
        return static_cast<uint32_t>(codeInfo_.GetSectionSize(sec));
    }

    template <class Callback>
    void IterateSecInfos(const Callback &cb) const
    {
        codeInfo_.IterateSecInfos(cb);
    }

    const CodeInfo &GetCodeInfo() const
    {
        return codeInfo_;
    }
protected:
    CodeInfo codeInfo_;
};

class CodeGeneratorImpl {
public:
    CodeGeneratorImpl() = default;

    virtual ~CodeGeneratorImpl() = default;

    virtual void GenerateCodeForStub(Circuit *circuit, const ControlFlowGraph &graph, size_t index,
                                     const CompilationConfig *cfg) = 0;

    virtual void GenerateCode(Circuit *circuit, const ControlFlowGraph &graph, const CompilationConfig *cfg,
                              const MethodLiteral *methodLiteral, const JSPandaFile *jsPandaFile,
                              const std::string &methodName, const FrameType frameType,
                              bool enableOptInlining, bool enableBranchProfiling) = 0;
};

class CodeGenerator {
public:
    CodeGenerator(std::unique_ptr<CodeGeneratorImpl> &impl, const std::string& methodName)
        : impl_(std::move(impl)), methodName_(methodName)
    {
    }

    ~CodeGenerator() = default;

    void RunForStub(Circuit *circuit, const ControlFlowGraph &graph, size_t index, const CompilationConfig *cfg)
    {
        impl_->GenerateCodeForStub(circuit, graph, index, cfg);
    }

    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    void Run(Circuit *circuit, const ControlFlowGraph &graph, const CompilationConfig *cfg,
             const MethodLiteral *methodLiteral, const JSPandaFile *jsPandaFile, const FrameType frameType,
             bool enableOptInlining, bool enableOptBranchProfiling)
    {
        impl_->GenerateCode(circuit, graph, cfg, methodLiteral, jsPandaFile, methodName_, frameType,
            enableOptInlining, enableOptBranchProfiling);
    }

private:
    std::unique_ptr<CodeGeneratorImpl> impl_{nullptr};
    std::string methodName_;
};
} // namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_CODE_GENERATOR_H
