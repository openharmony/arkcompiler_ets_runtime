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

#include "aarch64_isa.h"
#include "insn.h"

namespace maplebe {
/*
 * Get the ldp/stp corresponding to ldr/str
 * mop : a ldr or str machine operator
 */
MOperator GetMopPair(MOperator mop, bool isIncludeStrbStrh)
{
    switch (mop) {
        case MOP_xldr:
            return MOP_xldp;
        case MOP_wldr:
            return MOP_wldp;
        case MOP_xstr:
            return MOP_xstp;
        case MOP_wstr:
            return MOP_wstp;
        case MOP_dldr:
            return MOP_dldp;
        case MOP_qldr:
            return MOP_qldp;
        case MOP_sldr:
            return MOP_sldp;
        case MOP_dstr:
            return MOP_dstp;
        case MOP_sstr:
            return MOP_sstp;
        case MOP_qstr:
            return MOP_qstp;
        case MOP_wstrb:
            return isIncludeStrbStrh ? MOP_wstrh : MOP_undef;
        case MOP_wstrh:
            return isIncludeStrbStrh ? MOP_wstr : MOP_undef;
        default:
            DEBUG_ASSERT(false, "should not run here");
            return MOP_undef;
    }
}
namespace AArch64isa {
MOperator FlipConditionOp(MOperator flippedOp)
{
    switch (flippedOp) {
        case AArch64MopT::MOP_beq:
            return AArch64MopT::MOP_bne;
        case AArch64MopT::MOP_bge:
            return AArch64MopT::MOP_blt;
        case AArch64MopT::MOP_bgt:
            return AArch64MopT::MOP_ble;
        case AArch64MopT::MOP_bhi:
            return AArch64MopT::MOP_bls;
        case AArch64MopT::MOP_bhs:
            return AArch64MopT::MOP_blo;
        case AArch64MopT::MOP_ble:
            return AArch64MopT::MOP_bgt;
        case AArch64MopT::MOP_blo:
            return AArch64MopT::MOP_bhs;
        case AArch64MopT::MOP_bls:
            return AArch64MopT::MOP_bhi;
        case AArch64MopT::MOP_blt:
            return AArch64MopT::MOP_bge;
        case AArch64MopT::MOP_bne:
            return AArch64MopT::MOP_beq;
        case AArch64MopT::MOP_bpl:
            return AArch64MopT::MOP_bmi;
        case AArch64MopT::MOP_xcbnz:
            return AArch64MopT::MOP_xcbz;
        case AArch64MopT::MOP_wcbnz:
            return AArch64MopT::MOP_wcbz;
        case AArch64MopT::MOP_xcbz:
            return AArch64MopT::MOP_xcbnz;
        case AArch64MopT::MOP_wcbz:
            return AArch64MopT::MOP_wcbnz;
        case AArch64MopT::MOP_wtbnz:
            return AArch64MopT::MOP_wtbz;
        case AArch64MopT::MOP_wtbz:
            return AArch64MopT::MOP_wtbnz;
        case AArch64MopT::MOP_xtbnz:
            return AArch64MopT::MOP_xtbz;
        case AArch64MopT::MOP_xtbz:
            return AArch64MopT::MOP_xtbnz;
        default:
            break;
    }
    return AArch64MopT::MOP_undef;
}

uint32 GetJumpTargetIdx(const Insn &insn)
{
    MOperator curMop = insn.GetMachineOpcode();
    switch (curMop) {
        /* unconditional jump */
        case MOP_xuncond: {
            return kInsnFirstOpnd;
        }
        case MOP_xbr: {
            DEBUG_ASSERT(insn.GetOperandSize() == 2, "ERR");  // must have 2
            return kInsnSecondOpnd;
        }
            /* conditional jump */
        case MOP_bmi:
        case MOP_bvc:
        case MOP_bls:
        case MOP_blt:
        case MOP_ble:
        case MOP_blo:
        case MOP_beq:
        case MOP_bpl:
        case MOP_bhs:
        case MOP_bvs:
        case MOP_bhi:
        case MOP_bgt:
        case MOP_bge:
        case MOP_bne:
        case MOP_wcbz:
        case MOP_xcbz:
        case MOP_wcbnz:
        case MOP_xcbnz: {
            return kInsnSecondOpnd;
        }
        case MOP_wtbz:
        case MOP_xtbz:
        case MOP_wtbnz:
        case MOP_xtbnz: {
            return kInsnThirdOpnd;
        }
        default:
            CHECK_FATAL(false, "Not a jump insn");
    }
    return kInsnFirstOpnd;
}

// This api is only used for cgir verify, implemented by calling the memopndofst interface.
int64 GetMemOpndOffsetValue(Operand *o)
{
    auto *memOpnd = static_cast<MemOperand *>(o);
    CHECK_FATAL(memOpnd != nullptr, "memOpnd should not be nullptr");
    // kBOR memOpnd has no offsetvalue, so return 0 for verify.
    // todo: AArch64AddressingMode is different from BiShengC
    if (memOpnd->GetAddrMode() == MemOperand::kAddrModeBOrX) {
        return 0;
    }
    // Offset value of kBOI & kLo12Li can be got.
    OfstOperand *ofStOpnd = memOpnd->GetOffsetImmediate();
    int64 offsetValue = ofStOpnd ? ofStOpnd->GetOffsetValue() : 0LL;
    return offsetValue;
}

bool IsSub(const Insn &insn)
{
    MOperator curMop = insn.GetMachineOpcode();
    switch (curMop) {
        case MOP_xsubrrr:
        case MOP_xsubrrrs:
        case MOP_xsubrri24:
        case MOP_xsubrri12:
        case MOP_wsubrrr:
        case MOP_wsubrrrs:
        case MOP_wsubrri24:
        case MOP_wsubrri12:
            return true;
        default:
            return false;
    }
}

MOperator GetMopSub2Subs(const Insn &insn)
{
    MOperator curMop = insn.GetMachineOpcode();
    switch (curMop) {
        case MOP_xsubrrr:
            return MOP_xsubsrrr;
        case MOP_xsubrrrs:
            return MOP_xsubsrrrs;
        case MOP_xsubrri24:
            return MOP_xsubsrri24;
        case MOP_xsubrri12:
            return MOP_xsubsrri12;
        case MOP_wsubrrr:
            return MOP_wsubsrrr;
        case MOP_wsubrrrs:
            return MOP_wsubsrrrs;
        case MOP_wsubrri24:
            return MOP_wsubsrri24;
        case MOP_wsubrri12:
            return MOP_wsubsrri12;
        default:
            return curMop;
    }
}

// Returns the number of trailing 0-bits in x, starting at the least significant bit position.
// If x is 0, the result is -1.
int32 GetTail0BitNum(int64 val)
{
    uint32 bitNum = 0;
    for (; bitNum < k64BitSize; bitNum++) {
        if (((1ULL << bitNum) & static_cast<uint64>(val)) != 0) {
            break;
        }
    }
    if (bitNum == k64BitSize) {
        return -1;
    }
    return static_cast<int32>(bitNum);
}

// Returns the number of leading 0-bits in x, starting at the most significant bit position.
// If x is 0, the result is -1.
int32 GetHead0BitNum(int64 val)
{
    uint32 bitNum = 0;
    for (; bitNum < k64BitSize; bitNum++) {
        if (((0x8000000000000000ULL >> bitNum) & static_cast<uint64>(val)) != 0) {
            break;
        }
    }
    if (bitNum == k64BitSize) {
        return -1;
    }
    return static_cast<int32>(bitNum);
}
} /* namespace AArch64isa */
} /* namespace maplebe */
