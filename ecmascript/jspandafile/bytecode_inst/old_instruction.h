/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_JSPANDAFILE_BYTECODE_INST_OLD_INSTRUCTION_H
#define ECMASCRIPT_JSPANDAFILE_BYTECODE_INST_OLD_INSTRUCTION_H

#include "ecmascript/jspandafile/bytecode_inst/instruction.h"

namespace panda::ecmascript {
class OldBytecodeInst : OldBytecodeInstBase {
public:
#include "ecmascript/jspandafile/bytecode_inst/old_instruction_enum.h"

    explicit OldBytecodeInst(const uint8_t *pc) : OldBytecodeInstBase {pc}
    {
    }

    inline typename OldBytecodeInst::Opcode GetOpcode() const
    {
        uint8_t primary = ReadByte(0);
        if (primary >= 236) {  // 236: second level bytecode index
            uint8_t secondary = ReadByte(1);
            return static_cast<Opcode>((secondary << 8U) | primary);
        }
        return static_cast<Opcode>(primary);
    }

    OldBytecodeInst JumpTo(int32_t offset) const
    {
        return OldBytecodeInst(OldBytecodeInstBase::GetPointer(offset));
    }

    OldBytecodeInst GetNext() const
    {
        return JumpTo(GetSize());
    }

    const uint8_t *GetAddress() const
    {
        return OldBytecodeInstBase::GetAddress();
    }

    size_t GetSize() const
    {
        return Size(GetFormat(GetOpcode()));
    }

    uint32_t GetId() const
    {
        Format format = GetFormat(GetOpcode());
        if (format == Format::ID16) {
            return static_cast<uint32_t>(Read<8, 16>());
        }
        if (format == Format::ID32) {
            return static_cast<uint32_t>(Read<8, 32>());
        }
        if (format == Format::PREF_ID16_IMM16_IMM16_V8_V8) {
            return static_cast<uint32_t>(Read<16, 16>());
        }
        if (format == Format::PREF_ID16_IMM16_V8) {
            return static_cast<uint32_t>(Read<16, 16>());
        }
        if (format == Format::PREF_ID32) {
            return static_cast<uint32_t>(Read<16, 32>());
        }
        if (format == Format::PREF_ID32_IMM8) {
            return static_cast<uint32_t>(Read<16, 32>());
        }
        if (format == Format::PREF_ID32_V8) {
            return static_cast<uint32_t>(Read<16, 32>());
        }
        if (format == Format::V4_IMM4_ID16) {
            return static_cast<uint32_t>(Read<16, 16>());
        }
        if (format == Format::V4_V4_ID16) {
            return static_cast<uint32_t>(Read<16, 16>());
        }
        if (format == Format::V4_V4_V4_IMM4_ID16) {
            return static_cast<uint32_t>(Read<24, 16>());
        }
        if (format == Format::V4_V4_V4_V4_ID16) {
            return static_cast<uint32_t>(Read<24, 16>());
        }
        if (format == Format::V8_ID16) {
            return static_cast<uint32_t>(Read<16, 16>());
        }
        if (format == Format::V8_ID32) {
            return static_cast<uint32_t>(Read<16, 32>());
        }
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }

    template <Format format, size_t idx = 0>
    auto GetImm() const
    {
        if constexpr (format == Format::IMM16) {
            constexpr std::array<uint8_t, 1> OFFSETS{8};
            constexpr std::array<uint8_t, 1> WIDTHS{16};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::IMM16_V16) {
            constexpr std::array<uint8_t, 1> OFFSETS{8};
            constexpr std::array<uint8_t, 1> WIDTHS{16};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::IMM32) {
            constexpr std::array<uint8_t, 1> OFFSETS{8};
            constexpr std::array<uint8_t, 1> WIDTHS{32};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::IMM4_V4_V4_V4) {
            constexpr std::array<uint8_t, 1> OFFSETS{8};
            constexpr std::array<uint8_t, 1> WIDTHS{4};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::IMM4_V4_V4_V4_V4_V4) {
            constexpr std::array<uint8_t, 1> OFFSETS{8};
            constexpr std::array<uint8_t, 1> WIDTHS{4};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::IMM64) {
            constexpr std::array<uint8_t, 1> OFFSETS{8};
            constexpr std::array<uint8_t, 1> WIDTHS{64};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::IMM8) {
            constexpr std::array<uint8_t, 1> OFFSETS{8};
            constexpr std::array<uint8_t, 1> WIDTHS{8};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::PREF_ID16_IMM16_IMM16_V8_V8) {
            constexpr std::array<uint8_t, 2> OFFSETS{32, 48};
            constexpr std::array<uint8_t, 2> WIDTHS{16, 16};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::PREF_ID16_IMM16_V8) {
            constexpr std::array<uint8_t, 1> OFFSETS{32};
            constexpr std::array<uint8_t, 1> WIDTHS{16};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::PREF_ID32_IMM8) {
            constexpr std::array<uint8_t, 1> OFFSETS{48};
            constexpr std::array<uint8_t, 1> WIDTHS{8};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::PREF_IMM16) {
            constexpr std::array<uint8_t, 1> OFFSETS{16};
            constexpr std::array<uint8_t, 1> WIDTHS{16};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::PREF_IMM16_IMM16) {
            constexpr std::array<uint8_t, 2> OFFSETS{16, 32};
            constexpr std::array<uint8_t, 2> WIDTHS{16, 16};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::PREF_IMM16_IMM16_V8) {
            constexpr std::array<uint8_t, 2> OFFSETS{16, 32};
            constexpr std::array<uint8_t, 2> WIDTHS{16, 16};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::PREF_IMM16_V8) {
            constexpr std::array<uint8_t, 1> OFFSETS{16};
            constexpr std::array<uint8_t, 1> WIDTHS{16};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::PREF_IMM16_V8_V8) {
            constexpr std::array<uint8_t, 1> OFFSETS{16};
            constexpr std::array<uint8_t, 1> WIDTHS{16};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::PREF_IMM32) {
            constexpr std::array<uint8_t, 1> OFFSETS{16};
            constexpr std::array<uint8_t, 1> WIDTHS{32};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::PREF_IMM4_IMM4) {
            constexpr std::array<uint8_t, 2> OFFSETS{16, 20};
            constexpr std::array<uint8_t, 2> WIDTHS{4, 4};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::PREF_IMM4_IMM4_V8) {
            constexpr std::array<uint8_t, 2> OFFSETS{16, 20};
            constexpr std::array<uint8_t, 2> WIDTHS{4, 4};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::PREF_IMM8_IMM8) {
            constexpr std::array<uint8_t, 2> OFFSETS{16, 24};
            constexpr std::array<uint8_t, 2> WIDTHS{8, 8};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::PREF_IMM8_IMM8_V8) {
            constexpr std::array<uint8_t, 2> OFFSETS{16, 24};
            constexpr std::array<uint8_t, 2> WIDTHS{8, 8};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::PREF_V8_IMM32) {
            constexpr std::array<uint8_t, 1> OFFSETS{24};
            constexpr std::array<uint8_t, 1> WIDTHS{32};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::V4_IMM4) {
            constexpr std::array<uint8_t, 1> OFFSETS{12};
            constexpr std::array<uint8_t, 1> WIDTHS{4};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::V4_IMM4_ID16) {
            constexpr std::array<uint8_t, 1> OFFSETS{12};
            constexpr std::array<uint8_t, 1> WIDTHS{4};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::V4_V4_V4_IMM4_ID16) {
            constexpr std::array<uint8_t, 1> OFFSETS{20};
            constexpr std::array<uint8_t, 1> WIDTHS{4};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::V8_IMM16) {
            constexpr std::array<uint8_t, 1> OFFSETS{16};
            constexpr std::array<uint8_t, 1> WIDTHS{16};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::V8_IMM32) {
            constexpr std::array<uint8_t, 1> OFFSETS{16};
            constexpr std::array<uint8_t, 1> WIDTHS{32};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::V8_IMM64) {
            constexpr std::array<uint8_t, 1> OFFSETS{16};
            constexpr std::array<uint8_t, 1> WIDTHS{64};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        if constexpr (format == Format::V8_IMM8) {
            constexpr std::array<uint8_t, 1> OFFSETS{16};
            constexpr std::array<uint8_t, 1> WIDTHS{8};
            return Read<OFFSETS[idx], WIDTHS[idx], true>();
        }
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }

    static constexpr size_t Size(Format format)
    {
        switch (format) {
            case Format::ID16: {
                constexpr size_t SIZE = 3;
                return SIZE;
            }
            case Format::ID32: {
                constexpr size_t SIZE = 5;
                return SIZE;
            }
            case Format::IMM16: {
                constexpr size_t SIZE = 3;
                return SIZE;
            }
            case Format::IMM16_V16: {
                constexpr size_t SIZE = 5;
                return SIZE;
            }
            case Format::IMM32: {
                constexpr size_t SIZE = 5;
                return SIZE;
            }
            case Format::IMM4_V4_V4_V4: {
                constexpr size_t SIZE = 3;
                return SIZE;
            }
            case Format::IMM4_V4_V4_V4_V4_V4: {
                constexpr size_t SIZE = 4;
                return SIZE;
            }
            case Format::IMM64: {
                constexpr size_t SIZE = 9;
                return SIZE;
            }
            case Format::IMM8: {
                constexpr size_t SIZE = 2;
                return SIZE;
            }
            case Format::NONE: {
                constexpr size_t SIZE = 1;
                return SIZE;
            }
            case Format::PREF_ID16_IMM16_IMM16_V8_V8: {
                constexpr size_t SIZE = 10;
                return SIZE;
            }
            case Format::PREF_ID16_IMM16_V8: {
                constexpr size_t SIZE = 7;
                return SIZE;
            }
            case Format::PREF_ID32: {
                constexpr size_t SIZE = 6;
                return SIZE;
            }
            case Format::PREF_ID32_IMM8: {
                constexpr size_t SIZE = 7;
                return SIZE;
            }
            case Format::PREF_ID32_V8: {
                constexpr size_t SIZE = 7;
                return SIZE;
            }
            case Format::PREF_IMM16: {
                constexpr size_t SIZE = 4;
                return SIZE;
            }
            case Format::PREF_IMM16_IMM16: {
                constexpr size_t SIZE = 6;
                return SIZE;
            }
            case Format::PREF_IMM16_IMM16_V8: {
                constexpr size_t SIZE = 7;
                return SIZE;
            }
            case Format::PREF_IMM16_V8: {
                constexpr size_t SIZE = 5;
                return SIZE;
            }
            case Format::PREF_IMM16_V8_V8: {
                constexpr size_t SIZE = 6;
                return SIZE;
            }
            case Format::PREF_IMM32: {
                constexpr size_t SIZE = 6;
                return SIZE;
            }
            case Format::PREF_IMM4_IMM4: {
                constexpr size_t SIZE = 3;
                return SIZE;
            }
            case Format::PREF_IMM4_IMM4_V8: {
                constexpr size_t SIZE = 4;
                return SIZE;
            }
            case Format::PREF_IMM8_IMM8: {
                constexpr size_t SIZE = 4;
                return SIZE;
            }
            case Format::PREF_IMM8_IMM8_V8: {
                constexpr size_t SIZE = 5;
                return SIZE;
            }
            case Format::PREF_NONE: {
                constexpr size_t SIZE = 2;
                return SIZE;
            }
            case Format::PREF_V4_V4: {
                constexpr size_t SIZE = 3;
                return SIZE;
            }
            case Format::PREF_V8: {
                constexpr size_t SIZE = 3;
                return SIZE;
            }
            case Format::PREF_V8_IMM32: {
                constexpr size_t SIZE = 7;
                return SIZE;
            }
            case Format::PREF_V8_V8: {
                constexpr size_t SIZE = 4;
                return SIZE;
            }
            case Format::PREF_V8_V8_V8: {
                constexpr size_t SIZE = 5;
                return SIZE;
            }
            case Format::PREF_V8_V8_V8_V8: {
                constexpr size_t SIZE = 6;
                return SIZE;
            }
            case Format::V16_V16: {
                constexpr size_t SIZE = 5;
                return SIZE;
            }
            case Format::V4_IMM4: {
                constexpr size_t SIZE = 2;
                return SIZE;
            }
            case Format::V4_IMM4_ID16: {
                constexpr size_t SIZE = 4;
                return SIZE;
            }
            case Format::V4_V4: {
                constexpr size_t SIZE = 2;
                return SIZE;
            }
            case Format::V4_V4_ID16: {
                constexpr size_t SIZE = 4;
                return SIZE;
            }
            case Format::V4_V4_V4_IMM4_ID16: {
                constexpr size_t SIZE = 5;
                return SIZE;
            }
            case Format::V4_V4_V4_V4_ID16: {
                constexpr size_t SIZE = 5;
                return SIZE;
            }
            case Format::V8: {
                constexpr size_t SIZE = 2;
                return SIZE;
            }
            case Format::V8_ID16: {
                constexpr size_t SIZE = 4;
                return SIZE;
            }
            case Format::V8_ID32: {
                constexpr size_t SIZE = 6;
                return SIZE;
            }
            case Format::V8_IMM16: {
                constexpr size_t SIZE = 4;
                return SIZE;
            }
            case Format::V8_IMM32: {
                constexpr size_t SIZE = 6;
                return SIZE;
            }
            case Format::V8_IMM64: {
                constexpr size_t SIZE = 10;
                return SIZE;
            }
            case Format::V8_IMM8: {
                constexpr size_t SIZE = 3;
                return SIZE;
            }
            case Format::V8_V8: {
                constexpr size_t SIZE = 3;
                return SIZE;
            }
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
    }

    static constexpr Format GetFormat(Opcode opcode)
    {
        switch (opcode) {
            case Opcode::NOP:
                return Format::NONE;
            case Opcode::MOV_V4_V4:
                return Format::V4_V4;
            case Opcode::MOV_V8_V8:
                return Format::V8_V8;
            case Opcode::MOV_V16_V16:
                return Format::V16_V16;
            case Opcode::MOV_64_V4_V4:
                return Format::V4_V4;
            case Opcode::MOV_64_V16_V16:
                return Format::V16_V16;
            case Opcode::MOV_OBJ_V4_V4:
                return Format::V4_V4;
            case Opcode::MOV_OBJ_V8_V8:
                return Format::V8_V8;
            case Opcode::MOV_OBJ_V16_V16:
                return Format::V16_V16;
            case Opcode::MOVI_V4_IMM4:
                return Format::V4_IMM4;
            case Opcode::MOVI_V8_IMM8:
                return Format::V8_IMM8;
            case Opcode::MOVI_V8_IMM16:
                return Format::V8_IMM16;
            case Opcode::MOVI_V8_IMM32:
                return Format::V8_IMM32;
            case Opcode::MOVI_64_V8_IMM64:
                return Format::V8_IMM64;
            case Opcode::FMOVI_64_V8_IMM64:
                return Format::V8_IMM64;
            case Opcode::MOV_NULL_V8:
                return Format::V8;
            case Opcode::LDA_V8:
                return Format::V8;
            case Opcode::LDA_64_V8:
                return Format::V8;
            case Opcode::LDA_OBJ_V8:
                return Format::V8;
            case Opcode::LDAI_IMM8:
                return Format::IMM8;
            case Opcode::LDAI_IMM16:
                return Format::IMM16;
            case Opcode::LDAI_IMM32:
                return Format::IMM32;
            case Opcode::LDAI_64_IMM64:
                return Format::IMM64;
            case Opcode::FLDAI_64_IMM64:
                return Format::IMM64;
            case Opcode::LDA_STR_ID32:
                return Format::ID32;
            case Opcode::LDA_CONST_V8_ID32:
                return Format::V8_ID32;
            case Opcode::LDA_TYPE_ID16:
                return Format::ID16;
            case Opcode::LDA_NULL:
                return Format::NONE;
            case Opcode::STA_V8:
                return Format::V8;
            case Opcode::STA_64_V8:
                return Format::V8;
            case Opcode::STA_OBJ_V8:
                return Format::V8;
            case Opcode::CMP_64_V8:
                return Format::V8;
            case Opcode::FCMPL_64_V8:
                return Format::V8;
            case Opcode::FCMPG_64_V8:
                return Format::V8;
            case Opcode::JMP_IMM8:
                return Format::IMM8;
            case Opcode::JMP_IMM16:
                return Format::IMM16;
            case Opcode::JMP_IMM32:
                return Format::IMM32;
            case Opcode::JEQ_OBJ_V8_IMM8:
                return Format::V8_IMM8;
            case Opcode::JEQ_OBJ_V8_IMM16:
                return Format::V8_IMM16;
            case Opcode::JNE_OBJ_V8_IMM8:
                return Format::V8_IMM8;
            case Opcode::JNE_OBJ_V8_IMM16:
                return Format::V8_IMM16;
            case Opcode::JEQZ_OBJ_IMM8:
                return Format::IMM8;
            case Opcode::JEQZ_OBJ_IMM16:
                return Format::IMM16;
            case Opcode::JNEZ_OBJ_IMM8:
                return Format::IMM8;
            case Opcode::JNEZ_OBJ_IMM16:
                return Format::IMM16;
            case Opcode::JEQZ_IMM8:
                return Format::IMM8;
            case Opcode::JEQZ_IMM16:
                return Format::IMM16;
            case Opcode::JNEZ_IMM8:
                return Format::IMM8;
            case Opcode::JNEZ_IMM16:
                return Format::IMM16;
            case Opcode::JLTZ_IMM8:
                return Format::IMM8;
            case Opcode::JLTZ_IMM16:
                return Format::IMM16;
            case Opcode::JGTZ_IMM8:
                return Format::IMM8;
            case Opcode::JGTZ_IMM16:
                return Format::IMM16;
            case Opcode::JLEZ_IMM8:
                return Format::IMM8;
            case Opcode::JLEZ_IMM16:
                return Format::IMM16;
            case Opcode::JGEZ_IMM8:
                return Format::IMM8;
            case Opcode::JGEZ_IMM16:
                return Format::IMM16;
            case Opcode::JEQ_V8_IMM8:
                return Format::V8_IMM8;
            case Opcode::JEQ_V8_IMM16:
                return Format::V8_IMM16;
            case Opcode::JNE_V8_IMM8:
                return Format::V8_IMM8;
            case Opcode::JNE_V8_IMM16:
                return Format::V8_IMM16;
            case Opcode::JLT_V8_IMM8:
                return Format::V8_IMM8;
            case Opcode::JLT_V8_IMM16:
                return Format::V8_IMM16;
            case Opcode::JGT_V8_IMM8:
                return Format::V8_IMM8;
            case Opcode::JGT_V8_IMM16:
                return Format::V8_IMM16;
            case Opcode::JLE_V8_IMM8:
                return Format::V8_IMM8;
            case Opcode::JLE_V8_IMM16:
                return Format::V8_IMM16;
            case Opcode::JGE_V8_IMM8:
                return Format::V8_IMM8;
            case Opcode::JGE_V8_IMM16:
                return Format::V8_IMM16;
            case Opcode::FNEG_64:
                return Format::NONE;
            case Opcode::NEG:
                return Format::NONE;
            case Opcode::NEG_64:
                return Format::NONE;
            case Opcode::ADD2_V8:
                return Format::V8;
            case Opcode::ADD2_64_V8:
                return Format::V8;
            case Opcode::SUB2_V8:
                return Format::V8;
            case Opcode::SUB2_64_V8:
                return Format::V8;
            case Opcode::MUL2_V8:
                return Format::V8;
            case Opcode::MUL2_64_V8:
                return Format::V8;
            case Opcode::FADD2_64_V8:
                return Format::V8;
            case Opcode::FSUB2_64_V8:
                return Format::V8;
            case Opcode::FMUL2_64_V8:
                return Format::V8;
            case Opcode::FDIV2_64_V8:
                return Format::V8;
            case Opcode::FMOD2_64_V8:
                return Format::V8;
            case Opcode::DIV2_V8:
                return Format::V8;
            case Opcode::DIV2_64_V8:
                return Format::V8;
            case Opcode::MOD2_V8:
                return Format::V8;
            case Opcode::MOD2_64_V8:
                return Format::V8;
            case Opcode::ADDI_IMM8:
                return Format::IMM8;
            case Opcode::SUBI_IMM8:
                return Format::IMM8;
            case Opcode::MULI_IMM8:
                return Format::IMM8;
            case Opcode::ANDI_IMM32:
                return Format::IMM32;
            case Opcode::ORI_IMM32:
                return Format::IMM32;
            case Opcode::SHLI_IMM8:
                return Format::IMM8;
            case Opcode::SHRI_IMM8:
                return Format::IMM8;
            case Opcode::ASHRI_IMM8:
                return Format::IMM8;
            case Opcode::DIVI_IMM8:
                return Format::IMM8;
            case Opcode::MODI_IMM8:
                return Format::IMM8;
            case Opcode::ADD_V4_V4:
                return Format::V4_V4;
            case Opcode::SUB_V4_V4:
                return Format::V4_V4;
            case Opcode::MUL_V4_V4:
                return Format::V4_V4;
            case Opcode::DIV_V4_V4:
                return Format::V4_V4;
            case Opcode::MOD_V4_V4:
                return Format::V4_V4;
            case Opcode::INCI_V4_IMM4:
                return Format::V4_IMM4;
            case Opcode::LDARR_8_V8:
                return Format::V8;
            case Opcode::LDARRU_8_V8:
                return Format::V8;
            case Opcode::LDARR_16_V8:
                return Format::V8;
            case Opcode::LDARRU_16_V8:
                return Format::V8;
            case Opcode::LDARR_V8:
                return Format::V8;
            case Opcode::LDARR_64_V8:
                return Format::V8;
            case Opcode::FLDARR_32_V8:
                return Format::V8;
            case Opcode::FLDARR_64_V8:
                return Format::V8;
            case Opcode::LDARR_OBJ_V8:
                return Format::V8;
            case Opcode::STARR_8_V4_V4:
                return Format::V4_V4;
            case Opcode::STARR_16_V4_V4:
                return Format::V4_V4;
            case Opcode::STARR_V4_V4:
                return Format::V4_V4;
            case Opcode::STARR_64_V4_V4:
                return Format::V4_V4;
            case Opcode::FSTARR_32_V4_V4:
                return Format::V4_V4;
            case Opcode::FSTARR_64_V4_V4:
                return Format::V4_V4;
            case Opcode::STARR_OBJ_V4_V4:
                return Format::V4_V4;
            case Opcode::LENARR_V8:
                return Format::V8;
            case Opcode::NEWARR_V4_V4_ID16:
                return Format::V4_V4_ID16;
            case Opcode::NEWOBJ_V8_ID16:
                return Format::V8_ID16;
            case Opcode::INITOBJ_SHORT_V4_V4_ID16:
                return Format::V4_V4_ID16;
            case Opcode::INITOBJ_V4_V4_V4_V4_ID16:
                return Format::V4_V4_V4_V4_ID16;
            case Opcode::INITOBJ_RANGE_V8_ID16:
                return Format::V8_ID16;
            case Opcode::LDOBJ_V8_ID16:
                return Format::V8_ID16;
            case Opcode::LDOBJ_64_V8_ID16:
                return Format::V8_ID16;
            case Opcode::LDOBJ_OBJ_V8_ID16:
                return Format::V8_ID16;
            case Opcode::STOBJ_V8_ID16:
                return Format::V8_ID16;
            case Opcode::STOBJ_64_V8_ID16:
                return Format::V8_ID16;
            case Opcode::STOBJ_OBJ_V8_ID16:
                return Format::V8_ID16;
            case Opcode::LDOBJ_V_V4_V4_ID16:
                return Format::V4_V4_ID16;
            case Opcode::LDOBJ_V_64_V4_V4_ID16:
                return Format::V4_V4_ID16;
            case Opcode::LDOBJ_V_OBJ_V4_V4_ID16:
                return Format::V4_V4_ID16;
            case Opcode::STOBJ_V_V4_V4_ID16:
                return Format::V4_V4_ID16;
            case Opcode::STOBJ_V_64_V4_V4_ID16:
                return Format::V4_V4_ID16;
            case Opcode::STOBJ_V_OBJ_V4_V4_ID16:
                return Format::V4_V4_ID16;
            case Opcode::LDSTATIC_ID16:
                return Format::ID16;
            case Opcode::LDSTATIC_64_ID16:
                return Format::ID16;
            case Opcode::LDSTATIC_OBJ_ID16:
                return Format::ID16;
            case Opcode::STSTATIC_ID16:
                return Format::ID16;
            case Opcode::STSTATIC_64_ID16:
                return Format::ID16;
            case Opcode::STSTATIC_OBJ_ID16:
                return Format::ID16;
            case Opcode::RETURN:
                return Format::NONE;
            case Opcode::RETURN_64:
                return Format::NONE;
            case Opcode::RETURN_OBJ:
                return Format::NONE;
            case Opcode::RETURN_VOID:
                return Format::NONE;
            case Opcode::THROW_V8:
                return Format::V8;
            case Opcode::CHECKCAST_ID16:
                return Format::ID16;
            case Opcode::ISINSTANCE_ID16:
                return Format::ID16;
            case Opcode::CALL_SHORT_V4_V4_ID16:
                return Format::V4_V4_ID16;
            case Opcode::CALL_V4_V4_V4_V4_ID16:
                return Format::V4_V4_V4_V4_ID16;
            case Opcode::CALL_RANGE_V8_ID16:
                return Format::V8_ID16;
            case Opcode::CALL_ACC_SHORT_V4_IMM4_ID16:
                return Format::V4_IMM4_ID16;
            case Opcode::CALL_ACC_V4_V4_V4_IMM4_ID16:
                return Format::V4_V4_V4_IMM4_ID16;
            case Opcode::CALL_VIRT_SHORT_V4_V4_ID16:
                return Format::V4_V4_ID16;
            case Opcode::CALL_VIRT_V4_V4_V4_V4_ID16:
                return Format::V4_V4_V4_V4_ID16;
            case Opcode::CALL_VIRT_RANGE_V8_ID16:
                return Format::V8_ID16;
            case Opcode::CALL_VIRT_ACC_SHORT_V4_IMM4_ID16:
                return Format::V4_IMM4_ID16;
            case Opcode::CALL_VIRT_ACC_V4_V4_V4_IMM4_ID16:
                return Format::V4_V4_V4_IMM4_ID16;
            case Opcode::MOV_DYN_V8_V8:
                return Format::V8_V8;
            case Opcode::MOV_DYN_V16_V16:
                return Format::V16_V16;
            case Opcode::LDA_DYN_V8:
                return Format::V8;
            case Opcode::STA_DYN_V8:
                return Format::V8;
            case Opcode::LDAI_DYN_IMM32:
                return Format::IMM32;
            case Opcode::FLDAI_DYN_IMM64:
                return Format::IMM64;
            case Opcode::RETURN_DYN:
                return Format::NONE;
            case Opcode::CALLI_DYN_SHORT_IMM4_V4_V4_V4:
                return Format::IMM4_V4_V4_V4;
            case Opcode::CALLI_DYN_IMM4_V4_V4_V4_V4_V4:
                return Format::IMM4_V4_V4_V4_V4_V4;
            case Opcode::CALLI_DYN_RANGE_IMM16_V16:
                return Format::IMM16_V16;
            case Opcode::FMOVI_PREF_V8_IMM32:
                return Format::PREF_V8_IMM32;
            case Opcode::I32TOF64_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::UCMP_PREF_V8:
                return Format::PREF_V8;
            case Opcode::NOT_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::ECMA_LDNAN_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::FLDAI_PREF_IMM32:
                return Format::PREF_IMM32;
            case Opcode::U32TOF64_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::UCMP_64_PREF_V8:
                return Format::PREF_V8;
            case Opcode::NOT_64_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::ECMA_LDINFINITY_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::FCMPL_PREF_V8:
                return Format::PREF_V8;
            case Opcode::I64TOF64_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::DIVU2_PREF_V8:
                return Format::PREF_V8;
            case Opcode::AND2_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_LDGLOBALTHIS_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::FCMPG_PREF_V8:
                return Format::PREF_V8;
            case Opcode::U64TOF64_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::DIVU2_64_PREF_V8:
                return Format::PREF_V8;
            case Opcode::AND2_64_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_LDUNDEFINED_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::FNEG_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::F64TOI32_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::MODU2_PREF_V8:
                return Format::PREF_V8;
            case Opcode::OR2_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_LDNULL_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::FADD2_PREF_V8:
                return Format::PREF_V8;
            case Opcode::F64TOI64_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::MODU2_64_PREF_V8:
                return Format::PREF_V8;
            case Opcode::OR2_64_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_LDSYMBOL_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::FSUB2_PREF_V8:
                return Format::PREF_V8;
            case Opcode::F64TOU32_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::XOR2_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_LDGLOBAL_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::FMUL2_PREF_V8:
                return Format::PREF_V8;
            case Opcode::F64TOU64_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::XOR2_64_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_LDTRUE_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::FDIV2_PREF_V8:
                return Format::PREF_V8;
            case Opcode::I32TOU1_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::SHL2_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_LDFALSE_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::FMOD2_PREF_V8:
                return Format::PREF_V8;
            case Opcode::I64TOU1_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::SHL2_64_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_THROWDYN_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::I32TOF32_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::U32TOU1_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::SHR2_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_TYPEOFDYN_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::U32TOF32_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::U64TOU1_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::SHR2_64_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_LDLEXENVDYN_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::I64TOF32_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::I32TOI64_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::ASHR2_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_POPLEXENVDYN_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::U64TOF32_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::I32TOI16_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::ASHR2_64_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_GETUNMAPPEDARGS_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::F32TOF64_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::I32TOU16_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::XORI_PREF_IMM32:
                return Format::PREF_IMM32;
            case Opcode::ECMA_GETPROPITERATOR_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::F32TOI32_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::I32TOI8_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::AND_PREF_V4_V4:
                return Format::PREF_V4_V4;
            case Opcode::ECMA_ASYNCFUNCTIONENTER_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::F32TOI64_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::I32TOU8_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::OR_PREF_V4_V4:
                return Format::PREF_V4_V4;
            case Opcode::ECMA_LDHOLE_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::F32TOU32_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::I64TOI32_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::XOR_PREF_V4_V4:
                return Format::PREF_V4_V4;
            case Opcode::ECMA_RETURNUNDEFINED_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::F32TOU64_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::U32TOI64_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::SHL_PREF_V4_V4:
                return Format::PREF_V4_V4;
            case Opcode::ECMA_CREATEEMPTYOBJECT_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::F64TOF32_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::U32TOI16_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::SHR_PREF_V4_V4:
                return Format::PREF_V4_V4;
            case Opcode::ECMA_CREATEEMPTYARRAY_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::U32TOU16_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::ASHR_PREF_V4_V4:
                return Format::PREF_V4_V4;
            case Opcode::ECMA_GETITERATOR_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::U32TOI8_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::ECMA_THROWTHROWNOTEXISTS_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::U32TOU8_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::ECMA_THROWPATTERNNONCOERCIBLE_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::U64TOI32_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::ECMA_LDHOMEOBJECT_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::U64TOU32_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::ECMA_THROWDELETESUPERPROPERTY_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::ECMA_DEBUGGER_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::ECMA_ADD2DYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_SUB2DYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_MUL2DYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_DIV2DYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_MOD2DYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_EQDYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_NOTEQDYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_LESSDYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_LESSEQDYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_GREATERDYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_GREATEREQDYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_SHL2DYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_SHR2DYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_ASHR2DYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_AND2DYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_OR2DYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_XOR2DYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_TONUMBER_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_NEGDYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_NOTDYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_INCDYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_DECDYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_EXPDYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_ISINDYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_INSTANCEOFDYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_STRICTNOTEQDYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_STRICTEQDYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_RESUMEGENERATOR_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_GETRESUMEMODE_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_CREATEGENERATOROBJ_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_THROWCONSTASSIGNMENT_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_GETTEMPLATEOBJECT_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_GETNEXTPROPNAME_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_CALLARG0DYN_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_THROWIFNOTOBJECT_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_ITERNEXT_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_CLOSEITERATOR_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_COPYMODULE_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_SUPERCALLSPREAD_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_DELOBJPROP_PREF_V8_V8:
                return Format::PREF_V8_V8;
            case Opcode::ECMA_NEWOBJSPREADDYN_PREF_V8_V8:
                return Format::PREF_V8_V8;
            case Opcode::ECMA_CREATEITERRESULTOBJ_PREF_V8_V8:
                return Format::PREF_V8_V8;
            case Opcode::ECMA_SUSPENDGENERATOR_PREF_V8_V8:
                return Format::PREF_V8_V8;
            case Opcode::ECMA_ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8:
                return Format::PREF_V8_V8;
            case Opcode::ECMA_THROWUNDEFINEDIFHOLE_PREF_V8_V8:
                return Format::PREF_V8_V8;
            case Opcode::ECMA_CALLARG1DYN_PREF_V8_V8:
                return Format::PREF_V8_V8;
            case Opcode::ECMA_COPYDATAPROPERTIES_PREF_V8_V8:
                return Format::PREF_V8_V8;
            case Opcode::ECMA_STARRAYSPREAD_PREF_V8_V8:
                return Format::PREF_V8_V8;
            case Opcode::ECMA_GETITERATORNEXT_PREF_V8_V8:
                return Format::PREF_V8_V8;
            case Opcode::ECMA_SETOBJECTWITHPROTO_PREF_V8_V8:
                return Format::PREF_V8_V8;
            case Opcode::ECMA_LDOBJBYVALUE_PREF_V8_V8:
                return Format::PREF_V8_V8;
            case Opcode::ECMA_STOBJBYVALUE_PREF_V8_V8:
                return Format::PREF_V8_V8;
            case Opcode::ECMA_STOWNBYVALUE_PREF_V8_V8:
                return Format::PREF_V8_V8;
            case Opcode::ECMA_LDSUPERBYVALUE_PREF_V8_V8:
                return Format::PREF_V8_V8;
            case Opcode::ECMA_STSUPERBYVALUE_PREF_V8_V8:
                return Format::PREF_V8_V8;
            case Opcode::ECMA_LDOBJBYINDEX_PREF_V8_IMM32:
                return Format::PREF_V8_IMM32;
            case Opcode::ECMA_STOBJBYINDEX_PREF_V8_IMM32:
                return Format::PREF_V8_IMM32;
            case Opcode::ECMA_STOWNBYINDEX_PREF_V8_IMM32:
                return Format::PREF_V8_IMM32;
            case Opcode::ECMA_CALLSPREADDYN_PREF_V8_V8_V8:
                return Format::PREF_V8_V8_V8;
            case Opcode::ECMA_ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8:
                return Format::PREF_V8_V8_V8;
            case Opcode::ECMA_ASYNCFUNCTIONREJECT_PREF_V8_V8_V8:
                return Format::PREF_V8_V8_V8;
            case Opcode::ECMA_CALLARGS2DYN_PREF_V8_V8_V8:
                return Format::PREF_V8_V8_V8;
            case Opcode::ECMA_CALLARGS3DYN_PREF_V8_V8_V8_V8:
                return Format::PREF_V8_V8_V8_V8;
            case Opcode::ECMA_DEFINEGETTERSETTERBYVALUE_PREF_V8_V8_V8_V8:
                return Format::PREF_V8_V8_V8_V8;
            case Opcode::ECMA_NEWOBJDYNRANGE_PREF_IMM16_V8:
                return Format::PREF_IMM16_V8;
            case Opcode::ECMA_CALLRANGEDYN_PREF_IMM16_V8:
                return Format::PREF_IMM16_V8;
            case Opcode::ECMA_CALLTHISRANGEDYN_PREF_IMM16_V8:
                return Format::PREF_IMM16_V8;
            case Opcode::ECMA_SUPERCALL_PREF_IMM16_V8:
                return Format::PREF_IMM16_V8;
            case Opcode::ECMA_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8:
                return Format::PREF_IMM16_V8_V8;
            case Opcode::ECMA_DEFINEFUNCDYN_PREF_ID16_IMM16_V8:
                return Format::PREF_ID16_IMM16_V8;
            case Opcode::ECMA_DEFINENCFUNCDYN_PREF_ID16_IMM16_V8:
                return Format::PREF_ID16_IMM16_V8;
            case Opcode::ECMA_DEFINEGENERATORFUNC_PREF_ID16_IMM16_V8:
                return Format::PREF_ID16_IMM16_V8;
            case Opcode::ECMA_DEFINEASYNCFUNC_PREF_ID16_IMM16_V8:
                return Format::PREF_ID16_IMM16_V8;
            case Opcode::ECMA_DEFINEMETHOD_PREF_ID16_IMM16_V8:
                return Format::PREF_ID16_IMM16_V8;
            case Opcode::ECMA_NEWLEXENVDYN_PREF_IMM16:
                return Format::PREF_IMM16;
            case Opcode::ECMA_COPYRESTARGS_PREF_IMM16:
                return Format::PREF_IMM16;
            case Opcode::ECMA_CREATEARRAYWITHBUFFER_PREF_IMM16:
                return Format::PREF_IMM16;
            case Opcode::ECMA_CREATEOBJECTHAVINGMETHOD_PREF_IMM16:
                return Format::PREF_IMM16;
            case Opcode::ECMA_THROWIFSUPERNOTCORRECTCALL_PREF_IMM16:
                return Format::PREF_IMM16;
            case Opcode::ECMA_CREATEOBJECTWITHBUFFER_PREF_IMM16:
                return Format::PREF_IMM16;
            case Opcode::ECMA_LDLEXVARDYN_PREF_IMM4_IMM4:
                return Format::PREF_IMM4_IMM4;
            case Opcode::ECMA_LDLEXVARDYN_PREF_IMM8_IMM8:
                return Format::PREF_IMM8_IMM8;
            case Opcode::ECMA_LDLEXVARDYN_PREF_IMM16_IMM16:
                return Format::PREF_IMM16_IMM16;
            case Opcode::ECMA_STLEXVARDYN_PREF_IMM4_IMM4_V8:
                return Format::PREF_IMM4_IMM4_V8;
            case Opcode::ECMA_STLEXVARDYN_PREF_IMM8_IMM8_V8:
                return Format::PREF_IMM8_IMM8_V8;
            case Opcode::ECMA_STLEXVARDYN_PREF_IMM16_IMM16_V8:
                return Format::PREF_IMM16_IMM16_V8;
            case Opcode::ECMA_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8:
                return Format::PREF_ID16_IMM16_IMM16_V8_V8;
            case Opcode::ECMA_GETMODULENAMESPACE_PREF_ID32:
                return Format::PREF_ID32;
            case Opcode::ECMA_STMODULEVAR_PREF_ID32:
                return Format::PREF_ID32;
            case Opcode::ECMA_TRYLDGLOBALBYNAME_PREF_ID32:
                return Format::PREF_ID32;
            case Opcode::ECMA_TRYSTGLOBALBYNAME_PREF_ID32:
                return Format::PREF_ID32;
            case Opcode::ECMA_LDGLOBALVAR_PREF_ID32:
                return Format::PREF_ID32;
            case Opcode::ECMA_STGLOBALVAR_PREF_ID32:
                return Format::PREF_ID32;
            case Opcode::ECMA_LDOBJBYNAME_PREF_ID32_V8:
                return Format::PREF_ID32_V8;
            case Opcode::ECMA_STOBJBYNAME_PREF_ID32_V8:
                return Format::PREF_ID32_V8;
            case Opcode::ECMA_STOWNBYNAME_PREF_ID32_V8:
                return Format::PREF_ID32_V8;
            case Opcode::ECMA_LDSUPERBYNAME_PREF_ID32_V8:
                return Format::PREF_ID32_V8;
            case Opcode::ECMA_STSUPERBYNAME_PREF_ID32_V8:
                return Format::PREF_ID32_V8;
            case Opcode::ECMA_LDMODULEVAR_PREF_ID32_IMM8:
                return Format::PREF_ID32_IMM8;
            case Opcode::ECMA_CREATEREGEXPWITHLITERAL_PREF_ID32_IMM8:
                return Format::PREF_ID32_IMM8;
            case Opcode::ECMA_ISTRUE_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::ECMA_ISFALSE_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::ECMA_STCONSTTOGLOBALRECORD_PREF_ID32:
                return Format::PREF_ID32;
            case Opcode::ECMA_STLETTOGLOBALRECORD_PREF_ID32:
                return Format::PREF_ID32;
            case Opcode::ECMA_STCLASSTOGLOBALRECORD_PREF_ID32:
                return Format::PREF_ID32;
            case Opcode::ECMA_STOWNBYVALUEWITHNAMESET_PREF_V8_V8:
                return Format::PREF_V8_V8;
            case Opcode::ECMA_STOWNBYNAMEWITHNAMESET_PREF_ID32_V8:
                return Format::PREF_ID32_V8;
            case Opcode::ECMA_LDFUNCTION_PREF_NONE:
                return Format::PREF_NONE;
            case Opcode::ECMA_NEWLEXENVWITHNAMEDYN_PREF_IMM16_IMM16:
                return Format::PREF_IMM16_IMM16;
            case Opcode::ECMA_LDBIGINT_PREF_ID32:
                return Format::PREF_ID32;
            case Opcode::ECMA_TONUMERIC_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_CREATEASYNCGENERATOROBJ_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_ASYNCGENERATORRESOLVE_PREF_V8_V8_V8:
                return Format::PREF_V8_V8_V8;
            case Opcode::ECMA_DEFINEASYNCGENERATORFUNC_PREF_ID16_IMM16_V8:
                return Format::PREF_ID16_IMM16_V8;
            case Opcode::ECMA_DYNAMICIMPORT_PREF_V8:
                return Format::PREF_V8;
            case Opcode::ECMA_LDPATCHVAR_PREF_IMM16:
                return Format::PREF_IMM16;
            case Opcode::ECMA_STPATCHVAR_PREF_IMM16:
                return Format::PREF_IMM16;
            case Opcode::ECMA_ASYNCGENERATORREJECT_PREF_V8_V8:
                return Format::PREF_V8_V8;
            default:
                break;
        }
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }

    static constexpr bool HasId(Format format, size_t idx)
    {
        switch (format) {
            case Format::ID16:
                return idx < 1;
            case Format::ID32:
                return idx < 1;
            case Format::PREF_ID16_IMM16_IMM16_V8_V8:
                return idx < 1;
            case Format::PREF_ID16_IMM16_V8:
                return idx < 1;
            case Format::PREF_ID32:
                return idx < 1;
            case Format::PREF_ID32_IMM8:
                return idx < 1;
            case Format::PREF_ID32_V8:
                return idx < 1;
            case Format::V4_IMM4_ID16:
                return idx < 1;
            case Format::V4_V4_ID16:
                return idx < 1;
            case Format::V4_V4_V4_IMM4_ID16:
                return idx < 1;
            case Format::V4_V4_V4_V4_ID16:
                return idx < 1;
            case Format::V8_ID16:
                return idx < 1;
            case Format::V8_ID32:
                return idx < 1;
            default: {
                return false;
            }
        }
    }

    inline bool HasFlag(Flags flag) const
    {
        switch (GetOpcode()) {
            case Opcode::NOP:
                return ((Flags::ACC_NONE) & flag) == flag;
            case Opcode::MOV_V4_V4:
                return ((Flags::ACC_NONE) & flag) == flag;
            case Opcode::MOV_V8_V8:
                return ((Flags::ACC_NONE) & flag) == flag;
            case Opcode::MOV_V16_V16:
                return ((Flags::ACC_NONE) & flag) == flag;
            case Opcode::MOV_64_V4_V4:
                return ((Flags::ACC_NONE) & flag) == flag;
            case Opcode::MOV_64_V16_V16:
                return ((Flags::ACC_NONE) & flag) == flag;
            case Opcode::MOV_OBJ_V4_V4:
                return ((Flags::ACC_NONE) & flag) == flag;
            case Opcode::MOV_OBJ_V8_V8:
                return ((Flags::ACC_NONE) & flag) == flag;
            case Opcode::MOV_OBJ_V16_V16:
                return ((Flags::ACC_NONE) & flag) == flag;
            case Opcode::MOVI_V4_IMM4:
                return ((Flags::ACC_NONE) & flag) == flag;
            case Opcode::MOVI_V8_IMM8:
                return ((Flags::ACC_NONE) & flag) == flag;
            case Opcode::MOVI_V8_IMM16:
                return ((Flags::ACC_NONE) & flag) == flag;
            case Opcode::MOVI_V8_IMM32:
                return ((Flags::ACC_NONE) & flag) == flag;
            case Opcode::MOVI_64_V8_IMM64:
                return ((Flags::ACC_NONE) & flag) == flag;
            case Opcode::FMOVI_64_V8_IMM64:
                return ((Flags::FLOAT | Flags::ACC_NONE) & flag) == flag;
            case Opcode::MOV_NULL_V8:
                return ((Flags::ACC_NONE) & flag) == flag;
            case Opcode::LDA_V8:
                return ((Flags::ACC_WRITE) & flag) == flag;
            case Opcode::LDA_64_V8:
                return ((Flags::ACC_WRITE) & flag) == flag;
            case Opcode::LDA_OBJ_V8:
                return ((Flags::ACC_WRITE) & flag) == flag;
            case Opcode::LDAI_IMM8:
                return ((Flags::ACC_WRITE) & flag) == flag;
            case Opcode::LDAI_IMM16:
                return ((Flags::ACC_WRITE) & flag) == flag;
            case Opcode::LDAI_IMM32:
                return ((Flags::ACC_WRITE) & flag) == flag;
            case Opcode::LDAI_64_IMM64:
                return ((Flags::ACC_WRITE) & flag) == flag;
            case Opcode::FLDAI_64_IMM64:
                return ((Flags::FLOAT | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::LDA_STR_ID32:
                return ((Flags::STRING_ID | Flags::LANGUAGE_TYPE | Flags::MAYBE_DYNAMIC | Flags::ACC_WRITE) & flag) ==
                    flag;
            case Opcode::LDA_CONST_V8_ID32:
                return ((Flags::LITERALARRAY_ID | Flags::ACC_NONE) & flag) == flag;
            case Opcode::LDA_TYPE_ID16:
                return ((Flags::TYPE_ID | Flags::LANGUAGE_TYPE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::LDA_NULL:
                return ((Flags::ACC_WRITE) & flag) == flag;
            case Opcode::STA_V8:
                return ((Flags::ACC_READ) & flag) == flag;
            case Opcode::STA_64_V8:
                return ((Flags::ACC_READ) & flag) == flag;
            case Opcode::STA_OBJ_V8:
                return ((Flags::ACC_READ) & flag) == flag;
            case Opcode::CMP_64_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::FCMPL_64_V8:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::FCMPG_64_V8:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::JMP_IMM8:
                return ((Flags::JUMP | Flags::ACC_NONE) & flag) == flag;
            case Opcode::JMP_IMM16:
                return ((Flags::JUMP | Flags::ACC_NONE) & flag) == flag;
            case Opcode::JMP_IMM32:
                return ((Flags::JUMP | Flags::ACC_NONE) & flag) == flag;
            case Opcode::JEQ_OBJ_V8_IMM8:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JEQ_OBJ_V8_IMM16:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JNE_OBJ_V8_IMM8:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JNE_OBJ_V8_IMM16:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JEQZ_OBJ_IMM8:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JEQZ_OBJ_IMM16:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JNEZ_OBJ_IMM8:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JNEZ_OBJ_IMM16:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JEQZ_IMM8:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JEQZ_IMM16:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JNEZ_IMM8:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JNEZ_IMM16:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JLTZ_IMM8:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JLTZ_IMM16:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JGTZ_IMM8:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JGTZ_IMM16:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JLEZ_IMM8:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JLEZ_IMM16:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JGEZ_IMM8:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JGEZ_IMM16:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JEQ_V8_IMM8:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JEQ_V8_IMM16:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JNE_V8_IMM8:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JNE_V8_IMM16:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JLT_V8_IMM8:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JLT_V8_IMM16:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JGT_V8_IMM8:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JGT_V8_IMM16:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JLE_V8_IMM8:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JLE_V8_IMM16:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JGE_V8_IMM8:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::JGE_V8_IMM16:
                return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;
            case Opcode::FNEG_64:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::NEG:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::NEG_64:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ADD2_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ADD2_64_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::SUB2_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::SUB2_64_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::MUL2_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::MUL2_64_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::FADD2_64_V8:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::FSUB2_64_V8:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::FMUL2_64_V8:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::FDIV2_64_V8:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::FMOD2_64_V8:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::DIV2_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::DIV2_64_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::MOD2_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::MOD2_64_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ADDI_IMM8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::SUBI_IMM8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::MULI_IMM8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ANDI_IMM32:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ORI_IMM32:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::SHLI_IMM8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::SHRI_IMM8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ASHRI_IMM8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::DIVI_IMM8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::MODI_IMM8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ADD_V4_V4:
                return ((Flags::ACC_WRITE) & flag) == flag;
            case Opcode::SUB_V4_V4:
                return ((Flags::ACC_WRITE) & flag) == flag;
            case Opcode::MUL_V4_V4:
                return ((Flags::ACC_WRITE) & flag) == flag;
            case Opcode::DIV_V4_V4:
                return ((Flags::ACC_WRITE) & flag) == flag;
            case Opcode::MOD_V4_V4:
                return ((Flags::ACC_WRITE) & flag) == flag;
            case Opcode::INCI_V4_IMM4:
                return ((Flags::ACC_NONE) & flag) == flag;
            case Opcode::LDARR_8_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::LDARRU_8_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::LDARR_16_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::LDARRU_16_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::LDARR_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::LDARR_64_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::FLDARR_32_V8:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::FLDARR_64_V8:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::LDARR_OBJ_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::STARR_8_V4_V4:
                return ((Flags::ACC_READ) & flag) == flag;
            case Opcode::STARR_16_V4_V4:
                return ((Flags::ACC_READ) & flag) == flag;
            case Opcode::STARR_V4_V4:
                return ((Flags::ACC_READ) & flag) == flag;
            case Opcode::STARR_64_V4_V4:
                return ((Flags::ACC_READ) & flag) == flag;
            case Opcode::FSTARR_32_V4_V4:
                return ((Flags::FLOAT | Flags::ACC_READ) & flag) == flag;
            case Opcode::FSTARR_64_V4_V4:
                return ((Flags::FLOAT | Flags::ACC_READ) & flag) == flag;
            case Opcode::STARR_OBJ_V4_V4:
                return ((Flags::ACC_READ) & flag) == flag;
            case Opcode::LENARR_V8:
                return ((Flags::ACC_WRITE) & flag) == flag;
            case Opcode::NEWARR_V4_V4_ID16:
                return ((Flags::TYPE_ID | Flags::LANGUAGE_TYPE | Flags::INITIALIZE_TYPE | Flags::ACC_NONE) & flag) ==
                    flag;
            case Opcode::NEWOBJ_V8_ID16:
                return ((Flags::TYPE_ID | Flags::INITIALIZE_TYPE | Flags::ACC_NONE) & flag) == flag;
            case Opcode::INITOBJ_SHORT_V4_V4_ID16:
                return ((Flags::METHOD_ID | Flags::CALL | Flags::INITIALIZE_TYPE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::INITOBJ_V4_V4_V4_V4_ID16:
                return ((Flags::METHOD_ID | Flags::CALL | Flags::INITIALIZE_TYPE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::INITOBJ_RANGE_V8_ID16:
                return ((Flags::METHOD_ID | Flags::CALL | Flags::INITIALIZE_TYPE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::LDOBJ_V8_ID16:
                return ((Flags::FIELD_ID | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::LDOBJ_64_V8_ID16:
                return ((Flags::FIELD_ID | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::LDOBJ_OBJ_V8_ID16:
                return ((Flags::FIELD_ID | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::STOBJ_V8_ID16:
                return ((Flags::FIELD_ID | Flags::ACC_READ) & flag) == flag;
            case Opcode::STOBJ_64_V8_ID16:
                return ((Flags::FIELD_ID | Flags::ACC_READ) & flag) == flag;
            case Opcode::STOBJ_OBJ_V8_ID16:
                return ((Flags::FIELD_ID | Flags::ACC_READ) & flag) == flag;
            case Opcode::LDOBJ_V_V4_V4_ID16:
                return ((Flags::FIELD_ID | Flags::ACC_NONE) & flag) == flag;
            case Opcode::LDOBJ_V_64_V4_V4_ID16:
                return ((Flags::FIELD_ID | Flags::ACC_NONE) & flag) == flag;
            case Opcode::LDOBJ_V_OBJ_V4_V4_ID16:
                return ((Flags::FIELD_ID | Flags::ACC_NONE) & flag) == flag;
            case Opcode::STOBJ_V_V4_V4_ID16:
                return ((Flags::FIELD_ID | Flags::ACC_NONE) & flag) == flag;
            case Opcode::STOBJ_V_64_V4_V4_ID16:
                return ((Flags::FIELD_ID | Flags::ACC_NONE) & flag) == flag;
            case Opcode::STOBJ_V_OBJ_V4_V4_ID16:
                return ((Flags::FIELD_ID | Flags::ACC_NONE) & flag) == flag;
            case Opcode::LDSTATIC_ID16:
                return ((Flags::FIELD_ID | Flags::INITIALIZE_TYPE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::LDSTATIC_64_ID16:
                return ((Flags::FIELD_ID | Flags::INITIALIZE_TYPE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::LDSTATIC_OBJ_ID16:
                return ((Flags::FIELD_ID | Flags::INITIALIZE_TYPE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::STSTATIC_ID16:
                return ((Flags::FIELD_ID | Flags::INITIALIZE_TYPE | Flags::ACC_READ) & flag) == flag;
            case Opcode::STSTATIC_64_ID16:
                return ((Flags::FIELD_ID | Flags::INITIALIZE_TYPE | Flags::ACC_READ) & flag) == flag;
            case Opcode::STSTATIC_OBJ_ID16:
                return ((Flags::FIELD_ID | Flags::INITIALIZE_TYPE | Flags::ACC_READ) & flag) == flag;
            case Opcode::RETURN:
                return ((Flags::RETURN | Flags::ACC_READ) & flag) == flag;
            case Opcode::RETURN_64:
                return ((Flags::RETURN | Flags::ACC_READ) & flag) == flag;
            case Opcode::RETURN_OBJ:
                return ((Flags::RETURN | Flags::ACC_READ) & flag) == flag;
            case Opcode::RETURN_VOID:
                return ((Flags::RETURN | Flags::ACC_NONE) & flag) == flag;
            case Opcode::THROW_V8:
                return ((Flags::ACC_NONE) & flag) == flag;
            case Opcode::CHECKCAST_ID16:
                return ((Flags::TYPE_ID | Flags::ACC_READ) & flag) == flag;
            case Opcode::ISINSTANCE_ID16:
                return ((Flags::TYPE_ID | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::CALL_SHORT_V4_V4_ID16:
                return ((Flags::METHOD_ID | Flags::CALL | Flags::MAYBE_DYNAMIC | Flags::INITIALIZE_TYPE |
                    Flags::ACC_WRITE) & flag) == flag;
            case Opcode::CALL_V4_V4_V4_V4_ID16:
                return ((Flags::METHOD_ID | Flags::CALL | Flags::MAYBE_DYNAMIC | Flags::INITIALIZE_TYPE |
                    Flags::ACC_WRITE) & flag) == flag;
            case Opcode::CALL_RANGE_V8_ID16:
                return ((Flags::METHOD_ID | Flags::CALL | Flags::MAYBE_DYNAMIC | Flags::INITIALIZE_TYPE |
                    Flags::ACC_WRITE) & flag) == flag;
            case Opcode::CALL_ACC_SHORT_V4_IMM4_ID16:
                return ((Flags::METHOD_ID | Flags::CALL | Flags::MAYBE_DYNAMIC | Flags::ACC_WRITE | Flags::ACC_READ) &
                    flag) == flag;
            case Opcode::CALL_ACC_V4_V4_V4_IMM4_ID16:
                return ((Flags::METHOD_ID | Flags::CALL | Flags::MAYBE_DYNAMIC | Flags::ACC_WRITE | Flags::ACC_READ) &
                    flag) == flag;
            case Opcode::CALL_VIRT_SHORT_V4_V4_ID16:
                return ((Flags::METHOD_ID | Flags::CALL | Flags::CALL_VIRT | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::CALL_VIRT_V4_V4_V4_V4_ID16:
                return ((Flags::METHOD_ID | Flags::CALL | Flags::CALL_VIRT | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::CALL_VIRT_RANGE_V8_ID16:
                return ((Flags::METHOD_ID | Flags::CALL | Flags::CALL_VIRT | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::CALL_VIRT_ACC_SHORT_V4_IMM4_ID16:
                return ((Flags::METHOD_ID | Flags::CALL | Flags::CALL_VIRT | Flags::ACC_WRITE | Flags::ACC_READ) &
                    flag) == flag;
            case Opcode::CALL_VIRT_ACC_V4_V4_V4_IMM4_ID16:
                return ((Flags::METHOD_ID | Flags::CALL | Flags::CALL_VIRT | Flags::ACC_WRITE | Flags::ACC_READ) &
                    flag) == flag;
            case Opcode::MOV_DYN_V8_V8:
                return ((Flags::DYNAMIC | Flags::ACC_NONE) & flag) == flag;
            case Opcode::MOV_DYN_V16_V16:
                return ((Flags::DYNAMIC | Flags::ACC_NONE) & flag) == flag;
            case Opcode::LDA_DYN_V8:
                return ((Flags::DYNAMIC | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::STA_DYN_V8:
                return ((Flags::DYNAMIC | Flags::ACC_READ) & flag) == flag;
            case Opcode::LDAI_DYN_IMM32:
                return ((Flags::DYNAMIC | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::FLDAI_DYN_IMM64:
                return ((Flags::DYNAMIC | Flags::FLOAT | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::RETURN_DYN:
                return ((Flags::DYNAMIC | Flags::RETURN | Flags::ACC_READ) & flag) == flag;
            case Opcode::CALLI_DYN_SHORT_IMM4_V4_V4_V4:
                return ((Flags::DYNAMIC | Flags::CALL | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::CALLI_DYN_IMM4_V4_V4_V4_V4_V4:
                return ((Flags::DYNAMIC | Flags::CALL | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::CALLI_DYN_RANGE_IMM16_V16:
                return ((Flags::DYNAMIC | Flags::CALL | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::FMOVI_PREF_V8_IMM32:
                return ((Flags::FLOAT | Flags::ACC_NONE) & flag) == flag;
            case Opcode::I32TOF64_PREF_NONE:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::UCMP_PREF_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::NOT_PREF_NONE:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_LDNAN_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::FLDAI_PREF_IMM32:
                return ((Flags::FLOAT | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::U32TOF64_PREF_NONE:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::UCMP_64_PREF_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::NOT_64_PREF_NONE:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_LDINFINITY_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::FCMPL_PREF_V8:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::I64TOF64_PREF_NONE:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::DIVU2_PREF_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::AND2_PREF_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_LDGLOBALTHIS_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::FCMPG_PREF_V8:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::U64TOF64_PREF_NONE:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::DIVU2_64_PREF_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::AND2_64_PREF_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_LDUNDEFINED_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::FNEG_PREF_NONE:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::F64TOI32_PREF_NONE:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::MODU2_PREF_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::OR2_PREF_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_LDNULL_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::FADD2_PREF_V8:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::F64TOI64_PREF_NONE:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::MODU2_64_PREF_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::OR2_64_PREF_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_LDSYMBOL_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::FSUB2_PREF_V8:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::F64TOU32_PREF_NONE:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::XOR2_PREF_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_LDGLOBAL_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::FMUL2_PREF_V8:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::F64TOU64_PREF_NONE:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::XOR2_64_PREF_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_LDTRUE_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::FDIV2_PREF_V8:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::I32TOU1_PREF_NONE:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::SHL2_PREF_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_LDFALSE_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::FMOD2_PREF_V8:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::I64TOU1_PREF_NONE:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::SHL2_64_PREF_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_THROWDYN_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::I32TOF32_PREF_NONE:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::U32TOU1_PREF_NONE:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::SHR2_PREF_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_TYPEOFDYN_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::U32TOF32_PREF_NONE:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::U64TOU1_PREF_NONE:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::SHR2_64_PREF_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_LDLEXENVDYN_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::I64TOF32_PREF_NONE:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::I32TOI64_PREF_NONE:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ASHR2_PREF_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_POPLEXENVDYN_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::U64TOF32_PREF_NONE:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::I32TOI16_PREF_NONE:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ASHR2_64_PREF_V8:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_GETUNMAPPEDARGS_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::F32TOF64_PREF_NONE:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::I32TOU16_PREF_NONE:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::XORI_PREF_IMM32:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_GETPROPITERATOR_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::F32TOI32_PREF_NONE:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::I32TOI8_PREF_NONE:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::AND_PREF_V4_V4:
                return ((Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_ASYNCFUNCTIONENTER_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::F32TOI64_PREF_NONE:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::I32TOU8_PREF_NONE:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::OR_PREF_V4_V4:
                return ((Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_LDHOLE_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::F32TOU32_PREF_NONE:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::I64TOI32_PREF_NONE:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::XOR_PREF_V4_V4:
                return ((Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_RETURNUNDEFINED_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::RETURN | Flags::ACC_NONE) & flag) == flag;
            case Opcode::F32TOU64_PREF_NONE:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::U32TOI64_PREF_NONE:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::SHL_PREF_V4_V4:
                return ((Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_CREATEEMPTYOBJECT_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::F64TOF32_PREF_NONE:
                return ((Flags::FLOAT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::U32TOI16_PREF_NONE:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::SHR_PREF_V4_V4:
                return ((Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_CREATEEMPTYARRAY_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::U32TOU16_PREF_NONE:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ASHR_PREF_V4_V4:
                return ((Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_GETITERATOR_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::U32TOI8_PREF_NONE:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_THROWTHROWNOTEXISTS_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_NONE) & flag) == flag;
            case Opcode::U32TOU8_PREF_NONE:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_THROWPATTERNNONCOERCIBLE_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_NONE) & flag) == flag;
            case Opcode::U64TOI32_PREF_NONE:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_LDHOMEOBJECT_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::U64TOU32_PREF_NONE:
                return ((Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_THROWDELETESUPERPROPERTY_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_NONE) & flag) == flag;
            case Opcode::ECMA_DEBUGGER_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_NONE) & flag) == flag;
            case Opcode::ECMA_ADD2DYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_SUB2DYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_MUL2DYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_DIV2DYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_MOD2DYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_EQDYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_NOTEQDYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_LESSDYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_LESSEQDYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_GREATERDYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_GREATEREQDYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_SHL2DYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_SHR2DYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_ASHR2DYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_AND2DYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_OR2DYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_XOR2DYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_TONUMBER_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_NEGDYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_NOTDYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_INCDYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_DECDYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_EXPDYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_ISINDYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_INSTANCEOFDYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_STRICTNOTEQDYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_STRICTEQDYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_RESUMEGENERATOR_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_GETRESUMEMODE_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_CREATEGENERATOROBJ_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_THROWCONSTASSIGNMENT_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_NONE) & flag) == flag;
            case Opcode::ECMA_GETTEMPLATEOBJECT_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_GETNEXTPROPNAME_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_CALLARG0DYN_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_THROWIFNOTOBJECT_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_NONE) & flag) == flag;
            case Opcode::ECMA_ITERNEXT_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_CLOSEITERATOR_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_COPYMODULE_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_SUPERCALLSPREAD_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_DELOBJPROP_PREF_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_NEWOBJSPREADDYN_PREF_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_CREATEITERRESULTOBJ_PREF_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_SUSPENDGENERATOR_PREF_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_THROWUNDEFINEDIFHOLE_PREF_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_NONE) & flag) == flag;
            case Opcode::ECMA_CALLARG1DYN_PREF_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_COPYDATAPROPERTIES_PREF_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_STARRAYSPREAD_PREF_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_GETITERATORNEXT_PREF_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_SETOBJECTWITHPROTO_PREF_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_NONE) & flag) == flag;
            case Opcode::ECMA_LDOBJBYVALUE_PREF_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_STOBJBYVALUE_PREF_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_STOWNBYVALUE_PREF_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_LDSUPERBYVALUE_PREF_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_STSUPERBYVALUE_PREF_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_LDOBJBYINDEX_PREF_V8_IMM32:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_STOBJBYINDEX_PREF_V8_IMM32:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_STOWNBYINDEX_PREF_V8_IMM32:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_CALLSPREADDYN_PREF_V8_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_ASYNCFUNCTIONREJECT_PREF_V8_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_CALLARGS2DYN_PREF_V8_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_CALLARGS3DYN_PREF_V8_V8_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_DEFINEGETTERSETTERBYVALUE_PREF_V8_V8_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_NEWOBJDYNRANGE_PREF_IMM16_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_CALLRANGEDYN_PREF_IMM16_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_CALLTHISRANGEDYN_PREF_IMM16_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_SUPERCALL_PREF_IMM16_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_DEFINEFUNCDYN_PREF_ID16_IMM16_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::METHOD_ID | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_DEFINENCFUNCDYN_PREF_ID16_IMM16_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::METHOD_ID | Flags::ACC_WRITE | Flags::ACC_READ) &
                    flag) == flag;
            case Opcode::ECMA_DEFINEGENERATORFUNC_PREF_ID16_IMM16_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::METHOD_ID | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_DEFINEASYNCFUNC_PREF_ID16_IMM16_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::METHOD_ID | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_DEFINEMETHOD_PREF_ID16_IMM16_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::METHOD_ID | Flags::ACC_WRITE | Flags::ACC_READ) &
                    flag) == flag;
            case Opcode::ECMA_NEWLEXENVDYN_PREF_IMM16:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_COPYRESTARGS_PREF_IMM16:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_CREATEARRAYWITHBUFFER_PREF_IMM16:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_CREATEOBJECTHAVINGMETHOD_PREF_IMM16:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_THROWIFSUPERNOTCORRECTCALL_PREF_IMM16:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_CREATEOBJECTWITHBUFFER_PREF_IMM16:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_LDLEXVARDYN_PREF_IMM4_IMM4:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_LDLEXVARDYN_PREF_IMM8_IMM8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_LDLEXVARDYN_PREF_IMM16_IMM16:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_STLEXVARDYN_PREF_IMM4_IMM4_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_STLEXVARDYN_PREF_IMM8_IMM8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_STLEXVARDYN_PREF_IMM16_IMM16_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::METHOD_ID | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_GETMODULENAMESPACE_PREF_ID32:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_STMODULEVAR_PREF_ID32:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_TRYLDGLOBALBYNAME_PREF_ID32:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_TRYSTGLOBALBYNAME_PREF_ID32:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_LDGLOBALVAR_PREF_ID32:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_STGLOBALVAR_PREF_ID32:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_LDOBJBYNAME_PREF_ID32_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_STOBJBYNAME_PREF_ID32_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_STOWNBYNAME_PREF_ID32_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_LDSUPERBYNAME_PREF_ID32_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_STSUPERBYNAME_PREF_ID32_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_LDMODULEVAR_PREF_ID32_IMM8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_CREATEREGEXPWITHLITERAL_PREF_ID32_IMM8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_ISTRUE_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_ISFALSE_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_STCONSTTOGLOBALRECORD_PREF_ID32:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_STLETTOGLOBALRECORD_PREF_ID32:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_STCLASSTOGLOBALRECORD_PREF_ID32:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_STOWNBYVALUEWITHNAMESET_PREF_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_STOWNBYNAMEWITHNAMESET_PREF_ID32_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_LDFUNCTION_PREF_NONE:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_NEWLEXENVWITHNAMEDYN_PREF_IMM16_IMM16:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_LDBIGINT_PREF_ID32:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_TONUMERIC_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;
            case Opcode::ECMA_CREATEASYNCGENERATOROBJ_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_ASYNCGENERATORRESOLVE_PREF_V8_V8_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_DEFINEASYNCGENERATORFUNC_PREF_ID16_IMM16_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::METHOD_ID | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_DYNAMICIMPORT_PREF_V8:
                return ((Flags::ACC_READ | Flags::ACC_WRITE) & flag) == flag;
            case Opcode::ECMA_LDPATCHVAR_PREF_IMM16:
                return (Flags::ACC_WRITE & flag) == flag;
            case Opcode::ECMA_STPATCHVAR_PREF_IMM16:
                return (Flags::ACC_READ & flag) == flag;
            case Opcode::ECMA_ASYNCGENERATORREJECT_PREF_V8_V8:
                return (Flags::ACC_WRITE & flag) == flag;
            default:
                return false;
        }
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
};
}  // panda::ecmascript

#endif  // ECMASCRIPT_JSPANDAFILE_BYTECODE_INST_OLD_INSTRUCTION_H