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
#include "ecmascript/common.h"
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/compiler/assembler_module.h"
#include "ecmascript/compiler/codegen/llvm/llvm_codegen.h"
#include "ecmascript/compiler/codegen/llvm/llvm_ir_builder.h"
#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/compiler/ir_module.h"
#include "ecmascript/compiler/jit_compilation_env.h"
#include "ecmascript/stackmap/cg_stackmap.h"
#include "ecmascript/mem/machine_code.h"

namespace panda::ecmascript::kungfu {
class Module {
public:
    Module() = default;
    Module(IRModule *module, Assembler *assembler)
        : irModule_(module), assembler_(assembler)
    {
    }

    std::vector<uintptr_t> GetFuncEntryPoints();

    void CollectFuncEntryInfo(const std::vector<uintptr_t>& entrys, std::map<uintptr_t, std::string> &addr2name,
                              StubFileInfo &stubInfo, uint32_t moduleIndex, const CompilerLog &log);

    void CollectFuncEntryInfo(std::map<uintptr_t, std::string> &addr2name, AnFileInfo &aotInfo, uint32_t fileIndex,
                              uint32_t moduleIndex, const CompilerLog &log);

#ifdef COMPILE_MAPLE
    void CollectFuncEntryInfoByLiteCG(std::map<uintptr_t, std::string> &addr2name, AnFileInfo &aotInfo,
                                      uint32_t fileIndex, uint32_t moduleIndex);
#endif

    bool IsRelaSection(ElfSecName sec) const
    {
        return sec == ElfSecName::RELATEXT;
    }

    void CollectModuleSectionDes(ModuleSectionDes &moduleDes) const;

    void CollectAnModuleSectionDes(ModuleSectionDes &moduleDes, uint64_t textOffset,
                                   CGStackMapInfo &stackMapInfo) const;

    void CollectStackMapDes(ModuleSectionDes &moduleDes) const;

    void CollectAnStackMapDes(ModuleSectionDes& des, uint64_t textOffset,
                              CGStackMapInfo &stackMapInfo) const;

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

    void PUBLIC_API RunAssembler(const CompilerLog &log, bool fastCompileMode, bool isJit = false,
                                 const std::string &filename = "");

    void DisassemblerFunc(std::map<uintptr_t, std::string> &addr2name, uint64_t textOffset, const CompilerLog &log,
                          const MethodLogList &logList, std::ostringstream &codeStream);

    void DestroyModule();

    IRModule* GetModule() const
    {
        return irModule_;
    }

    bool IsLLVM() const
    {
        return irModule_->GetModuleKind() == MODULE_LLVM;
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

    IRModule *irModule_ {nullptr};
    Assembler *assembler_ {nullptr};
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
        if (codeStream_.str().empty()) {
            return;
        }
        LOG_COMPILER(INFO) << "\n" << codeStream_.str();
    }

protected:
    std::vector<Module> modulePackage_ {};
    const CompilerLog *log_ {nullptr};
    const MethodLogList *logList_ {nullptr};
    std::ostringstream codeStream_;

    virtual void RunLLVMAssembler()
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
    AOTFileGenerator(const CompilerLog *log, const MethodLogList *logList, CompilationEnv *env,
                     const std::string &triple, bool useLiteCG = false, size_t anFileMaxByteSize = 0)
        : FileGenerator(log, logList),
          cfg_(triple),
          compilationEnv_(env),
          useLiteCG_(useLiteCG),
          anFileMaxByteSize_(anFileMaxByteSize)
    {
    }

    ~AOTFileGenerator() override = default;

    Module* GetLatestModule();

    uint32_t GetModuleVecSize() const;

    Module* AddModule(const std::string &name, const std::string &triple, LOptions option, bool logDebug,
        bool isJit = false);

    void CompileLatestModuleThenDestroy(bool isJit = false);

    void DestroyCollectedStackMapInfo();

    void GenerateMergedStackmapSection();

    static bool CreateDirIfNotExist(const std::string& filename);

    bool SetFileModeAsDefault(const std::string &filename);

    // save function for aot files containing normal func translated from JS/TS
    bool PUBLIC_API SaveAOTFile(const std::string &filename, const std::string &appSignature,
                                const std::unordered_map<CString, uint32_t> &fileNameToChecksumMap);

    static void PUBLIC_API SaveEmptyAOTFile(const std::string& filename, const std::string& appSignature,
                                            bool isAnFile);

    bool PUBLIC_API SaveSnapshotFile();

    void SetCurrentCompileFileName(CString fileName)
    {
        curCompileFileName_ = fileName.c_str();
    }

    bool GetMemoryCodeInfos(MachineCodeDesc &machineCodeDesc);
    void JitCreateLitecgModule();
    bool isAArch64() const;

    bool PUBLIC_API CreateAOTCodeCommentFile(const std::string &filename);

    const std::string &GetAotCodeCommentFile() const
    {
        return aotCodeCommentFile_;
    }

    void SetAotCodeCommentFile(const std::string &filename)
    {
        aotCodeCommentFile_ = filename;
    }

protected:
    AnFileInfo aotInfo_;
    CompilationConfig cfg_;

private:
    // collect aot component info
    void CollectCodeInfo(Module *module, uint32_t moduleIdx);

    uint64_t RollbackTextSize(Module *module);

    CGStackMapInfo *stackMapInfo_ = nullptr;
    CompilationEnv *compilationEnv_ {nullptr};
    std::string curCompileFileName_;
    // MethodID->EntryIndex
    std::map<uint32_t, uint32_t> methodToEntryIndexMap_ {};
    const bool useLiteCG_;
    CodeInfo::CodeSpaceOnDemand jitCodeSpace_ {};
    std::string aotCodeCommentFile_ = "";
    size_t anFileMaxByteSize_ {0_MB};
};

enum class StubFileKind {
    BC,
    COM,
    BUILTIN,
    BASELINE
};

class StubFileGenerator : public FileGenerator {
public:
    StubFileGenerator(const CompilerLog* log, const MethodLogList* logList, const std::string& triple,
                      bool concurrentCompile)
        : FileGenerator(log, logList),
          cfg_(triple),
          concurrentCompile_(concurrentCompile)
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
protected:
    void RunLLVMAssembler() override;
private:
    void RunAsmAssembler();
    void CollectAsmStubCodeInfo(std::map<uintptr_t, std::string> &addr2name, uint32_t bridgeModuleIdx);
    void CollectCodeInfo();

    StubFileInfo stubInfo_;
    AssemblerModule asmModule_;
    CompilationConfig cfg_;
    CodeInfo::CodeSpaceOnDemand jitCodeSpace_ {};
    bool concurrentCompile_ {false};
};
}  // namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_FILE_GENERATORS_H
