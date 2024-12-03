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

#include "ecmascript/common.h"
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/compiler/pgo_type/pgo_type_manager.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/platform/code_sign.h"
#include "ecmascript/platform/directory.h"
#include "ecmascript/platform/os.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/stackmap/ark_stackmap_builder.h"
#include "ecmascript/stackmap/llvm/llvm_stackmap_parser.h"
#ifdef COMPILE_MAPLE
#include "ecmascript/compiler/codegen/maple/litecg_codegen.h"
#include "ecmascript/compiler/codegen/maple/litecg_ir_builder.h"
#include "ecmascript/compiler/codegen/maple/maple_be/include/litecg/litecg.h"
#include "ecmascript/stackmap/litecg/litecg_stackmap_type.h"
#ifdef JIT_ENABLE_CODE_SIGN
#include "ecmascript/compiler/jit_signcode.h"
#endif
#endif
#include "ecmascript/jit/jit.h"
#include "ecmascript/jit/jit_task.h"
#include "ecmascript/compiler/jit_compiler.h"

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
    std::tie(ptr, size) = builder.Run(std::move(stackmapPtr), textAddr, irModule_->GetTriple());
    des.EraseSec(ElfSecName::LLVM_STACKMAP);
    des.SetArkStackMapPtr(ptr);
    des.SetArkStackMapSize(size);
}

void Module::CollectAnStackMapDes(ModuleSectionDes& des, uint64_t textOffset,
                                  CGStackMapInfo &stackMapInfo) const
{
#ifdef COMPILE_MAPLE
    if (!IsLLVM()) {
        static_cast<LiteCGAssembler*>(assembler_)->CollectAnStackMap(stackMapInfo);
        return;
    }
#endif
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
    builder.Collect(std::move(stackmapPtr), textAddr, textOffset, stackMapInfo);
    des.EraseSec(ElfSecName::LLVM_STACKMAP);
}

std::vector<uintptr_t> Module::GetFuncEntryPoints()
{
    std::vector<uintptr_t> entrys;
    if (irModule_->GetModuleKind() != MODULE_LLVM) {
        std::cout << "GetFuncEntryPoints is not supported for litecg currently" << std::endl;
        return entrys;
    }
    LLVMModule *llvmModule = static_cast<LLVMModule *>(irModule_);
    LLVMAssembler *assembler = static_cast<LLVMAssembler *>(assembler_);
    auto engine = assembler->GetEngine();

    for (size_t j = 0; j < llvmModule->GetFuncCount(); j++) {
        LLVMValueRef func = llvmModule->GetFunction(j);
        ASSERT(func != nullptr);
        uintptr_t entry = reinterpret_cast<uintptr_t>(LLVMGetPointerToGlobal(engine, func));
        entrys.push_back(entry);
    }
    return entrys;
}

void Module::CollectFuncEntryInfo(const std::vector<uintptr_t>& entrys, std::map<uintptr_t, std::string> &addr2name,
                                  StubFileInfo &stubInfo, uint32_t moduleIndex, const CompilerLog &log)
{
    LLVMModule *llvmModule = static_cast<LLVMModule*>(irModule_);
    LLVMAssembler *assembler = static_cast<LLVMAssembler*>(assembler_);
    auto codeBuff = assembler->GetSectionAddr(ElfSecName::TEXT);
    auto callSigns = llvmModule->GetCSigns();
    const size_t funcCount = entrys.size();
    funcCount_ = funcCount;
    startIndex_ = stubInfo.GetEntrySize();

    for (size_t j = 0; j < funcCount; j++) {
        auto cs = callSigns[j];
        LLVMValueRef func = llvmModule->GetFunction(j);
        ASSERT(func != nullptr);
        int delta = assembler->GetFpDeltaPrevFramSp(func, log);
        ASSERT(delta >= 0 && (delta % sizeof(uintptr_t) == 0));
        uint32_t funcSize = 0;
        if (j < funcCount - 1) {
            funcSize = entrys[j + 1] - entrys[j];
        } else {
            funcSize = codeBuff + assembler->GetSectionSize(ElfSecName::TEXT) - entrys[j];
        }
        kungfu::CalleeRegAndOffsetVec info = assembler->GetCalleeReg2Offset(func, log);
        stubInfo.AddEntry(cs->GetTargetKind(), false, false, cs->GetID(), entrys[j] - codeBuff,
                          AOTFileManager::STUB_FILE_INDEX, moduleIndex, delta, funcSize, info);
        ASSERT(!cs->GetName().empty());
        addr2name[entrys[j]] = cs->GetName();
    }
}

void Module::CollectFuncEntryInfo(std::map<uintptr_t, std::string> &addr2name, AnFileInfo &aotInfo, uint32_t fileIndex,
                                  uint32_t moduleIndex, const CompilerLog &log)
{
#ifdef COMPILE_MAPLE
    if (irModule_->GetModuleKind() != MODULE_LLVM) {
        CollectFuncEntryInfoByLiteCG(addr2name, aotInfo, fileIndex, moduleIndex);
        return;
    }
#endif
    LLVMAssembler *assembler = static_cast<LLVMAssembler*>(assembler_);
    auto engine = assembler->GetEngine();
    std::vector<std::tuple<uint64_t, size_t, int, bool>> funcInfo; // entry idx delta
    std::vector<kungfu::CalleeRegAndOffsetVec> calleeSaveRegisters; // entry idx delta
    // 1.Compile all functions and collect function infos
    LLVMModule *llvmModule = static_cast<LLVMModule*>(irModule_);
    llvmModule->IteratefuncIndexMap([&](size_t idx, LLVMValueRef func, bool isFastCall) {
        uint64_t funcEntry = reinterpret_cast<uintptr_t>(LLVMGetPointerToGlobal(engine, func));
        uint64_t length = 0;
        std::string funcName(LLVMGetValueName2(func, reinterpret_cast<size_t *>(&length)));
        ASSERT(length != 0);
        addr2name[funcEntry] = funcName;
        int delta = assembler->GetFpDeltaPrevFramSp(func, log);
        ASSERT(delta >= 0 && (delta % sizeof(uintptr_t) == 0));
        funcInfo.emplace_back(std::tuple(funcEntry, idx, delta, isFastCall));
        kungfu::CalleeRegAndOffsetVec info = assembler->GetCalleeReg2Offset(func, log);
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
                         offset, fileIndex, moduleIndex, delta, funcSize, calleeSaveRegisters[i]);
    }
    aotInfo.UpdateCurTextSecOffset(textSize);
    if (rodataSizeAfterText != 0) {
        aotInfo.AlignTextSec(AOTFileInfo::RODATA_SEC_ALIGN);
        aotInfo.UpdateCurTextSecOffset(rodataSizeAfterText);
    }
}

#ifdef COMPILE_MAPLE
void Module::CollectFuncEntryInfoByLiteCG(std::map<uintptr_t, std::string> &addr2name, AnFileInfo &aotInfo,
                                          uint32_t fileIndex, uint32_t moduleIndex)
{
    std::vector<std::tuple<uint64_t, size_t, int, bool>> funcInfo; // entry idx delta
    std::vector<kungfu::CalleeRegAndOffsetVec> calleeSaveRegisters; // entry idx delta
    // 1.Compile all functions and collect function infos
    LMIRModule *lmirModule = static_cast<LMIRModule*>(irModule_);
    LiteCGAssembler *assembler = static_cast<LiteCGAssembler*>(assembler_);
    const auto &func2Addr = assembler->GetCodeInfo().GetFuncInfos();
    lmirModule->IteratefuncIndexMap([&](size_t idx, std::string funcName, bool isFastCall) {
        auto itr = func2Addr.find(funcName);
        if (itr == func2Addr.end()) {
            LOG_COMPILER(FATAL) << "get function address from emitter failed";
            UNREACHABLE();
        }
        uint64_t funcEntry = itr->second.addr;
        addr2name[funcEntry] = funcName;
        int delta = itr->second.fp2PrevFrameSpDelta;
        ASSERT(delta >= 0 && (delta % sizeof(uintptr_t) == 0));
        funcInfo.emplace_back(std::tuple(funcEntry, idx, delta, isFastCall));
        kungfu::CalleeRegAndOffsetVec info = itr->second.calleeRegInfo;
        calleeSaveRegisters.emplace_back(info);
    });
    // 2.After all functions compiled, the module sections would be fixed
    uint32_t textSize = GetTextSize();
    uint32_t rodataSizeBeforeText = 0;
    uint32_t rodataSizeAfterText = 0;

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
        uint64_t funcEntry = 0;
        size_t idx;
        int delta;
        bool isFastCall;
        uint32_t funcSize;
        std::tie(funcEntry, idx, delta, isFastCall) = funcInfo[i];
        if (i < funcCount - 1) {
            funcSize = std::get<0>(funcInfo[i + 1]) - funcEntry;
        } else {
            funcSize = textSize - funcEntry;
        }
        auto found = addr2name[funcEntry].find(panda::ecmascript::JSPandaFile::ENTRY_FUNCTION_NAME);
        bool isMainFunc = found != std::string::npos;
        uint64_t offset = funcEntry;
        aotInfo.AddEntry(CallSignature::TargetKind::JSFUNCTION, isMainFunc, isFastCall, idx,
                         offset, fileIndex, moduleIndex, delta, funcSize, calleeSaveRegisters[i]);
    }
    aotInfo.UpdateCurTextSecOffset(textSize);
    if (rodataSizeAfterText != 0) {
        aotInfo.AlignTextSec(AOTFileInfo::RODATA_SEC_ALIGN);
        aotInfo.UpdateCurTextSecOffset(rodataSizeAfterText);
    }
}
#endif

void Module::CollectModuleSectionDes(ModuleSectionDes &moduleDes) const
{
    if (irModule_->GetModuleKind() != MODULE_LLVM) {
        std::cout << "CollectModuleSectionDes is not supported for litecg currently" << std::endl;
        return;
    }
    ASSERT(assembler_ != nullptr);
    LLVMAssembler *assembler = static_cast<LLVMAssembler*>(assembler_);
    assembler->IterateSecInfos([&](size_t i, std::pair<uint8_t *, size_t> secInfo) {
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
                                       CGStackMapInfo &stackMapInfo) const
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
    CollectAnStackMapDes(moduleDes, textOffset, stackMapInfo);
}

uint32_t Module::GetSectionSize(ElfSecName sec) const
{
    return assembler_->GetSectionSize(sec);
}

uintptr_t Module::GetSectionAddr(ElfSecName sec) const
{
    return assembler_->GetSectionAddr(sec);
}

void Module::RunAssembler(const CompilerLog &log, bool fastCompileMode, bool isJit, const std::string &filename)
{
    if (!IsLLVM()) {
        assembler_->SetAotCodeCommentFile(filename);
    } else {
        assembler_->SetAotCodeCommentFile("");
    }
    assembler_->Run(log, fastCompileMode, isJit);
}

void Module::DisassemblerFunc(std::map<uintptr_t, std::string> &addr2name, uint64_t textOffset,
                              const CompilerLog &log, const MethodLogList &logList, std::ostringstream &codeStream)
{
    if (irModule_->GetModuleKind() != MODULE_LLVM) {
        std::cout << "DisassemblerFunc is not supported for litecg currently" << std::endl;
        return;
    }
    auto *assembler = static_cast<LLVMAssembler*>(assembler_);
    assembler->Disassemble(addr2name, textOffset, log, logList, codeStream);
}

void Module::DestroyModule()
{
    if (irModule_ != nullptr) {
        delete irModule_;
        irModule_ = nullptr;
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
        stubInfo_.AddEntry(cs->GetTargetKind(), false, false, cs->GetID(), entryOffset,
                           AOTFileManager::STUB_FILE_INDEX, bridgeModuleIdx, 0, funSize);
        ASSERT(!cs->GetName().empty());
        addr2name[entryOffset] = cs->GetName();
    }
}

void StubFileGenerator::CollectCodeInfo()
{
    std::map<uintptr_t, std::string> stubAddr2Name;
    std::vector<std::vector<uintptr_t>> entryPoints(modulePackage_.size());

    if (!concurrentCompile_) {
        for (size_t i = 0; i < modulePackage_.size(); ++i) {
            entryPoints[i] = modulePackage_[i].GetFuncEntryPoints();
        }
    } else if (!modulePackage_.empty()) {
        // For first module, run it in current thread.
        // For others, run them in child threads and wait for finish.
        std::vector<std::thread> threads;
        for (size_t i = 1; i < modulePackage_.size(); ++i) {
            threads.emplace_back([&, i]() {
                entryPoints[i] = modulePackage_[i].GetFuncEntryPoints();
            });
        }
        entryPoints[0] = modulePackage_[0].GetFuncEntryPoints();
        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

    for (size_t i = 0; i < modulePackage_.size(); ++i) {
        modulePackage_[i].CollectFuncEntryInfo(entryPoints[i], stubAddr2Name, stubInfo_, i, GetLog());
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
    if (module->IsLLVM()) {
        // In llvm the ro section is separated from the text section, but these all in text section in LiteCG.
        std::tie(rodataAddrBeforeText, rodataSizeBeforeText, rodataAddrAfterText, rodataSizeAfterText) =
            module->GetMergedRODataAddrAndSize(textAddr);
    }
    uint64_t textStart = 0;
    if (rodataSizeAfterText == 0) {
        textStart = aotInfo_.GetCurTextSecOffset() - textSize;
    } else {
        textStart = aotInfo_.GetCurTextSecOffset() - textSize - rodataSizeAfterText;
        textStart = AlignDown(textStart, AOTFileInfo::RODATA_SEC_ALIGN);
    }
    return textStart;
}

void AOTFileGenerator::CollectCodeInfo(Module *module, uint32_t moduleIdx)
{
    std::map<uintptr_t, std::string> addr2name;
    uint32_t lastEntryIdx = aotInfo_.GetEntrySize();
    pgo::ApEntityId abcId = INVALID_INDEX;
    pgo::PGOProfilerManager::GetInstance()->GetPandaFileId(curCompileFileName_.c_str(), abcId);
    module->CollectFuncEntryInfo(addr2name, aotInfo_, abcId, moduleIdx, GetLog());
    aotInfo_.MappingEntryFuncsToAbcFiles(curCompileFileName_, lastEntryIdx, aotInfo_.GetEntrySize());
    ModuleSectionDes des;
    uint64_t textOffset = RollbackTextSize(module);
    if (stackMapInfo_ == nullptr) {
        LOG_ECMA(FATAL) << "stackMapInfo_ isn't be initialized";
        UNREACHABLE();
    }
    module->CollectAnModuleSectionDes(des, textOffset, *stackMapInfo_);

    aotInfo_.AddModuleDes(des);
    if (module->IsLLVM() && log_->OutputASM()) {
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

Module* AOTFileGenerator::AddModule(const std::string &name, const std::string &triple,
                                    [[maybe_unused]] LOptions option, bool logDebug, [[maybe_unused]] bool isJit)
{
#ifdef COMPILE_MAPLE
    if (useLiteCG_) {
        LMIRModule *irModule = new LMIRModule(compilationEnv_->GetNativeAreaAllocator(), name, logDebug, triple, isJit);
        LiteCGAssembler *ass = new LiteCGAssembler(*irModule, jitCodeSpace_,
            compilationEnv_->GetJSOptions().GetCompilerCodegenOptions());
        modulePackage_.emplace_back(Module(irModule, ass));
        if (stackMapInfo_ == nullptr) {
            stackMapInfo_ = new LiteCGStackMapInfo();
        }
        return &modulePackage_.back();
    }
#endif
    LLVMModule *m = new LLVMModule(compilationEnv_->GetNativeAreaAllocator(), name, logDebug, triple);
    LLVMAssembler *ass = new LLVMAssembler(m, jitCodeSpace_, option);
    modulePackage_.emplace_back(Module(m, ass));
    if (stackMapInfo_ == nullptr) {
        stackMapInfo_ = new LLVMStackMapInfo();
    }
    return &modulePackage_.back();
}

Module* StubFileGenerator::AddModule(NativeAreaAllocator *allocator, const std::string &name, const std::string &triple,
                                     LOptions option, bool logDebug, StubFileKind kind)
{
    LLVMModule* m = new LLVMModule(allocator, name, logDebug, triple);
    switch (kind) {
        case StubFileKind::BC:
            m->SetUpForBytecodeHandlerStubs();
            break;
        case StubFileKind::COM:
            m->SetUpForCommonStubs();
            break;
        case StubFileKind::BUILTIN:
            m->SetUpForBuiltinsStubs();
            break;
        case StubFileKind::BASELINE:
            m->SetUpForBaselineStubs();
            break;
        default:
            LOG_ECMA(FATAL) << "unsupported stub file kind";
            UNREACHABLE();
            break;
    }
    LLVMAssembler* ass = new LLVMAssembler(m, jitCodeSpace_, option);
    modulePackage_.emplace_back(Module(m, ass));
    return &modulePackage_.back();
}

void StubFileGenerator::RunLLVMAssembler()
{
    if (!concurrentCompile_) {
        for (auto &m: modulePackage_) {
            m.RunAssembler(*(this->log_), false);
        }
    } else if (!modulePackage_.empty()) {
        // For first module, run it in current thread.
        // For others, run them in child threads and wait for finish.
        std::vector<std::thread> threads;
        for (size_t i = 1; i < modulePackage_.size(); ++i) {
            const CompilerLog &log = *(this->log_);
            threads.emplace_back([&, i] {
                modulePackage_[i].RunAssembler(log, false);
            });
        }
        modulePackage_[0].RunAssembler(*(this->log_), false);
        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }
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
    stubInfo_.AddAsmStubELFInfo(asmModule_.GetCSigns(), asmModule_.GetStubsOffset());
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

void AOTFileGenerator::CompileLatestModuleThenDestroy(bool isJit)
{
    Module *latestModule = GetLatestModule();
#ifdef COMPILE_MAPLE
    static uint32_t lastModulePC = 0;
    if (useLiteCG_ && compilationEnv_->IsJitCompiler()) {
        lastModulePC = 0;
    }
    if (latestModule->GetModule()->GetModuleKind() != MODULE_LLVM) {
        LMIRModule *lmirModule = static_cast<LMIRModule*>(latestModule->GetModule());
        lastModulePC = AlignUp(lastModulePC, AOTFileInfo::PAGE_ALIGN);
        lmirModule->GetModule()->SetLastModulePC(lastModulePC);
        // pass triple to litecg
        lmirModule->GetModule()->SetIsAArch64(isAArch64());
    }
#endif
    ASSERT(GetModuleVecSize() > 0);
    uint32_t latestModuleIdx = GetModuleVecSize() - 1;
    {
        TimeScope timescope("LLVMIROpt", const_cast<CompilerLog *>(log_));
        bool fastCompileMode = compilationEnv_->GetJSOptions().GetFastAOTCompileMode();
        latestModule->RunAssembler(*(log_), fastCompileMode, isJit, GetAotCodeCommentFile());
    }
    {
        TimeScope timescope("LLVMCodeGen", const_cast<CompilerLog *>(log_));
        CollectCodeInfo(latestModule, latestModuleIdx);
    }
    // message has been put into aotInfo, so latestModule could be destroyed
#ifdef COMPILE_MAPLE
    if (latestModule->GetModule()->GetModuleKind() != MODULE_LLVM) {
        LMIRModule *lmirModule = static_cast<LMIRModule*>(latestModule->GetModule());
        lastModulePC = lmirModule->GetModule()->GetCurModulePC();
    }
#endif
    latestModule->DestroyModule();
}

void AOTFileGenerator::DestroyCollectedStackMapInfo()
{
    if (stackMapInfo_ != nullptr) {
        delete stackMapInfo_;
        stackMapInfo_ = nullptr;
    }
}

void AOTFileGenerator::GenerateMergedStackmapSection()
{
    ArkStackMapBuilder builder;
    std::shared_ptr<uint8_t> ptr = nullptr;
    uint32_t size = 0;
    if (stackMapInfo_ == nullptr) {
        LOG_ECMA(FATAL) << "stackMapInfo_ isn't be initialized";
        UNREACHABLE();
    }
    std::tie(ptr, size) = builder.GenerateArkStackMap(*stackMapInfo_, cfg_.GetTriple());
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
        LOG_COMPILER(ERROR) << "Fail to make dir: " << path;
        return false;
    }
    return panda::ecmascript::SetDirModeAsDefault(path);
}

bool AOTFileGenerator::SaveAOTFile(const std::string &filename, const std::string &appSignature)
{
    if (aotInfo_.GetTotalCodeSize() == 0) {
        LOG_COMPILER(WARN) << "error: code size of generated an file is empty!";
        return false;
    }
    if (!CreateDirIfNotExist(filename)) {
        LOG_COMPILER(ERROR) << "Fail to access dir: " << filename;
        return false;
    }
    PrintMergedCodeComment();
    GenerateMergedStackmapSection();
    aotInfo_.GenerateMethodToEntryIndexMap();
    if (!aotInfo_.Save(filename, cfg_.GetTriple())) {
        LOG_COMPILER(ERROR) << "Fail to save an file: " << filename;
        return false;
    }
    if (!panda::ecmascript::SetFileModeAsDefault(filename)) {
        LOG_COMPILER(ERROR) << "Fail to set an file mode:" << filename;
        return false;
    }
    SetSecurityLabel(filename);
    panda::ecmascript::CodeSignatureForAOTFile(filename, appSignature);
    return true;
}

void AOTFileGenerator::SaveEmptyAOTFile(const std::string& filename, const std::string& appSignature, bool isAnFile)
{
    if (!CreateDirIfNotExist(filename)) {
        LOG_COMPILER(ERROR) << "Fail to access dir: " << filename;
        return;
    }
    std::string realPath;
    if (!RealPath(filename, realPath, false)) {
        LOG_COMPILER(ERROR) << "Fail to get realPath: " << filename;
        return;
    }
    if (FileExist(realPath.c_str())) {
        LOG_COMPILER(ERROR) << "AOT file: " << realPath << " exist, skip create empty file";
        return;
    }
    const char* rawPath = realPath.c_str();
    std::ofstream file(rawPath, std::ofstream::binary);
    file.close();
    if (!panda::ecmascript::SetFileModeAsDefault(filename)) {
        LOG_COMPILER(ERROR) << "Fail to set file mode: " << filename;
    }
    if (isAnFile) {
        panda::ecmascript::CodeSignatureForAOTFile(filename, appSignature);
    }
    LOG_COMPILER(ERROR) << "create empty AOT file: " << realPath << " due to illegal AP file";
}

bool AOTFileGenerator::GetMemoryCodeInfos(MachineCodeDesc &machineCodeDesc)
{
    if (aotInfo_.GetTotalCodeSize() == 0) {
        LOG_COMPILER(WARN) << "error: code size of generated an file is empty!";
        return false;
    }

    if (log_->OutputASM()) {
        PrintMergedCodeComment();
    }
    GenerateMergedStackmapSection();

    // get func entry Map
    aotInfo_.GenerateMethodToEntryIndexMap();

    uint64_t funcEntryAddr = reinterpret_cast<uint64_t>(aotInfo_.GetStubs().data());
    ASSERT(aotInfo_.GetStubs().size() <= 2); // jsfunc + __llvm_deoptimize, 2 : size
    uint32_t funcEntrySize = sizeof(AOTFileInfo::FuncEntryDes) * aotInfo_.GetStubs().size();

    ASSERT(aotInfo_.GetModuleSectionDes().size() == 1);
    auto &moduleSectionDes = aotInfo_.GetModuleSectionDes()[0];
    // get code data
    uint64_t textAddr = moduleSectionDes.GetSecAddr(ElfSecName::TEXT);
    size_t textSize = moduleSectionDes.GetSecSize(ElfSecName::TEXT);

    uint64_t rodataAddrBeforeText = 0;
    uint32_t rodataSizeBeforeText = 0;
    uint64_t rodataAddrAfterText = 0;
    uint32_t rodataSizeAfterText = 0;
    std::tie(rodataAddrBeforeText, rodataSizeBeforeText, rodataAddrAfterText, rodataSizeAfterText) =
        moduleSectionDes.GetMergedRODataAddrAndSize(textAddr);

    machineCodeDesc.rodataAddrBeforeText = rodataAddrBeforeText;
    machineCodeDesc.rodataSizeBeforeText = rodataSizeBeforeText;
    machineCodeDesc.rodataAddrAfterText = rodataAddrAfterText;
    machineCodeDesc.rodataSizeAfterText = rodataSizeAfterText;

#ifdef JIT_ENABLE_CODE_SIGN
    machineCodeDesc.codeSigner = 0;
    JitSignCode *singleton = JitSignCode::GetInstance();
    if (singleton->GetCodeSigner() != 0) {
        LOG_JIT(DEBUG) << "In GetMemoryCodeInfos, signer = " << singleton->GetCodeSigner();
        LOG_JIT(DEBUG) << "     signTableSize = " << singleton->signTableSize_;
        machineCodeDesc.codeSigner = reinterpret_cast<uintptr_t>(singleton->GetCodeSigner());
    }
#endif

    uint64_t stackMapPtr = reinterpret_cast<uint64_t>(moduleSectionDes.GetArkStackMapSharePtr().get());
    size_t stackMapSize = moduleSectionDes.GetArkStackMapSize();

    machineCodeDesc.codeAddr = textAddr;
    machineCodeDesc.codeSize = textSize;
    machineCodeDesc.funcEntryDesAddr = funcEntryAddr;
    machineCodeDesc.funcEntryDesSize = funcEntrySize;
    machineCodeDesc.stackMapOrOffsetTableAddr = stackMapPtr;
    machineCodeDesc.stackMapOrOffsetTableSize = stackMapSize;
    machineCodeDesc.codeType = MachineCodeType::FAST_JIT_CODE;

    if (Jit::GetInstance()->IsEnableJitFort() && Jit::GetInstance()->IsEnableAsyncCopyToFort() &&
        JitCompiler::AllocFromFortAndCopy(*compilationEnv_, machineCodeDesc) == false) {
        return false;
    }
    return true;
}

void AOTFileGenerator::JitCreateLitecgModule()
{
#ifdef COMPILE_MAPLE
    Module *latestModule = GetLatestModule();
    if (latestModule->GetModule()->GetModuleKind() != MODULE_LLVM) {
        LMIRModule *lmirModule = static_cast<LMIRModule*>(latestModule->GetModule());
        lmirModule->JitCreateLitecgModule();
    }
#endif
}

bool AOTFileGenerator::isAArch64() const
{
    return cfg_.IsAArch64();
}

bool AOTFileGenerator::SaveSnapshotFile()
{
    TimeScope timescope("LLVMCodeGenPass-AI", const_cast<CompilerLog *>(log_));
    Snapshot snapshot(compilationEnv_->GetEcmaVM());
    const CString snapshotPath(compilationEnv_->GetJSOptions().GetAOTOutputFile().c_str());
    const auto &methodToEntryIndexMap = aotInfo_.GetMethodToEntryIndexMap();
    PGOTypeManager *ptManager = compilationEnv_->GetPTManager();
    ptManager->GetAOTSnapshot().ResolveSnapshotData(methodToEntryIndexMap);

    CString aiPath = snapshotPath + AOTFileManager::FILE_EXTENSION_AI;
    if (!CreateDirIfNotExist(aiPath.c_str())) {
        LOG_COMPILER(ERROR) << "Fail to access dir: " << aiPath;
        return false;
    }
    snapshot.Serialize(aiPath);
    if (!panda::ecmascript::SetFileModeAsDefault(aiPath.c_str())) {
        LOG_COMPILER(ERROR) << "Fail to set ai file mode:" << aiPath;
        return false;
    }
    SetSecurityLabel(aiPath.c_str());
    return true;
}

bool AOTFileGenerator::CreateAOTCodeCommentFile(const std::string &filename)
{
    if (!CreateDirIfNotExist(filename)) {
        LOG_COMPILER(ERROR) << "Fail to access dir: " << filename;
        return false;
    }

    std::string realPath;
    if (!panda::ecmascript::RealPath(filename, realPath, false)) {
        LOG_COMPILER(ERROR) << "Fail to get realPath: " << filename;
        return false;
    }

    auto index = realPath.find_last_of('/');
    if (index == std::string::npos) {
        LOG_COMPILER(ERROR) << "Path: " << realPath << " is illegal";
        return false;
    }

    std::string aotCodeCommentFile = realPath.substr(0, index) + "/aot_code_comment.txt";
    SetAotCodeCommentFile(aotCodeCommentFile);
    if (FileExist(aotCodeCommentFile.c_str())) {
        if (Unlink(aotCodeCommentFile.c_str()) == -1) {
            SetAotCodeCommentFile("");
            LOG_COMPILER(ERROR) << "remove " << aotCodeCommentFile << " failed and errno is " << errno;
            return false;
        }
    }

    std::ofstream file(aotCodeCommentFile.c_str(), std::ofstream::app);
    if (!file.is_open()) {
        SetAotCodeCommentFile("");
        LOG_COMPILER(ERROR) << "Failed to create " << aotCodeCommentFile;
        return false;
    }
    file.close();

    if (!panda::ecmascript::SetFileModeAsDefault(aotCodeCommentFile)) {
        SetAotCodeCommentFile("");
        Unlink(aotCodeCommentFile.c_str());
        LOG_COMPILER(ERROR) << "Fail to set file mode: " << aotCodeCommentFile;
        return false;
    }
    return true;
}
}  // namespace panda::ecmascript::kungfu
