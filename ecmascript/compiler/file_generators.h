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

#ifndef ECMASCRIPT_COMPILER_FILE_GENERATORS_H
#define ECMASCRIPT_COMPILER_FILE_GENERATORS_H

#include "ecmascript/base/number_helper.h"
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/compiler/assembler_module.h"
#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/compiler/llvm_codegen.h"
#include "ecmascript/compiler/llvm_ir_builder.h"

namespace panda::ecmascript::kungfu {
class Module {
public:
    Module() = default;
    Module(LLVMModule *module, LLVMAssembler *assembler)
        : llvmModule_(module), assembler_(assembler)
    {
    }

    void CollectFuncEntryInfo(std::map<uintptr_t, std::string> &addr2name, StubFileInfo &stubInfo,
                              uint32_t moduleIndex, const CompilerLog &log);

    void CollectFuncEntryInfo(std::map<uintptr_t, std::string> &addr2name, AnFileInfo &aotInfo,
                              uint32_t moduleIndex, const CompilerLog &log);

    bool IsRelaSection(ElfSecName sec) const
    {
        return sec == ElfSecName::RELATEXT || sec == ElfSecName::STRTAB || sec == ElfSecName::SYMTAB;
    }

    void CollectModuleSectionDes(ModuleSectionDes &moduleDes) const;

    void CollectAnModuleSectionDes(ModuleSectionDes &moduleDes, uint64_t textOffset,
        std::vector<LLVMStackMapType::Pc2CallSiteInfo> &pc2CallsiteInfoVec,
        std::vector<LLVMStackMapType::Pc2Deopt> &pc2DeoptVec) const;

    void CollectStackMapDes(ModuleSectionDes &moduleDes) const;

    void CollectAnStackMapDes(ModuleSectionDes& des, uint64_t textOffset,
        std::vector<LLVMStackMapType::Pc2CallSiteInfo> &pc2CallsiteInfoVec,
        std::vector<LLVMStackMapType::Pc2Deopt> &pc2DeoptVec) const;

    uint32_t GetSectionSize(ElfSecName sec) const;

    uintptr_t GetSectionAddr(ElfSecName sec) const;

    std::tuple<uint64_t, uint32_t, uint64_t, uint32_t> GetMergedRODataAddrAndSize(uint64_t textAddr) const
    {
        uint64_t addrBeforeText = base::MAX_UINT64_VALUE;
        uint32_t sizeBeforeText = 0;
        uint64_t addrAfterText = base::MAX_UINT64_VALUE;
        uint32_t sizeAfterText = 0;
        for (uint8_t i = static_cast<uint8_t>(ElfSecName::RODATA); i <= static_cast<uint8_t>(ElfSecName::RODATA_CST32);
             i++) {
            UpdateRODataInfo(textAddr, addrBeforeText, sizeBeforeText, addrAfterText, sizeAfterText,
                static_cast<ElfSecName>(i));
        }
        return std::make_tuple(addrBeforeText, sizeBeforeText, addrAfterText, sizeAfterText);
    }

    void RunAssembler(const CompilerLog &log, bool fastCompileMode);

    void DisassemblerFunc(std::map<uintptr_t, std::string> &addr2name, uint64_t textOffset, const CompilerLog &log,
                          const MethodLogList &logList, std::ostringstream &codeStream);

    void DestroyModule();

    LLVMModule* GetModule() const
    {
        return llvmModule_;
    }

private:
    uintptr_t GetTextAddr() const
    {
        return assembler_->GetSectionAddr(ElfSecName::TEXT);
    }

    uint32_t GetTextSize() const
    {
        return assembler_->GetSectionSize(ElfSecName::TEXT);
    }

    void UpdateRODataInfo(uint64_t textAddr, uint64_t &addrBeforeText, uint32_t &sizeBeforeText,
                          uint64_t &addrAfterText, uint32_t &sizeAfterText, ElfSecName sec) const
    {
        uint64_t curSectionAddr = GetSectionAddr(sec);
        if (curSectionAddr == 0) {
            ASSERT(GetSectionSize(sec) == 0);
            return;
        }
        ASSERT(curSectionAddr != textAddr);
        if (curSectionAddr < textAddr) {
            addrBeforeText = (curSectionAddr < addrBeforeText) ? curSectionAddr : addrBeforeText;
            sizeBeforeText += GetSectionSize(sec);
        } else {
            addrAfterText = (curSectionAddr < addrAfterText) ? curSectionAddr : addrAfterText;
            sizeAfterText += GetSectionSize(sec);
        }
    }

    LLVMModule *llvmModule_ {nullptr};
    LLVMAssembler *assembler_ {nullptr};
    // record current module first function index in StubFileInfo/AnFileInfo
    uint32_t startIndex_ {static_cast<uint32_t>(-1)};
    uint32_t funcCount_ {0};
};

class FileGenerator {
public:
    FileGenerator(const CompilerLog *log, const MethodLogList *logList) : log_(log), logList_(logList) {};
    virtual ~FileGenerator()
    {
        codeStream_.clear();
        codeStream_.str("");
    }

    const CompilerLog GetLog() const
    {
        return *log_;
    }

    void PrintMergedCodeComment()
    {
        LOG_COMPILER(INFO) << "\n" << codeStream_.str();
    }

protected:
    std::vector<Module> modulePackage_ {};
    const CompilerLog *log_ {nullptr};
    const MethodLogList *logList_ {nullptr};
    std::ostringstream codeStream_;

    void RunLLVMAssembler()
    {
        for (auto m : modulePackage_) {
            m.RunAssembler(*(log_), false);
        }
    }

    void DestroyModule()
    {
        for (auto m : modulePackage_) {
            m.DestroyModule();
        }
    }

    void CollectStackMapDes(ModuleSectionDes& des);
};

class AOTFileGenerator : public FileGenerator {
public:
    AOTFileGenerator(const CompilerLog *log, const MethodLogList *logList, EcmaVM* vm, const std::string &triple)
        : FileGenerator(log, logList), vm_(vm), cfg_(triple) {}

    ~AOTFileGenerator() override = default;

    Module* GetLatestModule();

    uint32_t GetModuleVecSize() const;

    Module* AddModule(const std::string &name, const std::string &triple, LOptions option, bool logDebug);

    void GenerateMethodToEntryIndexMap();

    void CompileLatestModuleThenDestroy();

    void DestroyCollectedStackMapInfo();

    void GenerateMergedStackmapSection();

    bool CreateDirIfNotExist(const std::string &filename);

    bool SetFileModeAsDefault(const std::string &filename);

    // save function for aot files containing normal func translated from JS/TS
    void SaveAOTFile(const std::string &filename);
    void SaveSnapshotFile();

    std::vector<LLVMStackMapType::Pc2CallSiteInfo> &GetPc2StackMapVec()
    {
        return pc2CallSiteInfoVec_;
    }
    std::vector<LLVMStackMapType::Pc2Deopt> &GetPc2Deopt()
    {
        return pc2DeoptVec_;
    }

private:
    AnFileInfo aotInfo_;
    std::vector<LLVMStackMapType::Pc2CallSiteInfo> pc2CallSiteInfoVec_;
    std::vector<LLVMStackMapType::Pc2Deopt> pc2DeoptVec_;
    EcmaVM* vm_;
    CompilationConfig cfg_;
    // MethodID->EntryIndex
    std::map<uint32_t, uint32_t> methodToEntryIndexMap_ {};

    // collect aot component info
    void CollectCodeInfo(Module *module, uint32_t moduleIdx);

    uint64_t RollbackTextSize(Module *module);
};

enum class StubFileKind {
    BC,
    COM,
    BUILTIN
};

class StubFileGenerator : public FileGenerator {
public:
    StubFileGenerator(const CompilerLog *log, const MethodLogList *logList, const std::string &triple)
        : FileGenerator(log, logList),
          cfg_(triple)
    {
    }
    ~StubFileGenerator() override = default;

    Module* AddModule(NativeAreaAllocator *allocator, const std::string &name, const std::string &triple,
                      LOptions option, bool logDebug, StubFileKind k);

    void DisassembleEachFunc(std::map<uintptr_t, std::string> &addr2name)
    {
        for (auto m : modulePackage_) {
            m.DisassemblerFunc(addr2name, 0, *(log_), *(logList_), codeStream_);
        }
        PrintMergedCodeComment();
    }

    void DisassembleAsmStubs(std::map<uintptr_t, std::string> &addr2name);
    // save function funcs for aot files containing stubs
    void SaveStubFile(const std::string &filename);

private:
    void RunAsmAssembler();
    void CollectAsmStubCodeInfo(std::map<uintptr_t, std::string> &addr2name, uint32_t bridgeModuleIdx);
    void CollectCodeInfo();

    StubFileInfo stubInfo_;
    AssemblerModule asmModule_;
    CompilationConfig cfg_;
};
}  // namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_FILE_GENERATORS_H
