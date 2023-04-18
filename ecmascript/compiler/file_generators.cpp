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
        LOG_FULL(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
    std::shared_ptr<uint8_t> ptr = nullptr;
    uint32_t size = 0;
    ArkStackMapBuilder builder;
    std::tie(ptr, size) = builder.Run(std::move(stackmapPtr), textAddr, GetTriple());
    des.EraseSec(ElfSecName::LLVM_STACKMAP);
    des.SetArkStackMapPtr(ptr);
    des.SetArkStackMapSize(size);
}

Triple Module::GetTriple() const
{
    return llvmModule_->GetCompilationConfig()->GetTriple();
}

void Module::CollectFuncEntryInfo(std::map<uintptr_t, std::string> &addr2name, StubFileInfo &stubInfo,
                                  uint32_t moduleIndex, const CompilerLog &log)
{
    auto engine = assembler_->GetEngine();
    auto callSigns = llvmModule_->GetCSigns();
    std::map<uintptr_t, int> addr2FpToPrevFrameSpDelta;
    std::vector<uint64_t> funSizeVec;
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
        stubInfo.AddEntry(cs->GetTargetKind(), false, cs->GetID(), entrys[j] - codeBuff, moduleIndex, delta,
                          funcSize, info);
        ASSERT(!cs->GetName().empty());
        addr2name[entrys[j]] = cs->GetName();
    }
}

void Module::CollectFuncEntryInfo(std::map<uintptr_t, std::string> &addr2name, AnFileInfo &aotInfo,
                                  uint32_t moduleIndex, const CompilerLog &log)
{
    auto engine = assembler_->GetEngine();
    std::vector<std::tuple<uint64_t, size_t, int>> funcInfo; // entry idx delta
    std::vector<kungfu::CalleeRegAndOffsetVec> calleeSaveRegisters; // entry idx delta
    llvmModule_->IteratefuncIndexMap([&](size_t idx, LLVMValueRef func) {
        uint64_t funcEntry = reinterpret_cast<uintptr_t>(LLVMGetPointerToGlobal(engine, func));
        uint64_t length = 0;
        std::string funcName(LLVMGetValueName2(func, reinterpret_cast<size_t *>(&length)));
        ASSERT(length != 0);
        addr2name[funcEntry] = funcName;
        int delta = assembler_->GetFpDeltaPrevFramSp(func, log);
        ASSERT(delta >= 0 && (delta % sizeof(uintptr_t) == 0));
        funcInfo.emplace_back(std::tuple(funcEntry, idx, delta));
        kungfu::CalleeRegAndOffsetVec info = assembler_->GetCalleeReg2Offset(func, log);
        calleeSaveRegisters.emplace_back(info);
    });
    auto codeBuff = assembler_->GetSectionAddr(ElfSecName::TEXT);
    const size_t funcCount = funcInfo.size();
    funcCount_ = funcCount;
    startIndex_ = aotInfo.GetEntrySize();
    for (size_t i = 0; i < funcInfo.size(); i++) {
        uint64_t funcEntry;
        size_t idx;
        int delta;
        uint32_t funcSize;
        std::tie(funcEntry, idx, delta) = funcInfo[i];
        if (i < funcCount - 1) {
            funcSize = std::get<0>(funcInfo[i + 1]) - funcEntry;
        } else {
            funcSize = codeBuff + assembler_->GetSectionSize(ElfSecName::TEXT) - funcEntry;
        }
        auto found = addr2name[funcEntry].find(panda::ecmascript::JSPandaFile::ENTRY_FUNCTION_NAME);
        bool isMainFunc = found != std::string::npos;
        aotInfo.AddEntry(CallSignature::TargetKind::JSFUNCTION, isMainFunc, idx,
                         funcEntry - codeBuff, moduleIndex, delta, funcSize, calleeSaveRegisters[i]);
    }
}

void Module::CollectModuleSectionDes(ModuleSectionDes &moduleDes, bool stub) const
{
    ASSERT(assembler_ != nullptr);
    assembler_->IterateSecInfos([&](size_t i, std::pair<uint8_t *, size_t> secInfo) {
        auto curSec = ElfSection(i);
        ElfSecName sec = curSec.GetElfEnumValue();
        if (stub && IsRelaSection(sec)) {
            moduleDes.EraseSec(sec);
        } else { // aot need relocated; stub don't need collect relocated section
            moduleDes.SetSecAddr(reinterpret_cast<uint64_t>(secInfo.first), sec);
            moduleDes.SetSecSize(secInfo.second, sec);
            moduleDes.SetStartIndex(startIndex_);
            moduleDes.SetFuncCount(funcCount_);
        }
    });
    CollectStackMapDes(moduleDes);
}

uint32_t Module::GetSectionSize(ElfSecName sec) const
{
    return assembler_->GetSectionSize(sec);
}

uintptr_t Module::GetSectionAddr(ElfSecName sec) const
{
    return assembler_->GetSectionAddr(sec);
}

void Module::RunAssembler(const CompilerLog &log)
{
    assembler_->Run(log);
}

void Module::DisassemblerFunc(std::map<uintptr_t, std::string> &addr2name,
                              const CompilerLog &log, const MethodLogList &logList)
{
    assembler_->Disassemble(addr2name, log, logList);
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

void StubFileGenerator::CollectAsmStubCodeInfo(std::map<uintptr_t, std::string> &addr2name,
                                               uint32_t bridgeModuleIdx)
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
        stubInfo_.AddEntry(cs->GetTargetKind(), false, cs->GetID(), entryOffset, bridgeModuleIdx, 0, funSize);
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
        modulePackage_[i].CollectModuleSectionDes(des, true);
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

void AOTFileGenerator::CollectCodeInfo()
{
    std::map<uintptr_t, std::string> addr2name;
    for (size_t i = 0; i < modulePackage_.size(); i++) {
        modulePackage_[i].CollectFuncEntryInfo(addr2name, aotInfo_, i, GetLog());
        ModuleSectionDes des;
        modulePackage_[i].CollectModuleSectionDes(des);
        aotInfo_.AddModuleDes(des);
    }
    if (log_->OutputASM()) {
        DisassembleEachFunc(addr2name);
    }
}

Module* AOTFileGenerator::AddModule(const std::string &name, const std::string &triple, LOptions option)
{
    LLVMModule* m = new LLVMModule(vm_->GetNativeAreaAllocator(), name, triple);
    LLVMAssembler* ass = new LLVMAssembler(m->GetModule(), option);
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
    stubInfo_.Save(filename);
}

void AOTFileGenerator::SaveAOTFile(const std::string &filename)
{
    TimeScope timescope("LLVMCodeGenPass-AN", const_cast<CompilerLog *>(log_));
    RunLLVMAssembler();
    CollectCodeInfo();
    GenerateMethodToEntryIndexMap();
    aotInfo_.Save(filename, cfg_.GetTriple());
    DestroyModule();
}

void AOTFileGenerator::SaveSnapshotFile()
{
    TimeScope timescope("LLVMCodeGenPass-AI", const_cast<CompilerLog *>(log_));
    Snapshot snapshot(vm_);
    const CString snapshotPath(vm_->GetJSOptions().GetAOTOutputFile().c_str());
    vm_->GetTSManager()->ResolveSnapshotConstantPool(methodToEntryIndexMap_);
    snapshot.Serialize(snapshotPath + AOTFileManager::FILE_EXTENSION_AI);
}
}  // namespace panda::ecmascript::kungfu
