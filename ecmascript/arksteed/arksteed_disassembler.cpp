/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/arksteed/arksteed_disassembler.h"

#include <llvm-c/Disassembler.h>
#include <llvm-c/Target.h>

#include <iomanip>
#include <mutex>
#include <sstream>

#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript::arksteed {

void ArkSteedDisassembler::InitializeDisassemblerTarget()
{
    static std::once_flag initOnce;
    std::call_once(initOnce, []() {
#if defined(PANDA_TARGET_AMD64)
        LLVMInitializeX86TargetInfo();
        LLVMInitializeX86TargetMC();
        LLVMInitializeX86Disassembler();
        LLVMInitializeX86AsmPrinter();
        LLVMInitializeX86AsmParser();
        LLVMInitializeX86Target();
#elif defined(PANDA_TARGET_ARM64)
        LLVMInitializeAArch64TargetInfo();
        LLVMInitializeAArch64TargetMC();
        LLVMInitializeAArch64Disassembler();
        LLVMInitializeAArch64AsmPrinter();
        LLVMInitializeAArch64AsmParser();
        LLVMInitializeAArch64Target();
#endif
    });
}

ArkSteedDisassembler::ArkSteedDisassembler(const uint8_t *code, size_t codeSize, const uint8_t *commentsData,
                                           uint32_t commentsSize)
    : code_(code), codeSize_(codeSize), commentsData_(commentsData), commentsSize_(commentsSize)
{}

void ArkSteedDisassembler::Disassemble(std::ostream &os)
{
    InitializeDisassemblerTarget();
#if defined(PANDA_TARGET_AMD64)
    const char *triple = "x86_64-unknown-linux-gnu";
#elif defined(PANDA_TARGET_ARM64)
    const char *triple = "aarch64-unknown-linux-gnu";
#else
    LOG_JIT(ERROR) << "Unsupported architecture for disassembler";
    return;
#endif

    LLVMDisasmContextRef ctx = LLVMCreateDisasm(triple, nullptr, 0, nullptr, nullptr);
    if (!ctx) {
        LOG_JIT(ERROR) << "Could not create disassembler for triple: " << triple;
        return;
    }

    const uint8_t *instrAddr = code_;
    size_t numBytes = codeSize_;
    uint64_t pc = 0;
    const size_t outStringSize = 256;  // 256: buffer size for LLVM disassembly output string
    char outString[outStringSize];

    CommentList::Iterator cit(commentsData_, commentsSize_);

    while (numBytes > 0) {
        // Print comments whose pcOffset <= current PC before instruction
        while (cit.HasCurrent() && cit.GetPCOffset() <= pc) {
            os << "                  \033[34m[ArkSteed] " << cit.GetComment() << "\033[0m\n";
            cit.Next();
        }

        size_t instSize =
            LLVMDisasmInstruction(ctx, const_cast<uint8_t *>(instrAddr), numBytes, pc, outString, outStringSize);
        if (instSize == 0) {
            os << reinterpret_cast<const void *>(instrAddr) << "  " << std::setw(4) << std::hex << pc << std::dec
               << "  .byte 0x" << std::hex << static_cast<uint32_t>(*instrAddr) << std::dec << "\n";
            instSize = 1;
        } else {
            os << reinterpret_cast<const void *>(instrAddr) << "  " << std::setw(4) << std::hex << pc << std::dec
               << "  " << outString << "\n";
        }

        pc += instSize;
        instrAddr += instSize;
        numBytes -= instSize;
    }

    while (cit.HasCurrent()) {
        os << "                  \033[34m[ArkSteed] " << cit.GetComment() << "\033[0m\n";
        cit.Next();
    }

    LLVMDisasmDispose(ctx);
}

}  // namespace panda::ecmascript::arksteed
