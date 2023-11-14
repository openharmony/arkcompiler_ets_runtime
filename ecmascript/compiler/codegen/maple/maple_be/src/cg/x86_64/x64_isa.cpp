/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "x64_isa.h"
#include "insn.h"

namespace maplebe {
namespace x64 {
MOperator FlipConditionOp(MOperator flippedOp)
{
    switch (flippedOp) {
        case X64MOP_t::MOP_je_l:
            return X64MOP_t::MOP_jne_l;
        case X64MOP_t::MOP_jne_l:
            return X64MOP_t::MOP_je_l;
        case X64MOP_t::MOP_ja_l:
            return X64MOP_t::MOP_jbe_l;
        case X64MOP_t::MOP_jbe_l:
            return X64MOP_t::MOP_ja_l;
        case X64MOP_t::MOP_jae_l:
            return X64MOP_t::MOP_jb_l;
        case X64MOP_t::MOP_jb_l:
            return X64MOP_t::MOP_jae_l;
        case X64MOP_t::MOP_jg_l:
            return X64MOP_t::MOP_jle_l;
        case X64MOP_t::MOP_jle_l:
            return X64MOP_t::MOP_jg_l;
        case X64MOP_t::MOP_jge_l:
            return X64MOP_t::MOP_jl_l;
        case X64MOP_t::MOP_jl_l:
            return X64MOP_t::MOP_jge_l;
        default:
            break;
    }
    return X64MOP_t::MOP_begin;
}

uint32 GetJumpTargetIdx(const Insn &insn)
{
    CHECK_FATAL(insn.IsCondBranch() || insn.IsUnCondBranch(), "Not a jump insn");
    return kInsnFirstOpnd;
}
} /* namespace x64 */
} /* namespace maplebe */
