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

#include "operand.h"

namespace maplebe {
bool IsMoveWidableImmediate(uint64 val, uint32 bitLen)
{
    if (bitLen == k64BitSize) {
        /* 0xHHHH000000000000 or 0x0000HHHH00000000, return true */
        if (((val & ((static_cast<uint64>(0xffff)) << k48BitSize)) == val) ||
            ((val & ((static_cast<uint64>(0xffff)) << k32BitSize)) == val)) {
            return true;
        }
    } else {
        /* get lower 32 bits */
        val &= static_cast<uint64>(0xffffffff);
    }
    /* 0x00000000HHHH0000 or 0x000000000000HHHH, return true */
    return ((val & ((static_cast<uint64>(0xffff)) << k16BitSize)) == val ||
            (val & ((static_cast<uint64>(0xffff)) << 0)) == val);
}

bool BetterUseMOVZ(uint64 val)
{
    int32 n16zerosChunks = 0;
    int32 n16onesChunks = 0;
    uint64 sa = 0;
    /* a 64 bits number is split 4 chunks, each chunk has 16 bits. check each chunk whether is all 1 or is all 0 */
    for (uint64 i = 0; i < k4BitSize; ++i, sa += k16BitSize) {
        uint64 chunkVal = (val >> (static_cast<uint64>(sa))) & 0x0000FFFFUL;
        if (chunkVal == 0) {
            ++n16zerosChunks;
        } else if (chunkVal == 0xFFFFUL) {
            ++n16onesChunks;
        }
    }
    /*
     * note that since we already check if the value
     * can be movable with as a single mov instruction,
     * we should not expect either n_16zeros_chunks>=3 or n_16ones_chunks>=3
     */
#if DEBUG
    constexpr uint32 kN16ChunksCheck = 2;
    DEBUG_ASSERT(static_cast<uint32>(n16zerosChunks) <= kN16ChunksCheck, "n16zerosChunks ERR");
    DEBUG_ASSERT(static_cast<uint32>(n16onesChunks) <= kN16ChunksCheck, "n16onesChunks ERR");
#endif
    return (n16zerosChunks >= n16onesChunks);
}

bool RegOperand::operator==(const RegOperand &o) const
{
    regno_t myRn = GetRegisterNumber();
    uint32 mySz = GetSize();
    uint32 myFl = regFlag;
    regno_t otherRn = o.GetRegisterNumber();
    uint32 otherSz = o.GetSize();
    uint32 otherFl = o.regFlag;

    if (IsPhysicalRegister()) {
        return (myRn == otherRn && mySz == otherSz && myFl == otherFl);
    }
    return (myRn == otherRn && mySz == otherSz);
}

bool RegOperand::operator<(const RegOperand &o) const
{
    regno_t myRn = GetRegisterNumber();
    uint32 mySz = GetSize();
    uint32 myFl = regFlag;
    regno_t otherRn = o.GetRegisterNumber();
    uint32 otherSz = o.GetSize();
    uint32 otherFl = o.regFlag;
    return myRn < otherRn || (myRn == otherRn && mySz < otherSz) ||
           (myRn == otherRn && mySz == otherSz && myFl < otherFl);
}

Operand *MemOperand::GetOffset() const
{
    switch (addrMode) {
        case kAddrModeBOi:
            return GetOffsetOperand();
        case kAddrModeBOrX:
            return GetIndexRegister();
        case kAddrModeLiteral:
            break;
        case kAddrModeLo12Li:
            break;
        default:
            DEBUG_ASSERT(false, "error memoperand dump");
            break;
    }
    return nullptr;
}

bool MemOperand::Equals(Operand &op) const
{
    if (!op.IsMemoryAccessOperand()) {
        return false;
    }
    return Equals(static_cast<MemOperand &>(op));
}

bool MemOperand::Equals(const MemOperand &op) const
{
    if (&op == this) {
        return true;
    }

    if (addrMode == op.GetAddrMode()) {
        switch (addrMode) {
            case kAddrModeBOi:
                return (GetBaseRegister()->Equals(*op.GetBaseRegister()) &&
                        GetOffsetImmediate()->Equals(*op.GetOffsetImmediate()));
            case kAddrModeBOrX:
                return (GetBaseRegister()->Equals(*op.GetBaseRegister()) &&
                        GetIndexRegister()->Equals(*op.GetIndexRegister()) &&
                        GetExtendAsString() == op.GetExtendAsString() && ShiftAmount() == op.ShiftAmount());
            case kAddrModeLiteral:
                return GetSymbolName() == op.GetSymbolName();
            case kAddrModeLo12Li:
                return (GetBaseRegister()->Equals(*op.GetBaseRegister()) && GetSymbolName() == op.GetSymbolName() &&
                        GetOffsetImmediate()->Equals(*op.GetOffsetImmediate()));
            default:
                DEBUG_ASSERT(false, "error memoperand");
                break;
        }
    }
    return false;
}

bool MemOperand::Less(const Operand &right) const
{
    if (&right == this) {
        return false;
    }

    /* For different type. */
    if (GetKind() != right.GetKind()) {
        return GetKind() < right.GetKind();
    }

    const MemOperand *rightOpnd = static_cast<const MemOperand *>(&right);
    if (addrMode != rightOpnd->addrMode) {
        return addrMode < rightOpnd->addrMode;
    }

    switch (addrMode) {
        case kAddrModeBOi: {
            DEBUG_ASSERT(idxOpt == kIntact, "Should not compare pre/post index addressing.");

            RegOperand *baseReg = GetBaseRegister();
            RegOperand *rbaseReg = rightOpnd->GetBaseRegister();
            CHECK_FATAL(baseReg != nullptr, "nullptr check");
            int32 nRet = baseReg->RegCompare(*rbaseReg);
            if (nRet == 0) {
                Operand *ofstOpnd = GetOffsetOperand();
                const Operand *rofstOpnd = rightOpnd->GetOffsetOperand();
                return ofstOpnd->Less(*rofstOpnd);
            }
            return nRet < 0;
        }
        case kAddrModeBOrX: {
            if (noExtend != rightOpnd->noExtend) {
                return noExtend;
            }
            if (!noExtend && extend != rightOpnd->extend) {
                return extend < rightOpnd->extend;
            }
            RegOperand *indexReg = GetIndexRegister();
            const RegOperand *rindexReg = rightOpnd->GetIndexRegister();
            CHECK_FATAL(indexReg != nullptr, "nullptr check");
            return indexReg->Less(*rindexReg);
        }
        case kAddrModeLiteral: {
            return static_cast<const void *>(GetSymbol()) < static_cast<const void *>(rightOpnd->GetSymbol());
        }
        case kAddrModeLo12Li: {
            if (GetSymbol() != rightOpnd->GetSymbol()) {
                return static_cast<const void *>(GetSymbol()) < static_cast<const void *>(rightOpnd->GetSymbol());
            }
            Operand *ofstOpnd = GetOffsetOperand();
            const Operand *rofstOpnd = rightOpnd->GetOffsetOperand();
            return ofstOpnd->Less(*rofstOpnd);
        }
        default:
            DEBUG_ASSERT(false, "Internal error.");
            return false;
    }
}

const char *CondOperand::ccStrs[kCcLast] = {"EQ", "NE", "CS", "HS", "CC", "LO", "MI", "PL", "VS",
                                            "VC", "HI", "LS", "GE", "LT", "GT", "LE", "AL"};

bool CondOperand::Less(const Operand &right) const
{
    if (&right == this) {
        return false;
    }

    /* For different type. */
    if (GetKind() != right.GetKind()) {
        return GetKind() < right.GetKind();
    }

    const CondOperand *rightOpnd = static_cast<const CondOperand *>(&right);

    /* The same type. */
    if (cc == CC_AL || rightOpnd->cc == CC_AL) {
        return false;
    }
    return cc < rightOpnd->cc;
}

uint32 PhiOperand::GetLeastCommonValidBit() const
{
    uint32 leastCommonVb = 0;
    for (auto phiOpnd : phiList) {
        uint32 curVb = phiOpnd.second->GetValidBitsNum();
        if (curVb > leastCommonVb) {
            leastCommonVb = curVb;
        }
    }
    return leastCommonVb;
}
bool PhiOperand::IsRedundancy() const
{
    uint32 srcSsaIdx = 0;
    for (auto phiOpnd : phiList) {
        if (srcSsaIdx == 0) {
            srcSsaIdx = phiOpnd.second->GetRegisterNumber();
        }
        if (srcSsaIdx != phiOpnd.second->GetRegisterNumber()) {
            return false;
        }
    }
    return true;
}
} /* namespace maplebe */
