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

#include "ecmascript/compiler/file_generators.h"

#include "ecmascript/platform/code_sign.h"
#include "ecmascript/platform/directory.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/stackmap/ark_stackmap_builder.h"
#include "ecmascript/stackmap/llvm_stackmap_parser.h"

namespace panda::ecmascript::kungfu {
void Module::CollectStackMapDes(ModuleSectionDes& des) const
{
    uint32_t stackmapSize = des.GetSecSize(ElfSecName::LLVM_STACKMAP);
    std::unique_ptr<uint8_t[]> stackmapPtr(std::make_unique<uint8_t[]>(stackmapSize));
    uint64_t addr = des.GetSecAddr(ElfSecName::LLVM_STACKMAP);
    if (addr == 0) { // assembler stub don't existed llvm stackmap
        return;
    }
    uint64_t textAddr = des.GetSecAddr(ElfSecName::TEXT);
    if (memcpy_s(stackmapPtr.get(), stackmapSize, reinterpret_cast<void *>(addr), stackmapSize) != EOK) {
        LOG_COMPILER(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
    std::shared_ptr<uint8_t> ptr = nullptr;
    uint32_t size = 0;
    ArkStackMapBuilder builder;
    std::tie(ptr, size) = builder.Run(std::move(stackmapPtr), textAddr, llvmModule_->GetTriple());
    des.EraseSec(ElfSecName::LLVM_STACKMAP);
    des.SetArkStackMapPtr(ptr);
    des.SetArkStackMapSize(size);
}

void Module::CollectAnStackMapDes(ModuleSectionDes& des, uint64_t textOffset,
    std::vector<LLVMStackMapType::Pc2CallSiteInfo> &pc2CallsiteInfoVec,
    std::vector<LLVMStackMapType::Pc2Deopt> &pc2DeoptVec) const
{
    uint32_t stackmapSize = des.GetSecSize(ElfSecName::LLVM_STACKMAP);
    std::unique_ptr<uint8_t[]> stackmapPtr(std::make_unique<uint8_t[]>(stackmapSize));
    uint64_t addr = des.GetSecAddr(ElfSecName::LLVM_STACKMAP);
    if (addr == 0) { // assembler stub don't existed llvm stackmap
        return;
    }
    uint64_t textAddr = des.GetSecAddr(ElfSecName::TEXT);
    if (memcpy_s(stackmapPtr.get(), stackmapSize, reinterpret_cast<void *>(addr), stackmapSize) != EOK) {
        LOG_COMPILER(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
    ArkStackMapBuilder builder;
    builder.Collect(std::move(stackmapPtr), textAddr, textOffset, pc2CallsiteInfoVec, pc2DeoptVec);
    des.EraseSec(ElfSecName::LLVM_STACKMAP);
}

void Module::CollectFuncEntryInfo(std::map<uintptr_t, std::string> &addr2name, StubFileInfo &stubInfo,
                                  uint32_t moduleIndex, const CompilerLog &log)
{
    auto engine = assembler_->GetEngine();
    auto callSigns = llvmModule_->GetCSigns();
    std::vector<uintptr_t> entrys;
    for (size_t j = 0; j < llvmModule_->GetFuncCount(); j++) {
        LLVMValueRef func = llvmModule_->GetFunction(j);
        ASSERT(func != nullptr);
        uintptr_t entry = reinterpret_cast<uintptr_t>(LLVMGetPointerToGlobal(engine, func));
        entrys.push_back(entry);
    }
    auto codeBuff = assembler_->GetSectionAddr(ElfSecName::TEXT);
    const size_t funcCount = llvmModule_->GetFuncCount();
    funcCount_ = funcCount;
    startIndex_ = stubInfo.GetEntrySize();
    for (size_t j = 0; j < funcCount; j++) {
        auto cs = callSigns[j];
        LLVMValueRef func = llvmModule_->GetFunction(j);
        ASSERT(func != nullptr);
        int delta = assembler_->GetFpDeltaPrevFramSp(func, log);
        ASSERT(delta >= 0 && (delta % sizeof(uintptr_t) == 0));
        uint32_t funcSize = 0;
        if (j < funcCount - 1) {
            funcSize = entrys[j + 1] - entrys[j];
        } else {
            funcSize = codeBuff + assembler_->GetSectionSize(ElfSecName::TEXT) - entrys[j];
        }
        kungfu::CalleeRegAndOffsetVec info = assembler_->GetCalleeReg2Offset(func, log);
        stubInfo.AddEntry(cs->GetTargetKind(), false, false, cs->GetID(), entrys[j] - codeBuff, moduleIndex, delta,
                          funcSize, info);
        ASSERT(!cs->GetName().empty());
        addr2name[entrys[j]] = cs->GetName();
    }
}

void Module::CollectFuncEntryInfo(std::map<uintptr_t, std::string> &addr2name, AnFileInfo &aotInfo,
                                  uint32_t moduleIndex, const CompilerLog &log)
{
    auto engine = assembler_->GetEngine();
    std::vector<std::tuple<uint64_t, size_t, int, bool>> funcInfo; // entry idx delta
    std::vector<kungfu::CalleeRegAndOffsetVec> calleeSaveRegisters; // entry idx delta
    // 1.Compile all functions and collect function infos
    llvmModule_->IteratefuncIndexMap([&](size_t idx, LLVMValueRef func, bool isFastCall) {
        uint64_t funcEntry = reinterpret_cast<uintptr_t>(LLVMGetPointerToGlobal(engine, func));
        uint64_t length = 0;
        std::string funcName(LLVMGetValueName2(func, reinterpret_cast<size_t *>(&length)));
        ASSERT(length != 0);
        addr2name[funcEntry] = funcName;
        int delta = assembler_->GetFpDeltaPrevFramSp(func, log);
        ASSERT(delta >= 0 && (delta % sizeof(uintptr_t) == 0));
        funcInfo.emplace_back(std::tuple(funcEntry, idx, delta, isFastCall));
        kungfu::CalleeRegAndOffsetVec info = assembler_->GetCalleeReg2Offset(func, log);
        calleeSaveRegisters.emplace_back(info);
    });
    // 2.After all functions compiled, the module sections would be fixed
    uintptr_t textAddr = GetTextAddr();
    uint32_t textSize = GetTextSize();
    uintptr_t rodataAddrBeforeText = 0;
    uint32_t rodataSizeBeforeText = 0;
    uintptr_t rodataAddrAfterText = 0;
    uint32_t rodataSizeAfterText = 0;
    std::tie(rodataAddrBeforeText, rodataSizeBeforeText, rodataAddrAfterText, rodataSizeAfterText) =
        GetMergedRODataAddrAndSize(textAddr);
    aotInfo.AlignTextSec(AOTFileInfo::PAGE_ALIGN);
    if (rodataSizeBeforeText != 0) {
        aotInfo.UpdateCurTextSecOffset(rodataSizeBeforeText);
        aotInfo.AlignTextSec(AOTFileInfo::TEXT_SEC_ALIGN);
    }

    const size_t funcCount = funcInfo.size();
    funcCount_ = funcCount;
    startIndex_ = aotInfo.GetEntrySize();
    // 3.Add function entries based on the module sections
    for (size_t i = 0; i < funcInfo.size(); i++) {
        uint64_t funcEntry;
        size_t idx;
        int delta;
        bool isFastCall;
        uint32_t funcSize;
        std::tie(funcEntry, idx, delta, isFastCall) = funcInfo[i];
        if (i < funcCount - 1) {
            funcSize = std::get<0>(funcInfo[i + 1]) - funcEntry;
        } else {
            funcSize = textAddr + textSize - funcEntry;
        }
        auto found = addr2name[funcEntry].find(panda::ecmascript::JSPandaFile::ENTRY_FUNCTION_NAME);
        bool isMainFunc = found != std::string::npos;
        uint64_t offset = funcEntry - textAddr + aotInfo.GetCurTextSecOffset();
        aotInfo.AddEntry(CallSignature::TargetKind::JSFUNCTION, isMainFunc, isFastCall, idx,
                         offset, moduleIndex, delta, funcSize, calleeSaveRegisters[i]);
    }
    aotInfo.UpdateCurTextSecOffset(textSize);
    if (rodataSizeAfterText != 0) {
        aotInfo.AlignTextSec(AOTFileInfo::DATA_SEC_ALIGN);
        aotInfo.UpdateCurTextSecOffset(rodataSizeAfterText);
    }
}

void Module::CollectModuleSectionDes(ModuleSectionDes &moduleDes) const
{
    ASSERT(assembler_ != nullptr);
    assembler_->IterateSecInfos([&](size_t i, std::pair<uint8_t *, size_t> secInfo) {
        auto curSec = ElfSection(i);
        ElfSecName sec = curSec.GetElfEnumValue();
        if (IsRelaSection(sec)) {
            moduleDes.EraseSec(sec);
        } else { // aot need relocated; stub don't need collect relocated section
            moduleDes.SetSecAddrAndSize(sec, reinterpret_cast<uint64_t>(secInfo.first), secInfo.second);
            moduleDes.SetStartIndex(startIndex_);
            moduleDes.SetFuncCount(funcCount_);
        }
    });
    CollectStackMapDes(moduleDes);
}

void Module::CollectAnModuleSectionDes(ModuleSectionDes &moduleDes, uint64_t textOffset,
    std::vector<LLVMStackMapType::Pc2CallSiteInfo> &pc2CallsiteInfoVec,
    std::vector<LLVMStackMapType::Pc2Deopt> &pc2DeoptVec) const
{
    ASSERT(assembler_ != nullptr);
    assembler_->IterateSecInfos([&](size_t i, std::pair<uint8_t *, size_t> secInfo) {
        auto curSec = ElfSection(i);
        ElfSecName sec = curSec.GetElfEnumValue();
        // aot need relocated; stub don't need collect relocated section
        moduleDes.SetSecAddrAndSize(sec, reinterpret_cast<uint64_t>(secInfo.first), secInfo.second);
        moduleDes.SetStartIndex(startIndex_);
        moduleDes.SetFuncCount(funcCount_);
    });
    CollectAnStackMapDes(moduleDes, textOffset, pc2CallsiteInfoVec, pc2DeoptVec);
}

uint32_t Module::GetSectionSize(ElfSecName sec) const
{
    return assembler_->GetSectionSize(sec);
}

uintptr_t Module::GetSectionAddr(ElfSecName sec) const
{
    return assembler_->GetSectionAddr(sec);
}

void Module::RunAssembler(const CompilerLog &log, bool fastCompileMode)
{
    assembler_->Run(log, fastCompileMode);
}

void Module::DisassemblerFunc(std::map<uintptr_t, std::string> &addr2name, uint64_t textOffset,
                              const CompilerLog &log, const MethodLogList &logList, std::ostringstream &codeStream)
{
    assembler_->Disassemble(addr2name, textOffset, log, logList, codeStream);
}

void Module::DestroyModule()
{
    if (llvmModule_ != nullptr) {
        delete llvmModule_;
        llvmModule_ = nullptr;
    }
    if (assembler_ != nullptr) {
        delete assembler_;
        assembler_ = nullptr;
    }
}

void StubFileGenerator::CollectAsmStubCodeInfo(std::map<uintptr_t, std::string> &addr2name, uint32_t bridgeModuleIdx)
{
    uint32_t funSize = 0;
    for (size_t i = 0; i < asmModule_.GetFunctionCount(); i++) {
        auto cs = asmModule_.GetCSign(i);
        auto entryOffset = asmModule_.GetFunction(cs->GetID());
        if (i < asmModule_.GetFunctionCount() - 1) {
            auto nextcs = asmModule_.GetCSign(i + 1);
            funSize = asmModule_.GetFunction(nextcs->GetID()) - entryOffset;
        } else {
            funSize = asmModule_.GetBufferSize() - entryOffset;
        }
        stubInfo_.AddEntry(cs->GetTargetKind(), false, false, cs->GetID(), entryOffset, bridgeModuleIdx, 0, funSize);
        ASSERT(!cs->GetName().empty());
        addr2name[entryOffset] = cs->GetName();
    }
}

void StubFileGenerator::CollectCodeInfo()
{
    std::map<uintptr_t, std::string> stubAddr2Name;
    for (size_t i = 0; i < modulePackage_.size(); i++) {
        modulePackage_[i].CollectFuncEntryInfo(stubAddr2Name, stubInfo_, i, GetLog());
        ModuleSectionDes des;
        modulePackage_[i].CollectModuleSectionDes(des);
        stubInfo_.AddModuleDes(des);
    }
    std::map<uintptr_t, std::string> asmAddr2Name;
    // idx for bridge module is the one after last module in modulePackage
    CollectAsmStubCodeInfo(asmAddr2Name, modulePackage_.size());
    if (log_->OutputASM()) {
        DisassembleAsmStubs(asmAddr2Name);
        DisassembleEachFunc(stubAddr2Name);
    }
}

void StubFileGenerator::DisassembleAsmStubs(std::map<uintptr_t, std::string> &addr2name)
{
    std::string tri = cfg_.GetTripleStr();
    uint8_t *buf = reinterpret_cast<uint8_t*>(stubInfo_.GetAsmStubAddr());
    size_t size = stubInfo_.GetAsmStubSize();
    LLVMAssembler::Disassemble(&addr2name, tri, buf, size);
}

uint64_t AOTFileGenerator::RollbackTextSize(Module *module)
{
    uint64_t textAddr = module->GetSectionAddr(ElfSecName::TEXT);
    uint32_t textSize = module->GetSectionSize(ElfSecName::TEXT);
    uint64_t rodataAddrBeforeText = 0;
    uint32_t rodataSizeBeforeText = 0;
    uint64_t rodataAddrAfterText = 0;
    uint32_t rodataSizeAfterText = 0;
    std::tie(rodataAddrBeforeText, rodataSizeBeforeText, rodataAddrAfterText, rodataSizeAfterText) =
        module->GetMergedRODataAddrAndSize(textAddr);
    uint64_t textStart = 0;
    if (rodataSizeAfterText == 0) {
        textStart = aotInfo_.GetCurTextSecOffset() - textSize;
    } else {
        textStart = aotInfo_.GetCurTextSecOffset() - textSize - rodataSizeAfterText;
        textStart = AlignDown(textStart, AOTFileInfo::DATA_SEC_ALIGN);
    }
    return textStart;
}

void AOTFileGenerator::CollectCodeInfo(Module *module, uint32_t moduleIdx)
{
    std::map<uintptr_t, std::string> addr2name;
    module->CollectFuncEntryInfo(addr2name, aotInfo_, moduleIdx, GetLog());
    ModuleSectionDes des;
    uint64_t textOffset = RollbackTextSize(module);
    module->CollectAnModuleSectionDes(des, textOffset, pc2CallSiteInfoVec_, pc2DeoptVec_);
    aotInfo_.AddModuleDes(des);
    if (log_->OutputASM()) {
        module->DisassemblerFunc(addr2name, textOffset, *(log_), *(logList_), codeStream_);
    }
}

Module* AOTFileGenerator::GetLatestModule()
{
    return &modulePackage_.back();
}

uint32_t AOTFileGenerator::GetModuleVecSize() const
{
    return modulePackage_.size();
}

Module* AOTFileGenerator::AddModule(const std::string &name, const std::string &triple, LOptions option, bool logDebug)
{
    LLVMModule* m = new LLVMModule(vm_->GetNativeAreaAllocator(), name, logDebug, triple);
    LLVMAssembler* ass = new LLVMAssembler(m, option);
    modulePackage_.emplace_back(Module(m, ass));
    return &modulePackage_.back();
}

void AOTFileGenerator::GenerateMethodToEntryIndexMap()
{
    const std::vector<AOTFileInfo::FuncEntryDes> &entries = aotInfo_.GetStubs();
    uint32_t entriesSize = entries.size();
    for (uint32_t i = 0; i < entriesSize; ++i) {
        const AOTFileInfo::FuncEntryDes &entry = entries[i];
        methodToEntryIndexMap_[entry.indexInKindOrMethodId_] = i;
    }
}

Module* StubFileGenerator::AddModule(NativeAreaAllocator *allocator, const std::string &name, const std::string &triple,
                                     LOptions option, bool logDebug, StubFileKind kind)
{
    LLVMModule* m = new LLVMModule(allocator, name, logDebug, triple);
    if (kind == StubFileKind::BC) {
        m->SetUpForBytecodeHandlerStubs();
    } else if (kind == StubFileKind::COM) {
        m->SetUpForCommonStubs();
    } else {
        ASSERT(kind == StubFileKind::BUILTIN);
        m->SetUpForBuiltinsStubs();
    }
    LLVMAssembler* ass = new LLVMAssembler(m, option);
    modulePackage_.emplace_back(Module(m, ass));
    return &modulePackage_.back();
}

void StubFileGenerator::RunAsmAssembler()
{
    NativeAreaAllocator allocator;
    Chunk chunk(&allocator);
    asmModule_.Run(&cfg_, &chunk);

    auto buffer = asmModule_.GetBuffer();
    auto bufferSize = asmModule_.GetBufferSize();
    if (bufferSize == 0U) {
        return;
    }
    stubInfo_.FillAsmStubTempHolder(buffer, bufferSize);
    stubInfo_.accumulateTotalSize(bufferSize);
}

void StubFileGenerator::SaveStubFile(const std::string &filename)
{
    RunLLVMAssembler();
    RunAsmAssembler();
    CollectCodeInfo();
    stubInfo_.Save(filename, cfg_.GetTriple());
}

void AOTFileGenerator::CompileLatestModuleThenDestroy()
{
    Module *latestModule = GetLatestModule();
    uint32_t latestModuleIdx = GetModuleVecSize() - 1;
    {
        TimeScope timescope("LLVMIROpt", const_cast<CompilerLog *>(log_));
        bool fastCompileMode = vm_->GetJSOptions().GetFastAOTCompileMode();
        latestModule->RunAssembler(*(log_), fastCompileMode);
    }
    {
        TimeScope timescope("LLVMCodeGen", const_cast<CompilerLog *>(log_));
        CollectCodeInfo(latestModule, latestModuleIdx);
    }
    // message has been put into aotInfo, so latestModule could be destroyed
    latestModule->DestroyModule();
}

void AOTFileGenerator::DestroyCollectedStackMapInfo()
{
    pc2CallSiteInfoVec_.clear();
    pc2DeoptVec_.clear();
}

void AOTFileGenerator::GenerateMergedStackmapSection()
{
    ArkStackMapBuilder builder;
    std::shared_ptr<uint8_t> ptr = nullptr;
    uint32_t size = 0;
    std::tie(ptr, size) = builder.GenerateArkStackMap(pc2CallSiteInfoVec_, pc2DeoptVec_, cfg_.GetTriple());
    aotInfo_.UpdateStackMap(ptr, size, 0);
    DestroyCollectedStackMapInfo();
}

bool AOTFileGenerator::CreateDirIfNotExist(const std::string &filename)
{
    std::string realPath;
    if (!panda::ecmascript::RealPath(filename, realPath, false)) {
        return false;
    }
    auto index = realPath.find_last_of('/');
    if (index == std::string::npos) {
        return true;
    }
    std::string path = realPath.substr(0, index);
    if (!panda::ecmascript::ForceCreateDirectory(path)) {
        LOG_COMPILER(ERROR) << "Fail to make dir:" << path;
        return false;
    }
    return panda::ecmascript::SetDirModeAsDefault(path);
}

void AOTFileGenerator::SaveAOTFile(const std::string &filename)
{
    if (aotInfo_.GetTotalCodeSize() == 0) {
        LOG_HOST_TOOL_ERROR << "The an file generated by the aot compiler is empty! "
                            << "Maybe file in apPath is empty or all methods in ap file are mismatched";
        LOG_COMPILER(ERROR) << "error: code size of generated an file is empty!";
        return;
    }
    if (!CreateDirIfNotExist(filename)) {
        LOG_COMPILER(ERROR) << "Fail to access dir:" << filename;
        return;
    }
    PrintMergedCodeComment();
    GenerateMergedStackmapSection();
    GenerateMethodToEntryIndexMap();
    aotInfo_.Save(filename, cfg_.GetTriple());
    if (!panda::ecmascript::SetFileModeAsDefault(filename)) {
        LOG_COMPILER(ERROR) << "Fail to set an file mode:" << filename;
    }
    panda::ecmascript::CodeSignForAOTFile(filename);
}

void AOTFileGenerator::SaveSnapshotFile()
{
    TimeScope timescope("LLVMCodeGenPass-AI", const_cast<CompilerLog *>(log_));
    Snapshot snapshot(vm_);
    const CString snapshotPath(vm_->GetJSOptions().GetAOTOutputFile().c_str());
    vm_->GetJSThread()->GetCurrentEcmaContext()->GetTSManager()->ResolveSnapshotConstantPool(methodToEntryIndexMap_);
    CString aiPath = snapshotPath + AOTFileManager::FILE_EXTENSION_AI;
    snapshot.Serialize(aiPath);
    if (!panda::ecmascript::SetFileModeAsDefault(aiPath.c_str())) {
        LOG_COMPILER(ERROR) << "Fail to set ai file mode:" << aiPath;
    }
}
}  // namespace panda::ecmascript::kungfu
