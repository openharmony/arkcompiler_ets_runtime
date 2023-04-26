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

#include "ecmascript/compiler/llvm_codegen.h"
#if defined(PANDA_TARGET_MACOS) || defined(PANDA_TARGET_IOS)
#include "ecmascript/base/llvm_helper.h"
#endif

#include <cstring>
#include <iomanip>
#include <vector>

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
#include "llvm-c/DisassemblerTypes.h"
#include "llvm-c/Target.h"
#include "llvm-c/Transforms/PassManagerBuilder.h"
#include "llvm/CodeGen/BuiltinGCs.h"
#include "llvm/DebugInfo/DWARF/DWARFContext.h"
#include "llvm/DebugInfo/DIContext.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/Scalar.h"

#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/compiler/compiler_log.h"
#include "ecmascript/compiler/debug_info.h"
#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/stackmap/llvm_stackmap_parser.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;
using namespace llvm;

CodeInfo::CodeInfo()
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

CodeInfo::~CodeInfo()
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

uint8_t *CodeInfo::AllocaInReqSecBuffer(uintptr_t size, bool alignFlag)
{
    return Alloca(size, reqSecs_, reqBufPos_, alignFlag);
}

uint8_t *CodeInfo::AllocaInNotReqSecBuffer(uintptr_t size)
{
    return Alloca(size, unreqSecs_, unreqBufPos_);
}

uint8_t *CodeInfo::AllocaCodeSection(uintptr_t size, const char *sectionName)
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

uint8_t *CodeInfo::AllocaDataSection(uintptr_t size, const char *sectionName)
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

void CodeInfo::Reset()
{
    codeInfo_.clear();
    reqBufPos_ = 0;
    unreqBufPos_ = 0;
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

std::vector<std::pair<uint8_t *, uintptr_t>> CodeInfo::GetCodeInfo() const
{
    return codeInfo_;
}

uint8_t *CodeInfo::Alloca(uintptr_t size, uint8_t *bufBegin, size_t &curPos, bool alignFlag)
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

void LLVMIRGeneratorImpl::GenerateCodeForStub(Circuit *circuit, const ControlFlowGraph &graph, size_t index,
                                              const CompilationConfig *cfg)
{
    LLVMValueRef function = module_->GetFunction(index);
    const CallSignature* cs = module_->GetCSign(index);
    LLVMIRBuilder builder(&graph, circuit, module_, function, cfg, cs->GetCallConv(), enableLog_, cs->GetName());
    builder.Build();
}

void LLVMIRGeneratorImpl::GenerateCode(Circuit *circuit, const ControlFlowGraph &graph, const CompilationConfig *cfg,
                                       const panda::ecmascript::MethodLiteral *methodLiteral,
                                       const JSPandaFile *jsPandaFile, const std::string &methodName)
{
    auto function = module_->AddFunc(methodLiteral, jsPandaFile);
    circuit->SetFrameType(FrameType::OPTIMIZED_JS_FUNCTION_FRAME);
    LLVMIRBuilder builder(&graph, circuit, module_, function, cfg, CallSignature::CallConv::WebKitJSCallConv,
                          enableLog_, methodName);
    builder.Build();
}

static uint8_t *RoundTripAllocateCodeSection(void *object, uintptr_t size, [[maybe_unused]] unsigned alignment,
                                             [[maybe_unused]] unsigned sectionID, const char *sectionName)
{
    struct CodeInfo& state = *static_cast<struct CodeInfo*>(object);
    return state.AllocaCodeSection(size, sectionName);
}

static uint8_t *RoundTripAllocateDataSection(void *object, uintptr_t size, [[maybe_unused]] unsigned alignment,
                                             [[maybe_unused]] unsigned sectionID, const char *sectionName,
                                             [[maybe_unused]] LLVMBool isReadOnly)
{
    struct CodeInfo& state = *static_cast<struct CodeInfo*>(object);
    return state.AllocaDataSection(size, sectionName);
}

static LLVMBool RoundTripFinalizeMemory([[maybe_unused]] void *object, [[maybe_unused]] char **errMsg)
{
    return 0;
}

static void RoundTripDestroy([[maybe_unused]] void *object)
{
    return;
}

void LLVMAssembler::UseRoundTripSectionMemoryManager()
{
    auto sectionMemoryManager = std::make_unique<llvm::SectionMemoryManager>();
    options_.MCJMM =
        LLVMCreateSimpleMCJITMemoryManager(&codeInfo_, RoundTripAllocateCodeSection,
            RoundTripAllocateDataSection, RoundTripFinalizeMemory, RoundTripDestroy);
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

    // pass manager creation:rs4gc pass is the only pass in modPass, other opt module-based pass are in modPass1
    LLVMPassManagerRef funcPass = LLVMCreateFunctionPassManagerForModule(module_);
    LLVMPassManagerRef modPass = LLVMCreatePassManager();

    // add pass into pass managers
    LLVMPassManagerBuilderPopulateFunctionPassManager(pmBuilder, funcPass);
    llvm::unwrap(modPass)->add(llvm::createRewriteStatepointsForGCLegacyPass()); // rs4gc pass added

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

LLVMAssembler::LLVMAssembler(LLVMModule *lm, LOptions option)
    : llvmModule_(lm),
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

void LLVMAssembler::Run(const CompilerLog &log)
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
    UseRoundTripSectionMemoryManager();
    if (!BuildMCJITEngine()) {
        return;
    }
    llvm::unwrap(engine_)->setProcessAllSections(true);
    BuildAndRunPasses();
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
    if (triple.compare("x86_64-unknown-linux-gnu") == 0) {
#if defined(PANDA_TARGET_MACOS) || !defined(PANDA_TARGET_ARM64)
        LLVMInitializeX86TargetInfo();
        LLVMInitializeX86TargetMC();
        LLVMInitializeX86Disassembler();
        /* this method must be called, ohterwise "Target does not support MC emission" */
        LLVMInitializeX86AsmPrinter();
        LLVMInitializeX86AsmParser();
        LLVMInitializeX86Target();
#endif
    } else if (triple.compare("aarch64-unknown-linux-gnu") == 0) {
        LLVMInitializeAArch64TargetInfo();
        LLVMInitializeAArch64TargetMC();
        LLVMInitializeAArch64Disassembler();
        LLVMInitializeAArch64AsmPrinter();
        LLVMInitializeAArch64AsmParser();
        LLVMInitializeAArch64Target();
    } else if (triple.compare("arm-unknown-linux-gnu") == 0) {
#if defined(PANDA_TARGET_MACOS) || !defined(PANDA_TARGET_ARM64)
        LLVMInitializeARMTargetInfo();
        LLVMInitializeARMTargetMC();
        LLVMInitializeARMDisassembler();
        LLVMInitializeARMAsmPrinter();
        LLVMInitializeARMAsmParser();
        LLVMInitializeARMTarget();
#endif
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }

    llvm::linkAllBuiltinGCs();
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
    for (const auto &Attr : func->getAttributes().getFnAttributes()) {
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

void LLVMAssembler::PrintInstAndStep(uint64_t &instrOffset, uint8_t **instrAddr, uintptr_t &numBytes,
                                     size_t instSize, char *outString, bool logFlag)
{
    if (instSize == 0) {
        instSize = 4; // 4: default instruction step size while instruction can't be resolved or be constant
    }
    if (logFlag) {
        // 8: length of output content
        LOG_COMPILER(INFO) << std::setw(8) << std::setfill('0') << std::hex << instrOffset << ":" << std::setw(8)
                           << GetInstrValue(instSize, *instrAddr) << " " << outString;
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
    while (numBytes > 0) {
        uint64_t addr = reinterpret_cast<uint64_t>(instrAddr) - bufAddr;
        if (addr2name != nullptr && addr2name->find(addr) != addr2name->end()) {
            std::string methodName = addr2name->at(addr);
            LOG_COMPILER(INFO) << "------------------- asm code [" << methodName << "] -------------------";
        }
        size_t instSize = LLVMDisasmInstruction(ctx, instrAddr, numBytes, instrOffset, outString, outStringSize);
        PrintInstAndStep(instrOffset, &instrAddr, numBytes, instSize, outString);
    }
    LLVMDisasmDispose(ctx);
}

static void DecodeDebugInfo(uint64_t addr, uint64_t secIndex, char* outString, size_t outStringSize,
                            DWARFContext *ctx, LLVMModule* module, const std::string &funcName)
{
    object::SectionedAddress secAddr = {addr, secIndex};
    DILineInfoSpecifier spec;
    spec.FNKind = DINameKind::ShortName;

    DILineInfo info = ctx->getLineInfoForAddress(secAddr, spec);
    if (info && info.Line > 0) {
        std::string debugInfo = "\t\t;";
        debugInfo += module->GetDebugInfo()->GetComment(funcName, info.Line - 1);
        size_t len = strlen(outString);
        if (len + debugInfo.size() <= outStringSize) {
            if (strcpy_s(outString + len, outStringSize - len, debugInfo.c_str()) != EOK) {
                LOG_FULL(FATAL) << "strcpy_s failed";
                UNREACHABLE();
            }
        }
    }
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

void LLVMAssembler::Disassemble(const std::map<uintptr_t, std::string> &addr2name,
                                const CompilerLog &log, const MethodLogList &logList) const
{
    const uint64_t textSecIndex = GetTextSectionIndex();
    LLVMDisasmContextRef disCtx = LLVMCreateDisasm(LLVMGetTarget(module_), nullptr, 0, nullptr, SymbolLookupCallback);
    bool logFlag = false;
    std::unique_ptr<DWARFContext> dwarfCtx = DWARFContext::create(*objFile_);

    for (auto it : codeInfo_.GetCodeInfo()) {
        uint8_t *instrAddr = it.first;
        size_t numBytes = it.second;
        uint64_t instrOffset = 0;

        const size_t outStringSize = 256;
        char outString[outStringSize] = {'\0'};
        std::string methodName;

        while (numBytes > 0) {
            uint64_t addr = reinterpret_cast<uint64_t>(instrAddr);
            if (addr2name.find(addr) != addr2name.end()) {
                methodName = addr2name.at(addr);
                logFlag = log.OutputASM();
                if (log.CertainMethod()) {
                    logFlag = logFlag && logList.IncludesMethod(methodName);
                } else if (log.NoneMethod()) {
                    logFlag = false;
                }
                if (logFlag) {
                    LOG_COMPILER(INFO) << "------------------- asm code [" << methodName << "] -------------------";
                }
            }

            size_t instSize = LLVMDisasmInstruction(disCtx, instrAddr, numBytes, instrOffset, outString, outStringSize);
            DecodeDebugInfo(instrOffset, textSecIndex, outString, outStringSize,
                            dwarfCtx.get(), llvmModule_, methodName);
            PrintInstAndStep(instrOffset, &instrAddr, numBytes, instSize, outString, logFlag);
        }
    }
    LLVMDisasmDispose(disCtx);
}
}  // namespace panda::ecmascript::kungfu
