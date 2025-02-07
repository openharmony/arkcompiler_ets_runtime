/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/jit/rewriter/reloc_rewriter_aarch64.h"
#include "ecmascript/compiler/assembler/aarch64/assembler_aarch64_constants.h"
#include <set>

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;

bool RelocWriterAArch64::RewriteRelocInfo(uint8_t *codeAddr, uint8_t *jitAllocAddr, RelocMap &relocInfo)
{
    for (auto [startPc, endPc, dest] : relocInfo) {
        intptr_t distance = dest - (intptr_t) jitAllocAddr - endPc;
        intptr_t imm = distance / 4;
        if (distance > MAX_JUMP_SIZE || distance < -MAX_JUMP_SIZE) {
            continue;
        } else {
            size_t numMovInstr = (endPc - startPc) / INSTRUCT_SIZE;
            for (size_t i = 0; i < numMovInstr; i++) {
                uint32_t nopInstr = aarch64::NopOpCode::Nop;
                memcpy_s(codeAddr + startPc + i * INSTRUCT_SIZE,
                         INSTRUCT_SIZE, reinterpret_cast<uint8_t *>(&nopInstr), INSTRUCT_SIZE);
            }
            uint32_t blInstr = aarch64::CallOpCode::BL | ((imm << aarch64::BRANCH_Imm26_LOWBITS) &
                               aarch64::BRANCH_Imm26_MASK);
            memcpy_s(codeAddr + endPc, INSTRUCT_SIZE, reinterpret_cast<uint8_t *>(&blInstr), INSTRUCT_SIZE);
        }
    }
    return true;
}
}  // namespace panda::ecmascript::kungfu
