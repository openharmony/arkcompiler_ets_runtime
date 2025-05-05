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

#include "ecmascript/compiler/assembler/aarch64/macro_assembler_aarch64.h"
#include "ecmascript/compiler/assembler/macro_assembler.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;
constexpr uint32_t k4BitSize = 4;
constexpr uint32_t k16BitSize = 16;
constexpr uint32_t k32BitSize = 32;
constexpr uint32_t k48BitSize = 48;
constexpr uint32_t k64BitSize = 64;

const std::set<uint64_t> ValidBitmaskImmSet = {
#include "valid_bitmask_imm.txt"
};
constexpr uint32_t kMaxBitTableSize = 5;
constexpr std::array<uint64_t, kMaxBitTableSize> kBitmaskImmMultTable = {
    0x0000000100000001UL, 0x0001000100010001UL, 0x0101010101010101UL, 0x1111111111111111UL, 0x5555555555555555UL,
};

void MacroAssemblerAArch64::Move(const StackSlotOperand &dstStackSlot, Immediate value)
{
    aarch64::Register baseReg = (dstStackSlot.IsFrameBase()) ? aarch64::Register(aarch64::FP) :
                                                               aarch64::Register(aarch64::SP);
    aarch64::MemoryOperand dstOpnd(baseReg, static_cast<int64_t>(dstStackSlot.GetOffset()));
    assembler.Mov(LOCAL_SCOPE_REGISTER, aarch64::Immediate(value.GetValue()));
    PickLoadStoreInsn(LOCAL_SCOPE_REGISTER, dstOpnd, false);
}

void MacroAssemblerAArch64::Move(const StackSlotOperand &dstStackSlot,
                                 const StackSlotOperand &srcStackSlot)
{
    aarch64::Register dstBaseReg = (dstStackSlot.IsFrameBase()) ? aarch64::Register(aarch64::FP) :
                                                                  aarch64::Register(aarch64::SP);
    aarch64::MemoryOperand dstOpnd(dstBaseReg, static_cast<int64_t>(dstStackSlot.GetOffset()));
    aarch64::Register srcBaseReg = (srcStackSlot.IsFrameBase()) ? aarch64::Register(aarch64::FP) :
                                                                  aarch64::Register(aarch64::SP);
    aarch64::MemoryOperand srcOpnd(srcBaseReg, static_cast<int64_t>(srcStackSlot.GetOffset()));
    PickLoadStoreInsn(LOCAL_SCOPE_REGISTER, srcOpnd);
    PickLoadStoreInsn(LOCAL_SCOPE_REGISTER, dstOpnd, false);
}

void MacroAssemblerAArch64::Cmp(const StackSlotOperand &stackSlot, Immediate value)
{
    aarch64::Register baseReg = (stackSlot.IsFrameBase()) ? aarch64::Register(aarch64::FP) :
                                                            aarch64::Register(aarch64::SP);
    aarch64::MemoryOperand opnd(baseReg, static_cast<int64_t>(stackSlot.GetOffset()));
    aarch64::Operand immOpnd = aarch64::Immediate(value.GetValue());
    PickLoadStoreInsn(LOCAL_SCOPE_REGISTER, opnd);
    assembler.Cmp(LOCAL_SCOPE_REGISTER, immOpnd);
}

void MacroAssemblerAArch64::Bind(JumpLabel &label)
{
    assembler.Bind(&label);
}

void MacroAssemblerAArch64::Jz(JumpLabel &label)
{
    assembler.B(aarch64::EQ, &label);
}

void MacroAssemblerAArch64::Jnz(JumpLabel &label)
{
    assembler.B(aarch64::NE, &label);
}

void MacroAssemblerAArch64::Jump(JumpLabel &label)
{
    assembler.B(&label);
}

void MacroAssemblerAArch64::CallBuiltin(Address funcAddress,
                                        const std::vector<MacroParameter> &parameters)
{
    for (size_t i = 0; i < parameters.size(); ++i) {
        auto param = parameters[i];
        if (i == PARAM_REGISTER_COUNT) {
            std::cout << "not support aarch64 baseline stack parameter " << std::endl;
            std::abort();
            break;
        }
        MovParameterIntoParamReg(param, registerParamVec[i]);
    }
    size_t startPc = assembler.GetCurPos();
    assembler.Mov(LOCAL_SCOPE_REGISTER, aarch64::Immediate(funcAddress));
    assembler.Blr(LOCAL_SCOPE_REGISTER);
    size_t endPc = assembler.GetCurPos() - INSTRUCT_SIZE;
    assembler.RecordRelocInfo(startPc, endPc, funcAddress);
}

void MacroAssemblerAArch64::SaveReturnRegister(const StackSlotOperand &dstStackSlot)
{
    aarch64::Register baseReg = (dstStackSlot.IsFrameBase()) ? aarch64::Register(aarch64::FP) :
                                                               aarch64::Register(aarch64::SP);
    aarch64::MemoryOperand dstOpnd(baseReg, static_cast<int64_t>(dstStackSlot.GetOffset()));
    PickLoadStoreInsn(RETURN_REGISTER, dstOpnd, false);
}

void MacroAssemblerAArch64::MovParameterIntoParamReg(MacroParameter param, aarch64::Register paramReg)
{
    if (std::holds_alternative<BaselineSpecialParameter>(param)) {
        auto specialParam = std::get<BaselineSpecialParameter>(param);
        switch (specialParam) {
            case BaselineSpecialParameter::GLUE: {
                assembler.Mov(paramReg, GLUE_REGISTER);
                break;
            }
            case BaselineSpecialParameter::PROFILE_TYPE_INFO: {
                assembler.Ldur(LOCAL_SCOPE_REGISTER,
                               aarch64::MemoryOperand(aarch64::Register(aarch64::X29),
                                                      static_cast<int64_t>(FUNCTION_OFFSET_FROM_SP)));
                assembler.Ldr(LOCAL_SCOPE_REGISTER,
                              aarch64::MemoryOperand(LOCAL_SCOPE_REGISTER, JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET));
                assembler.Ldr(paramReg,
                              aarch64::MemoryOperand(LOCAL_SCOPE_REGISTER, ProfileTypeInfoCell::VALUE_OFFSET));
                break;
            }
            case BaselineSpecialParameter::SP: {
                assembler.Mov(paramReg, aarch64::Register(aarch64::X29));
                break;
            }
            case BaselineSpecialParameter::HOTNESS_COUNTER: {
                assembler.Ldur(LOCAL_SCOPE_REGISTER, aarch64::MemoryOperand(aarch64::Register(aarch64::X29),
                    static_cast<int64_t>(FUNCTION_OFFSET_FROM_SP)));
                assembler.Ldr(LOCAL_SCOPE_REGISTER,
                              aarch64::MemoryOperand(LOCAL_SCOPE_REGISTER, JSFunctionBase::METHOD_OFFSET));
                assembler.Ldr(paramReg,
                              aarch64::MemoryOperand(LOCAL_SCOPE_REGISTER, Method::LITERAL_INFO_OFFSET));
                break;
            }
            default: {
                std::cout << "not supported other BaselineSpecialParameter currently" << std::endl;
                std::abort();
            }
        }
        return;
    }
    if (std::holds_alternative<int8_t>(param)) {
        int16_t num = std::get<int8_t>(param);
        assembler.Mov(paramReg, aarch64::Immediate(static_cast<int64_t>(num)));
        return;
    }
    if (std::holds_alternative<int16_t>(param)) {
        int16_t num = std::get<int16_t>(param);
        assembler.Mov(paramReg, aarch64::Immediate(static_cast<int64_t>(num)));
        return;
    }
    if (std::holds_alternative<int32_t>(param)) {
        int32_t num = std::get<int32_t>(param);
        assembler.Mov(paramReg, aarch64::Immediate(static_cast<int64_t>(num)));
        return;
    }
    if (std::holds_alternative<int64_t>(param)) {
        int64_t num = std::get<int64_t>(param);
        CopyImm(paramReg, num, k64BitSize);
        return;
    }
    if (std::holds_alternative<StackSlotOperand>(param)) {
        StackSlotOperand stackSlotOpnd = std::get<StackSlotOperand>(param);
        aarch64::Register dstBaseReg = (stackSlotOpnd.IsFrameBase()) ? aarch64::Register(aarch64::FP) :
                                                                       aarch64::Register(aarch64::SP);
        aarch64::MemoryOperand paramOpnd(dstBaseReg, static_cast<int64_t>(stackSlotOpnd.GetOffset()));
        PickLoadStoreInsn(paramReg, paramOpnd);
        return;
    }
    std::cout << "not supported other type of aarch64 baseline parameters currently" << std::endl;
    std::abort();
}

void MacroAssemblerAArch64::PickLoadStoreInsn(aarch64::Register reg, aarch64::MemoryOperand memOpnd, bool isLoad)
{
    int64_t maxNineBitsSignedValue = 255;
    int64_t minNineBitsSignedValue = -256;
    int64_t value = memOpnd.GetImmediate().Value();
    if (value < minNineBitsSignedValue) {
        std::cout << "not support aarch64 offset in memory operand is less than -256!" << std::endl;
        std::abort();
    }
    if (value > maxNineBitsSignedValue && isLoad) {
        if (value % 8 != 0) { // 8: offset in memory operand must be a multiple of 8 for ldr insn
            std::cout << "not support offset in memory operand is not a multiple of 8 for ldr insn!" << std::endl;
            std::abort();
        }
        assembler.Ldr(reg, memOpnd);
    }
    if (value > maxNineBitsSignedValue && !isLoad) {
        if (value % 8 != 0) { // 8: offset in memory operand must be a multiple of 8 for str insn
            std::cout << "not support offset in memory operand is not a multiple of 8 for str insn!" << std::endl;
            std::abort();
        }
        assembler.Str(reg, memOpnd);
    }
    if (value <= maxNineBitsSignedValue && isLoad) {
        assembler.Ldur(reg, memOpnd);
    }
    if (value <= maxNineBitsSignedValue && !isLoad) {
        assembler.Stur(reg, memOpnd);
    }
}

bool MacroAssemblerAArch64::IsMoveWidableImmediate(uint64_t val, uint32_t bitLen)
{
    if (bitLen == k64BitSize) {
        /* 0xHHHH000000000000 or 0x0000HHHH00000000, return true */
        if (((val & ((static_cast<uint64_t>(0xffff)) << k48BitSize)) == val) ||
            ((val & ((static_cast<uint64_t>(0xffff)) << k32BitSize)) == val)) {
            return true;
        }
    } else {
        /* get lower 32 bits */
        val &= static_cast<uint64_t>(0xffffffff);
    }
    /* 0x00000000HHHH0000 or 0x000000000000HHHH, return true */
    return ((val & ((static_cast<uint64_t>(0xffff)) << k16BitSize)) == val ||
            (val & ((static_cast<uint64_t>(0xffff)) << 0)) == val);
}

bool MacroAssemblerAArch64::IsBitmaskImmediate(uint64_t val, uint32_t bitLen)
{
    if (static_cast<int64_t>(val) == -1 || val == 0) {
        std::cout << "IsBitmaskImmediate() don't accept 0 or -1!" << std::endl;
        std::abort();
    }
    if ((bitLen == k32BitSize) && (static_cast<int32_t>(val) == -1)) {
        return false;
    }
    uint64_t val2 = val;
    if (bitLen == k32BitSize) {
        val2 = (val2 << k32BitSize) | (val2 & ((1ULL << k32BitSize) - 1));
    }
    bool expectedOutcome = (ValidBitmaskImmSet.find(val2) != ValidBitmaskImmSet.end());

    if ((val & 0x1) != 0) {
        /*
         * we want to work with
         * 0000000000000000000000000000000000000000000001100000000000000000
         * instead of
         * 1111111111111111111111111111111111111111111110011111111111111111
         */
        val = ~val;
    }

    if (bitLen == k32BitSize) {
        val = (val << k32BitSize) | (val & ((1ULL << k32BitSize) - 1));
    }

    /* get the least significant bit set and add it to 'val' */
    uint64_t tmpVal = val + (val & static_cast<uint64_t>(UINT64_MAX - val + 1));

    /* now check if tmp is a power of 2 or tmpVal==0. */
    if (tmpVal < 1 || tmpVal > UINT64_MAX) {
        std::cout << "tmpVal value overflow!" << std::endl;
        std::abort();
    }
    tmpVal = tmpVal & (tmpVal - 1);
    if (tmpVal == 0) {
        if (!expectedOutcome) {
            return false;
        }
        /* power of two or zero ; return true */
        return true;
    }

    int32_t p0 = __builtin_ctzll(val);
    int32_t p1 = __builtin_ctzll(tmpVal);
    int64_t diff = p1 - p0;

    /* check if diff is a power of two; return false if not. */
    if (static_cast<uint64_t>(diff) < 1 || static_cast<uint64_t>(diff) > UINT64_MAX) {
        std::cout << "diff value overflow!" << std::endl;
        std::abort();
    }
    if ((static_cast<uint64_t>(diff) & (static_cast<uint64_t>(diff) - 1)) != 0) {
        return false;
    }

    uint32_t logDiff = static_cast<uint32_t>(__builtin_ctzll(static_cast<uint64_t>(diff)));
    uint64_t pattern = val & ((1ULL << static_cast<uint64_t>(diff)) - 1);
    return val == pattern * kBitmaskImmMultTable[kMaxBitTableSize - logDiff];
}

bool MacroAssemblerAArch64::IsSingleInstructionMovable(uint64_t val, uint32_t size)
{
    return (IsMoveWidableImmediate(val, size) ||
           IsMoveWidableImmediate(~val, size) || IsBitmaskImmediate(val, size));
}

bool MacroAssemblerAArch64::BetterUseMOVZ(uint64_t val)
{
    int32_t n16zerosChunks = 0;
    int32_t n16onesChunks = 0;
    uint64_t sa = 0;
    /* a 64 bits number is split 4 chunks, each chunk has 16 bits. check each chunk whether is all 1 or is all 0 */
    for (uint64_t i = 0; i < k4BitSize; ++i, sa += k16BitSize) {
        uint64_t chunkVal = (val >> (static_cast<uint64_t>(sa))) & 0x0000FFFFUL;
        if (chunkVal == 0) {
            ++n16zerosChunks;
        } else if (chunkVal == 0xFFFFUL) {
            ++n16onesChunks;
        }
    }
    return (n16zerosChunks >= n16onesChunks);
}

void MacroAssemblerAArch64::CopyImm(aarch64::Register destReg, int64_t imm, uint32_t size)
{
    uint64_t srcVal = static_cast<uint64_t>(imm);

    if (IsSingleInstructionMovable(srcVal, size)) {
        assembler.Mov(destReg, aarch64::Immediate(imm));
        return;
    }

    if (size != k32BitSize && size != k64BitSize) {
        std::cout << "only support 32 and 64 bits size!" << std::endl;
        std::abort();
    }

    if (size == k32BitSize) {
        /* check lower 16 bits and higher 16 bits respectively */
        if ((srcVal & 0x0000FFFFULL) == 0 || (srcVal & 0x0000FFFFULL) == 0xFFFFULL) {
            std::cout << "unexpected val!" << std::endl;
            std::abort();
        }
        if (((srcVal >> k16BitSize) & 0x0000FFFFULL) == 0 || ((srcVal >> k16BitSize) & 0x0000FFFFULL) == 0xFFFFULL) {
            std::cout << "unexpected val" << std::endl;
            std::abort();
        }
        /* create an imm opereand which represents lower 16 bits of the immediate */
        int64_t srcLower = static_cast<int64_t>(srcVal & 0x0000FFFFULL);
        assembler.Mov(destReg, aarch64::Immediate(srcLower));
        /* create an imm opereand which represents upper 16 bits of the immediate */
        int64_t srcUpper = static_cast<int64_t>((srcVal >> k16BitSize) & 0x0000FFFFULL);
        assembler.Movk(destReg, srcUpper, k16BitSize);
    } else {
        CopyImmSize64(destReg, srcVal);
    }
}

void MacroAssemblerAArch64::CopyImmSize64(aarch64::Register destReg, uint64_t srcVal)
{
    /*
     * partition it into 4 16-bit chunks
     * if more 0's than 0xFFFF's, use movz as the initial instruction.
     * otherwise, movn.
     */
    bool useMovz = BetterUseMOVZ(srcVal);
    bool useMovk = false;
    /* get lower 32 bits of the immediate */
    uint64_t chunkLval = srcVal & 0xFFFFFFFFULL;
    /* get upper 32 bits of the immediate */
    uint64_t chunkHval = (srcVal >> k32BitSize) & 0xFFFFFFFFULL;
    int32_t maxLoopTime = 4;

    if (chunkLval == chunkHval) {
        /* compute lower 32 bits, and then copy to higher 32 bits, so only 2 chunks need be processed */
        maxLoopTime = 2;
    }

    uint64_t sa = 0;
    for (int64_t i = 0; i < maxLoopTime; ++i, sa += k16BitSize) {
        /* create an imm opereand which represents the i-th 16-bit chunk of the immediate */
        uint64_t chunkVal = (srcVal >> (static_cast<uint64_t>(sa))) & 0x0000FFFFULL;
        if (useMovz ? (chunkVal == 0) : (chunkVal == 0x0000FFFFULL)) {
            continue;
        }
        int64_t src16 = static_cast<int64_t>(chunkVal);
        if (!useMovk) {
            /* use movz or movn */
            if (!useMovz) {
                src16 = ~(static_cast<uint64_t>(src16)) & ((1ULL << k16BitSize) - 1UL);
                assembler.Movn(destReg, src16, sa);
            } else {
                assembler.Movz(destReg, src16, sa);
            }
            useMovk = true;
        } else {
            assembler.Movk(destReg, src16, sa);
        }
    }

    if (maxLoopTime == 2) { /* as described above, only 2 chunks need be processed */
        /* copy lower 32 bits to higher 32 bits */
        uint32_t immr = -k32BitSize % k64BitSize; // immr = -shift % size
        uint32_t imms = k32BitSize - 1; // imms = width - 1
        assembler.Bfm(destReg, destReg, immr, imms);
    }
}

}  // namespace panda::ecmascript::kungfu
