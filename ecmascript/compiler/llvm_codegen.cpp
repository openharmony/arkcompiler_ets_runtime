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

#include <cstring>
#include <iomanip>
#include <vector>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "llvm-c/Analysis.h"
#include "llvm-c/Core.h"
#include "llvm-c/Disassembler.h"
#include "llvm-c/DisassemblerTypes.h"
#include "llvm-c/Target.h"
#include "llvm-c/Transforms/PassManagerBuilder.h"
#include "llvm-c/Transforms/Scalar.h"
#include "llvm/ADT/APInt.h"
#include "llvm/CodeGen/BuiltinGCs.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Scalar.h"

#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/stackmap/llvm_stackmap_parser.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

using namespace panda::ecmascript;
namespace panda::ecmascript::kungfu {
void LLVMIRGeneratorImpl::GenerateCodeForStub(Circuit *circuit, const ControlFlowGraph &graph, size_t index,
                                              const CompilationConfig *cfg)
{
    LLVMValueRef function = module_->GetFunction(index);
    const CallSignature* cs = module_->GetCSign(index);
    LLVMIRBuilder builder(&graph, circuit, module_, function, cfg, cs->GetCallConv(), enableLog_);
    builder.Build();
}

void LLVMIRGeneratorImpl::GenerateCode(Circuit *circuit, const ControlFlowGraph &graph, const CompilationConfig *cfg,
                                       const panda::ecmascript::MethodLiteral *methodLiteral,
                                       const JSPandaFile *jsPandaFile)
{
    auto function = module_->AddFunc(methodLiteral, jsPandaFile);
    circuit->SetFrameType(FrameType::OPTIMIZED_JS_FUNCTION_FRAME);
    LLVMIRBuilder builder(&graph, circuit, module_, function, cfg, CallSignature::CallConv::WebKitJSCallConv,
                          enableLog_);
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

LLVMAssembler::LLVMAssembler(LLVMModuleRef module, LOptions option) : module_(module)
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
    if (!log.NoneMethod() && log.OutputLLIR()) {
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
    if (!log.NoneMethod() && log.OutputLLIR()) {
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
                                        uint64_t *referenceType, [[maybe_unused]]uint64_t referencePC,
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
            if (str.size() >= expectedKey.size() &&
                str.substr(0, expectedKey.size()) == expectedKey) {
                int RegNum = std::stoi(str.substr(expectedKey.size(), str.size() - expectedKey.size()));
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
    LLVMAttributeRef attrirbuteRef = LLVMGetStringAttributeAtIndex(fn,
        llvm::AttributeList::FunctionIndex, attrKey, strlen(attrKey));
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

void LLVMAssembler::PrintInstAndStep(unsigned &pc, uint8_t **byteSp, uintptr_t &numBytes,
    size_t instSize, char *outString, bool logFlag)
{
    if (instSize == 0) {
        instSize = 4; // 4: default instruction step size while instruction can't be resolved or be constant
    }
    if (logFlag) {
        // 8: length of output content
        LOG_COMPILER(INFO) << std::setw(8) << std::setfill('0') << std::hex << pc << ":" << std::setw(8)
                            << *reinterpret_cast<uint32_t *>(*byteSp) << " " << outString;
    }
    pc += instSize;
    *byteSp += instSize;
    numBytes -= instSize;
}

void LLVMAssembler::Disassemble(uint8_t *buf, size_t size)
{
    std::string triple = "x86_64-unknown-linux-gnu";
    LLVMModuleRef module = LLVMModuleCreateWithName("Emit");
    LLVMSetTarget(module, triple.c_str());
    LLVMDisasmContextRef dcr = LLVMCreateDisasm(LLVMGetTarget(module), nullptr, 0, nullptr, SymbolLookupCallback);
    if (!dcr) {
        LOG_COMPILER(ERROR) << "ERROR: Couldn't create disassembler for triple!";
        return;
    }
    uint8_t *byteSp = buf;
    uintptr_t numBytes = size;
    unsigned pc = 0;
    const size_t outStringSize = 128;
    char outString[outStringSize];
    while (numBytes > 0) {
        size_t instSize = LLVMDisasmInstruction(dcr, byteSp, numBytes, pc, outString, outStringSize);
        PrintInstAndStep(pc, &byteSp, numBytes, instSize, outString);
    }
    LLVMDisasmDispose(dcr);
}

void LLVMAssembler::Disassemble(const std::map<uintptr_t, std::string> &addr2name,
                                const CompilerLog &log, const MethodLogList &logList) const
{
    LLVMDisasmContextRef dcr = LLVMCreateDisasm(LLVMGetTarget(module_), nullptr, 0, nullptr, SymbolLookupCallback);
    bool logFlag = false;
    unsigned pc = 0;

    for (auto it : codeInfo_.GetCodeInfo()) {
        uint8_t *byteSp = it.first;
        uintptr_t numBytes = it.second;

        const size_t outStringSize = 128;
        char outString[outStringSize];
        std::string methodName;
        while (numBytes > 0) {
            uint64_t addr = reinterpret_cast<uint64_t>(byteSp);
            if (addr2name.find(addr) != addr2name.end()) {
                methodName = addr2name.at(addr);
                logFlag = log.OutputASM();
                if (log.CertainMethod()) {
                    logFlag = logFlag && logList.IncludesMethod(methodName);
                } else if (log.NoneMethod()) {
                    logFlag = false;
                }
                if (logFlag) {
                    LOG_COMPILER(INFO) << "\033[34m"
                                       << "========================  Generated Asm Code ============================="
                                       << "\033[0m";
                    LOG_COMPILER(INFO) << "\033[34m" << "aot method [" << methodName << "]:" << "\033[0m";
                }
            }

            size_t instSize = LLVMDisasmInstruction(dcr, byteSp, numBytes, pc, outString, outStringSize);
            PrintInstAndStep(pc, &byteSp, numBytes, instSize, outString, logFlag);
        }
    }
    LLVMDisasmDispose(dcr);
}
}  // namespace panda::ecmascript::kungfu
