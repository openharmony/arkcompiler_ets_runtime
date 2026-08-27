/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/codegen/llvm/llvm_codegen.h"
#include <algorithm>
#include <vector>
#if defined(PANDA_TARGET_MACOS) || defined(PANDA_TARGET_IOS)
#include "ecmascript/base/llvm_helper.h"
#endif


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

#include "llvm-c/Analysis.h"
#include "llvm-c/Disassembler.h"
#include "llvm-c/Transforms/PassManagerBuilder.h"
#include "llvm/DebugInfo/DWARF/DWARFContext.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/LegacyPassManager.h"
#include "lib/llvm_interface.h"

#include "ecmascript/compiler/aot_file/aot_file_info.h"
#include "ecmascript/compiler/codegen/llvm/llvm_ir_builder.h"
#include "ecmascript/compiler/compiler_log.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;
using namespace llvm;

CodeInfo::CodeInfo(CodeSpaceOnDemand &codeSpaceOnDemand, bool useOwnSpace)
    : codeSpaceOnDemand_(codeSpaceOnDemand),
      useOwnSpace_(useOwnSpace)
{
    secInfos_.fill(std::make_pair(nullptr, 0));
    if (useOwnSpace_) {
        ownCodeSpace_ = std::make_unique<CodeSpace>();
    }
}

CodeInfo::~CodeInfo()
{
    Reset();
}

CodeInfo::CodeSpace *CodeInfo::CodeSpace::GetInstance()
{
    static CodeSpace *codeSpace = new CodeSpace();
    return codeSpace;
}

CodeInfo::CodeSpace::CodeSpace()
{
    ASSERT(REQUIRED_SECS_LIMIT == AlignUp(REQUIRED_SECS_LIMIT, PageSize()));
    reqSecs_ = static_cast<uint8_t *>(PageMap(REQUIRED_SECS_LIMIT, PAGE_PROT_READWRITE).GetMem());
    if (reqSecs_ == reinterpret_cast<uint8_t *>(-1)) {
        reqSecs_ = nullptr;
    }
    ASSERT(UNREQUIRED_SECS_LIMIT == AlignUp(UNREQUIRED_SECS_LIMIT, PageSize()));
    unreqSecs_ = static_cast<uint8_t *>(PageMap(UNREQUIRED_SECS_LIMIT, PAGE_PROT_READWRITE).GetMem());
    if (unreqSecs_ == reinterpret_cast<uint8_t *>(-1)) {
        unreqSecs_ = nullptr;
    }
}

CodeInfo::CodeSpace::~CodeSpace()
{
    reqBufPos_ = 0;
    unreqBufPos_ = 0;
    if (reqSecs_ != nullptr) {
        PageUnmap(MemMap(reqSecs_, REQUIRED_SECS_LIMIT));
    }
    reqSecs_ = nullptr;
    if (unreqSecs_ != nullptr) {
        PageUnmap(MemMap(unreqSecs_, UNREQUIRED_SECS_LIMIT));
    }
    unreqSecs_ = nullptr;
}

uint8_t *CodeInfo::CodeSpace::Alloca(uintptr_t size, bool isReq, size_t alignSize)
{
    // Wait other threads arrived here, then allocate in same time.
    ConcurrentMonitor::monitor_.ArriveAndWait();
    uint8_t *addr = nullptr;
    auto bufBegin = isReq ? reqSecs_ : unreqSecs_;
    auto &curPos = isReq ? reqBufPos_ : unreqBufPos_;
    size_t limit = isReq ? REQUIRED_SECS_LIMIT : UNREQUIRED_SECS_LIMIT;
    if (curPos + size > limit) {
        LOG_COMPILER(ERROR) << std::hex << "Alloca Section failed. Current curPos:" << curPos
                            << " plus size:" << size << "exceed limit:" << limit;
        exit(-1);
    }
    if (alignSize > 0) {
        curPos = AlignUp(curPos, alignSize);
    }
    addr = bufBegin + curPos;
    curPos += size;
    return addr;
}

uint8_t *CodeInfo::CodeSpaceOnDemand::Alloca(uintptr_t size, [[maybe_unused]] bool isReq, size_t alignSize)
{
    // Always apply for an aligned memory block here.
    auto alignedSize = alignSize > 0 ? AlignUp(size, alignSize) : size;
    // Verify the size and temporarily use REQUIREd_SECS.LIMITED as the online option, allowing for adjustments.
    if (alignedSize > SECTION_LIMIT) {
        LOG_COMPILER(FATAL) << std::hex << "invalid memory size: " << alignedSize;
        return nullptr;
    }
    uint8_t *addr = static_cast<uint8_t *>(malloc(alignedSize));
    if (addr == nullptr) {
        LOG_COMPILER(FATAL) << "malloc section failed.";
        return nullptr;
    }
    sections_.push_back({addr, alignedSize});
    return addr;
}

CodeInfo::CodeSpaceOnDemand::~CodeSpaceOnDemand()
{
    // release all used memory.
    for (auto &section : sections_) {
        if ((section.first != nullptr) && (section.second != 0)) {
            free(section.first);
        }
    }
    sections_.clear();
}

uint8_t *CodeInfo::AllocaOnDemand(uintptr_t size, size_t alignSize)
{
    return codeSpaceOnDemand_.Alloca(size, true, alignSize);
}

uint8_t *CodeInfo::AllocaInReqSecBuffer(uintptr_t size, size_t alignSize)
{
    if (useOwnSpace_) {
        return ownCodeSpace_->Alloca(size, true, alignSize);
    }
    return CodeSpace::GetInstance()->Alloca(size, true, alignSize);
}

uint8_t *CodeInfo::AllocaInNotReqSecBuffer(uintptr_t size, size_t alignSize)
{
    if (useOwnSpace_) {
        return ownCodeSpace_->Alloca(size, false, alignSize);
    }
    return CodeSpace::GetInstance()->Alloca(size, false, alignSize);
}

uint8_t *CodeInfo::AllocaCodeSectionImp(uintptr_t size, const char *sectionName,
                                        AllocaSectionCallback allocaInReqSecBuffer)
{
    uint8_t *addr = nullptr;
    auto curSec = ElfSection(sectionName);
    if (curSec.isValidAOTSec()) {
        if (!alreadyPageAlign_) {
            addr = (this->*allocaInReqSecBuffer)(size, AOTFileInfo::PAGE_ALIGN);
            alreadyPageAlign_ = true;
            VerifyAddress(reinterpret_cast<uintptr_t>(addr), size, AOTFileInfo::PAGE_ALIGN);
        } else {
            addr = (this->*allocaInReqSecBuffer)(size, AOTFileInfo::TEXT_SEC_ALIGN);
            VerifyAddress(reinterpret_cast<uintptr_t>(addr), size, AOTFileInfo::TEXT_SEC_ALIGN);
        }
    } else {
        addr = (this->*allocaInReqSecBuffer)(size, 0);
        VerifyAddress(reinterpret_cast<uintptr_t>(addr), size, 0);
    }
    codeInfo_.push_back({addr, size});
    if (curSec.isValidAOTSec()) {
        secInfos_[curSec.GetIntIndex()] = std::make_pair(addr, size);
    }
    return addr;
}

uint8_t *CodeInfo::AllocaCodeSection(uintptr_t size, const char *sectionName)
{
    return AllocaCodeSectionImp(size, sectionName, &CodeInfo::AllocaInReqSecBuffer);
}

uint8_t *CodeInfo::AllocaCodeSectionOnDemand(uintptr_t size, const char *sectionName)
{
    return AllocaCodeSectionImp(size, sectionName, &CodeInfo::AllocaOnDemand);
}

void CodeInfo::VerifyAddress(uintptr_t addr, uintptr_t size, uintptr_t alignSize)
{
    if (!useOwnSpace_) {
        return;
    }
    if (lastAddr_ == 0) {
        lastAddr_ = addr;
        lastSize_ = size;
        return;
    }
    uintptr_t expectAddr = lastAddr_ + lastSize_;
    if (alignSize != 0 && !IsAligned(expectAddr, alignSize)) {
        expectAddr = AlignUp(expectAddr, alignSize);
    }
    if (expectAddr != addr) {
        LOG_COMPILER(FATAL) << "VerifyAddress failed: " << lastAddr_ << " " << lastSize_ << " " << alignSize <<
                ", addr: " << addr;
    }
    lastAddr_ = addr;
    lastSize_ = size;
}

uint8_t *CodeInfo::AllocaDataSectionImp(uintptr_t size, const char *sectionName,
                                        AllocaSectionCallback allocaInReqSecBuffer,
                                        AllocaSectionCallback allocaInNotReqSecBuffer)
{
    uint8_t *addr = nullptr;
    auto curSec = ElfSection(sectionName);
    // rodata section needs 16 bytes alignment
    if (curSec.InRodataSection()) {
        size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_REGION));
        if (!alreadyPageAlign_) {
            addr = curSec.isSequentialAOTSec() ? (this->*allocaInReqSecBuffer)(size, AOTFileInfo::PAGE_ALIGN)
                                               : (this->*allocaInNotReqSecBuffer)(size, AOTFileInfo::PAGE_ALIGN);
            alreadyPageAlign_ = true;
        } else {
            uint32_t alignedSize = curSec.InRodataSection() ? AOTFileInfo::RODATA_SEC_ALIGN
                                                            : AOTFileInfo::DATA_SEC_ALIGN;
            addr = curSec.isSequentialAOTSec() ? (this->*allocaInReqSecBuffer)(size, alignedSize)
                                               : (this->*allocaInNotReqSecBuffer)(size, alignedSize);
        }
    } else {
        addr = curSec.isSequentialAOTSec() ? (this->*allocaInReqSecBuffer)(size, 0)
                                           : (this->*allocaInNotReqSecBuffer)(size, 0);
    }
    if (curSec.isValidAOTSec()) {
        secInfos_[curSec.GetIntIndex()] = std::make_pair(addr, size);
    }
    return addr;

}

uint8_t *CodeInfo::AllocaDataSection(uintptr_t size, const char *sectionName)
{
    return AllocaDataSectionImp(size, sectionName, &CodeInfo::AllocaInReqSecBuffer, &CodeInfo::AllocaInNotReqSecBuffer);
}

uint8_t *CodeInfo::AllocaDataSectionOnDemand(uintptr_t size, const char *sectionName)
{
    return AllocaDataSectionImp(size, sectionName, &CodeInfo::AllocaOnDemand, &CodeInfo::AllocaOnDemand);
}

void CodeInfo::SaveFunc2Addr(std::string funcName, uint32_t address)
{
    auto itr = func2FuncInfo.find(funcName);
    if (itr != func2FuncInfo.end()) {
        itr->second.addr = address;
        return;
    }
    func2FuncInfo.insert(
        std::pair<std::string, FuncInfo>(funcName, {address, 0, kungfu::CalleeRegAndOffsetVec()}));
}

void CodeInfo::SaveFunc2FPtoPrevSPDelta(std::string funcName, int32_t fp2PrevSpDelta)
{
    auto itr = func2FuncInfo.find(funcName);
    if (itr != func2FuncInfo.end()) {
        itr->second.fp2PrevFrameSpDelta = fp2PrevSpDelta;
        return;
    }
    func2FuncInfo.insert(
        std::pair<std::string, FuncInfo>(funcName, {0, fp2PrevSpDelta, kungfu::CalleeRegAndOffsetVec()}));
}

void CodeInfo::SaveFunc2CalleeOffsetInfo(std::string funcName, kungfu::CalleeRegAndOffsetVec calleeRegInfo)
{
    auto itr = func2FuncInfo.find(funcName);
    if (itr != func2FuncInfo.end()) {
        itr->second.calleeRegInfo = calleeRegInfo;
        return;
    }
    func2FuncInfo.insert(
        std::pair<std::string, FuncInfo>(funcName, {0, 0, calleeRegInfo}));
}

void CodeInfo::SavePC2DeoptInfo(uint64_t pc, std::vector<uint8_t> deoptInfo)
{
    pc2DeoptInfo.insert(std::pair<uint64_t, std::vector<uint8_t>>(pc, deoptInfo));
}

void CodeInfo::SavePC2CallSiteInfo(uint64_t pc, std::vector<uint8_t> callSiteInfo)
{
    pc2CallsiteInfo.insert(std::pair<uint64_t, std::vector<uint8_t>>(pc, callSiteInfo));
}

const std::map<std::string, CodeInfo::FuncInfo> &CodeInfo::GetFuncInfos() const
{
    return func2FuncInfo;
}

const std::map<uint64_t, std::vector<uint8_t>> &CodeInfo::GetPC2DeoptInfo() const
{
    return pc2DeoptInfo;
}

const std::unordered_map<uint64_t, std::vector<uint8_t>> &CodeInfo::GetPC2CallsiteInfo() const
{
    return pc2CallsiteInfo;
}

void CodeInfo::Reset()
{
    codeInfo_.clear();
}

uint8_t *CodeInfo::GetSectionAddr(ElfSecName sec) const
{
    auto curSection = ElfSection(sec);
    auto idx = curSection.GetIntIndex();
    return const_cast<uint8_t *>(secInfos_[idx].first);
}

size_t CodeInfo::GetSectionSize(ElfSecName sec) const
{
    auto curSection = ElfSection(sec);
    auto idx = curSection.GetIntIndex();
    return secInfos_[idx].second;
}

const std::vector<std::pair<uint8_t *, uintptr_t>> &CodeInfo::GetCodeInfo() const
{
    return codeInfo_;
}

void LLVMIRGeneratorImpl::GenerateCodeForStub(Circuit *circuit, const ControlFlowGraph &graph, size_t index,
                                              const CompilationConfig *cfg)
{
    LLVMValueRef function = module_->GetFunction(index);
    const CallSignature* cs = module_->GetCSign(index);
    LLVMIRBuilder builder(&graph, circuit, module_, function, cfg, cs->GetCallConv(), enableLog_, false, cs->GetName(),
                          true, false, true, cs->IsStwCopyStub());
    builder.Build();
}

void LLVMIRGeneratorImpl::GenerateCode(Circuit *circuit, const ControlFlowGraph &graph, const CompilationConfig *cfg,
                                       const panda::ecmascript::MethodLiteral *methodLiteral,
                                       const JSPandaFile *jsPandaFile, const std::string &methodName,
                                       const FrameType frameType, bool enableOptInlining, bool enableOptBranchProfiling)
{
    auto function = module_->AddFunc(methodLiteral, jsPandaFile);
    circuit->SetFrameType(frameType);
    CallSignature::CallConv conv;
    if (methodLiteral->IsFastCall()) {
        conv = CallSignature::CallConv::CCallConv;
    } else {
        conv = CallSignature::CallConv::WebKitJSCallConv;
    }
    LLVMIRBuilder builder(&graph, circuit, module_, function, cfg, conv,
                          enableLog_, methodLiteral->IsFastCall(), methodName,
                          false, enableOptInlining, enableOptBranchProfiling);
    builder.Build();
}

static uint8_t *RoundTripAllocateCodeSection(void *object, uintptr_t size, [[maybe_unused]] unsigned alignment,
                                             [[maybe_unused]] unsigned sectionID, const char *sectionName)
{
    struct CodeInfo& state = *static_cast<struct CodeInfo*>(object);
    return state.AllocaCodeSection(size, sectionName);
}

static uint8_t *RoundTripAllocateCodeSectionOnDemand(void *object, uintptr_t size, [[maybe_unused]] unsigned alignment,
                                                     [[maybe_unused]] unsigned sectionID, const char *sectionName)
{
    struct CodeInfo& state = *static_cast<struct CodeInfo*>(object);
    return state.AllocaCodeSectionOnDemand(size, sectionName);
}

static uint8_t *RoundTripAllocateDataSection(void *object, uintptr_t size, [[maybe_unused]] unsigned alignment,
                                             [[maybe_unused]] unsigned sectionID, const char *sectionName,
                                             [[maybe_unused]] LLVMBool isReadOnly)
{
    struct CodeInfo& state = *static_cast<struct CodeInfo*>(object);
    return state.AllocaDataSection(size, sectionName);
}

static uint8_t *RoundTripAllocateDataSectionOnDemand(void *object, uintptr_t size, [[maybe_unused]] unsigned alignment,
                                                     [[maybe_unused]] unsigned sectionID, const char *sectionName,
                                                     [[maybe_unused]] LLVMBool isReadOnly)
{
    ASSERT(object != nullptr);
    struct CodeInfo& state = *static_cast<struct CodeInfo*>(object);
    return state.AllocaDataSectionOnDemand(size, sectionName);
}

static LLVMBool RoundTripFinalizeMemory([[maybe_unused]] void *object, [[maybe_unused]] char **errMsg)
{
    return 0;
}

static void RoundTripDestroy([[maybe_unused]] void *object)
{
    return;
}

void LLVMAssembler::UseRoundTripSectionMemoryManager(bool isJit)
{
    auto sectionMemoryManager = std::make_unique<llvm::SectionMemoryManager>();
    options_.MCJMM = LLVMCreateSimpleMCJITMemoryManager(
        &codeInfo_, isJit ? RoundTripAllocateCodeSectionOnDemand : RoundTripAllocateCodeSection,
        isJit ? RoundTripAllocateDataSectionOnDemand : RoundTripAllocateDataSection, RoundTripFinalizeMemory,
        RoundTripDestroy);
}

bool LLVMAssembler::BuildMCJITEngine()
{
    LLVMBool ret = LLVMCreateMCJITCompilerForModule(&engine_, module_, &options_, sizeof(options_), &error_);
    if (ret) {
        LOG_COMPILER(FATAL) << "error_ : " << error_;
        return false;
    }
    llvm::unwrap(engine_)->RegisterJITEventListener(&listener_);
    return true;
}

void LLVMAssembler::BuildAndRunPasses()
{
    LLVMPassManagerBuilderRef pmBuilder = LLVMPassManagerBuilderCreate();
    LLVMPassManagerBuilderSetOptLevel(pmBuilder, options_.OptLevel); // using O3 optimization level
    LLVMPassManagerBuilderSetSizeLevel(pmBuilder, 0);
    LLVMPassManagerBuilderSetDisableUnrollLoops(pmBuilder, 0);

    // pass manager creation:rs4gc pass is the only pass in modPass, other opt module-based pass are in modPass1
    LLVMPassManagerRef funcPass = LLVMCreateFunctionPassManagerForModule(module_);
    LLVMPassManagerRef modPass = LLVMCreatePassManager();
    LLVMPassManagerRef modPass1 = LLVMCreatePassManager();

    // add pass into pass managers
    LLVMPassManagerBuilderPopulateFunctionPassManager(pmBuilder, funcPass);
    llvm::unwrap(modPass)->add(LLVMCreateRewriteStatepointsForGCLegacyPass()); // rs4gc pass added
    LLVMPassManagerBuilderPopulateModulePassManager(pmBuilder, modPass1);

    LLVMRunPassManager(modPass, module_);
    LLVMInitializeFunctionPassManager(funcPass);
    for (LLVMValueRef fn = LLVMGetFirstFunction(module_); fn; fn = LLVMGetNextFunction(fn)) {
        LLVMRunFunctionPassManager(funcPass, fn);
    }
    LLVMFinalizeFunctionPassManager(funcPass);
    LLVMRunPassManager(modPass1, module_);

    LLVMPassManagerBuilderDispose(pmBuilder);
    LLVMDisposePassManager(funcPass);
    LLVMDisposePassManager(modPass);
    LLVMDisposePassManager(modPass1);
}

void LLVMAssembler::BuildAndRunPassesFastMode()
{
    LLVMPassManagerBuilderRef pmBuilder = LLVMPassManagerBuilderCreate();
    LLVMPassManagerBuilderSetOptLevel(pmBuilder, options_.OptLevel); // using O3 optimization level
    LLVMPassManagerBuilderSetSizeLevel(pmBuilder, 0);

    // pass manager creation:rs4gc pass is the only pass in modPass, other opt module-based pass are in modPass1
    LLVMPassManagerRef funcPass = LLVMCreateFunctionPassManagerForModule(module_);
    LLVMPassManagerRef modPass = LLVMCreatePassManager();

    // add pass into pass managers
    LLVMPassManagerBuilderPopulateFunctionPassManager(pmBuilder, funcPass);
    llvm::unwrap(modPass)->add(LLVMCreateRewriteStatepointsForGCLegacyPass()); // rs4gc pass added

    LLVMInitializeFunctionPassManager(funcPass);
    for (LLVMValueRef fn = LLVMGetFirstFunction(module_); fn; fn = LLVMGetNextFunction(fn)) {
        LLVMRunFunctionPassManager(funcPass, fn);
    }
    LLVMFinalizeFunctionPassManager(funcPass);
    LLVMRunPassManager(modPass, module_);

    LLVMPassManagerBuilderDispose(pmBuilder);
    LLVMDisposePassManager(funcPass);
    LLVMDisposePassManager(modPass);
}

LLVMAssembler::LLVMAssembler(LLVMModule *lm, CodeInfo::CodeSpaceOnDemand &codeSpaceOnDemand, LOptions option,
                             bool isStubCompiler)
    : Assembler(codeSpaceOnDemand, isStubCompiler),
      llvmModule_(lm),
      module_(llvmModule_->GetModule()),
      listener_(this)
{
    Initialize(option);
}

LLVMAssembler::~LLVMAssembler()
{
    if (engine_ != nullptr) {
        if (module_ != nullptr) {
            char *error = nullptr;
            LLVMRemoveModule(engine_, module_, &module_, &error);
            if (error != nullptr) {
                LLVMDisposeMessage(error);
            }
        }
        LLVMDisposeExecutionEngine(engine_);
        engine_ = nullptr;
    }
    module_ = nullptr;
    error_ = nullptr;
}

void LLVMAssembler::Run(const CompilerLog &log, bool fastCompileMode, bool isJit)
{
    char *error = nullptr;
    std::string originName = llvm::unwrap(module_)->getModuleIdentifier() + ".ll";
    std::string optName = llvm::unwrap(module_)->getModuleIdentifier() + "_opt.ll";
    if (log.OutputLLIR()) {
        LLVMPrintModuleToFile(module_, originName.c_str(), &error);
        std::string errInfo = (error != nullptr) ? error : "";
        LOG_COMPILER(INFO) << "generate " << originName << " " << errInfo;
    }
    LLVMVerifyModule(module_, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);
    UseRoundTripSectionMemoryManager(isJit);
    if (!BuildMCJITEngine()) {
        return;
    }
    llvm::unwrap(engine_)->setProcessAllSections(true);
    if (fastCompileMode) {
        BuildAndRunPassesFastMode();
    } else {
        BuildAndRunPasses();
    }
    if (log.OutputLLIR()) {
        error = nullptr;
        LLVMPrintModuleToFile(module_, optName.c_str(), &error);
        std::string errInfo = (error != nullptr) ? error : "";
        LOG_COMPILER(INFO) << "generate " << optName << " " << errInfo;
    }
}

void LLVMAssembler::Initialize(LOptions option)
{
    std::string triple(LLVMGetTarget(module_));
    if (triple.compare(TARGET_X64) == 0) {
#if defined(PANDA_TARGET_MACOS) || !defined(PANDA_TARGET_ARM64)
        LLVMInitializeX86TargetInfo();
        LLVMInitializeX86TargetMC();
        LLVMInitializeX86Disassembler();
        /* this method must be called, ohterwise "Target does not support MC emission" */
        LLVMInitializeX86AsmPrinter();
        LLVMInitializeX86AsmParser();
        LLVMInitializeX86Target();
#endif
    } else if (triple.compare(TARGET_AARCH64) == 0) {
        LLVMInitializeAArch64TargetInfo();
        LLVMInitializeAArch64TargetMC();
        LLVMInitializeAArch64Disassembler();
        LLVMInitializeAArch64AsmPrinter();
        LLVMInitializeAArch64AsmParser();
        LLVMInitializeAArch64Target();
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }

    LLVMLinkAllBuiltinGCs();
    LLVMInitializeMCJITCompilerOptions(&options_, sizeof(options_));
    options_.OptLevel = option.optLevel;
    // NOTE: Just ensure that this field still exists for PIC option
    options_.RelMode = static_cast<LLVMRelocMode>(option.relocMode);
    options_.NoFramePointerElim = static_cast<int32_t>(option.genFp);
    options_.CodeModel = LLVMCodeModelSmall;
}

static const char *SymbolLookupCallback([[maybe_unused]] void *disInfo, [[maybe_unused]] uint64_t referenceValue,
                                        uint64_t *referenceType, [[maybe_unused]] uint64_t referencePC,
                                        [[maybe_unused]] const char **referenceName)
{
    *referenceType = LLVMDisassembler_ReferenceType_InOut_None;
    return nullptr;
}

kungfu::CalleeRegAndOffsetVec LLVMAssembler::GetCalleeReg2Offset(LLVMValueRef fn, const CompilerLog &log)
{
    kungfu::CalleeRegAndOffsetVec info;
    llvm::Function* func = llvm::unwrap<llvm::Function>(fn);
    ASSERT(func != nullptr);
#if defined(PANDA_TARGET_MACOS)
    for (const auto &Attr : func->getAttributes().getFnAttributes()) {
#else
    for (const auto &Attr : func->getAttributes().getFnAttrs()) {
#endif
        if (Attr.isStringAttribute()) {
            std::string str = std::string(Attr.getKindAsString().data());
            std::string expectedKey = "DwarfReg";
            size_t keySZ = expectedKey.size();
            size_t strSZ = str.size();
            if (strSZ >= keySZ && str.substr(0, keySZ) == expectedKey) {
                int RegNum = std::stoi(str.substr(keySZ, strSZ - keySZ));
                auto value = std::stoi(std::string(Attr.getValueAsString()));
                info.push_back(std::make_pair(RegNum, value));
                (void)log;
            }
        }
    }
    return info;
}

int LLVMAssembler::GetFpDeltaPrevFramSp(LLVMValueRef fn, const CompilerLog &log)
{
    int fpToCallerSpDelta = 0;
    const char attrKey[] = "fpToCallerSpDelta"; // this key must consistent with llvm backend.
    LLVMAttributeRef attrirbuteRef = LLVMGetStringAttributeAtIndex(fn, llvm::AttributeList::FunctionIndex,
                                                                   attrKey, strlen(attrKey));
    if (attrirbuteRef) {
        llvm::Attribute attr = llvm::unwrap(attrirbuteRef);
        auto value = attr.getValueAsString().data();
        fpToCallerSpDelta = atoi(value);
        if (log.AllMethod()) {
            size_t length;
            LOG_COMPILER(DEBUG) << " funcName: " << LLVMGetValueName2(fn, &length) << " fpToCallerSpDelta:"
            << fpToCallerSpDelta;
        }
    }
    return fpToCallerSpDelta;
}

static uint32_t GetInstrValue(size_t instrSize, uint8_t *instrAddr)
{
    uint32_t value = 0;
    if (instrSize <= sizeof(uint32_t)) {
        if (memcpy_s(&value, sizeof(uint32_t), instrAddr, instrSize) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    }
    return value;
}

namespace {
// Markers identify the first and subsequent instructions of one gate's generated code.
constexpr const char *MARKER_FIRST = "\xe2\x94\x8c";  // ┌ group start
constexpr const char *MARKER_MID = "\xe2\x94\x82";    // │ group continuation

constexpr size_t ASM_FIELD_WIDTH = 34; // asm text padded to this width so markers align
constexpr size_t HEX_FIELD_WIDTH = 8;  // width of offset / instruction-hex fields
constexpr size_t OUT_STRING_SIZE = 512;
constexpr size_t HEX_MAX_DIGITS = sizeof(uint64_t) * 2; // hex digits for a full uint64
constexpr uint32_t HEX_BITS_PER_DIGIT = 4;              // bits per hex digit (one nibble)
constexpr uint64_t HEX_DIGIT_MASK = 0xf;                // bitmask of the lowest hex digit

const std::string EMPTY_COMMENT;

using FuncNameMap = std::map<uintptr_t, std::string>;

struct CodeDisassemblyState {
    uint8_t *instrAddr {nullptr};
    size_t remainingBytes {0};
    uint64_t instrOffset {0};
    bool logEnabled {false};
    std::string methodName;
    FuncNameMap::const_iterator nextFunction;
    uint32_t previousLine {0};
};

struct AsmOutputLine {
    std::string &codeStream;
    uint64_t offset {0};
    uint32_t hex {0};
    char *text {nullptr};
};

struct CodeCommentOutputContext {
    uint64_t textOffset {0};
    uint64_t textSecIndex {0};
    const DILineInfoSpecifier &debugSpecifier;
    DWARFContext *dwarfCtx {nullptr};
    LLVMModule *llvmModule {nullptr};
};

void AppendHex(std::string &s, uint64_t value)
{
    static constexpr char hexDigits[] = "0123456789abcdef";
    char buffer[HEX_MAX_DIGITS];
    size_t length = 0;
    do {
        buffer[sizeof(buffer) - 1 - length] = hexDigits[value & HEX_DIGIT_MASK];
        value >>= HEX_BITS_PER_DIGIT;
        length++;
    } while (value != 0);
    if (length < HEX_FIELD_WIDTH) {
        s.append(HEX_FIELD_WIDTH - length, '0');
    }
    s.append(buffer + sizeof(buffer) - length, length);
}

void EmitRawAsmLine(const AsmOutputLine &line)
{
    AppendHex(line.codeStream, line.offset);
    line.codeStream += ':';
    AppendHex(line.codeStream, line.hex);
    line.codeStream += ' ';
    line.codeStream += line.text;
    line.codeStream += '\n';
}

// Tabs -> spaces so std::setw aligns by display column (a tab would drift with the viewer's tab width).
// asmText points at LLVM's mutable output buffer and is consumed before the next disassembly call.
void EmitAsmLine(const AsmOutputLine &line, const char *marker, const std::string &comment)
{
    size_t asmLength = strlen(line.text);
    std::replace(line.text, line.text + asmLength, '\t', ' ');
    AppendHex(line.codeStream, line.offset);
    line.codeStream += ':';
    AppendHex(line.codeStream, line.hex);
    line.codeStream += ' ';
    if (marker == nullptr) {
        line.codeStream += line.text;
    } else {
        line.codeStream += line.text;
        if (asmLength < ASM_FIELD_WIDTH) {
            line.codeStream.append(ASM_FIELD_WIDTH - asmLength, ' ');
        }
        line.codeStream += marker;
        if (!comment.empty()) {
            line.codeStream += ' ';
            line.codeStream += comment;
        }
    }
    line.codeStream += '\n';
}

void AdvanceInstruction(CodeDisassemblyState &state, size_t instSize)
{
    state.instrOffset += instSize;
    state.instrAddr += instSize;
    state.remainingBytes -= instSize;
}

void UpdateMethodLogState(CodeDisassemblyState &state, const FuncNameMap &addr2name, const CompilerLog &log,
                          const MethodLogList &logList, std::string &codeStream)
{
    uint64_t addr = reinterpret_cast<uint64_t>(state.instrAddr);
    if (state.nextFunction == addr2name.end() || state.nextFunction->first != addr) {
        return;
    }

    state.methodName = state.nextFunction->second;
    ++state.nextFunction;
    state.previousLine = 0;
    state.logEnabled = log.OutputASM();
    if (log.CertainMethod()) {
        state.logEnabled = state.logEnabled && logList.IncludesMethod(state.methodName);
    } else if (log.NoneMethod()) {
        state.logEnabled = false;
    }
    if (state.logEnabled) {
        codeStream += "------------------- asm code [";
        codeStream += state.methodName;
        codeStream += "] -------------------\n";
    }
}

size_t DisassembleInstruction(LLVMDisasmContextRef disCtx, const CodeDisassemblyState &state, char *outString,
                              size_t outStringSize)
{
    size_t instSize = LLVMDisasmInstruction(disCtx, state.instrAddr, state.remainingBytes, state.instrOffset,
                                            outString, outStringSize);
    return instSize == 0 ? 4 : instSize; // 4: default step size while instruction cannot be resolved
}

void EmitInstructionWithComment(CodeDisassemblyState &state, const AsmOutputLine &line,
                                const CodeCommentOutputContext &context)
{
    if (!state.logEnabled) {
        return;
    }

    object::SectionedAddress secAddr = {state.instrOffset, context.textSecIndex};
    DILineInfo lineInfo = context.dwarfCtx->getLineInfoForAddress(secAddr, context.debugSpecifier);
    uint32_t debugLine = (lineInfo && lineInfo.Line > 0) ? lineInfo.Line : 0;
    if (debugLine == 0) {
        state.previousLine = 0;
        EmitRawAsmLine(line);
        return;
    }

    const bool isFirst = debugLine != state.previousLine;
    const std::string &comment = isFirst
        ? context.llvmModule->GetDebugInfo()->GetComment(state.methodName, debugLine - 1) : EMPTY_COMMENT;
    EmitAsmLine(line, isFirst ? MARKER_FIRST : MARKER_MID, comment);
    state.previousLine = debugLine;
}
} // namespace

void LLVMAssembler::PrintInstAndStep(uint64_t &instrOffset, uint8_t **instrAddr, uintptr_t &numBytes,
                                     size_t instSize, uint64_t textOffset, char *outString,
                                     std::string &codeStream, bool logFlag)
{
    if (instSize == 0) {
        instSize = 4; // 4: default instruction step size while instruction can't be resolved or be constant
    }
    if (logFlag) {
        AsmOutputLine line {codeStream, instrOffset + textOffset, GetInstrValue(instSize, *instrAddr), outString};
        EmitRawAsmLine(line);
    }
    instrOffset += instSize;
    *instrAddr += instSize;
    numBytes -= instSize;
}

void LLVMAssembler::Disassemble(const std::map<uintptr_t, std::string> *addr2name,
                                const std::string& triple, uint8_t *buf, size_t size)
{
    LLVMModuleRef module = LLVMModuleCreateWithName("Emit");
    LLVMSetTarget(module, triple.c_str());
    LLVMDisasmContextRef ctx = LLVMCreateDisasm(LLVMGetTarget(module), nullptr, 0, nullptr, SymbolLookupCallback);
    if (!ctx) {
        LOG_COMPILER(ERROR) << "ERROR: Couldn't create disassembler for triple!";
        return;
    }
    uint8_t *instrAddr = buf;
    uint64_t bufAddr = reinterpret_cast<uint64_t>(buf);
    size_t numBytes = size;
    uint64_t instrOffset = 0;
    const size_t outStringSize = 256;
    char outString[outStringSize];
    std::string codeStream;
    while (numBytes > 0) {
        uint64_t addr = reinterpret_cast<uint64_t>(instrAddr) - bufAddr;
        if (addr2name != nullptr && addr2name->find(addr) != addr2name->end()) {
            std::string methodName = addr2name->at(addr);
            codeStream += "------------------- asm code [";
            codeStream += methodName;
            codeStream += "] -------------------\n";
        }
        size_t instSize = LLVMDisasmInstruction(ctx, instrAddr, numBytes, instrOffset, outString, outStringSize);
        PrintInstAndStep(instrOffset, &instrAddr, numBytes, instSize, 0, outString, codeStream);
    }
    LOG_ECMA(INFO) << "\n" << codeStream;
    LLVMDisasmDispose(ctx);
}

uint64_t LLVMAssembler::GetTextSectionIndex() const
{
    uint64_t index = 0;
    for (object::section_iterator it = objFile_->section_begin(); it != objFile_->section_end(); ++it) {
        auto name = it->getName();
        if (name) {
            std::string str = name->str();
            if (str == ".text") {
                index = it->getIndex();
                ASSERT(it->isText());
                break;
            }
        }
    }
    return index;
}

void LLVMAssembler::Disassemble(const std::map<uintptr_t, std::string> &addr2name, uint64_t textOffset,
                                const CompilerLog &log, const MethodLogList &logList,
                                std::string &codeStream) const
{
    const uint64_t textSecIndex = GetTextSectionIndex();
    LLVMDisasmContextRef disCtx = LLVMCreateDisasm(LLVMGetTarget(module_), nullptr, 0, nullptr, SymbolLookupCallback);
    std::unique_ptr<DWARFContext> dwarfCtx = DWARFContext::create(*objFile_);
    DILineInfoSpecifier debugSpecifier;
    debugSpecifier.FNKind = DINameKind::ShortName;
    CodeCommentOutputContext outputContext {textOffset, textSecIndex, debugSpecifier, dwarfCtx.get(), llvmModule_};

    for (auto it : codeInfo_.GetCodeInfo()) {
        CodeDisassemblyState state {it.first, it.second, 0, false, "",
                                    addr2name.lower_bound(reinterpret_cast<uintptr_t>(it.first)), 0};
        char outString[OUT_STRING_SIZE] = {'\0'};

        while (state.remainingBytes > 0) {
            UpdateMethodLogState(state, addr2name, log, logList, codeStream);
            size_t instSize = DisassembleInstruction(disCtx, state, outString, sizeof(outString));
            AsmOutputLine line {codeStream, state.instrOffset + textOffset,
                                GetInstrValue(instSize, state.instrAddr), outString};
            EmitInstructionWithComment(state, line, outputContext);
            AdvanceInstruction(state, instSize);
        }
    }
    LLVMDisasmDispose(disCtx);
}
}  // namespace panda::ecmascript::kungfu
