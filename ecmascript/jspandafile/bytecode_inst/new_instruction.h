/**
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef LIBPANDAFILE_BYTECODE_INSTRUCTION_INL_H
#define LIBPANDAFILE_BYTECODE_INSTRUCTION_INL_H

#include "file.h"

#include <cstdint>
#include <cstddef>
#include <type_traits>

#include "macros.h"

#include "utils/bit_helpers.h"

#if !PANDA_TARGET_WINDOWS
#include "securec.h"
#endif

namespace panda::ecmascript {

enum class BytecodeInstMode { FAST, SAFE };

template <const BytecodeInstMode>
class BytecodeInstBase;

class BytecodeId {
public:
    constexpr explicit BytecodeId(uint32_t id) : id_(id) {}

    constexpr BytecodeId() = default;

    ~BytecodeId() = default;

    DEFAULT_COPY_SEMANTIC(BytecodeId);
    NO_MOVE_SEMANTIC(BytecodeId);

    panda_file::File::Index AsIndex() const
    {
        ASSERT(id_ < std::numeric_limits<uint16_t>::max());
        return id_;
    }

    panda_file::File::EntityId AsFileId() const
    {
        return panda_file::File::EntityId(id_);
    }

    uint32_t AsRawValue() const
    {
        return id_;
    }

    bool IsValid() const
    {
        return id_ != INVALID;
    }

    bool operator==(BytecodeId id) const noexcept
    {
        return id_ == id.id_;
    }

    friend std::ostream &operator<<(std::ostream &stream, BytecodeId id)
    {
        return stream << id.id_;
    }

private:
    static constexpr size_t INVALID = std::numeric_limits<uint32_t>::max();

    uint32_t id_ {INVALID};
};

template <>
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, hicpp-special-member-functions)
class BytecodeInstBase<BytecodeInstMode::FAST> {
public:
    BytecodeInstBase() = default;
    explicit BytecodeInstBase(const uint8_t *pc) : pc_ {pc} {}
    ~BytecodeInstBase() = default;

protected:
    const uint8_t *GetPointer(int32_t offset) const
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return pc_ + offset;
    }

    const uint8_t *GetAddress() const
    {
        return pc_;
    }

    const uint8_t *GetAddress() volatile const
    {
        return pc_;
    }

    template <class T>
    T Read(size_t offset) const
    {
        using unaligned_type __attribute__((aligned(1))) = const T;
        return *reinterpret_cast<unaligned_type *>(GetPointer(offset));
    }

    void Write(uint32_t value, uint32_t offset, uint32_t width)
    {
        auto *dst = const_cast<uint8_t *>(GetPointer(offset));
        if (memcpy_s(dst, width, &value, width) != 0) {
            LOG(FATAL, PANDAFILE) << "Cannot write value : " << value << "at the dst offset : " << offset;
        }
    }

    uint8_t ReadByte(size_t offset) const
    {
        return Read<uint8_t>(offset);
    }

private:
    const uint8_t *pc_ {nullptr};
};

template <>
class BytecodeInstBase<BytecodeInstMode::SAFE> {
public:
    BytecodeInstBase() = default;
    explicit BytecodeInstBase(const uint8_t *pc, const uint8_t *from, const uint8_t *to)
        : pc_ {pc}, from_ {from}, to_ {to}, valid_ {true}
    {
        ASSERT(from_ <= to_ && pc_ >= from_ && pc_ <= to_);
    }

protected:
    const uint8_t *GetPointer(int32_t offset) const
    {
        return GetPointer(offset, 1);
    }

    bool IsLast(size_t size) const
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const uint8_t *ptr_next = pc_ + size;
        return ptr_next > to_;
    }

    const uint8_t *GetPointer(int32_t offset, size_t size) const
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const uint8_t *ptr_from = pc_ + offset;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const uint8_t *ptr_to = ptr_from + size - 1;
        if (from_ == nullptr || ptr_from < from_ || ptr_to > to_) {
            valid_ = false;
            return from_;
        }
        return ptr_from;
    }

    const uint8_t *GetAddress() const
    {
        return pc_;
    }

    const uint8_t *GetFrom() const
    {
        return from_;
    }

    const uint8_t *GetTo() const
    {
        return to_;
    }

    uint32_t GetOffset() const
    {
        return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pc_) - reinterpret_cast<uintptr_t>(from_));
    }

    const uint8_t *GetAddress() volatile const
    {
        return pc_;
    }

    template <class T>
    T Read(size_t offset) const
    {
        using unaligned_type __attribute__((aligned(1))) = const T;
        auto ptr = reinterpret_cast<unaligned_type *>(GetPointer(offset, sizeof(T)));
        if (IsValid()) {
            return *ptr;
        }
        return {};
    }

    bool IsValid() const
    {
        return valid_;
    }

private:
    const uint8_t *pc_ {nullptr};
    const uint8_t *from_ {nullptr};
    const uint8_t *to_ {nullptr};
    mutable bool valid_ {false};
};

template <const BytecodeInstMode Mode = BytecodeInstMode::FAST>

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, hicpp-special-member-functions)
class BytecodeInst : public BytecodeInstBase<Mode> {
    using Base = BytecodeInstBase<Mode>;

public:
/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

enum class Format : uint8_t {
    ID16,
    IMM16,
    IMM16_ID16,
    IMM16_ID16_ID16_IMM16_V8,
    IMM16_ID16_IMM8,
    IMM16_ID16_V8,
    IMM16_IMM16,
    IMM16_IMM8_V8,
    IMM16_V8,
    IMM16_V8_IMM16,
    IMM16_V8_V8,
    IMM32,
    IMM4_IMM4,
    IMM64,
    IMM8,
    IMM8_ID16,
    IMM8_ID16_ID16_IMM16_V8,
    IMM8_ID16_IMM8,
    IMM8_ID16_V8,
    IMM8_IMM16,
    IMM8_IMM8,
    IMM8_IMM8_V8,
    IMM8_V8,
    IMM8_V8_IMM16,
    IMM8_V8_V8,
    IMM8_V8_V8_V8,
    IMM8_V8_V8_V8_V8,
    NONE,
    PREF_ID16_IMM16_IMM16_V8_V8,
    PREF_ID32,
    PREF_ID32_IMM8,
    PREF_ID32_V8,
    PREF_IMM16,
    PREF_IMM16_ID16,
    PREF_IMM16_IMM16,
    PREF_IMM16_IMM16_V8,
    PREF_IMM16_V8,
    PREF_IMM16_V8_V8,
    PREF_IMM32,
    PREF_IMM4_IMM4_V8,
    PREF_IMM8,
    PREF_IMM8_IMM8_V8,
    PREF_NONE,
    PREF_V8,
    PREF_V8_IMM32,
    PREF_V8_V8,
    PREF_V8_V8_V8,
    PREF_V8_V8_V8_V8,
    V16_V16,
    V4_V4,
    V8,
    V8_IMM16,
    V8_IMM8,
    V8_V8,
    V8_V8_V8,
    V8_V8_V8_V8,
};

enum class Opcode {
    LDNAN = 0,
    LDINFINITY = 1,
    LDNEWTARGET = 2,
    LDUNDEFINED = 3,
    LDNULL = 4,
    LDSYMBOL = 5,
    LDGLOBAL = 6,
    LDTRUE = 7,
    LDFALSE = 8,
    LDHOLE = 9,
    LDTHIS = 10,
    POPLEXENV = 11,
    GETUNMAPPEDARGS = 12,
    ASYNCFUNCTIONENTER = 13,
    CREATEASYNCGENERATOROBJ_V8 = 14,
    LDFUNCTION = 15,
    DEBUGGER = 16,
    GETPROPITERATOR = 17,
    GETITERATOR_IMM8 = 18,
    GETITERATOR_IMM16 = 19,
    CLOSEITERATOR_IMM8_V8 = 20,
    CLOSEITERATOR_IMM16_V8 = 21,
    ASYNCGENERATORRESOLVE_V8_V8_V8 = 22,
    CREATEEMPTYOBJECT = 23,
    CREATEEMPTYARRAY_IMM8 = 24,
    CREATEEMPTYARRAY_IMM16 = 25,
    CREATEGENERATOROBJ_V8 = 26,
    CREATEITERRESULTOBJ_V8_V8 = 27,
    CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8 = 28,
    CALLTHIS0_IMM8_V8 = 29,
    CREATEARRAYWITHBUFFER_IMM8_ID16 = 30,
    CREATEARRAYWITHBUFFER_IMM16_ID16 = 31,
    CALLTHIS1_IMM8_V8_V8 = 32,
    CALLTHIS2_IMM8_V8_V8_V8 = 33,
    CREATEOBJECTWITHBUFFER_IMM8_ID16 = 34,
    CREATEOBJECTWITHBUFFER_IMM16_ID16 = 35,
    CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8 = 36,
    CREATEREGEXPWITHLITERAL_IMM16_ID16_IMM8 = 37,
    NEWOBJAPPLY_IMM8_V8 = 38,
    NEWOBJAPPLY_IMM16_V8 = 39,
    NEWOBJRANGE_IMM8_IMM8_V8 = 40,
    NEWOBJRANGE_IMM16_IMM8_V8 = 41,
    NEWLEXENV_IMM8 = 42,
    NEWLEXENVWITHNAME_IMM8_ID16 = 43,
    ADD2_IMM8_V8 = 44,
    SUB2_IMM8_V8 = 45,
    MUL2_IMM8_V8 = 46,
    DIV2_IMM8_V8 = 47,
    MOD2_IMM8_V8 = 48,
    EQ_IMM8_V8 = 49,
    NOTEQ_IMM8_V8 = 50,
    LESS_IMM8_V8 = 51,
    LESSEQ_IMM8_V8 = 52,
    GREATER_IMM8_V8 = 53,
    GREATEREQ_IMM8_V8 = 54,
    SHL2_IMM8_V8 = 55,
    SHR2_IMM8_V8 = 56,
    ASHR2_IMM8_V8 = 57,
    AND2_IMM8_V8 = 58,
    OR2_IMM8_V8 = 59,
    XOR2_IMM8_V8 = 60,
    EXP_IMM8_V8 = 61,
    TYPEOF_IMM8 = 62,
    TYPEOF_IMM16 = 63,
    TONUMBER_IMM8 = 64,
    TONUMERIC_IMM8 = 65,
    NEG_IMM8 = 66,
    NOT_IMM8 = 67,
    INC_IMM8 = 68,
    DEC_IMM8 = 69,
    ISIN_IMM8_V8 = 70,
    INSTANCEOF_IMM8_V8 = 71,
    STRICTNOTEQ_IMM8_V8 = 72,
    STRICTEQ_IMM8_V8 = 73,
    ISTRUE = 74,
    ISFALSE = 75,
    CALLTHIS3_IMM8_V8_V8_V8_V8 = 76,
    CALLTHISRANGE_IMM8_IMM8_V8 = 77,
    SUPERCALLTHISRANGE_IMM8_IMM8_V8 = 78,
    SUPERCALLARROWRANGE_IMM8_IMM8_V8 = 79,
    DEFINEFUNC_IMM8_ID16_IMM8 = 80,
    DEFINEFUNC_IMM16_ID16_IMM8 = 81,
    DEFINEMETHOD_IMM8_ID16_IMM8 = 82,
    DEFINEMETHOD_IMM16_ID16_IMM8 = 83,
    CALLARG0_IMM8 = 84,
    SUPERCALLSPREAD_IMM8_V8 = 85,
    APPLY_IMM8_V8_V8 = 86,
    CALLARGS2_IMM8_V8_V8 = 87,
    CALLARGS3_IMM8_V8_V8_V8 = 88,
    CALLRANGE_IMM8_IMM8_V8 = 89,
    LDEXTERNALMODULEVAR_IMM8 = 90,
    LDTHISBYNAME_IMM8_ID16 = 91,
    DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8 = 92,
    LDTHISBYNAME_IMM16_ID16 = 93,
    STTHISBYNAME_IMM8_ID16 = 94,
    STTHISBYNAME_IMM16_ID16 = 95,
    LDTHISBYVALUE_IMM8 = 96,
    LDTHISBYVALUE_IMM16 = 97,
    STTHISBYVALUE_IMM8_V8 = 98,
    STTHISBYVALUE_IMM16_V8 = 99,
    LDPATCHVAR_IMM8 = 100,
    STPATCHVAR_IMM8_V8 = 101,
    DYNAMICIMPORT = 102,
    DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8 = 103,
    DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8 = 104,
    RESUMEGENERATOR = 105,
    GETRESUMEMODE = 106,
    GETTEMPLATEOBJECT_IMM8 = 107,
    GETTEMPLATEOBJECT_IMM16 = 108,
    GETNEXTPROPNAME_V8 = 109,
    JEQZ_IMM8 = 110,
    JEQZ_IMM16 = 111,
    SETOBJECTWITHPROTO_IMM8_V8 = 112,
    DELOBJPROP_V8 = 113,
    SUSPENDGENERATOR_V8 = 114,
    ASYNCFUNCTIONAWAITUNCAUGHT_V8 = 115,
    COPYDATAPROPERTIES_V8 = 116,
    STARRAYSPREAD_V8_V8 = 117,
    SETOBJECTWITHPROTO_IMM16_V8 = 118,
    LDOBJBYVALUE_IMM8_V8 = 119,
    LDOBJBYVALUE_IMM16_V8 = 120,
    STOBJBYVALUE_IMM8_V8_V8 = 121,
    STOBJBYVALUE_IMM16_V8_V8 = 122,
    STOWNBYVALUE_IMM8_V8_V8 = 123,
    STOWNBYVALUE_IMM16_V8_V8 = 124,
    LDSUPERBYVALUE_IMM8_V8 = 125,
    LDSUPERBYVALUE_IMM16_V8 = 126,
    STSUPERBYVALUE_IMM8_V8_V8 = 127,
    STSUPERBYVALUE_IMM16_V8_V8 = 128,
    LDOBJBYINDEX_IMM8_IMM16 = 129,
    LDOBJBYINDEX_IMM16_IMM16 = 130,
    STOBJBYINDEX_IMM8_V8_IMM16 = 131,
    STOBJBYINDEX_IMM16_V8_IMM16 = 132,
    STOWNBYINDEX_IMM8_V8_IMM16 = 133,
    STOWNBYINDEX_IMM16_V8_IMM16 = 134,
    ASYNCFUNCTIONRESOLVE_V8 = 135,
    ASYNCFUNCTIONREJECT_V8 = 136,
    COPYRESTARGS_IMM8 = 137,
    LDLEXVAR_IMM4_IMM4 = 138,
    STLEXVAR_IMM4_IMM4 = 139,
    GETMODULENAMESPACE_IMM8 = 140,
    STMODULEVAR_IMM8 = 141,
    TRYLDGLOBALBYNAME_IMM8_ID16 = 142,
    TRYLDGLOBALBYNAME_IMM16_ID16 = 143,
    TRYSTGLOBALBYNAME_IMM8_ID16 = 144,
    TRYSTGLOBALBYNAME_IMM16_ID16 = 145,
    LDGLOBALVAR_IMM16_ID16 = 146,
    STGLOBALVAR_IMM16_ID16 = 147,
    LDOBJBYNAME_IMM8_ID16 = 148,
    LDOBJBYNAME_IMM16_ID16 = 149,
    STOBJBYNAME_IMM8_ID16_V8 = 150,
    STOBJBYNAME_IMM16_ID16_V8 = 151,
    STOWNBYNAME_IMM8_ID16_V8 = 152,
    STOWNBYNAME_IMM16_ID16_V8 = 153,
    LDSUPERBYNAME_IMM8_ID16 = 154,
    LDSUPERBYNAME_IMM16_ID16 = 155,
    STSUPERBYNAME_IMM8_ID16_V8 = 156,
    STSUPERBYNAME_IMM16_ID16_V8 = 157,
    LDLOCALMODULEVAR_IMM8 = 158,
    STCONSTTOGLOBALRECORD_IMM16_ID16 = 159,
    STTOGLOBALRECORD_IMM16_ID16 = 160,
    JEQZ_IMM32 = 161,
    JNEZ_IMM8 = 162,
    JNEZ_IMM16 = 163,
    JNEZ_IMM32 = 164,
    STOWNBYVALUEWITHNAMESET_IMM8_V8_V8 = 165,
    STOWNBYVALUEWITHNAMESET_IMM16_V8_V8 = 166,
    STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8 = 167,
    STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8 = 168,
    LDBIGINT_ID16 = 169,
    LDA_STR_ID16 = 170,
    JMP_IMM8 = 171,
    JMP_IMM16 = 172,
    JMP_IMM32 = 173,
    JSTRICTEQZ_IMM8 = 174,
    JSTRICTEQZ_IMM16 = 175,
    JNSTRICTEQZ_IMM8 = 176,
    JNSTRICTEQZ_IMM16 = 177,
    JEQNULL_IMM8 = 178,
    JEQNULL_IMM16 = 179,
    LDA_V8 = 180,
    STA_V8 = 181,
    LDAI_IMM32 = 182,
    FLDAI_IMM64 = 183,
    RETURN = 184,
    RETURNUNDEFINED = 185,
    LDLEXVAR_IMM8_IMM8 = 186,
    JNENULL_IMM8 = 187,
    STLEXVAR_IMM8_IMM8 = 188,
    JNENULL_IMM16 = 189,
    CALLARG1_IMM8_V8 = 190,
    JSTRICTEQNULL_IMM8 = 191,
    JSTRICTEQNULL_IMM16 = 192,
    JNSTRICTEQNULL_IMM8 = 193,
    JNSTRICTEQNULL_IMM16 = 194,
    JEQUNDEFINED_IMM8 = 195,
    JEQUNDEFINED_IMM16 = 196,
    JNEUNDEFINED_IMM8 = 197,
    JNEUNDEFINED_IMM16 = 198,
    JSTRICTEQUNDEFINED_IMM8 = 199,
    JSTRICTEQUNDEFINED_IMM16 = 200,
    JNSTRICTEQUNDEFINED_IMM8 = 201,
    JNSTRICTEQUNDEFINED_IMM16 = 202,
    JEQ_V8_IMM8 = 203,
    JEQ_V8_IMM16 = 204,
    JNE_V8_IMM8 = 205,
    JNE_V8_IMM16 = 206,
    JSTRICTEQ_V8_IMM8 = 207,
    JSTRICTEQ_V8_IMM16 = 208,
    JNSTRICTEQ_V8_IMM8 = 209,
    JNSTRICTEQ_V8_IMM16 = 210,
    MOV_V4_V4 = 211,
    MOV_V8_V8 = 212,
    MOV_V16_V16 = 213,
    ASYNCGENERATORREJECT_V8_V8 = 214,
    NOP = 215,
    DEPRECATED_LDLEXENV_PREF_NONE = 252,
    WIDE_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8 = 253,
    THROW_PREF_NONE = 254,
    DEPRECATED_POPLEXENV_PREF_NONE = 508,
    WIDE_NEWOBJRANGE_PREF_IMM16_V8 = 509,
    THROW_NOTEXISTS_PREF_NONE = 510,
    DEPRECATED_GETITERATORNEXT_PREF_V8_V8 = 764,
    WIDE_NEWLEXENV_PREF_IMM16 = 765,
    THROW_PATTERNNONCOERCIBLE_PREF_NONE = 766,
    DEPRECATED_CREATEARRAYWITHBUFFER_PREF_IMM16 = 1020,
    WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16 = 1021,
    THROW_DELETESUPERPROPERTY_PREF_NONE = 1022,
    DEPRECATED_CREATEOBJECTWITHBUFFER_PREF_IMM16 = 1276,
    WIDE_CALLRANGE_PREF_IMM16_V8 = 1277,
    THROW_CONSTASSIGNMENT_PREF_V8 = 1278,
    DEPRECATED_TONUMBER_PREF_V8 = 1532,
    WIDE_CALLTHISRANGE_PREF_IMM16_V8 = 1533,
    THROW_IFNOTOBJECT_PREF_V8 = 1534,
    DEPRECATED_TONUMERIC_PREF_V8 = 1788,
    WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8 = 1789,
    THROW_UNDEFINEDIFHOLE_PREF_V8_V8 = 1790,
    DEPRECATED_NEG_PREF_V8 = 2044,
    WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8 = 2045,
    THROW_IFSUPERNOTCORRECTCALL_PREF_IMM8 = 2046,
    DEPRECATED_NOT_PREF_V8 = 2300,
    WIDE_LDOBJBYINDEX_PREF_IMM32 = 2301,
    THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16 = 2302,
    DEPRECATED_INC_PREF_V8 = 2556,
    WIDE_STOBJBYINDEX_PREF_V8_IMM32 = 2557,
    DEPRECATED_DEC_PREF_V8 = 2812,
    WIDE_STOWNBYINDEX_PREF_V8_IMM32 = 2813,
    DEPRECATED_CALLARG0_PREF_V8 = 3068,
    WIDE_COPYRESTARGS_PREF_IMM16 = 3069,
    DEPRECATED_CALLARG1_PREF_V8_V8 = 3324,
    WIDE_LDLEXVAR_PREF_IMM16_IMM16 = 3325,
    DEPRECATED_CALLARGS2_PREF_V8_V8_V8 = 3580,
    WIDE_STLEXVAR_PREF_IMM16_IMM16 = 3581,
    DEPRECATED_CALLARGS3_PREF_V8_V8_V8_V8 = 3836,
    WIDE_GETMODULENAMESPACE_PREF_IMM16 = 3837,
    DEPRECATED_CALLRANGE_PREF_IMM16_V8 = 4092,
    WIDE_STMODULEVAR_PREF_IMM16 = 4093,
    DEPRECATED_CALLSPREAD_PREF_V8_V8_V8 = 4348,
    WIDE_LDLOCALMODULEVAR_PREF_IMM16 = 4349,
    DEPRECATED_CALLTHISRANGE_PREF_IMM16_V8 = 4604,
    WIDE_LDEXTERNALMODULEVAR_PREF_IMM16 = 4605,
    DEPRECATED_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8 = 4860,
    WIDE_LDPATCHVAR_PREF_IMM16 = 4861,
    DEPRECATED_RESUMEGENERATOR_PREF_V8 = 5116,
    WIDE_STPATCHVAR_PREF_IMM16 = 5117,
    DEPRECATED_GETRESUMEMODE_PREF_V8 = 5372,
    DEPRECATED_GETTEMPLATEOBJECT_PREF_V8 = 5628,
    DEPRECATED_DELOBJPROP_PREF_V8_V8 = 5884,
    DEPRECATED_SUSPENDGENERATOR_PREF_V8_V8 = 6140,
    DEPRECATED_ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8 = 6396,
    DEPRECATED_COPYDATAPROPERTIES_PREF_V8_V8 = 6652,
    DEPRECATED_SETOBJECTWITHPROTO_PREF_V8_V8 = 6908,
    DEPRECATED_LDOBJBYVALUE_PREF_V8_V8 = 7164,
    DEPRECATED_LDSUPERBYVALUE_PREF_V8_V8 = 7420,
    DEPRECATED_LDOBJBYINDEX_PREF_V8_IMM32 = 7676,
    DEPRECATED_ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8 = 7932,
    DEPRECATED_ASYNCFUNCTIONREJECT_PREF_V8_V8_V8 = 8188,
    DEPRECATED_STLEXVAR_PREF_IMM4_IMM4_V8 = 8444,
    DEPRECATED_STLEXVAR_PREF_IMM8_IMM8_V8 = 8700,
    DEPRECATED_STLEXVAR_PREF_IMM16_IMM16_V8 = 8956,
    DEPRECATED_GETMODULENAMESPACE_PREF_ID32 = 9212,
    DEPRECATED_STMODULEVAR_PREF_ID32 = 9468,
    DEPRECATED_LDOBJBYNAME_PREF_ID32_V8 = 9724,
    DEPRECATED_LDSUPERBYNAME_PREF_ID32_V8 = 9980,
    DEPRECATED_LDMODULEVAR_PREF_ID32_IMM8 = 10236,
    DEPRECATED_STCONSTTOGLOBALRECORD_PREF_ID32 = 10492,
    DEPRECATED_STLETTOGLOBALRECORD_PREF_ID32 = 10748,
    DEPRECATED_STCLASSTOGLOBALRECORD_PREF_ID32 = 11004,
    DEPRECATED_LDHOMEOBJECT_PREF_NONE = 11260,
    DEPRECATED_CREATEOBJECTHAVINGMETHOD_PREF_IMM16 = 11516,
    DEPRECATED_DYNAMICIMPORT_PREF_V8 = 11772,
    LAST = DEPRECATED_DYNAMICIMPORT_PREF_V8
};

enum Flags : uint32_t {
    TYPE_ID = 0x1,
    METHOD_ID = 0x2,
    STRING_ID = 0x4,
    LITERALARRAY_ID = 0x8,
    FIELD_ID = 0x10,
    CALL = 0x20,
    CALL_VIRT = 0x40,
    RETURN = 0x80,
    SUSPEND = 0x100,
    JUMP = 0x200,
    CONDITIONAL = 0x400,
    FLOAT = 0x800,
    DYNAMIC = 0x1000,
    MAYBE_DYNAMIC = 0x2000,
    LANGUAGE_TYPE = 0x4000,
    INITIALIZE_TYPE = 0x8000,
    IC_SLOT = 0x10000,
    JIT_IC_SLOT = 0x20000,
    ONE_SLOT = 0x40000,
    TWO_SLOT = 0x80000,
    ACC_NONE = 0x100000,
    ACC_READ = 0x200000,
    ACC_WRITE = 0x400000,
};

enum Exceptions : uint32_t {
    X_NONE = 0x1,
    X_NULL = 0x2,
    X_BOUNDS = 0x4,
    X_NEGSIZE = 0x8,
    X_STORE = 0x10,
    X_ABSTRACT = 0x20,
    X_ARITH = 0x40,
    X_CAST = 0x80,
    X_CLASSDEF = 0x100,
    X_OOM = 0x200,
    X_INIT = 0x400,
    X_CALL = 0x800,
    X_THROW = 0x1000,
    X_LINK = 0x2000,
};

    BytecodeInst() = default;

    ~BytecodeInst() = default;

    template <const BytecodeInstMode M = Mode, typename = std::enable_if_t<M == BytecodeInstMode::FAST>>
    explicit BytecodeInst(const uint8_t *pc) : Base {pc}
    {
    }

    template <const BytecodeInstMode M = Mode, typename = std::enable_if_t<M == BytecodeInstMode::SAFE>>
    explicit BytecodeInst(const uint8_t *pc, const uint8_t *from, const uint8_t *to) : Base {pc, from, to}
    {
    }

    template <Format format, size_t idx = 0>
    BytecodeId GetId() const;

    template <Format format, size_t idx = 0>
    uint16_t GetVReg() const;

    template <Format format, size_t idx = 0>
    auto GetImm() const;

    BytecodeId GetId(size_t idx = 0) const;

    void UpdateId(BytecodeId new_id, uint32_t idx = 0);

    uint16_t GetVReg(size_t idx = 0) const;

    // Read imm and return it as int64_t/uint64_t
    auto GetImm64(size_t idx = 0) const;

    /**
     * Primary and Secondary Opcodes are used in interpreter/verifier instruction dispatch
     * while full Opcode is typically used for various instruction property query.
     *
     * Implementation note: one can describe Opcode in terms of Primary/Secondary opcodes
     * or vice versa. The first way is more preferable, because Primary/Secondary opcodes
     * are more performance critical and compiler is not always clever enough to reduce them
     * to simple byte reads.
     */
    BytecodeInst::Opcode GetOpcode() const;

    uint8_t GetPrimaryOpcode() const
    {
        return ReadByte(0);
    }

    bool IsPrimaryOpcodeValid() const;

    uint8_t GetSecondaryOpcode() const;

    bool IsPrefixed() const;

    static constexpr uint8_t GetMinPrefixOpcodeIndex();

    template <const BytecodeInstMode M = Mode>
    auto JumpTo(int32_t offset) const -> std::enable_if_t<M == BytecodeInstMode::FAST, BytecodeInst>
    {
        return BytecodeInst(Base::GetPointer(offset));
    }

    template <const BytecodeInstMode M = Mode>
    auto JumpTo(int32_t offset) const -> std::enable_if_t<M == BytecodeInstMode::SAFE, BytecodeInst>
    {
        if (!IsValid()) {
            return {};
        }
        const uint8_t *ptr = Base::GetPointer(offset);
        if (!IsValid()) {
            return {};
        }
        return BytecodeInst(ptr, Base::GetFrom(), Base::GetTo());
    }

    template <const BytecodeInstMode M = Mode>
    auto IsLast() const -> std::enable_if_t<M == BytecodeInstMode::SAFE, bool>
    {
        return Base::IsLast(GetSize());
    }

    template <const BytecodeInstMode M = Mode>
    auto IsValid() const -> std::enable_if_t<M == BytecodeInstMode::SAFE, bool>
    {
        return Base::IsValid();
    }

    template <Format format>
    BytecodeInst GetNext() const
    {
        return JumpTo(Size(format));
    }

    BytecodeInst GetNext() const
    {
        return JumpTo(GetSize());
    }

    const uint8_t *GetAddress() const
    {
        return Base::GetAddress();
    }

    const uint8_t *GetAddress() volatile const
    {
        return Base::GetAddress();
    }

    template <const BytecodeInstMode M = Mode>
    auto GetFrom() const -> std::enable_if_t<M == BytecodeInstMode::SAFE, const uint8_t *>
    {
        return Base::GetFrom();
    }

    template <const BytecodeInstMode M = Mode>
    auto GetTo() const -> std::enable_if_t<M == BytecodeInstMode::SAFE, const uint8_t *>
    {
        return Base::GetTo();
    }

    template <const BytecodeInstMode M = Mode>
    auto GetOffset() const -> std::enable_if_t<M == BytecodeInstMode::SAFE, uint32_t>
    {
        return Base::GetOffset();
    }

    uint8_t ReadByte(size_t offset) const
    {
        return Base::template Read<uint8_t>(offset);
    }

    template <class R, class S>
    auto ReadHelper(size_t byteoffset, size_t bytecount, size_t offset, size_t width) const;

    template <size_t offset, size_t width, bool is_signed = false>
    auto Read() const;

    template <bool is_signed = false>
    auto Read64(size_t offset, size_t width) const;

    size_t GetSize() const;

    Format GetFormat() const;

    bool HasFlag(Flags flag) const;

    bool IsThrow(Exceptions exception) const;

    bool CanThrow() const;

    bool IsTerminator() const
    {
        return HasFlag(Flags::RETURN) || HasFlag(Flags::JUMP) || IsThrow(Exceptions::X_THROW);
    }

    bool IsSuspend() const
    {
        return HasFlag(Flags::SUSPEND);
    }

    static constexpr bool HasId(Format format, size_t idx);

    static constexpr bool HasVReg(Format format, size_t idx);

    static constexpr bool HasImm(Format format, size_t idx);

    static constexpr Format GetFormat(Opcode opcode);

    static constexpr size_t Size(Format format);

    static constexpr size_t Size(Opcode opcode)
    {
        return Size(GetFormat(opcode));
    }
};

template <const BytecodeInstMode Mode>
std::ostream &operator<<(std::ostream &os, const BytecodeInst<Mode> &inst);

using BytecodeInstruction = BytecodeInst<BytecodeInstMode::FAST>;
using BytecodeInstructionSafe = BytecodeInst<BytecodeInstMode::SAFE>;

template <const BytecodeInstMode Mode>
template <class R, class S>
inline auto BytecodeInst<Mode>::ReadHelper(size_t byteoffset, size_t bytecount, size_t offset, size_t width) const
{
    constexpr size_t BYTE_WIDTH = 8;

    size_t right_shift = offset % BYTE_WIDTH;

    S v = 0;
    for (size_t i = 0; i < bytecount; i++) {
        S mask = static_cast<S>(ReadByte(byteoffset + i)) << (i * BYTE_WIDTH);
        v |= mask;
    }

    v >>= right_shift;
    size_t left_shift = sizeof(R) * BYTE_WIDTH - width;

    // Do sign extension using arithmetic shift. It's implementation defined
    // so we check such behavior using static assert
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    static_assert((-1 >> 1) == -1);

    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    return static_cast<R>(v << left_shift) >> left_shift;
}

template <const BytecodeInstMode Mode>
template <size_t offset, size_t width, bool is_signed /* = false */>
inline auto BytecodeInst<Mode>::Read() const
{
    constexpr size_t BYTE_WIDTH = 8;
    constexpr size_t BYTE_OFFSET = offset / BYTE_WIDTH;
    constexpr size_t BYTE_OFFSET_END = (offset + width + BYTE_WIDTH - 1) / BYTE_WIDTH;
    constexpr size_t BYTE_COUNT = BYTE_OFFSET_END - BYTE_OFFSET;

    using storage_type = helpers::TypeHelperT<BYTE_COUNT * BYTE_WIDTH, false>;
    using return_type = helpers::TypeHelperT<width, is_signed>;

    return ReadHelper<return_type, storage_type>(BYTE_OFFSET, BYTE_COUNT, offset, width);
}

template <const BytecodeInstMode Mode>
template <bool is_signed /* = false */>
inline auto BytecodeInst<Mode>::Read64(size_t offset, size_t width) const
{
    constexpr size_t BIT64 = 64;
    constexpr size_t BYTE_WIDTH = 8;

    ASSERT((offset % BYTE_WIDTH) + width <= BIT64);

    size_t byteoffset = offset / BYTE_WIDTH;
    size_t byteoffset_end = (offset + width + BYTE_WIDTH - 1) / BYTE_WIDTH;
    size_t bytecount = byteoffset_end - byteoffset;

    using storage_type = helpers::TypeHelperT<BIT64, false>;
    using return_type = helpers::TypeHelperT<BIT64, is_signed>;

    return ReadHelper<return_type, storage_type>(byteoffset, bytecount, offset, width);
}

template <const BytecodeInstMode Mode>
inline size_t BytecodeInst<Mode>::GetSize() const
{
    return Size(GetFormat());
}
/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

/* static */
template<const BytecodeInstMode Mode>
constexpr bool BytecodeInst<Mode>::HasId(Format format, size_t idx) {
    switch (format) {
    case Format::ID16:
        return idx < 1;
    case Format::IMM16_ID16:
        return idx < 1;
    case Format::IMM16_ID16_ID16_IMM16_V8:
        return idx < 2;
    case Format::IMM16_ID16_IMM8:
        return idx < 1;
    case Format::IMM16_ID16_V8:
        return idx < 1;
    case Format::IMM8_ID16:
        return idx < 1;
    case Format::IMM8_ID16_ID16_IMM16_V8:
        return idx < 2;
    case Format::IMM8_ID16_IMM8:
        return idx < 1;
    case Format::IMM8_ID16_V8:
        return idx < 1;
    case Format::PREF_ID16_IMM16_IMM16_V8_V8:
        return idx < 1;
    case Format::PREF_ID32:
        return idx < 1;
    case Format::PREF_ID32_IMM8:
        return idx < 1;
    case Format::PREF_ID32_V8:
        return idx < 1;
    case Format::PREF_IMM16_ID16:
        return idx < 1;
    default: {
        return false;
    }
    }

    UNREACHABLE_CONSTEXPR();
}

/* static */
template<const BytecodeInstMode Mode>
constexpr bool BytecodeInst<Mode>::HasVReg(Format format, size_t idx) {
    switch (format) {
    case Format::IMM16_ID16_ID16_IMM16_V8:
        return idx < 1;  // NOLINT(readability-magic-numbers)
    case Format::IMM16_ID16_V8:
        return idx < 1;  // NOLINT(readability-magic-numbers)
    case Format::IMM16_IMM8_V8:
        return idx < 1;  // NOLINT(readability-magic-numbers)
    case Format::IMM16_V8:
        return idx < 1;  // NOLINT(readability-magic-numbers)
    case Format::IMM16_V8_IMM16:
        return idx < 1;  // NOLINT(readability-magic-numbers)
    case Format::IMM16_V8_V8:
        return idx < 2;  // NOLINT(readability-magic-numbers)
    case Format::IMM8_ID16_ID16_IMM16_V8:
        return idx < 1;  // NOLINT(readability-magic-numbers)
    case Format::IMM8_ID16_V8:
        return idx < 1;  // NOLINT(readability-magic-numbers)
    case Format::IMM8_IMM8_V8:
        return idx < 1;  // NOLINT(readability-magic-numbers)
    case Format::IMM8_V8:
        return idx < 1;  // NOLINT(readability-magic-numbers)
    case Format::IMM8_V8_IMM16:
        return idx < 1;  // NOLINT(readability-magic-numbers)
    case Format::IMM8_V8_V8:
        return idx < 2;  // NOLINT(readability-magic-numbers)
    case Format::IMM8_V8_V8_V8:
        return idx < 3;  // NOLINT(readability-magic-numbers)
    case Format::IMM8_V8_V8_V8_V8:
        return idx < 4;  // NOLINT(readability-magic-numbers)
    case Format::PREF_ID16_IMM16_IMM16_V8_V8:
        return idx < 2;  // NOLINT(readability-magic-numbers)
    case Format::PREF_ID32_V8:
        return idx < 1;  // NOLINT(readability-magic-numbers)
    case Format::PREF_IMM16_IMM16_V8:
        return idx < 1;  // NOLINT(readability-magic-numbers)
    case Format::PREF_IMM16_V8:
        return idx < 1;  // NOLINT(readability-magic-numbers)
    case Format::PREF_IMM16_V8_V8:
        return idx < 2;  // NOLINT(readability-magic-numbers)
    case Format::PREF_IMM4_IMM4_V8:
        return idx < 1;  // NOLINT(readability-magic-numbers)
    case Format::PREF_IMM8_IMM8_V8:
        return idx < 1;  // NOLINT(readability-magic-numbers)
    case Format::PREF_V8:
        return idx < 1;  // NOLINT(readability-magic-numbers)
    case Format::PREF_V8_IMM32:
        return idx < 1;  // NOLINT(readability-magic-numbers)
    case Format::PREF_V8_V8:
        return idx < 2;  // NOLINT(readability-magic-numbers)
    case Format::PREF_V8_V8_V8:
        return idx < 3;  // NOLINT(readability-magic-numbers)
    case Format::PREF_V8_V8_V8_V8:
        return idx < 4;  // NOLINT(readability-magic-numbers)
    case Format::V16_V16:
        return idx < 2;  // NOLINT(readability-magic-numbers)
    case Format::V4_V4:
        return idx < 2;  // NOLINT(readability-magic-numbers)
    case Format::V8:
        return idx < 1;  // NOLINT(readability-magic-numbers)
    case Format::V8_IMM16:
        return idx < 1;  // NOLINT(readability-magic-numbers)
    case Format::V8_IMM8:
        return idx < 1;  // NOLINT(readability-magic-numbers)
    case Format::V8_V8:
        return idx < 2;  // NOLINT(readability-magic-numbers)
    case Format::V8_V8_V8:
        return idx < 3;  // NOLINT(readability-magic-numbers)
    case Format::V8_V8_V8_V8:
        return idx < 4;  // NOLINT(readability-magic-numbers)
    default: {
        return false;
    }
    }

    UNREACHABLE_CONSTEXPR();
}

/* static */
template<const BytecodeInstMode Mode>
constexpr bool BytecodeInst<Mode>::HasImm(Format format, size_t idx) {
    switch (format) {
    case Format::IMM16:
        return idx < 1;
    case Format::IMM16_ID16:
        return idx < 1;
    case Format::IMM16_ID16_ID16_IMM16_V8:
        return idx < 2;
    case Format::IMM16_ID16_IMM8:
        return idx < 2;
    case Format::IMM16_ID16_V8:
        return idx < 1;
    case Format::IMM16_IMM16:
        return idx < 2;
    case Format::IMM16_IMM8_V8:
        return idx < 2;
    case Format::IMM16_V8:
        return idx < 1;
    case Format::IMM16_V8_IMM16:
        return idx < 2;
    case Format::IMM16_V8_V8:
        return idx < 1;
    case Format::IMM32:
        return idx < 1;
    case Format::IMM4_IMM4:
        return idx < 2;
    case Format::IMM64:
        return idx < 1;
    case Format::IMM8:
        return idx < 1;
    case Format::IMM8_ID16:
        return idx < 1;
    case Format::IMM8_ID16_ID16_IMM16_V8:
        return idx < 2;
    case Format::IMM8_ID16_IMM8:
        return idx < 2;
    case Format::IMM8_ID16_V8:
        return idx < 1;
    case Format::IMM8_IMM16:
        return idx < 2;
    case Format::IMM8_IMM8:
        return idx < 2;
    case Format::IMM8_IMM8_V8:
        return idx < 2;
    case Format::IMM8_V8:
        return idx < 1;
    case Format::IMM8_V8_IMM16:
        return idx < 2;
    case Format::IMM8_V8_V8:
        return idx < 1;
    case Format::IMM8_V8_V8_V8:
        return idx < 1;
    case Format::IMM8_V8_V8_V8_V8:
        return idx < 1;
    case Format::PREF_ID16_IMM16_IMM16_V8_V8:
        return idx < 2;
    case Format::PREF_ID32_IMM8:
        return idx < 1;
    case Format::PREF_IMM16:
        return idx < 1;
    case Format::PREF_IMM16_ID16:
        return idx < 1;
    case Format::PREF_IMM16_IMM16:
        return idx < 2;
    case Format::PREF_IMM16_IMM16_V8:
        return idx < 2;
    case Format::PREF_IMM16_V8:
        return idx < 1;
    case Format::PREF_IMM16_V8_V8:
        return idx < 1;
    case Format::PREF_IMM32:
        return idx < 1;
    case Format::PREF_IMM4_IMM4_V8:
        return idx < 2;
    case Format::PREF_IMM8:
        return idx < 1;
    case Format::PREF_IMM8_IMM8_V8:
        return idx < 2;
    case Format::PREF_V8_IMM32:
        return idx < 1;
    case Format::V8_IMM16:
        return idx < 1;
    case Format::V8_IMM8:
        return idx < 1;
    default: {
        return false;
    }
    }

    UNREACHABLE_CONSTEXPR();
}

/* static */
template<const BytecodeInstMode Mode>
constexpr size_t BytecodeInst<Mode>::Size(Format format) {  // NOLINTNEXTLINE(readability-function-size)
    switch (format) {
    case Format::ID16: {
        constexpr size_t SIZE = 3;
        return SIZE;
    }
    case Format::IMM16: {
        constexpr size_t SIZE = 3;
        return SIZE;
    }
    case Format::IMM16_ID16: {
        constexpr size_t SIZE = 5;
        return SIZE;
    }
    case Format::IMM16_ID16_ID16_IMM16_V8: {
        constexpr size_t SIZE = 10;
        return SIZE;
    }
    case Format::IMM16_ID16_IMM8: {
        constexpr size_t SIZE = 6;
        return SIZE;
    }
    case Format::IMM16_ID16_V8: {
        constexpr size_t SIZE = 6;
        return SIZE;
    }
    case Format::IMM16_IMM16: {
        constexpr size_t SIZE = 5;
        return SIZE;
    }
    case Format::IMM16_IMM8_V8: {
        constexpr size_t SIZE = 5;
        return SIZE;
    }
    case Format::IMM16_V8: {
        constexpr size_t SIZE = 4;
        return SIZE;
    }
    case Format::IMM16_V8_IMM16: {
        constexpr size_t SIZE = 6;
        return SIZE;
    }
    case Format::IMM16_V8_V8: {
        constexpr size_t SIZE = 5;
        return SIZE;
    }
    case Format::IMM32: {
        constexpr size_t SIZE = 5;
        return SIZE;
    }
    case Format::IMM4_IMM4: {
        constexpr size_t SIZE = 2;
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
    case Format::IMM8_ID16: {
        constexpr size_t SIZE = 4;
        return SIZE;
    }
    case Format::IMM8_ID16_ID16_IMM16_V8: {
        constexpr size_t SIZE = 9;
        return SIZE;
    }
    case Format::IMM8_ID16_IMM8: {
        constexpr size_t SIZE = 5;
        return SIZE;
    }
    case Format::IMM8_ID16_V8: {
        constexpr size_t SIZE = 5;
        return SIZE;
    }
    case Format::IMM8_IMM16: {
        constexpr size_t SIZE = 4;
        return SIZE;
    }
    case Format::IMM8_IMM8: {
        constexpr size_t SIZE = 3;
        return SIZE;
    }
    case Format::IMM8_IMM8_V8: {
        constexpr size_t SIZE = 4;
        return SIZE;
    }
    case Format::IMM8_V8: {
        constexpr size_t SIZE = 3;
        return SIZE;
    }
    case Format::IMM8_V8_IMM16: {
        constexpr size_t SIZE = 5;
        return SIZE;
    }
    case Format::IMM8_V8_V8: {
        constexpr size_t SIZE = 4;
        return SIZE;
    }
    case Format::IMM8_V8_V8_V8: {
        constexpr size_t SIZE = 5;
        return SIZE;
    }
    case Format::IMM8_V8_V8_V8_V8: {
        constexpr size_t SIZE = 6;
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
    case Format::PREF_IMM16_ID16: {
        constexpr size_t SIZE = 6;
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
    case Format::PREF_IMM4_IMM4_V8: {
        constexpr size_t SIZE = 4;
        return SIZE;
    }
    case Format::PREF_IMM8: {
        constexpr size_t SIZE = 3;
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
    case Format::V4_V4: {
        constexpr size_t SIZE = 2;
        return SIZE;
    }
    case Format::V8: {
        constexpr size_t SIZE = 2;
        return SIZE;
    }
    case Format::V8_IMM16: {
        constexpr size_t SIZE = 4;
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
    case Format::V8_V8_V8: {
        constexpr size_t SIZE = 4;
        return SIZE;
    }
    case Format::V8_V8_V8_V8: {
        constexpr size_t SIZE = 5;
        return SIZE;
    }
    }

    UNREACHABLE_CONSTEXPR();
}

template <const BytecodeInstMode Mode>
template <typename BytecodeInst<Mode>::Format format, size_t idx /* = 0 */>
inline BytecodeId BytecodeInst<Mode>::GetId() const {
    static_assert(HasId(format, idx), "Instruction doesn't have id operand with such index");

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::ID16) {
        return BytecodeId(static_cast<uint32_t>(Read<8, 16>()));
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::IMM16_ID16) {
        return BytecodeId(static_cast<uint32_t>(Read<24, 16>()));
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::IMM16_ID16_ID16_IMM16_V8) {
        constexpr std::array<size_t, 2> OFFSETS{24, 40};
        constexpr std::array<size_t, 2> WIDTHS{16, 16};
        return BytecodeId(static_cast<uint32_t>(Read<OFFSETS[idx], WIDTHS[idx]>()));
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::IMM16_ID16_IMM8) {
        return BytecodeId(static_cast<uint32_t>(Read<24, 16>()));
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::IMM16_ID16_V8) {
        return BytecodeId(static_cast<uint32_t>(Read<24, 16>()));
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::IMM8_ID16) {
        return BytecodeId(static_cast<uint32_t>(Read<16, 16>()));
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::IMM8_ID16_ID16_IMM16_V8) {
        constexpr std::array<size_t, 2> OFFSETS{16, 32};
        constexpr std::array<size_t, 2> WIDTHS{16, 16};
        return BytecodeId(static_cast<uint32_t>(Read<OFFSETS[idx], WIDTHS[idx]>()));
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::IMM8_ID16_IMM8) {
        return BytecodeId(static_cast<uint32_t>(Read<16, 16>()));
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::IMM8_ID16_V8) {
        return BytecodeId(static_cast<uint32_t>(Read<16, 16>()));
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::PREF_ID16_IMM16_IMM16_V8_V8) {
        return BytecodeId(static_cast<uint32_t>(Read<16, 16>()));
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::PREF_ID32) {
        return BytecodeId(static_cast<uint32_t>(Read<16, 32>()));
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::PREF_ID32_IMM8) {
        return BytecodeId(static_cast<uint32_t>(Read<16, 32>()));
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::PREF_ID32_V8) {
        return BytecodeId(static_cast<uint32_t>(Read<16, 32>()));
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::PREF_IMM16_ID16) {
        return BytecodeId(static_cast<uint32_t>(Read<32, 16>()));
    }

    UNREACHABLE();
}

template <const BytecodeInstMode Mode>
inline void BytecodeInst<Mode>::UpdateId(BytecodeId new_id, uint32_t idx /* = 0 */) {
    Format format = GetFormat();
    ASSERT_PRINT(HasId(format, idx), "Instruction doesn't have imm operand with such index");

    if (!HasId(format, idx)) {
        return;
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::ID16) {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{16};
        this->Write(new_id.AsRawValue(), OFFSETS[idx] / 8, WIDTHS[idx] / 8);
        return;
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::IMM16_ID16) {
        constexpr std::array<size_t, 1> OFFSETS{24};
        constexpr std::array<size_t, 1> WIDTHS{16};
        this->Write(new_id.AsRawValue(), OFFSETS[idx] / 8, WIDTHS[idx] / 8);
        return;
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::IMM16_ID16_ID16_IMM16_V8) {
        constexpr std::array<size_t, 2> OFFSETS{24, 40};
        constexpr std::array<size_t, 2> WIDTHS{16, 16};
        this->Write(new_id.AsRawValue(), OFFSETS[idx] / 8, WIDTHS[idx] / 8);
        return;
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::IMM16_ID16_IMM8) {
        constexpr std::array<size_t, 1> OFFSETS{24};
        constexpr std::array<size_t, 1> WIDTHS{16};
        this->Write(new_id.AsRawValue(), OFFSETS[idx] / 8, WIDTHS[idx] / 8);
        return;
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::IMM16_ID16_V8) {
        constexpr std::array<size_t, 1> OFFSETS{24};
        constexpr std::array<size_t, 1> WIDTHS{16};
        this->Write(new_id.AsRawValue(), OFFSETS[idx] / 8, WIDTHS[idx] / 8);
        return;
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::IMM8_ID16) {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{16};
        this->Write(new_id.AsRawValue(), OFFSETS[idx] / 8, WIDTHS[idx] / 8);
        return;
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::IMM8_ID16_ID16_IMM16_V8) {
        constexpr std::array<size_t, 2> OFFSETS{16, 32};
        constexpr std::array<size_t, 2> WIDTHS{16, 16};
        this->Write(new_id.AsRawValue(), OFFSETS[idx] / 8, WIDTHS[idx] / 8);
        return;
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::IMM8_ID16_IMM8) {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{16};
        this->Write(new_id.AsRawValue(), OFFSETS[idx] / 8, WIDTHS[idx] / 8);
        return;
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::IMM8_ID16_V8) {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{16};
        this->Write(new_id.AsRawValue(), OFFSETS[idx] / 8, WIDTHS[idx] / 8);
        return;
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::PREF_ID16_IMM16_IMM16_V8_V8) {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{16};
        this->Write(new_id.AsRawValue(), OFFSETS[idx] / 8, WIDTHS[idx] / 8);
        return;
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::PREF_ID32) {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{32};
        this->Write(new_id.AsRawValue(), OFFSETS[idx] / 8, WIDTHS[idx] / 8);
        return;
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::PREF_ID32_IMM8) {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{32};
        this->Write(new_id.AsRawValue(), OFFSETS[idx] / 8, WIDTHS[idx] / 8);
        return;
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::PREF_ID32_V8) {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{32};
        this->Write(new_id.AsRawValue(), OFFSETS[idx] / 8, WIDTHS[idx] / 8);
        return;
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if (format == Format::PREF_IMM16_ID16) {
        constexpr std::array<size_t, 1> OFFSETS{32};
        constexpr std::array<size_t, 1> WIDTHS{16};
        this->Write(new_id.AsRawValue(), OFFSETS[idx] / 8, WIDTHS[idx] / 8);
        return;
    }

    UNREACHABLE();
}

template<const BytecodeInstMode Mode>
inline BytecodeId BytecodeInst<Mode>::GetId(size_t idx /* = 0 */) const {
    Format format = GetFormat();
    ASSERT_PRINT(HasId(format, idx), "Instruction doesn't have id operand with such index");

    if (!HasId(format, idx)) {
        return {};
    }

    switch (format) {
    case Format::ID16: {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return BytecodeId(static_cast<uint32_t>(Read64(OFFSETS[idx], WIDTHS[idx])));
    }
    case Format::IMM16_ID16: {
        constexpr std::array<size_t, 1> OFFSETS{24};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return BytecodeId(static_cast<uint32_t>(Read64(OFFSETS[idx], WIDTHS[idx])));
    }
    case Format::IMM16_ID16_ID16_IMM16_V8: {
        constexpr std::array<size_t, 2> OFFSETS{24, 40};
        constexpr std::array<size_t, 2> WIDTHS{16, 16};
        return BytecodeId(static_cast<uint32_t>(Read64(OFFSETS[idx], WIDTHS[idx])));
    }
    case Format::IMM16_ID16_IMM8: {
        constexpr std::array<size_t, 1> OFFSETS{24};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return BytecodeId(static_cast<uint32_t>(Read64(OFFSETS[idx], WIDTHS[idx])));
    }
    case Format::IMM16_ID16_V8: {
        constexpr std::array<size_t, 1> OFFSETS{24};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return BytecodeId(static_cast<uint32_t>(Read64(OFFSETS[idx], WIDTHS[idx])));
    }
    case Format::IMM8_ID16: {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return BytecodeId(static_cast<uint32_t>(Read64(OFFSETS[idx], WIDTHS[idx])));
    }
    case Format::IMM8_ID16_ID16_IMM16_V8: {
        constexpr std::array<size_t, 2> OFFSETS{16, 32};
        constexpr std::array<size_t, 2> WIDTHS{16, 16};
        return BytecodeId(static_cast<uint32_t>(Read64(OFFSETS[idx], WIDTHS[idx])));
    }
    case Format::IMM8_ID16_IMM8: {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return BytecodeId(static_cast<uint32_t>(Read64(OFFSETS[idx], WIDTHS[idx])));
    }
    case Format::IMM8_ID16_V8: {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return BytecodeId(static_cast<uint32_t>(Read64(OFFSETS[idx], WIDTHS[idx])));
    }
    case Format::PREF_ID16_IMM16_IMM16_V8_V8: {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return BytecodeId(static_cast<uint32_t>(Read64(OFFSETS[idx], WIDTHS[idx])));
    }
    case Format::PREF_ID32: {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{32};
        return BytecodeId(static_cast<uint32_t>(Read64(OFFSETS[idx], WIDTHS[idx])));
    }
    case Format::PREF_ID32_IMM8: {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{32};
        return BytecodeId(static_cast<uint32_t>(Read64(OFFSETS[idx], WIDTHS[idx])));
    }
    case Format::PREF_ID32_V8: {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{32};
        return BytecodeId(static_cast<uint32_t>(Read64(OFFSETS[idx], WIDTHS[idx])));
    }
    case Format::PREF_IMM16_ID16: {
        constexpr std::array<size_t, 1> OFFSETS{32};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return BytecodeId(static_cast<uint32_t>(Read64(OFFSETS[idx], WIDTHS[idx])));
    }
    default: {
        break;
    }
    }

    UNREACHABLE();
}

template <const BytecodeInstMode Mode>
template <typename BytecodeInst<Mode>::Format format, size_t idx /* = 0 */>
__attribute__ ((visibility("hidden")))
ALWAYS_INLINE inline uint16_t BytecodeInst<Mode>::GetVReg() const {  // NOLINTNEXTLINE(readability-function-size)
    static_assert(HasVReg(format, idx), "Instruction doesn't have vreg operand with such index");

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM16_ID16_ID16_IMM16_V8) {
        constexpr std::array<size_t, 1> OFFSETS{72};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM16_ID16_V8) {
        constexpr std::array<size_t, 1> OFFSETS{40};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM16_IMM8_V8) {
        constexpr std::array<size_t, 1> OFFSETS{32};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM16_V8) {
        constexpr std::array<size_t, 1> OFFSETS{24};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM16_V8_IMM16) {
        constexpr std::array<size_t, 1> OFFSETS{24};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM16_V8_V8) {
        constexpr std::array<size_t, 2> OFFSETS{24, 32};
        constexpr std::array<size_t, 2> WIDTHS{8, 8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8_ID16_ID16_IMM16_V8) {
        constexpr std::array<size_t, 1> OFFSETS{64};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8_ID16_V8) {
        constexpr std::array<size_t, 1> OFFSETS{32};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8_IMM8_V8) {
        constexpr std::array<size_t, 1> OFFSETS{24};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8_V8) {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8_V8_IMM16) {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8_V8_V8) {
        constexpr std::array<size_t, 2> OFFSETS{16, 24};
        constexpr std::array<size_t, 2> WIDTHS{8, 8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8_V8_V8_V8) {
        constexpr std::array<size_t, 3> OFFSETS{16, 24, 32};
        constexpr std::array<size_t, 3> WIDTHS{8, 8, 8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8_V8_V8_V8_V8) {
        constexpr std::array<size_t, 4> OFFSETS{16, 24, 32, 40};
        constexpr std::array<size_t, 4> WIDTHS{8, 8, 8, 8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_ID16_IMM16_IMM16_V8_V8) {
        constexpr std::array<size_t, 2> OFFSETS{64, 72};
        constexpr std::array<size_t, 2> WIDTHS{8, 8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_ID32_V8) {
        constexpr std::array<size_t, 1> OFFSETS{48};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_IMM16_IMM16_V8) {
        constexpr std::array<size_t, 1> OFFSETS{48};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_IMM16_V8) {
        constexpr std::array<size_t, 1> OFFSETS{32};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_IMM16_V8_V8) {
        constexpr std::array<size_t, 2> OFFSETS{32, 40};
        constexpr std::array<size_t, 2> WIDTHS{8, 8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_IMM4_IMM4_V8) {
        constexpr std::array<size_t, 1> OFFSETS{24};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_IMM8_IMM8_V8) {
        constexpr std::array<size_t, 1> OFFSETS{32};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_V8) {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_V8_IMM32) {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_V8_V8) {
        constexpr std::array<size_t, 2> OFFSETS{16, 24};
        constexpr std::array<size_t, 2> WIDTHS{8, 8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_V8_V8_V8) {
        constexpr std::array<size_t, 3> OFFSETS{16, 24, 32};
        constexpr std::array<size_t, 3> WIDTHS{8, 8, 8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_V8_V8_V8_V8) {
        constexpr std::array<size_t, 4> OFFSETS{16, 24, 32, 40};
        constexpr std::array<size_t, 4> WIDTHS{8, 8, 8, 8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::V16_V16) {
        constexpr std::array<size_t, 2> OFFSETS{8, 24};
        constexpr std::array<size_t, 2> WIDTHS{16, 16};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::V4_V4) {
        constexpr std::array<size_t, 2> OFFSETS{8, 12};
        constexpr std::array<size_t, 2> WIDTHS{4, 4};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::V8) {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::V8_IMM16) {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::V8_IMM8) {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::V8_V8) {
        constexpr std::array<size_t, 2> OFFSETS{8, 16};
        constexpr std::array<size_t, 2> WIDTHS{8, 8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::V8_V8_V8) {
        constexpr std::array<size_t, 3> OFFSETS{8, 16, 24};
        constexpr std::array<size_t, 3> WIDTHS{8, 8, 8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::V8_V8_V8_V8) {
        constexpr std::array<size_t, 4> OFFSETS{8, 16, 24, 32};
        constexpr std::array<size_t, 4> WIDTHS{8, 8, 8, 8};
        return static_cast<uint16_t>(Read<OFFSETS[idx], WIDTHS[idx]>());
    }

    UNREACHABLE();
}

template<const BytecodeInstMode Mode>
__attribute__ ((visibility("hidden")))
ALWAYS_INLINE inline uint16_t BytecodeInst<Mode>::GetVReg(size_t idx /* = 0 */) const {  // NOLINTNEXTLINE(readability-function-size)
    Format format = GetFormat();
    ASSERT_PRINT(HasVReg(format, idx), "Instruction doesn't have vreg operand with such index");

    if (!HasVReg(format, idx)) {
        return 0;
    }

    switch (format) {
    case Format::IMM16_ID16_ID16_IMM16_V8: {
        constexpr std::array<size_t, 1> OFFSETS{72};
        constexpr std::array<size_t, 1> WIDTHS{8};
        if (idx > 0) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::IMM16_ID16_V8: {
        constexpr std::array<size_t, 1> OFFSETS{40};
        constexpr std::array<size_t, 1> WIDTHS{8};
        if (idx > 0) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::IMM16_IMM8_V8: {
        constexpr std::array<size_t, 1> OFFSETS{32};
        constexpr std::array<size_t, 1> WIDTHS{8};
        if (idx > 0) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::IMM16_V8: {
        constexpr std::array<size_t, 1> OFFSETS{24};
        constexpr std::array<size_t, 1> WIDTHS{8};
        if (idx > 0) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::IMM16_V8_IMM16: {
        constexpr std::array<size_t, 1> OFFSETS{24};
        constexpr std::array<size_t, 1> WIDTHS{8};
        if (idx > 0) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::IMM16_V8_V8: {
        constexpr std::array<size_t, 2> OFFSETS{24, 32};
        constexpr std::array<size_t, 2> WIDTHS{8, 8};
        if (idx > 1) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::IMM8_ID16_ID16_IMM16_V8: {
        constexpr std::array<size_t, 1> OFFSETS{64};
        constexpr std::array<size_t, 1> WIDTHS{8};
        if (idx > 0) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::IMM8_ID16_V8: {
        constexpr std::array<size_t, 1> OFFSETS{32};
        constexpr std::array<size_t, 1> WIDTHS{8};
        if (idx > 0) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::IMM8_IMM8_V8: {
        constexpr std::array<size_t, 1> OFFSETS{24};
        constexpr std::array<size_t, 1> WIDTHS{8};
        if (idx > 0) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::IMM8_V8: {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{8};
        if (idx > 0) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::IMM8_V8_IMM16: {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{8};
        if (idx > 0) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::IMM8_V8_V8: {
        constexpr std::array<size_t, 2> OFFSETS{16, 24};
        constexpr std::array<size_t, 2> WIDTHS{8, 8};
        if (idx > 1) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::IMM8_V8_V8_V8: {
        constexpr std::array<size_t, 3> OFFSETS{16, 24, 32};
        constexpr std::array<size_t, 3> WIDTHS{8, 8, 8};
        if (idx > 2) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::IMM8_V8_V8_V8_V8: {
        constexpr std::array<size_t, 4> OFFSETS{16, 24, 32, 40};
        constexpr std::array<size_t, 4> WIDTHS{8, 8, 8, 8};
        if (idx > 3) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::PREF_ID16_IMM16_IMM16_V8_V8: {
        constexpr std::array<size_t, 2> OFFSETS{64, 72};
        constexpr std::array<size_t, 2> WIDTHS{8, 8};
        if (idx > 1) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::PREF_ID32_V8: {
        constexpr std::array<size_t, 1> OFFSETS{48};
        constexpr std::array<size_t, 1> WIDTHS{8};
        if (idx > 0) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::PREF_IMM16_IMM16_V8: {
        constexpr std::array<size_t, 1> OFFSETS{48};
        constexpr std::array<size_t, 1> WIDTHS{8};
        if (idx > 0) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::PREF_IMM16_V8: {
        constexpr std::array<size_t, 1> OFFSETS{32};
        constexpr std::array<size_t, 1> WIDTHS{8};
        if (idx > 0) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::PREF_IMM16_V8_V8: {
        constexpr std::array<size_t, 2> OFFSETS{32, 40};
        constexpr std::array<size_t, 2> WIDTHS{8, 8};
        if (idx > 1) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::PREF_IMM4_IMM4_V8: {
        constexpr std::array<size_t, 1> OFFSETS{24};
        constexpr std::array<size_t, 1> WIDTHS{8};
        if (idx > 0) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::PREF_IMM8_IMM8_V8: {
        constexpr std::array<size_t, 1> OFFSETS{32};
        constexpr std::array<size_t, 1> WIDTHS{8};
        if (idx > 0) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::PREF_V8: {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{8};
        if (idx > 0) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::PREF_V8_IMM32: {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{8};
        if (idx > 0) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::PREF_V8_V8: {
        constexpr std::array<size_t, 2> OFFSETS{16, 24};
        constexpr std::array<size_t, 2> WIDTHS{8, 8};
        if (idx > 1) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::PREF_V8_V8_V8: {
        constexpr std::array<size_t, 3> OFFSETS{16, 24, 32};
        constexpr std::array<size_t, 3> WIDTHS{8, 8, 8};
        if (idx > 2) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::PREF_V8_V8_V8_V8: {
        constexpr std::array<size_t, 4> OFFSETS{16, 24, 32, 40};
        constexpr std::array<size_t, 4> WIDTHS{8, 8, 8, 8};
        if (idx > 3) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::V16_V16: {
        constexpr std::array<size_t, 2> OFFSETS{8, 24};
        constexpr std::array<size_t, 2> WIDTHS{16, 16};
        if (idx > 1) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::V4_V4: {
        constexpr std::array<size_t, 2> OFFSETS{8, 12};
        constexpr std::array<size_t, 2> WIDTHS{4, 4};
        if (idx > 1) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::V8: {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{8};
        if (idx > 0) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::V8_IMM16: {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{8};
        if (idx > 0) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::V8_IMM8: {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{8};
        if (idx > 0) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::V8_V8: {
        constexpr std::array<size_t, 2> OFFSETS{8, 16};
        constexpr std::array<size_t, 2> WIDTHS{8, 8};
        if (idx > 1) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::V8_V8_V8: {
        constexpr std::array<size_t, 3> OFFSETS{8, 16, 24};
        constexpr std::array<size_t, 3> WIDTHS{8, 8, 8};
        if (idx > 2) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    case Format::V8_V8_V8_V8: {
        constexpr std::array<size_t, 4> OFFSETS{8, 16, 24, 32};
        constexpr std::array<size_t, 4> WIDTHS{8, 8, 8, 8};
        if (idx > 3) {
            break;
        }
        return static_cast<uint16_t>(Read64(OFFSETS[idx], WIDTHS[idx]));
    }
    default: {
        break;
    }
    }

    UNREACHABLE();
}

template <const BytecodeInstMode Mode>
template <typename BytecodeInst<Mode>::Format format, size_t idx /* = 0 */>
inline auto BytecodeInst<Mode>::GetImm() const {  // NOLINTNEXTLINE(readability-function-size)
    static_assert(HasImm(format, idx), "Instruction doesn't have imm operand with such index");

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM16) {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM16_ID16) {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM16_ID16_ID16_IMM16_V8) {
        constexpr std::array<size_t, 2> OFFSETS{8, 56};
        constexpr std::array<size_t, 2> WIDTHS{16, 16};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM16_ID16_IMM8) {
        constexpr std::array<size_t, 2> OFFSETS{8, 40};
        constexpr std::array<size_t, 2> WIDTHS{16, 8};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM16_ID16_V8) {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM16_IMM16) {
        constexpr std::array<size_t, 2> OFFSETS{8, 24};
        constexpr std::array<size_t, 2> WIDTHS{16, 16};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM16_IMM8_V8) {
        constexpr std::array<size_t, 2> OFFSETS{8, 24};
        constexpr std::array<size_t, 2> WIDTHS{16, 8};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM16_V8) {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM16_V8_IMM16) {
        constexpr std::array<size_t, 2> OFFSETS{8, 32};
        constexpr std::array<size_t, 2> WIDTHS{16, 16};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM16_V8_V8) {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM32) {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{32};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM4_IMM4) {
        constexpr std::array<size_t, 2> OFFSETS{8, 12};
        constexpr std::array<size_t, 2> WIDTHS{4, 4};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM64) {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{64};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8) {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8_ID16) {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8_ID16_ID16_IMM16_V8) {
        constexpr std::array<size_t, 2> OFFSETS{8, 48};
        constexpr std::array<size_t, 2> WIDTHS{8, 16};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8_ID16_IMM8) {
        constexpr std::array<size_t, 2> OFFSETS{8, 32};
        constexpr std::array<size_t, 2> WIDTHS{8, 8};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8_ID16_V8) {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8_IMM16) {
        constexpr std::array<size_t, 2> OFFSETS{8, 16};
        constexpr std::array<size_t, 2> WIDTHS{8, 16};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8_IMM8) {
        constexpr std::array<size_t, 2> OFFSETS{8, 16};
        constexpr std::array<size_t, 2> WIDTHS{8, 8};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8_IMM8_V8) {
        constexpr std::array<size_t, 2> OFFSETS{8, 16};
        constexpr std::array<size_t, 2> WIDTHS{8, 8};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8_V8) {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8_V8_IMM16) {
        constexpr std::array<size_t, 2> OFFSETS{8, 24};
        constexpr std::array<size_t, 2> WIDTHS{8, 16};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8_V8_V8) {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8_V8_V8_V8) {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::IMM8_V8_V8_V8_V8) {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_ID16_IMM16_IMM16_V8_V8) {
        constexpr std::array<size_t, 2> OFFSETS{32, 48};
        constexpr std::array<size_t, 2> WIDTHS{16, 16};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_ID32_IMM8) {
        constexpr std::array<size_t, 1> OFFSETS{48};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_IMM16) {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_IMM16_ID16) {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_IMM16_IMM16) {
        constexpr std::array<size_t, 2> OFFSETS{16, 32};
        constexpr std::array<size_t, 2> WIDTHS{16, 16};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_IMM16_IMM16_V8) {
        constexpr std::array<size_t, 2> OFFSETS{16, 32};
        constexpr std::array<size_t, 2> WIDTHS{16, 16};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_IMM16_V8) {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_IMM16_V8_V8) {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_IMM32) {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{32};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_IMM4_IMM4_V8) {
        constexpr std::array<size_t, 2> OFFSETS{16, 20};
        constexpr std::array<size_t, 2> WIDTHS{4, 4};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_IMM8) {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_IMM8_IMM8_V8) {
        constexpr std::array<size_t, 2> OFFSETS{16, 24};
        constexpr std::array<size_t, 2> WIDTHS{8, 8};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::PREF_V8_IMM32) {
        constexpr std::array<size_t, 1> OFFSETS{24};
        constexpr std::array<size_t, 1> WIDTHS{32};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::V8_IMM16) {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    // Disable check due to clang-tidy bug https://bugs.llvm.org/show_bug.cgi?id=32203
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (format == Format::V8_IMM8) {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return Read<OFFSETS[idx], WIDTHS[idx], true>();
    }

    UNREACHABLE();
}

template<const BytecodeInstMode Mode>
inline auto BytecodeInst<Mode>::GetImm64(size_t idx /* = 0 */) const {
    Format format = GetFormat();
    ASSERT_PRINT(HasImm(format, idx), "Instruction doesn't have imm operand with such index");

    if (!HasImm(format, idx)) {
        return static_cast<int64_t>(0);
    }

    switch (format) {
    case Format::IMM16: {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM16_ID16: {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM16_ID16_ID16_IMM16_V8: {
        constexpr std::array<size_t, 2> OFFSETS{8, 56};
        constexpr std::array<size_t, 2> WIDTHS{16, 16};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM16_ID16_IMM8: {
        constexpr std::array<size_t, 2> OFFSETS{8, 40};
        constexpr std::array<size_t, 2> WIDTHS{16, 8};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM16_ID16_V8: {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM16_IMM16: {
        constexpr std::array<size_t, 2> OFFSETS{8, 24};
        constexpr std::array<size_t, 2> WIDTHS{16, 16};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM16_IMM8_V8: {
        constexpr std::array<size_t, 2> OFFSETS{8, 24};
        constexpr std::array<size_t, 2> WIDTHS{16, 8};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM16_V8: {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM16_V8_IMM16: {
        constexpr std::array<size_t, 2> OFFSETS{8, 32};
        constexpr std::array<size_t, 2> WIDTHS{16, 16};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM16_V8_V8: {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM32: {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{32};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM4_IMM4: {
        constexpr std::array<size_t, 2> OFFSETS{8, 12};
        constexpr std::array<size_t, 2> WIDTHS{4, 4};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM64: {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{64};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM8: {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM8_ID16: {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM8_ID16_ID16_IMM16_V8: {
        constexpr std::array<size_t, 2> OFFSETS{8, 48};
        constexpr std::array<size_t, 2> WIDTHS{8, 16};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM8_ID16_IMM8: {
        constexpr std::array<size_t, 2> OFFSETS{8, 32};
        constexpr std::array<size_t, 2> WIDTHS{8, 8};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM8_ID16_V8: {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM8_IMM16: {
        constexpr std::array<size_t, 2> OFFSETS{8, 16};
        constexpr std::array<size_t, 2> WIDTHS{8, 16};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM8_IMM8: {
        constexpr std::array<size_t, 2> OFFSETS{8, 16};
        constexpr std::array<size_t, 2> WIDTHS{8, 8};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM8_IMM8_V8: {
        constexpr std::array<size_t, 2> OFFSETS{8, 16};
        constexpr std::array<size_t, 2> WIDTHS{8, 8};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM8_V8: {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM8_V8_IMM16: {
        constexpr std::array<size_t, 2> OFFSETS{8, 24};
        constexpr std::array<size_t, 2> WIDTHS{8, 16};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM8_V8_V8: {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM8_V8_V8_V8: {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::IMM8_V8_V8_V8_V8: {
        constexpr std::array<size_t, 1> OFFSETS{8};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::PREF_ID16_IMM16_IMM16_V8_V8: {
        constexpr std::array<size_t, 2> OFFSETS{32, 48};
        constexpr std::array<size_t, 2> WIDTHS{16, 16};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::PREF_ID32_IMM8: {
        constexpr std::array<size_t, 1> OFFSETS{48};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::PREF_IMM16: {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::PREF_IMM16_ID16: {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::PREF_IMM16_IMM16: {
        constexpr std::array<size_t, 2> OFFSETS{16, 32};
        constexpr std::array<size_t, 2> WIDTHS{16, 16};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::PREF_IMM16_IMM16_V8: {
        constexpr std::array<size_t, 2> OFFSETS{16, 32};
        constexpr std::array<size_t, 2> WIDTHS{16, 16};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::PREF_IMM16_V8: {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::PREF_IMM16_V8_V8: {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::PREF_IMM32: {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{32};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::PREF_IMM4_IMM4_V8: {
        constexpr std::array<size_t, 2> OFFSETS{16, 20};
        constexpr std::array<size_t, 2> WIDTHS{4, 4};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::PREF_IMM8: {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::PREF_IMM8_IMM8_V8: {
        constexpr std::array<size_t, 2> OFFSETS{16, 24};
        constexpr std::array<size_t, 2> WIDTHS{8, 8};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::PREF_V8_IMM32: {
        constexpr std::array<size_t, 1> OFFSETS{24};
        constexpr std::array<size_t, 1> WIDTHS{32};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::V8_IMM16: {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{16};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    case Format::V8_IMM8: {
        constexpr std::array<size_t, 1> OFFSETS{16};
        constexpr std::array<size_t, 1> WIDTHS{8};
        return Read64<true>(OFFSETS[idx], WIDTHS[idx]);
    }
    default: {
        break;
    }
    }

    UNREACHABLE();
}

template <const BytecodeInstMode Mode>
inline typename BytecodeInst<Mode>::Opcode BytecodeInst<Mode>::GetOpcode() const {
    uint8_t primary = GetPrimaryOpcode();
    if (primary >= 252) {  // NOLINT(readability-magic-numbers)
        uint8_t secondary = GetSecondaryOpcode();
        return static_cast<BytecodeInst::Opcode>((secondary << 8U) | primary);  // NOLINT(hicpp-signed-bitwise)
    }
    return static_cast<BytecodeInst::Opcode>(primary);
}

template <const BytecodeInstMode Mode>
inline uint8_t BytecodeInst<Mode>::GetSecondaryOpcode() const {
    ASSERT(GetPrimaryOpcode() >= 252);  // NOLINT(readability-magic-numbers)
    return ReadByte(1);
}

/* static */
template <const BytecodeInstMode Mode>
constexpr uint8_t BytecodeInst<Mode>::GetMinPrefixOpcodeIndex() {
    return 252;  // NOLINT(readability-magic-numbers)
}

template <const BytecodeInstMode Mode>
inline bool BytecodeInst<Mode>::IsPrefixed() const {
    return GetPrimaryOpcode() >= 252;  // NOLINT(readability-magic-numbers)
}

template <const BytecodeInstMode Mode>
inline typename BytecodeInst<Mode>::Format BytecodeInst<Mode>::GetFormat() const {  // NOLINT(readability-function-size)
    return GetFormat(GetOpcode());
}

/* static */
template <const BytecodeInstMode Mode>
constexpr typename BytecodeInst<Mode>::Format BytecodeInst<Mode>::GetFormat(Opcode opcode) {  // NOLINT(readability-function-size)
    switch(opcode) {
    case BytecodeInst<Mode>::Opcode::LDNAN:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::LDINFINITY:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::LDNEWTARGET:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::LDUNDEFINED:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::LDNULL:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::LDSYMBOL:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::LDGLOBAL:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::LDTRUE:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::LDFALSE:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::LDHOLE:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::LDTHIS:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::POPLEXENV:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::GETUNMAPPEDARGS:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONENTER:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::CREATEASYNCGENERATOROBJ_V8:
        return BytecodeInst<Mode>::Format::V8;
    case BytecodeInst<Mode>::Opcode::LDFUNCTION:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::DEBUGGER:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::GETPROPITERATOR:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::GETITERATOR_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::GETITERATOR_IMM16:
        return BytecodeInst<Mode>::Format::IMM16;
    case BytecodeInst<Mode>::Opcode::CLOSEITERATOR_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::CLOSEITERATOR_IMM16_V8:
        return BytecodeInst<Mode>::Format::IMM16_V8;
    case BytecodeInst<Mode>::Opcode::ASYNCGENERATORRESOLVE_V8_V8_V8:
        return BytecodeInst<Mode>::Format::V8_V8_V8;
    case BytecodeInst<Mode>::Opcode::CREATEEMPTYOBJECT:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::CREATEEMPTYARRAY_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::CREATEEMPTYARRAY_IMM16:
        return BytecodeInst<Mode>::Format::IMM16;
    case BytecodeInst<Mode>::Opcode::CREATEGENERATOROBJ_V8:
        return BytecodeInst<Mode>::Format::V8;
    case BytecodeInst<Mode>::Opcode::CREATEITERRESULTOBJ_V8_V8:
        return BytecodeInst<Mode>::Format::V8_V8;
    case BytecodeInst<Mode>::Opcode::CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8_V8;
    case BytecodeInst<Mode>::Opcode::CALLTHIS0_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
        return BytecodeInst<Mode>::Format::IMM8_ID16;
    case BytecodeInst<Mode>::Opcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
        return BytecodeInst<Mode>::Format::IMM16_ID16;
    case BytecodeInst<Mode>::Opcode::CALLTHIS1_IMM8_V8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8_V8;
    case BytecodeInst<Mode>::Opcode::CALLTHIS2_IMM8_V8_V8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8_V8_V8;
    case BytecodeInst<Mode>::Opcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
        return BytecodeInst<Mode>::Format::IMM8_ID16;
    case BytecodeInst<Mode>::Opcode::CREATEOBJECTWITHBUFFER_IMM16_ID16:
        return BytecodeInst<Mode>::Format::IMM16_ID16;
    case BytecodeInst<Mode>::Opcode::CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8:
        return BytecodeInst<Mode>::Format::IMM8_ID16_IMM8;
    case BytecodeInst<Mode>::Opcode::CREATEREGEXPWITHLITERAL_IMM16_ID16_IMM8:
        return BytecodeInst<Mode>::Format::IMM16_ID16_IMM8;
    case BytecodeInst<Mode>::Opcode::NEWOBJAPPLY_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::NEWOBJAPPLY_IMM16_V8:
        return BytecodeInst<Mode>::Format::IMM16_V8;
    case BytecodeInst<Mode>::Opcode::NEWOBJRANGE_IMM8_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_IMM8_V8;
    case BytecodeInst<Mode>::Opcode::NEWOBJRANGE_IMM16_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM16_IMM8_V8;
    case BytecodeInst<Mode>::Opcode::NEWLEXENV_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::NEWLEXENVWITHNAME_IMM8_ID16:
        return BytecodeInst<Mode>::Format::IMM8_ID16;
    case BytecodeInst<Mode>::Opcode::ADD2_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::SUB2_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::MUL2_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::DIV2_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::MOD2_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::EQ_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::NOTEQ_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::LESS_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::LESSEQ_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::GREATER_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::GREATEREQ_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::SHL2_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::SHR2_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::ASHR2_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::AND2_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::OR2_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::XOR2_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::EXP_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::TYPEOF_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::TYPEOF_IMM16:
        return BytecodeInst<Mode>::Format::IMM16;
    case BytecodeInst<Mode>::Opcode::TONUMBER_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::TONUMERIC_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::NEG_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::NOT_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::INC_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::DEC_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::ISIN_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::INSTANCEOF_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::STRICTNOTEQ_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::STRICTEQ_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::ISTRUE:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::ISFALSE:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8_V8_V8_V8;
    case BytecodeInst<Mode>::Opcode::CALLTHISRANGE_IMM8_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_IMM8_V8;
    case BytecodeInst<Mode>::Opcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_IMM8_V8;
    case BytecodeInst<Mode>::Opcode::SUPERCALLARROWRANGE_IMM8_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_IMM8_V8;
    case BytecodeInst<Mode>::Opcode::DEFINEFUNC_IMM8_ID16_IMM8:
        return BytecodeInst<Mode>::Format::IMM8_ID16_IMM8;
    case BytecodeInst<Mode>::Opcode::DEFINEFUNC_IMM16_ID16_IMM8:
        return BytecodeInst<Mode>::Format::IMM16_ID16_IMM8;
    case BytecodeInst<Mode>::Opcode::DEFINEMETHOD_IMM8_ID16_IMM8:
        return BytecodeInst<Mode>::Format::IMM8_ID16_IMM8;
    case BytecodeInst<Mode>::Opcode::DEFINEMETHOD_IMM16_ID16_IMM8:
        return BytecodeInst<Mode>::Format::IMM16_ID16_IMM8;
    case BytecodeInst<Mode>::Opcode::CALLARG0_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::SUPERCALLSPREAD_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::APPLY_IMM8_V8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8_V8;
    case BytecodeInst<Mode>::Opcode::CALLARGS2_IMM8_V8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8_V8;
    case BytecodeInst<Mode>::Opcode::CALLARGS3_IMM8_V8_V8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8_V8_V8;
    case BytecodeInst<Mode>::Opcode::CALLRANGE_IMM8_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_IMM8_V8;
    case BytecodeInst<Mode>::Opcode::LDEXTERNALMODULEVAR_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::LDTHISBYNAME_IMM8_ID16:
        return BytecodeInst<Mode>::Format::IMM8_ID16;
    case BytecodeInst<Mode>::Opcode::DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8:
        return BytecodeInst<Mode>::Format::V8_V8_V8_V8;
    case BytecodeInst<Mode>::Opcode::LDTHISBYNAME_IMM16_ID16:
        return BytecodeInst<Mode>::Format::IMM16_ID16;
    case BytecodeInst<Mode>::Opcode::STTHISBYNAME_IMM8_ID16:
        return BytecodeInst<Mode>::Format::IMM8_ID16;
    case BytecodeInst<Mode>::Opcode::STTHISBYNAME_IMM16_ID16:
        return BytecodeInst<Mode>::Format::IMM16_ID16;
    case BytecodeInst<Mode>::Opcode::LDTHISBYVALUE_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::LDTHISBYVALUE_IMM16:
        return BytecodeInst<Mode>::Format::IMM16;
    case BytecodeInst<Mode>::Opcode::STTHISBYVALUE_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::STTHISBYVALUE_IMM16_V8:
        return BytecodeInst<Mode>::Format::IMM16_V8;
    case BytecodeInst<Mode>::Opcode::LDPATCHVAR_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::STPATCHVAR_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::DYNAMICIMPORT:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8:
        return BytecodeInst<Mode>::Format::IMM8_ID16_ID16_IMM16_V8;
    case BytecodeInst<Mode>::Opcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8:
        return BytecodeInst<Mode>::Format::IMM16_ID16_ID16_IMM16_V8;
    case BytecodeInst<Mode>::Opcode::RESUMEGENERATOR:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::GETRESUMEMODE:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::GETTEMPLATEOBJECT_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::GETTEMPLATEOBJECT_IMM16:
        return BytecodeInst<Mode>::Format::IMM16;
    case BytecodeInst<Mode>::Opcode::GETNEXTPROPNAME_V8:
        return BytecodeInst<Mode>::Format::V8;
    case BytecodeInst<Mode>::Opcode::JEQZ_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::JEQZ_IMM16:
        return BytecodeInst<Mode>::Format::IMM16;
    case BytecodeInst<Mode>::Opcode::SETOBJECTWITHPROTO_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::DELOBJPROP_V8:
        return BytecodeInst<Mode>::Format::V8;
    case BytecodeInst<Mode>::Opcode::SUSPENDGENERATOR_V8:
        return BytecodeInst<Mode>::Format::V8;
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONAWAITUNCAUGHT_V8:
        return BytecodeInst<Mode>::Format::V8;
    case BytecodeInst<Mode>::Opcode::COPYDATAPROPERTIES_V8:
        return BytecodeInst<Mode>::Format::V8;
    case BytecodeInst<Mode>::Opcode::STARRAYSPREAD_V8_V8:
        return BytecodeInst<Mode>::Format::V8_V8;
    case BytecodeInst<Mode>::Opcode::SETOBJECTWITHPROTO_IMM16_V8:
        return BytecodeInst<Mode>::Format::IMM16_V8;
    case BytecodeInst<Mode>::Opcode::LDOBJBYVALUE_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::LDOBJBYVALUE_IMM16_V8:
        return BytecodeInst<Mode>::Format::IMM16_V8;
    case BytecodeInst<Mode>::Opcode::STOBJBYVALUE_IMM8_V8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8_V8;
    case BytecodeInst<Mode>::Opcode::STOBJBYVALUE_IMM16_V8_V8:
        return BytecodeInst<Mode>::Format::IMM16_V8_V8;
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUE_IMM8_V8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8_V8;
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUE_IMM16_V8_V8:
        return BytecodeInst<Mode>::Format::IMM16_V8_V8;
    case BytecodeInst<Mode>::Opcode::LDSUPERBYVALUE_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::LDSUPERBYVALUE_IMM16_V8:
        return BytecodeInst<Mode>::Format::IMM16_V8;
    case BytecodeInst<Mode>::Opcode::STSUPERBYVALUE_IMM8_V8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8_V8;
    case BytecodeInst<Mode>::Opcode::STSUPERBYVALUE_IMM16_V8_V8:
        return BytecodeInst<Mode>::Format::IMM16_V8_V8;
    case BytecodeInst<Mode>::Opcode::LDOBJBYINDEX_IMM8_IMM16:
        return BytecodeInst<Mode>::Format::IMM8_IMM16;
    case BytecodeInst<Mode>::Opcode::LDOBJBYINDEX_IMM16_IMM16:
        return BytecodeInst<Mode>::Format::IMM16_IMM16;
    case BytecodeInst<Mode>::Opcode::STOBJBYINDEX_IMM8_V8_IMM16:
        return BytecodeInst<Mode>::Format::IMM8_V8_IMM16;
    case BytecodeInst<Mode>::Opcode::STOBJBYINDEX_IMM16_V8_IMM16:
        return BytecodeInst<Mode>::Format::IMM16_V8_IMM16;
    case BytecodeInst<Mode>::Opcode::STOWNBYINDEX_IMM8_V8_IMM16:
        return BytecodeInst<Mode>::Format::IMM8_V8_IMM16;
    case BytecodeInst<Mode>::Opcode::STOWNBYINDEX_IMM16_V8_IMM16:
        return BytecodeInst<Mode>::Format::IMM16_V8_IMM16;
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONRESOLVE_V8:
        return BytecodeInst<Mode>::Format::V8;
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONREJECT_V8:
        return BytecodeInst<Mode>::Format::V8;
    case BytecodeInst<Mode>::Opcode::COPYRESTARGS_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::LDLEXVAR_IMM4_IMM4:
        return BytecodeInst<Mode>::Format::IMM4_IMM4;
    case BytecodeInst<Mode>::Opcode::STLEXVAR_IMM4_IMM4:
        return BytecodeInst<Mode>::Format::IMM4_IMM4;
    case BytecodeInst<Mode>::Opcode::GETMODULENAMESPACE_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::STMODULEVAR_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::TRYLDGLOBALBYNAME_IMM8_ID16:
        return BytecodeInst<Mode>::Format::IMM8_ID16;
    case BytecodeInst<Mode>::Opcode::TRYLDGLOBALBYNAME_IMM16_ID16:
        return BytecodeInst<Mode>::Format::IMM16_ID16;
    case BytecodeInst<Mode>::Opcode::TRYSTGLOBALBYNAME_IMM8_ID16:
        return BytecodeInst<Mode>::Format::IMM8_ID16;
    case BytecodeInst<Mode>::Opcode::TRYSTGLOBALBYNAME_IMM16_ID16:
        return BytecodeInst<Mode>::Format::IMM16_ID16;
    case BytecodeInst<Mode>::Opcode::LDGLOBALVAR_IMM16_ID16:
        return BytecodeInst<Mode>::Format::IMM16_ID16;
    case BytecodeInst<Mode>::Opcode::STGLOBALVAR_IMM16_ID16:
        return BytecodeInst<Mode>::Format::IMM16_ID16;
    case BytecodeInst<Mode>::Opcode::LDOBJBYNAME_IMM8_ID16:
        return BytecodeInst<Mode>::Format::IMM8_ID16;
    case BytecodeInst<Mode>::Opcode::LDOBJBYNAME_IMM16_ID16:
        return BytecodeInst<Mode>::Format::IMM16_ID16;
    case BytecodeInst<Mode>::Opcode::STOBJBYNAME_IMM8_ID16_V8:
        return BytecodeInst<Mode>::Format::IMM8_ID16_V8;
    case BytecodeInst<Mode>::Opcode::STOBJBYNAME_IMM16_ID16_V8:
        return BytecodeInst<Mode>::Format::IMM16_ID16_V8;
    case BytecodeInst<Mode>::Opcode::STOWNBYNAME_IMM8_ID16_V8:
        return BytecodeInst<Mode>::Format::IMM8_ID16_V8;
    case BytecodeInst<Mode>::Opcode::STOWNBYNAME_IMM16_ID16_V8:
        return BytecodeInst<Mode>::Format::IMM16_ID16_V8;
    case BytecodeInst<Mode>::Opcode::LDSUPERBYNAME_IMM8_ID16:
        return BytecodeInst<Mode>::Format::IMM8_ID16;
    case BytecodeInst<Mode>::Opcode::LDSUPERBYNAME_IMM16_ID16:
        return BytecodeInst<Mode>::Format::IMM16_ID16;
    case BytecodeInst<Mode>::Opcode::STSUPERBYNAME_IMM8_ID16_V8:
        return BytecodeInst<Mode>::Format::IMM8_ID16_V8;
    case BytecodeInst<Mode>::Opcode::STSUPERBYNAME_IMM16_ID16_V8:
        return BytecodeInst<Mode>::Format::IMM16_ID16_V8;
    case BytecodeInst<Mode>::Opcode::LDLOCALMODULEVAR_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::STCONSTTOGLOBALRECORD_IMM16_ID16:
        return BytecodeInst<Mode>::Format::IMM16_ID16;
    case BytecodeInst<Mode>::Opcode::STTOGLOBALRECORD_IMM16_ID16:
        return BytecodeInst<Mode>::Format::IMM16_ID16;
    case BytecodeInst<Mode>::Opcode::JEQZ_IMM32:
        return BytecodeInst<Mode>::Format::IMM32;
    case BytecodeInst<Mode>::Opcode::JNEZ_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::JNEZ_IMM16:
        return BytecodeInst<Mode>::Format::IMM16;
    case BytecodeInst<Mode>::Opcode::JNEZ_IMM32:
        return BytecodeInst<Mode>::Format::IMM32;
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUEWITHNAMESET_IMM8_V8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8_V8;
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUEWITHNAMESET_IMM16_V8_V8:
        return BytecodeInst<Mode>::Format::IMM16_V8_V8;
    case BytecodeInst<Mode>::Opcode::STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8:
        return BytecodeInst<Mode>::Format::IMM8_ID16_V8;
    case BytecodeInst<Mode>::Opcode::STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8:
        return BytecodeInst<Mode>::Format::IMM16_ID16_V8;
    case BytecodeInst<Mode>::Opcode::LDBIGINT_ID16:
        return BytecodeInst<Mode>::Format::ID16;
    case BytecodeInst<Mode>::Opcode::LDA_STR_ID16:
        return BytecodeInst<Mode>::Format::ID16;
    case BytecodeInst<Mode>::Opcode::JMP_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::JMP_IMM16:
        return BytecodeInst<Mode>::Format::IMM16;
    case BytecodeInst<Mode>::Opcode::JMP_IMM32:
        return BytecodeInst<Mode>::Format::IMM32;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQZ_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQZ_IMM16:
        return BytecodeInst<Mode>::Format::IMM16;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQZ_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQZ_IMM16:
        return BytecodeInst<Mode>::Format::IMM16;
    case BytecodeInst<Mode>::Opcode::JEQNULL_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::JEQNULL_IMM16:
        return BytecodeInst<Mode>::Format::IMM16;
    case BytecodeInst<Mode>::Opcode::LDA_V8:
        return BytecodeInst<Mode>::Format::V8;
    case BytecodeInst<Mode>::Opcode::STA_V8:
        return BytecodeInst<Mode>::Format::V8;
    case BytecodeInst<Mode>::Opcode::LDAI_IMM32:
        return BytecodeInst<Mode>::Format::IMM32;
    case BytecodeInst<Mode>::Opcode::FLDAI_IMM64:
        return BytecodeInst<Mode>::Format::IMM64;
    case BytecodeInst<Mode>::Opcode::RETURN:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::RETURNUNDEFINED:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::LDLEXVAR_IMM8_IMM8:
        return BytecodeInst<Mode>::Format::IMM8_IMM8;
    case BytecodeInst<Mode>::Opcode::JNENULL_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::STLEXVAR_IMM8_IMM8:
        return BytecodeInst<Mode>::Format::IMM8_IMM8;
    case BytecodeInst<Mode>::Opcode::JNENULL_IMM16:
        return BytecodeInst<Mode>::Format::IMM16;
    case BytecodeInst<Mode>::Opcode::CALLARG1_IMM8_V8:
        return BytecodeInst<Mode>::Format::IMM8_V8;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQNULL_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQNULL_IMM16:
        return BytecodeInst<Mode>::Format::IMM16;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQNULL_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQNULL_IMM16:
        return BytecodeInst<Mode>::Format::IMM16;
    case BytecodeInst<Mode>::Opcode::JEQUNDEFINED_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::JEQUNDEFINED_IMM16:
        return BytecodeInst<Mode>::Format::IMM16;
    case BytecodeInst<Mode>::Opcode::JNEUNDEFINED_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::JNEUNDEFINED_IMM16:
        return BytecodeInst<Mode>::Format::IMM16;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQUNDEFINED_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQUNDEFINED_IMM16:
        return BytecodeInst<Mode>::Format::IMM16;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQUNDEFINED_IMM8:
        return BytecodeInst<Mode>::Format::IMM8;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQUNDEFINED_IMM16:
        return BytecodeInst<Mode>::Format::IMM16;
    case BytecodeInst<Mode>::Opcode::JEQ_V8_IMM8:
        return BytecodeInst<Mode>::Format::V8_IMM8;
    case BytecodeInst<Mode>::Opcode::JEQ_V8_IMM16:
        return BytecodeInst<Mode>::Format::V8_IMM16;
    case BytecodeInst<Mode>::Opcode::JNE_V8_IMM8:
        return BytecodeInst<Mode>::Format::V8_IMM8;
    case BytecodeInst<Mode>::Opcode::JNE_V8_IMM16:
        return BytecodeInst<Mode>::Format::V8_IMM16;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQ_V8_IMM8:
        return BytecodeInst<Mode>::Format::V8_IMM8;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQ_V8_IMM16:
        return BytecodeInst<Mode>::Format::V8_IMM16;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQ_V8_IMM8:
        return BytecodeInst<Mode>::Format::V8_IMM8;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQ_V8_IMM16:
        return BytecodeInst<Mode>::Format::V8_IMM16;
    case BytecodeInst<Mode>::Opcode::MOV_V4_V4:
        return BytecodeInst<Mode>::Format::V4_V4;
    case BytecodeInst<Mode>::Opcode::MOV_V8_V8:
        return BytecodeInst<Mode>::Format::V8_V8;
    case BytecodeInst<Mode>::Opcode::MOV_V16_V16:
        return BytecodeInst<Mode>::Format::V16_V16;
    case BytecodeInst<Mode>::Opcode::ASYNCGENERATORREJECT_V8_V8:
        return BytecodeInst<Mode>::Format::V8_V8;
    case BytecodeInst<Mode>::Opcode::NOP:
        return BytecodeInst<Mode>::Format::NONE;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDLEXENV_PREF_NONE:
        return BytecodeInst<Mode>::Format::PREF_NONE;
    case BytecodeInst<Mode>::Opcode::WIDE_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8:
        return BytecodeInst<Mode>::Format::PREF_IMM16_V8_V8;
    case BytecodeInst<Mode>::Opcode::THROW_PREF_NONE:
        return BytecodeInst<Mode>::Format::PREF_NONE;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_POPLEXENV_PREF_NONE:
        return BytecodeInst<Mode>::Format::PREF_NONE;
    case BytecodeInst<Mode>::Opcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8:
        return BytecodeInst<Mode>::Format::PREF_IMM16_V8;
    case BytecodeInst<Mode>::Opcode::THROW_NOTEXISTS_PREF_NONE:
        return BytecodeInst<Mode>::Format::PREF_NONE;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETITERATORNEXT_PREF_V8_V8:
        return BytecodeInst<Mode>::Format::PREF_V8_V8;
    case BytecodeInst<Mode>::Opcode::WIDE_NEWLEXENV_PREF_IMM16:
        return BytecodeInst<Mode>::Format::PREF_IMM16;
    case BytecodeInst<Mode>::Opcode::THROW_PATTERNNONCOERCIBLE_PREF_NONE:
        return BytecodeInst<Mode>::Format::PREF_NONE;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CREATEARRAYWITHBUFFER_PREF_IMM16:
        return BytecodeInst<Mode>::Format::PREF_IMM16;
    case BytecodeInst<Mode>::Opcode::WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16:
        return BytecodeInst<Mode>::Format::PREF_IMM16_ID16;
    case BytecodeInst<Mode>::Opcode::THROW_DELETESUPERPROPERTY_PREF_NONE:
        return BytecodeInst<Mode>::Format::PREF_NONE;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CREATEOBJECTWITHBUFFER_PREF_IMM16:
        return BytecodeInst<Mode>::Format::PREF_IMM16;
    case BytecodeInst<Mode>::Opcode::WIDE_CALLRANGE_PREF_IMM16_V8:
        return BytecodeInst<Mode>::Format::PREF_IMM16_V8;
    case BytecodeInst<Mode>::Opcode::THROW_CONSTASSIGNMENT_PREF_V8:
        return BytecodeInst<Mode>::Format::PREF_V8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_TONUMBER_PREF_V8:
        return BytecodeInst<Mode>::Format::PREF_V8;
    case BytecodeInst<Mode>::Opcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8:
        return BytecodeInst<Mode>::Format::PREF_IMM16_V8;
    case BytecodeInst<Mode>::Opcode::THROW_IFNOTOBJECT_PREF_V8:
        return BytecodeInst<Mode>::Format::PREF_V8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_TONUMERIC_PREF_V8:
        return BytecodeInst<Mode>::Format::PREF_V8;
    case BytecodeInst<Mode>::Opcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
        return BytecodeInst<Mode>::Format::PREF_IMM16_V8;
    case BytecodeInst<Mode>::Opcode::THROW_UNDEFINEDIFHOLE_PREF_V8_V8:
        return BytecodeInst<Mode>::Format::PREF_V8_V8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_NEG_PREF_V8:
        return BytecodeInst<Mode>::Format::PREF_V8;
    case BytecodeInst<Mode>::Opcode::WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8:
        return BytecodeInst<Mode>::Format::PREF_IMM16_V8;
    case BytecodeInst<Mode>::Opcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM8:
        return BytecodeInst<Mode>::Format::PREF_IMM8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_NOT_PREF_V8:
        return BytecodeInst<Mode>::Format::PREF_V8;
    case BytecodeInst<Mode>::Opcode::WIDE_LDOBJBYINDEX_PREF_IMM32:
        return BytecodeInst<Mode>::Format::PREF_IMM32;
    case BytecodeInst<Mode>::Opcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16:
        return BytecodeInst<Mode>::Format::PREF_IMM16;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_INC_PREF_V8:
        return BytecodeInst<Mode>::Format::PREF_V8;
    case BytecodeInst<Mode>::Opcode::WIDE_STOBJBYINDEX_PREF_V8_IMM32:
        return BytecodeInst<Mode>::Format::PREF_V8_IMM32;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DEC_PREF_V8:
        return BytecodeInst<Mode>::Format::PREF_V8;
    case BytecodeInst<Mode>::Opcode::WIDE_STOWNBYINDEX_PREF_V8_IMM32:
        return BytecodeInst<Mode>::Format::PREF_V8_IMM32;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARG0_PREF_V8:
        return BytecodeInst<Mode>::Format::PREF_V8;
    case BytecodeInst<Mode>::Opcode::WIDE_COPYRESTARGS_PREF_IMM16:
        return BytecodeInst<Mode>::Format::PREF_IMM16;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARG1_PREF_V8_V8:
        return BytecodeInst<Mode>::Format::PREF_V8_V8;
    case BytecodeInst<Mode>::Opcode::WIDE_LDLEXVAR_PREF_IMM16_IMM16:
        return BytecodeInst<Mode>::Format::PREF_IMM16_IMM16;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARGS2_PREF_V8_V8_V8:
        return BytecodeInst<Mode>::Format::PREF_V8_V8_V8;
    case BytecodeInst<Mode>::Opcode::WIDE_STLEXVAR_PREF_IMM16_IMM16:
        return BytecodeInst<Mode>::Format::PREF_IMM16_IMM16;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARGS3_PREF_V8_V8_V8_V8:
        return BytecodeInst<Mode>::Format::PREF_V8_V8_V8_V8;
    case BytecodeInst<Mode>::Opcode::WIDE_GETMODULENAMESPACE_PREF_IMM16:
        return BytecodeInst<Mode>::Format::PREF_IMM16;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLRANGE_PREF_IMM16_V8:
        return BytecodeInst<Mode>::Format::PREF_IMM16_V8;
    case BytecodeInst<Mode>::Opcode::WIDE_STMODULEVAR_PREF_IMM16:
        return BytecodeInst<Mode>::Format::PREF_IMM16;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLSPREAD_PREF_V8_V8_V8:
        return BytecodeInst<Mode>::Format::PREF_V8_V8_V8;
    case BytecodeInst<Mode>::Opcode::WIDE_LDLOCALMODULEVAR_PREF_IMM16:
        return BytecodeInst<Mode>::Format::PREF_IMM16;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLTHISRANGE_PREF_IMM16_V8:
        return BytecodeInst<Mode>::Format::PREF_IMM16_V8;
    case BytecodeInst<Mode>::Opcode::WIDE_LDEXTERNALMODULEVAR_PREF_IMM16:
        return BytecodeInst<Mode>::Format::PREF_IMM16;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8:
        return BytecodeInst<Mode>::Format::PREF_ID16_IMM16_IMM16_V8_V8;
    case BytecodeInst<Mode>::Opcode::WIDE_LDPATCHVAR_PREF_IMM16:
        return BytecodeInst<Mode>::Format::PREF_IMM16;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_RESUMEGENERATOR_PREF_V8:
        return BytecodeInst<Mode>::Format::PREF_V8;
    case BytecodeInst<Mode>::Opcode::WIDE_STPATCHVAR_PREF_IMM16:
        return BytecodeInst<Mode>::Format::PREF_IMM16;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETRESUMEMODE_PREF_V8:
        return BytecodeInst<Mode>::Format::PREF_V8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETTEMPLATEOBJECT_PREF_V8:
        return BytecodeInst<Mode>::Format::PREF_V8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DELOBJPROP_PREF_V8_V8:
        return BytecodeInst<Mode>::Format::PREF_V8_V8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_SUSPENDGENERATOR_PREF_V8_V8:
        return BytecodeInst<Mode>::Format::PREF_V8_V8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8:
        return BytecodeInst<Mode>::Format::PREF_V8_V8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_COPYDATAPROPERTIES_PREF_V8_V8:
        return BytecodeInst<Mode>::Format::PREF_V8_V8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_SETOBJECTWITHPROTO_PREF_V8_V8:
        return BytecodeInst<Mode>::Format::PREF_V8_V8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDOBJBYVALUE_PREF_V8_V8:
        return BytecodeInst<Mode>::Format::PREF_V8_V8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDSUPERBYVALUE_PREF_V8_V8:
        return BytecodeInst<Mode>::Format::PREF_V8_V8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDOBJBYINDEX_PREF_V8_IMM32:
        return BytecodeInst<Mode>::Format::PREF_V8_IMM32;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8:
        return BytecodeInst<Mode>::Format::PREF_V8_V8_V8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_ASYNCFUNCTIONREJECT_PREF_V8_V8_V8:
        return BytecodeInst<Mode>::Format::PREF_V8_V8_V8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLEXVAR_PREF_IMM4_IMM4_V8:
        return BytecodeInst<Mode>::Format::PREF_IMM4_IMM4_V8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLEXVAR_PREF_IMM8_IMM8_V8:
        return BytecodeInst<Mode>::Format::PREF_IMM8_IMM8_V8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLEXVAR_PREF_IMM16_IMM16_V8:
        return BytecodeInst<Mode>::Format::PREF_IMM16_IMM16_V8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETMODULENAMESPACE_PREF_ID32:
        return BytecodeInst<Mode>::Format::PREF_ID32;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STMODULEVAR_PREF_ID32:
        return BytecodeInst<Mode>::Format::PREF_ID32;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDOBJBYNAME_PREF_ID32_V8:
        return BytecodeInst<Mode>::Format::PREF_ID32_V8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDSUPERBYNAME_PREF_ID32_V8:
        return BytecodeInst<Mode>::Format::PREF_ID32_V8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDMODULEVAR_PREF_ID32_IMM8:
        return BytecodeInst<Mode>::Format::PREF_ID32_IMM8;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STCONSTTOGLOBALRECORD_PREF_ID32:
        return BytecodeInst<Mode>::Format::PREF_ID32;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLETTOGLOBALRECORD_PREF_ID32:
        return BytecodeInst<Mode>::Format::PREF_ID32;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STCLASSTOGLOBALRECORD_PREF_ID32:
        return BytecodeInst<Mode>::Format::PREF_ID32;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDHOMEOBJECT_PREF_NONE:
        return BytecodeInst<Mode>::Format::PREF_NONE;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CREATEOBJECTHAVINGMETHOD_PREF_IMM16:
        return BytecodeInst<Mode>::Format::PREF_IMM16;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DYNAMICIMPORT_PREF_V8:
        return BytecodeInst<Mode>::Format::PREF_V8;
    default:
        break;
    }

    UNREACHABLE();
}

// NOLINTNEXTLINE(readability-function-size)
template<const BytecodeInstMode Mode> inline bool BytecodeInst<Mode>::HasFlag(Flags flag) const {
    switch(GetOpcode()) {
    case BytecodeInst<Mode>::Opcode::LDNAN:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDINFINITY:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDNEWTARGET:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDUNDEFINED:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDNULL:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDSYMBOL:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDGLOBAL:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDTRUE:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDFALSE:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDHOLE:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDTHIS:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::POPLEXENV:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GETUNMAPPEDARGS:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONENTER:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEASYNCGENERATOROBJ_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDFUNCTION:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEBUGGER:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_NONE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GETPROPITERATOR:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GETITERATOR_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GETITERATOR_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CLOSEITERATOR_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CLOSEITERATOR_IMM16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ASYNCGENERATORRESOLVE_V8_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEEMPTYOBJECT:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEEMPTYARRAY_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEEMPTYARRAY_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEGENERATOROBJ_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEITERRESULTOBJ_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CALLTHIS0_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::LITERALARRAY_ID | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::LITERALARRAY_ID | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CALLTHIS1_IMM8_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CALLTHIS2_IMM8_V8_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::LITERALARRAY_ID | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEOBJECTWITHBUFFER_IMM16_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::LITERALARRAY_ID | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEREGEXPWITHLITERAL_IMM16_ID16_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::NEWOBJAPPLY_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::NEWOBJAPPLY_IMM16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::NEWOBJRANGE_IMM8_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::NEWOBJRANGE_IMM16_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::NEWLEXENV_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::NEWLEXENVWITHNAME_IMM8_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::LITERALARRAY_ID | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ADD2_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::SUB2_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::MUL2_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DIV2_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::MOD2_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::EQ_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::NOTEQ_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LESS_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LESSEQ_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GREATER_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GREATEREQ_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::SHL2_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::SHR2_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ASHR2_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::AND2_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::OR2_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::XOR2_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::EXP_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::TYPEOF_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::TYPEOF_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::TONUMBER_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::TONUMERIC_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::NEG_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::NOT_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::INC_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEC_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ISIN_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::INSTANCEOF_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STRICTNOTEQ_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STRICTEQ_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ISTRUE:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ISFALSE:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CALLTHISRANGE_IMM8_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::SUPERCALLARROWRANGE_IMM8_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEFINEFUNC_IMM8_ID16_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::METHOD_ID | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEFINEFUNC_IMM16_ID16_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::METHOD_ID | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEFINEMETHOD_IMM8_ID16_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::METHOD_ID | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEFINEMETHOD_IMM16_ID16_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::METHOD_ID | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CALLARG0_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::SUPERCALLSPREAD_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::APPLY_IMM8_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CALLARGS2_IMM8_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CALLARGS3_IMM8_V8_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CALLRANGE_IMM8_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDEXTERNALMODULEVAR_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDTHISBYNAME_IMM8_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDTHISBYNAME_IMM16_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STTHISBYNAME_IMM8_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STTHISBYNAME_IMM16_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDTHISBYVALUE_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDTHISBYVALUE_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STTHISBYVALUE_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STTHISBYVALUE_IMM16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDPATCHVAR_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STPATCHVAR_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_NONE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DYNAMICIMPORT:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::METHOD_ID | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::LITERALARRAY_ID | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::METHOD_ID | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::LITERALARRAY_ID | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::RESUMEGENERATOR:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GETRESUMEMODE:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GETTEMPLATEOBJECT_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GETTEMPLATEOBJECT_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GETNEXTPROPNAME_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JEQZ_IMM8:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JEQZ_IMM16:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::SETOBJECTWITHPROTO_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DELOBJPROP_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::SUSPENDGENERATOR_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONAWAITUNCAUGHT_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::COPYDATAPROPERTIES_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STARRAYSPREAD_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::SETOBJECTWITHPROTO_IMM16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDOBJBYVALUE_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDOBJBYVALUE_IMM16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOBJBYVALUE_IMM8_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOBJBYVALUE_IMM16_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUE_IMM8_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUE_IMM16_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDSUPERBYVALUE_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDSUPERBYVALUE_IMM16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STSUPERBYVALUE_IMM8_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STSUPERBYVALUE_IMM16_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDOBJBYINDEX_IMM8_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDOBJBYINDEX_IMM16_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOBJBYINDEX_IMM8_V8_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOBJBYINDEX_IMM16_V8_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOWNBYINDEX_IMM8_V8_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOWNBYINDEX_IMM16_V8_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONRESOLVE_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONREJECT_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::COPYRESTARGS_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDLEXVAR_IMM4_IMM4:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STLEXVAR_IMM4_IMM4:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GETMODULENAMESPACE_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STMODULEVAR_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::TRYLDGLOBALBYNAME_IMM8_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::TRYLDGLOBALBYNAME_IMM16_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::TRYSTGLOBALBYNAME_IMM8_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::TRYSTGLOBALBYNAME_IMM16_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDGLOBALVAR_IMM16_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STGLOBALVAR_IMM16_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDOBJBYNAME_IMM8_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDOBJBYNAME_IMM16_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOBJBYNAME_IMM8_ID16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOBJBYNAME_IMM16_ID16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOWNBYNAME_IMM8_ID16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOWNBYNAME_IMM16_ID16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDSUPERBYNAME_IMM8_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDSUPERBYNAME_IMM16_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STSUPERBYNAME_IMM8_ID16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STSUPERBYNAME_IMM16_ID16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDLOCALMODULEVAR_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STCONSTTOGLOBALRECORD_IMM16_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STTOGLOBALRECORD_IMM16_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::ONE_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JEQZ_IMM32:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNEZ_IMM8:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNEZ_IMM16:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNEZ_IMM32:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUEWITHNAMESET_IMM8_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUEWITHNAMESET_IMM16_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::IC_SLOT | Flags::TWO_SLOT | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDBIGINT_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDA_STR_ID16:
        return ((Flags::STRING_ID | Flags::LANGUAGE_TYPE | Flags::MAYBE_DYNAMIC | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JMP_IMM8:
        return ((Flags::JUMP | Flags::ACC_NONE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JMP_IMM16:
        return ((Flags::JUMP | Flags::ACC_NONE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JMP_IMM32:
        return ((Flags::JUMP | Flags::ACC_NONE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JSTRICTEQZ_IMM8:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JSTRICTEQZ_IMM16:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQZ_IMM8:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQZ_IMM16:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JEQNULL_IMM8:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JEQNULL_IMM16:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDA_V8:
        return ((Flags::DYNAMIC | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STA_V8:
        return ((Flags::DYNAMIC | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDAI_IMM32:
        return ((Flags::DYNAMIC | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::FLDAI_IMM64:
        return ((Flags::DYNAMIC | Flags::FLOAT | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::RETURN:
        return ((Flags::DYNAMIC | Flags::RETURN | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::RETURNUNDEFINED:
        return ((Flags::DYNAMIC | Flags::RETURN | Flags::ACC_NONE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDLEXVAR_IMM8_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNENULL_IMM8:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STLEXVAR_IMM8_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNENULL_IMM16:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CALLARG1_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::JIT_IC_SLOT | Flags::TWO_SLOT | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JSTRICTEQNULL_IMM8:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JSTRICTEQNULL_IMM16:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQNULL_IMM8:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQNULL_IMM16:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JEQUNDEFINED_IMM8:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JEQUNDEFINED_IMM16:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNEUNDEFINED_IMM8:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNEUNDEFINED_IMM16:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JSTRICTEQUNDEFINED_IMM8:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JSTRICTEQUNDEFINED_IMM16:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQUNDEFINED_IMM8:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQUNDEFINED_IMM16:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JEQ_V8_IMM8:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JEQ_V8_IMM16:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNE_V8_IMM8:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNE_V8_IMM16:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JSTRICTEQ_V8_IMM8:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JSTRICTEQ_V8_IMM16:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQ_V8_IMM8:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQ_V8_IMM16:
        return ((Flags::JUMP | Flags::CONDITIONAL | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::MOV_V4_V4:
        return ((Flags::DYNAMIC | Flags::ACC_NONE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::MOV_V8_V8:
        return ((Flags::DYNAMIC | Flags::ACC_NONE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::MOV_V16_V16:
        return ((Flags::DYNAMIC | Flags::ACC_NONE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ASYNCGENERATORREJECT_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::NOP:
        return ((Flags::ACC_NONE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDLEXENV_PREF_NONE:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::THROW_PREF_NONE:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_POPLEXENV_PREF_NONE:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::THROW_NOTEXISTS_PREF_NONE:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_NONE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETITERATORNEXT_PREF_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_NEWLEXENV_PREF_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::THROW_PATTERNNONCOERCIBLE_PREF_NONE:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_NONE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CREATEARRAYWITHBUFFER_PREF_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::LITERALARRAY_ID | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::THROW_DELETESUPERPROPERTY_PREF_NONE:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_NONE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CREATEOBJECTWITHBUFFER_PREF_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_CALLRANGE_PREF_IMM16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::THROW_CONSTASSIGNMENT_PREF_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_NONE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_TONUMBER_PREF_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::THROW_IFNOTOBJECT_PREF_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_NONE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_TONUMERIC_PREF_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::THROW_UNDEFINEDIFHOLE_PREF_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_NONE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_NEG_PREF_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_NOT_PREF_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_LDOBJBYINDEX_PREF_IMM32:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_INC_PREF_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_STOBJBYINDEX_PREF_V8_IMM32:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DEC_PREF_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_STOWNBYINDEX_PREF_V8_IMM32:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARG0_PREF_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_COPYRESTARGS_PREF_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARG1_PREF_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_LDLEXVAR_PREF_IMM16_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARGS2_PREF_V8_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_STLEXVAR_PREF_IMM16_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARGS3_PREF_V8_V8_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_GETMODULENAMESPACE_PREF_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLRANGE_PREF_IMM16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_STMODULEVAR_PREF_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLSPREAD_PREF_V8_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_LDLOCALMODULEVAR_PREF_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLTHISRANGE_PREF_IMM16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_LDEXTERNALMODULEVAR_PREF_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::METHOD_ID | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_LDPATCHVAR_PREF_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_RESUMEGENERATOR_PREF_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_STPATCHVAR_PREF_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETRESUMEMODE_PREF_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETTEMPLATEOBJECT_PREF_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DELOBJPROP_PREF_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_SUSPENDGENERATOR_PREF_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_COPYDATAPROPERTIES_PREF_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_SETOBJECTWITHPROTO_PREF_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_NONE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDOBJBYVALUE_PREF_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDSUPERBYVALUE_PREF_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDOBJBYINDEX_PREF_V8_IMM32:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_ASYNCFUNCTIONREJECT_PREF_V8_V8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLEXVAR_PREF_IMM4_IMM4_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_NONE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLEXVAR_PREF_IMM8_IMM8_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_NONE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLEXVAR_PREF_IMM16_IMM16_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_NONE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETMODULENAMESPACE_PREF_ID32:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STMODULEVAR_PREF_ID32:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDOBJBYNAME_PREF_ID32_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDSUPERBYNAME_PREF_ID32_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDMODULEVAR_PREF_ID32_IMM8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STCONSTTOGLOBALRECORD_PREF_ID32:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLETTOGLOBALRECORD_PREF_ID32:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STCLASSTOGLOBALRECORD_PREF_ID32:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::STRING_ID | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDHOMEOBJECT_PREF_NONE:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CREATEOBJECTHAVINGMETHOD_PREF_IMM16:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE | Flags::ACC_READ) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DYNAMICIMPORT_PREF_V8:
        return ((Flags::ACC_READ | Flags::ACC_WRITE | Flags::ACC_WRITE) & flag) == flag;  // NOLINT(hicpp-signed-bitwise)
    default:
        return false;
    }

    UNREACHABLE();
}

// NOLINTNEXTLINE(readability-function-size)
template<const BytecodeInstMode Mode> inline bool BytecodeInst<Mode>::IsThrow(Exceptions exception) const {
    switch(GetOpcode()) {
    case BytecodeInst<Mode>::Opcode::LDNAN:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDINFINITY:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDNEWTARGET:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDUNDEFINED:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDNULL:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDSYMBOL:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDGLOBAL:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDTRUE:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDFALSE:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDHOLE:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDTHIS:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::POPLEXENV:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GETUNMAPPEDARGS:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONENTER:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEASYNCGENERATOROBJ_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDFUNCTION:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEBUGGER:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GETPROPITERATOR:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GETITERATOR_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GETITERATOR_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CLOSEITERATOR_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CLOSEITERATOR_IMM16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ASYNCGENERATORRESOLVE_V8_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEEMPTYOBJECT:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEEMPTYARRAY_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEEMPTYARRAY_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEGENERATOROBJ_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEITERRESULTOBJ_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CALLTHIS0_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CALLTHIS1_IMM8_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CALLTHIS2_IMM8_V8_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEOBJECTWITHBUFFER_IMM16_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CREATEREGEXPWITHLITERAL_IMM16_ID16_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::NEWOBJAPPLY_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::NEWOBJAPPLY_IMM16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::NEWOBJRANGE_IMM8_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::NEWOBJRANGE_IMM16_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::NEWLEXENV_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::NEWLEXENVWITHNAME_IMM8_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ADD2_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::SUB2_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::MUL2_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DIV2_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::MOD2_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::EQ_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::NOTEQ_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LESS_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LESSEQ_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GREATER_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GREATEREQ_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::SHL2_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::SHR2_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ASHR2_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::AND2_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::OR2_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::XOR2_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::EXP_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::TYPEOF_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::TYPEOF_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::TONUMBER_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::TONUMERIC_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::NEG_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::NOT_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::INC_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEC_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ISIN_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::INSTANCEOF_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STRICTNOTEQ_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STRICTEQ_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ISTRUE:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ISFALSE:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CALLTHISRANGE_IMM8_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::SUPERCALLARROWRANGE_IMM8_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEFINEFUNC_IMM8_ID16_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEFINEFUNC_IMM16_ID16_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEFINEMETHOD_IMM8_ID16_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEFINEMETHOD_IMM16_ID16_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CALLARG0_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::SUPERCALLSPREAD_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::APPLY_IMM8_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CALLARGS2_IMM8_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CALLARGS3_IMM8_V8_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CALLRANGE_IMM8_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDEXTERNALMODULEVAR_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDTHISBYNAME_IMM8_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDTHISBYNAME_IMM16_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STTHISBYNAME_IMM8_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STTHISBYNAME_IMM16_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDTHISBYVALUE_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDTHISBYVALUE_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STTHISBYVALUE_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STTHISBYVALUE_IMM16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDPATCHVAR_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STPATCHVAR_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DYNAMICIMPORT:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::RESUMEGENERATOR:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GETRESUMEMODE:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GETTEMPLATEOBJECT_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GETTEMPLATEOBJECT_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GETNEXTPROPNAME_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JEQZ_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JEQZ_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::SETOBJECTWITHPROTO_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DELOBJPROP_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::SUSPENDGENERATOR_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONAWAITUNCAUGHT_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::COPYDATAPROPERTIES_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STARRAYSPREAD_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::SETOBJECTWITHPROTO_IMM16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDOBJBYVALUE_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDOBJBYVALUE_IMM16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOBJBYVALUE_IMM8_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOBJBYVALUE_IMM16_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUE_IMM8_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUE_IMM16_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDSUPERBYVALUE_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDSUPERBYVALUE_IMM16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STSUPERBYVALUE_IMM8_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STSUPERBYVALUE_IMM16_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDOBJBYINDEX_IMM8_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDOBJBYINDEX_IMM16_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOBJBYINDEX_IMM8_V8_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOBJBYINDEX_IMM16_V8_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOWNBYINDEX_IMM8_V8_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOWNBYINDEX_IMM16_V8_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONRESOLVE_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONREJECT_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::COPYRESTARGS_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDLEXVAR_IMM4_IMM4:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STLEXVAR_IMM4_IMM4:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::GETMODULENAMESPACE_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STMODULEVAR_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::TRYLDGLOBALBYNAME_IMM8_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::TRYLDGLOBALBYNAME_IMM16_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::TRYSTGLOBALBYNAME_IMM8_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::TRYSTGLOBALBYNAME_IMM16_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDGLOBALVAR_IMM16_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STGLOBALVAR_IMM16_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDOBJBYNAME_IMM8_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDOBJBYNAME_IMM16_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOBJBYNAME_IMM8_ID16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOBJBYNAME_IMM16_ID16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOWNBYNAME_IMM8_ID16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOWNBYNAME_IMM16_ID16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDSUPERBYNAME_IMM8_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDSUPERBYNAME_IMM16_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STSUPERBYNAME_IMM8_ID16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STSUPERBYNAME_IMM16_ID16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDLOCALMODULEVAR_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STCONSTTOGLOBALRECORD_IMM16_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STTOGLOBALRECORD_IMM16_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JEQZ_IMM32:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNEZ_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNEZ_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNEZ_IMM32:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUEWITHNAMESET_IMM8_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUEWITHNAMESET_IMM16_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDBIGINT_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDA_STR_ID16:
        return ((Exceptions::X_OOM) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JMP_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JMP_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JMP_IMM32:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JSTRICTEQZ_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JSTRICTEQZ_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQZ_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQZ_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JEQNULL_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JEQNULL_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDA_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STA_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDAI_IMM32:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::FLDAI_IMM64:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::RETURN:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::RETURNUNDEFINED:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::LDLEXVAR_IMM8_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNENULL_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::STLEXVAR_IMM8_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNENULL_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::CALLARG1_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JSTRICTEQNULL_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JSTRICTEQNULL_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQNULL_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQNULL_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JEQUNDEFINED_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JEQUNDEFINED_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNEUNDEFINED_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNEUNDEFINED_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JSTRICTEQUNDEFINED_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JSTRICTEQUNDEFINED_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQUNDEFINED_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQUNDEFINED_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JEQ_V8_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JEQ_V8_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNE_V8_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNE_V8_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JSTRICTEQ_V8_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JSTRICTEQ_V8_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQ_V8_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQ_V8_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::MOV_V4_V4:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::MOV_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::MOV_V16_V16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::ASYNCGENERATORREJECT_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::NOP:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDLEXENV_PREF_NONE:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::THROW_PREF_NONE:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_POPLEXENV_PREF_NONE:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::THROW_NOTEXISTS_PREF_NONE:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETITERATORNEXT_PREF_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_NEWLEXENV_PREF_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::THROW_PATTERNNONCOERCIBLE_PREF_NONE:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CREATEARRAYWITHBUFFER_PREF_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::THROW_DELETESUPERPROPERTY_PREF_NONE:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CREATEOBJECTWITHBUFFER_PREF_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_CALLRANGE_PREF_IMM16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::THROW_CONSTASSIGNMENT_PREF_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_TONUMBER_PREF_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::THROW_IFNOTOBJECT_PREF_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_TONUMERIC_PREF_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::THROW_UNDEFINEDIFHOLE_PREF_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_NEG_PREF_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_NOT_PREF_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_LDOBJBYINDEX_PREF_IMM32:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_INC_PREF_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_STOBJBYINDEX_PREF_V8_IMM32:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DEC_PREF_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_STOWNBYINDEX_PREF_V8_IMM32:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARG0_PREF_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_COPYRESTARGS_PREF_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARG1_PREF_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_LDLEXVAR_PREF_IMM16_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARGS2_PREF_V8_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_STLEXVAR_PREF_IMM16_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARGS3_PREF_V8_V8_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_GETMODULENAMESPACE_PREF_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLRANGE_PREF_IMM16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_STMODULEVAR_PREF_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLSPREAD_PREF_V8_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_LDLOCALMODULEVAR_PREF_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLTHISRANGE_PREF_IMM16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_LDEXTERNALMODULEVAR_PREF_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_LDPATCHVAR_PREF_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_RESUMEGENERATOR_PREF_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::WIDE_STPATCHVAR_PREF_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETRESUMEMODE_PREF_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETTEMPLATEOBJECT_PREF_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DELOBJPROP_PREF_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_SUSPENDGENERATOR_PREF_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_COPYDATAPROPERTIES_PREF_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_SETOBJECTWITHPROTO_PREF_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDOBJBYVALUE_PREF_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDSUPERBYVALUE_PREF_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDOBJBYINDEX_PREF_V8_IMM32:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_ASYNCFUNCTIONREJECT_PREF_V8_V8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLEXVAR_PREF_IMM4_IMM4_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLEXVAR_PREF_IMM8_IMM8_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLEXVAR_PREF_IMM16_IMM16_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETMODULENAMESPACE_PREF_ID32:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STMODULEVAR_PREF_ID32:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDOBJBYNAME_PREF_ID32_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDSUPERBYNAME_PREF_ID32_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDMODULEVAR_PREF_ID32_IMM8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STCONSTTOGLOBALRECORD_PREF_ID32:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLETTOGLOBALRECORD_PREF_ID32:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STCLASSTOGLOBALRECORD_PREF_ID32:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDHOMEOBJECT_PREF_NONE:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CREATEOBJECTHAVINGMETHOD_PREF_IMM16:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DYNAMICIMPORT_PREF_V8:
        return ((Exceptions::X_NONE) & exception) == exception;  // NOLINT(hicpp-signed-bitwise)
    default:
        return false;
    }

    UNREACHABLE();
}

// NOLINTNEXTLINE(readability-function-size)
template<const BytecodeInstMode Mode> inline bool BytecodeInst<Mode>::CanThrow() const {
    switch(GetOpcode()) {
    case BytecodeInst<Mode>::Opcode::LDNAN:
        return false;
    case BytecodeInst<Mode>::Opcode::LDINFINITY:
        return false;
    case BytecodeInst<Mode>::Opcode::LDNEWTARGET:
        return false;
    case BytecodeInst<Mode>::Opcode::LDUNDEFINED:
        return false;
    case BytecodeInst<Mode>::Opcode::LDNULL:
        return false;
    case BytecodeInst<Mode>::Opcode::LDSYMBOL:
        return false;
    case BytecodeInst<Mode>::Opcode::LDGLOBAL:
        return false;
    case BytecodeInst<Mode>::Opcode::LDTRUE:
        return false;
    case BytecodeInst<Mode>::Opcode::LDFALSE:
        return false;
    case BytecodeInst<Mode>::Opcode::LDHOLE:
        return false;
    case BytecodeInst<Mode>::Opcode::LDTHIS:
        return false;
    case BytecodeInst<Mode>::Opcode::POPLEXENV:
        return false;
    case BytecodeInst<Mode>::Opcode::GETUNMAPPEDARGS:
        return false;
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONENTER:
        return false;
    case BytecodeInst<Mode>::Opcode::CREATEASYNCGENERATOROBJ_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::LDFUNCTION:
        return false;
    case BytecodeInst<Mode>::Opcode::DEBUGGER:
        return false;
    case BytecodeInst<Mode>::Opcode::GETPROPITERATOR:
        return false;
    case BytecodeInst<Mode>::Opcode::GETITERATOR_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::GETITERATOR_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::CLOSEITERATOR_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::CLOSEITERATOR_IMM16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::ASYNCGENERATORRESOLVE_V8_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::CREATEEMPTYOBJECT:
        return false;
    case BytecodeInst<Mode>::Opcode::CREATEEMPTYARRAY_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::CREATEEMPTYARRAY_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::CREATEGENERATOROBJ_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::CREATEITERRESULTOBJ_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::CALLTHIS0_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::CALLTHIS1_IMM8_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::CALLTHIS2_IMM8_V8_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::CREATEOBJECTWITHBUFFER_IMM16_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::CREATEREGEXPWITHLITERAL_IMM16_ID16_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::NEWOBJAPPLY_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::NEWOBJAPPLY_IMM16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::NEWOBJRANGE_IMM8_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::NEWOBJRANGE_IMM16_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::NEWLEXENV_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::NEWLEXENVWITHNAME_IMM8_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::ADD2_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::SUB2_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::MUL2_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DIV2_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::MOD2_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::EQ_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::NOTEQ_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::LESS_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::LESSEQ_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::GREATER_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::GREATEREQ_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::SHL2_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::SHR2_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::ASHR2_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::AND2_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::OR2_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::XOR2_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::EXP_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::TYPEOF_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::TYPEOF_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::TONUMBER_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::TONUMERIC_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::NEG_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::NOT_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::INC_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEC_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::ISIN_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::INSTANCEOF_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::STRICTNOTEQ_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::STRICTEQ_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::ISTRUE:
        return false;
    case BytecodeInst<Mode>::Opcode::ISFALSE:
        return false;
    case BytecodeInst<Mode>::Opcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::CALLTHISRANGE_IMM8_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::SUPERCALLARROWRANGE_IMM8_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEFINEFUNC_IMM8_ID16_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEFINEFUNC_IMM16_ID16_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEFINEMETHOD_IMM8_ID16_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEFINEMETHOD_IMM16_ID16_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::CALLARG0_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::SUPERCALLSPREAD_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::APPLY_IMM8_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::CALLARGS2_IMM8_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::CALLARGS3_IMM8_V8_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::CALLRANGE_IMM8_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::LDEXTERNALMODULEVAR_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::LDTHISBYNAME_IMM8_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::LDTHISBYNAME_IMM16_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::STTHISBYNAME_IMM8_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::STTHISBYNAME_IMM16_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::LDTHISBYVALUE_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::LDTHISBYVALUE_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::STTHISBYVALUE_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::STTHISBYVALUE_IMM16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::LDPATCHVAR_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::STPATCHVAR_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DYNAMICIMPORT:
        return false;
    case BytecodeInst<Mode>::Opcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::RESUMEGENERATOR:
        return false;
    case BytecodeInst<Mode>::Opcode::GETRESUMEMODE:
        return false;
    case BytecodeInst<Mode>::Opcode::GETTEMPLATEOBJECT_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::GETTEMPLATEOBJECT_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::GETNEXTPROPNAME_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::JEQZ_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::JEQZ_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::SETOBJECTWITHPROTO_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DELOBJPROP_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::SUSPENDGENERATOR_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONAWAITUNCAUGHT_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::COPYDATAPROPERTIES_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::STARRAYSPREAD_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::SETOBJECTWITHPROTO_IMM16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::LDOBJBYVALUE_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::LDOBJBYVALUE_IMM16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::STOBJBYVALUE_IMM8_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::STOBJBYVALUE_IMM16_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUE_IMM8_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUE_IMM16_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::LDSUPERBYVALUE_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::LDSUPERBYVALUE_IMM16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::STSUPERBYVALUE_IMM8_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::STSUPERBYVALUE_IMM16_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::LDOBJBYINDEX_IMM8_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::LDOBJBYINDEX_IMM16_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::STOBJBYINDEX_IMM8_V8_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::STOBJBYINDEX_IMM16_V8_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::STOWNBYINDEX_IMM8_V8_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::STOWNBYINDEX_IMM16_V8_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONRESOLVE_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONREJECT_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::COPYRESTARGS_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::LDLEXVAR_IMM4_IMM4:
        return false;
    case BytecodeInst<Mode>::Opcode::STLEXVAR_IMM4_IMM4:
        return false;
    case BytecodeInst<Mode>::Opcode::GETMODULENAMESPACE_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::STMODULEVAR_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::TRYLDGLOBALBYNAME_IMM8_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::TRYLDGLOBALBYNAME_IMM16_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::TRYSTGLOBALBYNAME_IMM8_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::TRYSTGLOBALBYNAME_IMM16_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::LDGLOBALVAR_IMM16_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::STGLOBALVAR_IMM16_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::LDOBJBYNAME_IMM8_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::LDOBJBYNAME_IMM16_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::STOBJBYNAME_IMM8_ID16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::STOBJBYNAME_IMM16_ID16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::STOWNBYNAME_IMM8_ID16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::STOWNBYNAME_IMM16_ID16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::LDSUPERBYNAME_IMM8_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::LDSUPERBYNAME_IMM16_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::STSUPERBYNAME_IMM8_ID16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::STSUPERBYNAME_IMM16_ID16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::LDLOCALMODULEVAR_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::STCONSTTOGLOBALRECORD_IMM16_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::STTOGLOBALRECORD_IMM16_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::JEQZ_IMM32:
        return false;
    case BytecodeInst<Mode>::Opcode::JNEZ_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::JNEZ_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::JNEZ_IMM32:
        return false;
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUEWITHNAMESET_IMM8_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUEWITHNAMESET_IMM16_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::LDBIGINT_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::LDA_STR_ID16:
        return true;
    case BytecodeInst<Mode>::Opcode::JMP_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::JMP_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::JMP_IMM32:
        return false;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQZ_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQZ_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQZ_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQZ_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::JEQNULL_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::JEQNULL_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::LDA_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::STA_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::LDAI_IMM32:
        return false;
    case BytecodeInst<Mode>::Opcode::FLDAI_IMM64:
        return false;
    case BytecodeInst<Mode>::Opcode::RETURN:
        return false;
    case BytecodeInst<Mode>::Opcode::RETURNUNDEFINED:
        return false;
    case BytecodeInst<Mode>::Opcode::LDLEXVAR_IMM8_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::JNENULL_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::STLEXVAR_IMM8_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::JNENULL_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::CALLARG1_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQNULL_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQNULL_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQNULL_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQNULL_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::JEQUNDEFINED_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::JEQUNDEFINED_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::JNEUNDEFINED_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::JNEUNDEFINED_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQUNDEFINED_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQUNDEFINED_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQUNDEFINED_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQUNDEFINED_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::JEQ_V8_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::JEQ_V8_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::JNE_V8_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::JNE_V8_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQ_V8_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQ_V8_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQ_V8_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQ_V8_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::MOV_V4_V4:
        return false;
    case BytecodeInst<Mode>::Opcode::MOV_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::MOV_V16_V16:
        return false;
    case BytecodeInst<Mode>::Opcode::ASYNCGENERATORREJECT_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::NOP:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDLEXENV_PREF_NONE:
        return false;
    case BytecodeInst<Mode>::Opcode::WIDE_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::THROW_PREF_NONE:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_POPLEXENV_PREF_NONE:
        return false;
    case BytecodeInst<Mode>::Opcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::THROW_NOTEXISTS_PREF_NONE:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETITERATORNEXT_PREF_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::WIDE_NEWLEXENV_PREF_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::THROW_PATTERNNONCOERCIBLE_PREF_NONE:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CREATEARRAYWITHBUFFER_PREF_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16:
        return false;
    case BytecodeInst<Mode>::Opcode::THROW_DELETESUPERPROPERTY_PREF_NONE:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CREATEOBJECTWITHBUFFER_PREF_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::WIDE_CALLRANGE_PREF_IMM16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::THROW_CONSTASSIGNMENT_PREF_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_TONUMBER_PREF_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::THROW_IFNOTOBJECT_PREF_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_TONUMERIC_PREF_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::THROW_UNDEFINEDIFHOLE_PREF_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_NEG_PREF_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_NOT_PREF_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::WIDE_LDOBJBYINDEX_PREF_IMM32:
        return false;
    case BytecodeInst<Mode>::Opcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_INC_PREF_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::WIDE_STOBJBYINDEX_PREF_V8_IMM32:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DEC_PREF_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::WIDE_STOWNBYINDEX_PREF_V8_IMM32:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARG0_PREF_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::WIDE_COPYRESTARGS_PREF_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARG1_PREF_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::WIDE_LDLEXVAR_PREF_IMM16_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARGS2_PREF_V8_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::WIDE_STLEXVAR_PREF_IMM16_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARGS3_PREF_V8_V8_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::WIDE_GETMODULENAMESPACE_PREF_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLRANGE_PREF_IMM16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::WIDE_STMODULEVAR_PREF_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLSPREAD_PREF_V8_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::WIDE_LDLOCALMODULEVAR_PREF_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLTHISRANGE_PREF_IMM16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::WIDE_LDEXTERNALMODULEVAR_PREF_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::WIDE_LDPATCHVAR_PREF_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_RESUMEGENERATOR_PREF_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::WIDE_STPATCHVAR_PREF_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETRESUMEMODE_PREF_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETTEMPLATEOBJECT_PREF_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DELOBJPROP_PREF_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_SUSPENDGENERATOR_PREF_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_COPYDATAPROPERTIES_PREF_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_SETOBJECTWITHPROTO_PREF_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDOBJBYVALUE_PREF_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDSUPERBYVALUE_PREF_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDOBJBYINDEX_PREF_V8_IMM32:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_ASYNCFUNCTIONREJECT_PREF_V8_V8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLEXVAR_PREF_IMM4_IMM4_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLEXVAR_PREF_IMM8_IMM8_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLEXVAR_PREF_IMM16_IMM16_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETMODULENAMESPACE_PREF_ID32:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STMODULEVAR_PREF_ID32:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDOBJBYNAME_PREF_ID32_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDSUPERBYNAME_PREF_ID32_V8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDMODULEVAR_PREF_ID32_IMM8:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STCONSTTOGLOBALRECORD_PREF_ID32:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLETTOGLOBALRECORD_PREF_ID32:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STCLASSTOGLOBALRECORD_PREF_ID32:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDHOMEOBJECT_PREF_NONE:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CREATEOBJECTHAVINGMETHOD_PREF_IMM16:
        return false;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DYNAMICIMPORT_PREF_V8:
        return false;
    default:
        return false;
    }

    UNREACHABLE();
}

// NOLINTNEXTLINE(readability-function-size)
template<const BytecodeInstMode Mode> std::ostream& operator<<(std::ostream& os, const BytecodeInst<Mode>& inst) {
    switch(inst.GetOpcode()) {
    case BytecodeInst<Mode>::Opcode::LDNAN:
        os << "ldnan";
        break;
    case BytecodeInst<Mode>::Opcode::LDINFINITY:
        os << "ldinfinity";
        break;
    case BytecodeInst<Mode>::Opcode::LDNEWTARGET:
        os << "ldnewtarget";
        break;
    case BytecodeInst<Mode>::Opcode::LDUNDEFINED:
        os << "ldundefined";
        break;
    case BytecodeInst<Mode>::Opcode::LDNULL:
        os << "ldnull";
        break;
    case BytecodeInst<Mode>::Opcode::LDSYMBOL:
        os << "ldsymbol";
        break;
    case BytecodeInst<Mode>::Opcode::LDGLOBAL:
        os << "ldglobal";
        break;
    case BytecodeInst<Mode>::Opcode::LDTRUE:
        os << "ldtrue";
        break;
    case BytecodeInst<Mode>::Opcode::LDFALSE:
        os << "ldfalse";
        break;
    case BytecodeInst<Mode>::Opcode::LDHOLE:
        os << "ldhole";
        break;
    case BytecodeInst<Mode>::Opcode::LDTHIS:
        os << "ldthis";
        break;
    case BytecodeInst<Mode>::Opcode::POPLEXENV:
        os << "poplexenv";
        break;
    case BytecodeInst<Mode>::Opcode::GETUNMAPPEDARGS:
        os << "getunmappedargs";
        break;
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONENTER:
        os << "asyncfunctionenter";
        break;
    case BytecodeInst<Mode>::Opcode::CREATEASYNCGENERATOROBJ_V8:
        os << "createasyncgeneratorobj";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LDFUNCTION:
        os << "ldfunction";
        break;
    case BytecodeInst<Mode>::Opcode::DEBUGGER:
        os << "debugger";
        break;
    case BytecodeInst<Mode>::Opcode::GETPROPITERATOR:
        os << "getpropiterator";
        break;
    case BytecodeInst<Mode>::Opcode::GETITERATOR_IMM8:
        os << "getiterator";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::GETITERATOR_IMM16:
        os << "getiterator";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::CLOSEITERATOR_IMM8_V8:
        os << "closeiterator";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::CLOSEITERATOR_IMM16_V8:
        os << "closeiterator";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::ASYNCGENERATORRESOLVE_V8_V8_V8:
        os << "asyncgeneratorresolve";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_V8_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_V8_V8, 2>();
        break;
    case BytecodeInst<Mode>::Opcode::CREATEEMPTYOBJECT:
        os << "createemptyobject";
        break;
    case BytecodeInst<Mode>::Opcode::CREATEEMPTYARRAY_IMM8:
        os << "createemptyarray";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::CREATEEMPTYARRAY_IMM16:
        os << "createemptyarray";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::CREATEGENERATOROBJ_V8:
        os << "creategeneratorobj";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::CREATEITERRESULTOBJ_V8_V8:
        os << "createiterresultobj";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8:
        os << "createobjectwithexcludedkeys";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::CALLTHIS0_IMM8_V8:
        os << "callthis0";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
        os << "createarraywithbuffer";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM8_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
        os << "createarraywithbuffer";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::CALLTHIS1_IMM8_V8_V8:
        os << "callthis1";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::CALLTHIS2_IMM8_V8_V8_V8:
        os << "callthis2";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8_V8, 2>();
        break;
    case BytecodeInst<Mode>::Opcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
        os << "createobjectwithbuffer";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM8_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::CREATEOBJECTWITHBUFFER_IMM16_ID16:
        os << "createobjectwithbuffer";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8:
        os << "createregexpwithliteral";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16_IMM8, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM8_ID16_IMM8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16_IMM8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::CREATEREGEXPWITHLITERAL_IMM16_ID16_IMM8:
        os << "createregexpwithliteral";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16_IMM8, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16_IMM8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16_IMM8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::NEWOBJAPPLY_IMM8_V8:
        os << "newobjapply";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::NEWOBJAPPLY_IMM16_V8:
        os << "newobjapply";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::NEWOBJRANGE_IMM8_IMM8_V8:
        os << "newobjrange";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_IMM8_V8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_IMM8_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::NEWOBJRANGE_IMM16_IMM8_V8:
        os << "newobjrange";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_IMM8_V8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_IMM8_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::NEWLEXENV_IMM8:
        os << "newlexenv";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::NEWLEXENVWITHNAME_IMM8_ID16:
        os << "newlexenvwithname";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM8_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::ADD2_IMM8_V8:
        os << "add2";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::SUB2_IMM8_V8:
        os << "sub2";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::MUL2_IMM8_V8:
        os << "mul2";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DIV2_IMM8_V8:
        os << "div2";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::MOD2_IMM8_V8:
        os << "mod2";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::EQ_IMM8_V8:
        os << "eq";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::NOTEQ_IMM8_V8:
        os << "noteq";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LESS_IMM8_V8:
        os << "less";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LESSEQ_IMM8_V8:
        os << "lesseq";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::GREATER_IMM8_V8:
        os << "greater";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::GREATEREQ_IMM8_V8:
        os << "greatereq";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::SHL2_IMM8_V8:
        os << "shl2";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::SHR2_IMM8_V8:
        os << "shr2";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::ASHR2_IMM8_V8:
        os << "ashr2";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::AND2_IMM8_V8:
        os << "and2";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::OR2_IMM8_V8:
        os << "or2";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::XOR2_IMM8_V8:
        os << "xor2";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::EXP_IMM8_V8:
        os << "exp";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::TYPEOF_IMM8:
        os << "typeof";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::TYPEOF_IMM16:
        os << "typeof";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::TONUMBER_IMM8:
        os << "tonumber";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::TONUMERIC_IMM8:
        os << "tonumeric";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::NEG_IMM8:
        os << "neg";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::NOT_IMM8:
        os << "not";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::INC_IMM8:
        os << "inc";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEC_IMM8:
        os << "dec";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::ISIN_IMM8_V8:
        os << "isin";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::INSTANCEOF_IMM8_V8:
        os << "instanceof";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STRICTNOTEQ_IMM8_V8:
        os << "strictnoteq";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STRICTEQ_IMM8_V8:
        os << "stricteq";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::ISTRUE:
        os << "istrue";
        break;
    case BytecodeInst<Mode>::Opcode::ISFALSE:
        os << "isfalse";
        break;
    case BytecodeInst<Mode>::Opcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
        os << "callthis3";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8_V8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8_V8_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8_V8_V8, 2>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8_V8_V8, 3>();
        break;
    case BytecodeInst<Mode>::Opcode::CALLTHISRANGE_IMM8_IMM8_V8:
        os << "callthisrange";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_IMM8_V8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_IMM8_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
        os << "supercallthisrange";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_IMM8_V8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_IMM8_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::SUPERCALLARROWRANGE_IMM8_IMM8_V8:
        os << "supercallarrowrange";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_IMM8_V8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_IMM8_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEFINEFUNC_IMM8_ID16_IMM8:
        os << "definefunc";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16_IMM8, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM8_ID16_IMM8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16_IMM8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::DEFINEFUNC_IMM16_ID16_IMM8:
        os << "definefunc";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16_IMM8, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16_IMM8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16_IMM8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::DEFINEMETHOD_IMM8_ID16_IMM8:
        os << "definemethod";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16_IMM8, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM8_ID16_IMM8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16_IMM8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::DEFINEMETHOD_IMM16_ID16_IMM8:
        os << "definemethod";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16_IMM8, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16_IMM8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16_IMM8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::CALLARG0_IMM8:
        os << "callarg0";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::SUPERCALLSPREAD_IMM8_V8:
        os << "supercallspread";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::APPLY_IMM8_V8_V8:
        os << "apply";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::CALLARGS2_IMM8_V8_V8:
        os << "callargs2";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::CALLARGS3_IMM8_V8_V8_V8:
        os << "callargs3";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8_V8, 2>();
        break;
    case BytecodeInst<Mode>::Opcode::CALLRANGE_IMM8_IMM8_V8:
        os << "callrange";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_IMM8_V8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_IMM8_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LDEXTERNALMODULEVAR_IMM8:
        os << "ldexternalmodulevar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LDTHISBYNAME_IMM8_ID16:
        os << "ldthisbyname";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM8_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8:
        os << "definegettersetterbyvalue";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_V8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_V8_V8_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_V8_V8_V8, 2>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_V8_V8_V8, 3>();
        break;
    case BytecodeInst<Mode>::Opcode::LDTHISBYNAME_IMM16_ID16:
        os << "ldthisbyname";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STTHISBYNAME_IMM8_ID16:
        os << "stthisbyname";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM8_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STTHISBYNAME_IMM16_ID16:
        os << "stthisbyname";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LDTHISBYVALUE_IMM8:
        os << "ldthisbyvalue";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LDTHISBYVALUE_IMM16:
        os << "ldthisbyvalue";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STTHISBYVALUE_IMM8_V8:
        os << "stthisbyvalue";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STTHISBYVALUE_IMM16_V8:
        os << "stthisbyvalue";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LDPATCHVAR_IMM8:
        os << "ldpatchvar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STPATCHVAR_IMM8_V8:
        os << "stpatchvar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DYNAMICIMPORT:
        os << "dynamicimport";
        break;
    case BytecodeInst<Mode>::Opcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8:
        os << "defineclasswithbuffer";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16_ID16_IMM16_V8, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM8_ID16_ID16_IMM16_V8, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM8_ID16_ID16_IMM16_V8, 1>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16_ID16_IMM16_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_ID16_ID16_IMM16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8:
        os << "defineclasswithbuffer";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16_ID16_IMM16_V8, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16_ID16_IMM16_V8, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16_ID16_IMM16_V8, 1>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16_ID16_IMM16_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_ID16_ID16_IMM16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::RESUMEGENERATOR:
        os << "resumegenerator";
        break;
    case BytecodeInst<Mode>::Opcode::GETRESUMEMODE:
        os << "getresumemode";
        break;
    case BytecodeInst<Mode>::Opcode::GETTEMPLATEOBJECT_IMM8:
        os << "gettemplateobject";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::GETTEMPLATEOBJECT_IMM16:
        os << "gettemplateobject";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::GETNEXTPROPNAME_V8:
        os << "getnextpropname";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JEQZ_IMM8:
        os << "jeqz";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JEQZ_IMM16:
        os << "jeqz";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::SETOBJECTWITHPROTO_IMM8_V8:
        os << "setobjectwithproto";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DELOBJPROP_V8:
        os << "delobjprop";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::SUSPENDGENERATOR_V8:
        os << "suspendgenerator";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONAWAITUNCAUGHT_V8:
        os << "asyncfunctionawaituncaught";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::COPYDATAPROPERTIES_V8:
        os << "copydataproperties";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STARRAYSPREAD_V8_V8:
        os << "starrayspread";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::SETOBJECTWITHPROTO_IMM16_V8:
        os << "setobjectwithproto";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LDOBJBYVALUE_IMM8_V8:
        os << "ldobjbyvalue";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LDOBJBYVALUE_IMM16_V8:
        os << "ldobjbyvalue";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STOBJBYVALUE_IMM8_V8_V8:
        os << "stobjbyvalue";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::STOBJBYVALUE_IMM16_V8_V8:
        os << "stobjbyvalue";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUE_IMM8_V8_V8:
        os << "stownbyvalue";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUE_IMM16_V8_V8:
        os << "stownbyvalue";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::LDSUPERBYVALUE_IMM8_V8:
        os << "ldsuperbyvalue";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LDSUPERBYVALUE_IMM16_V8:
        os << "ldsuperbyvalue";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STSUPERBYVALUE_IMM8_V8_V8:
        os << "stsuperbyvalue";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::STSUPERBYVALUE_IMM16_V8_V8:
        os << "stsuperbyvalue";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::LDOBJBYINDEX_IMM8_IMM16:
        os << "ldobjbyindex";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_IMM16, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_IMM16, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::LDOBJBYINDEX_IMM16_IMM16:
        os << "ldobjbyindex";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_IMM16, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_IMM16, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::STOBJBYINDEX_IMM8_V8_IMM16:
        os << "stobjbyindex";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8_IMM16, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_IMM16, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8_IMM16, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::STOBJBYINDEX_IMM16_V8_IMM16:
        os << "stobjbyindex";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_V8_IMM16, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_V8_IMM16, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_V8_IMM16, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::STOWNBYINDEX_IMM8_V8_IMM16:
        os << "stownbyindex";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8_IMM16, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_IMM16, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8_IMM16, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::STOWNBYINDEX_IMM16_V8_IMM16:
        os << "stownbyindex";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_V8_IMM16, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_V8_IMM16, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_V8_IMM16, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONRESOLVE_V8:
        os << "asyncfunctionresolve";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONREJECT_V8:
        os << "asyncfunctionreject";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::COPYRESTARGS_IMM8:
        os << "copyrestargs";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LDLEXVAR_IMM4_IMM4:
        os << "ldlexvar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM4_IMM4, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM4_IMM4, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::STLEXVAR_IMM4_IMM4:
        os << "stlexvar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM4_IMM4, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM4_IMM4, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::GETMODULENAMESPACE_IMM8:
        os << "getmodulenamespace";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STMODULEVAR_IMM8:
        os << "stmodulevar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::TRYLDGLOBALBYNAME_IMM8_ID16:
        os << "tryldglobalbyname";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM8_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::TRYLDGLOBALBYNAME_IMM16_ID16:
        os << "tryldglobalbyname";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::TRYSTGLOBALBYNAME_IMM8_ID16:
        os << "trystglobalbyname";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM8_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::TRYSTGLOBALBYNAME_IMM16_ID16:
        os << "trystglobalbyname";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LDGLOBALVAR_IMM16_ID16:
        os << "ldglobalvar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STGLOBALVAR_IMM16_ID16:
        os << "stglobalvar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LDOBJBYNAME_IMM8_ID16:
        os << "ldobjbyname";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM8_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LDOBJBYNAME_IMM16_ID16:
        os << "ldobjbyname";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STOBJBYNAME_IMM8_ID16_V8:
        os << "stobjbyname";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16_V8, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM8_ID16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_ID16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STOBJBYNAME_IMM16_ID16_V8:
        os << "stobjbyname";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16_V8, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_ID16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STOWNBYNAME_IMM8_ID16_V8:
        os << "stownbyname";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16_V8, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM8_ID16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_ID16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STOWNBYNAME_IMM16_ID16_V8:
        os << "stownbyname";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16_V8, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_ID16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LDSUPERBYNAME_IMM8_ID16:
        os << "ldsuperbyname";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM8_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LDSUPERBYNAME_IMM16_ID16:
        os << "ldsuperbyname";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STSUPERBYNAME_IMM8_ID16_V8:
        os << "stsuperbyname";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16_V8, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM8_ID16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_ID16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STSUPERBYNAME_IMM16_ID16_V8:
        os << "stsuperbyname";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16_V8, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_ID16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LDLOCALMODULEVAR_IMM8:
        os << "ldlocalmodulevar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STCONSTTOGLOBALRECORD_IMM16_ID16:
        os << "stconsttoglobalrecord";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STTOGLOBALRECORD_IMM16_ID16:
        os << "sttoglobalrecord";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JEQZ_IMM32:
        os << "jeqz";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM32, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JNEZ_IMM8:
        os << "jnez";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JNEZ_IMM16:
        os << "jnez";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JNEZ_IMM32:
        os << "jnez";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM32, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUEWITHNAMESET_IMM8_V8_V8:
        os << "stownbyvaluewithnameset";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUEWITHNAMESET_IMM16_V8_V8:
        os << "stownbyvaluewithnameset";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8:
        os << "stownbynamewithnameset";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_ID16_V8, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM8_ID16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_ID16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8:
        os << "stownbynamewithnameset";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16_ID16_V8, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::IMM16_ID16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM16_ID16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LDBIGINT_ID16:
        os << "ldbigint";
        os << " id" << inst.template GetId<BytecodeInst<Mode>::Format::ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LDA_STR_ID16:
        os << "lda.str";
        os << " id" << inst.template GetId<BytecodeInst<Mode>::Format::ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JMP_IMM8:
        os << "jmp";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JMP_IMM16:
        os << "jmp";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JMP_IMM32:
        os << "jmp";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM32, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQZ_IMM8:
        os << "jstricteqz";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQZ_IMM16:
        os << "jstricteqz";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQZ_IMM8:
        os << "jnstricteqz";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQZ_IMM16:
        os << "jnstricteqz";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JEQNULL_IMM8:
        os << "jeqnull";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JEQNULL_IMM16:
        os << "jeqnull";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LDA_V8:
        os << "lda";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STA_V8:
        os << "sta";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::LDAI_IMM32:
        os << "ldai";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM32, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::FLDAI_IMM64:
        os << "fldai";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM64, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::RETURN:
        os << "return";
        break;
    case BytecodeInst<Mode>::Opcode::RETURNUNDEFINED:
        os << "returnundefined";
        break;
    case BytecodeInst<Mode>::Opcode::LDLEXVAR_IMM8_IMM8:
        os << "ldlexvar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_IMM8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_IMM8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::JNENULL_IMM8:
        os << "jnenull";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::STLEXVAR_IMM8_IMM8:
        os << "stlexvar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_IMM8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_IMM8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::JNENULL_IMM16:
        os << "jnenull";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::CALLARG1_IMM8_V8:
        os << "callarg1";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQNULL_IMM8:
        os << "jstricteqnull";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQNULL_IMM16:
        os << "jstricteqnull";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQNULL_IMM8:
        os << "jnstricteqnull";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQNULL_IMM16:
        os << "jnstricteqnull";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JEQUNDEFINED_IMM8:
        os << "jequndefined";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JEQUNDEFINED_IMM16:
        os << "jequndefined";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JNEUNDEFINED_IMM8:
        os << "jneundefined";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JNEUNDEFINED_IMM16:
        os << "jneundefined";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQUNDEFINED_IMM8:
        os << "jstrictequndefined";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQUNDEFINED_IMM16:
        os << "jstrictequndefined";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQUNDEFINED_IMM8:
        os << "jnstrictequndefined";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQUNDEFINED_IMM16:
        os << "jnstrictequndefined";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JEQ_V8_IMM8:
        os << "jeq";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_IMM8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::V8_IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JEQ_V8_IMM16:
        os << "jeq";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_IMM16, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::V8_IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JNE_V8_IMM8:
        os << "jne";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_IMM8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::V8_IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JNE_V8_IMM16:
        os << "jne";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_IMM16, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::V8_IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQ_V8_IMM8:
        os << "jstricteq";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_IMM8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::V8_IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQ_V8_IMM16:
        os << "jstricteq";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_IMM16, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::V8_IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQ_V8_IMM8:
        os << "jnstricteq";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_IMM8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::V8_IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQ_V8_IMM16:
        os << "jnstricteq";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_IMM16, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::V8_IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::MOV_V4_V4:
        os << "mov";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V4_V4, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V4_V4, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::MOV_V8_V8:
        os << "mov";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::MOV_V16_V16:
        os << "mov";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V16_V16, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V16_V16, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::ASYNCGENERATORREJECT_V8_V8:
        os << "asyncgeneratorreject";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::NOP:
        os << "nop";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDLEXENV_PREF_NONE:
        os << "deprecated.ldlexenv";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8:
        os << "wide.createobjectwithexcludedkeys";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_IMM16_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_IMM16_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::THROW_PREF_NONE:
        os << "throw";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_POPLEXENV_PREF_NONE:
        os << "deprecated.poplexenv";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8:
        os << "wide.newobjrange";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_IMM16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::THROW_NOTEXISTS_PREF_NONE:
        os << "throw.notexists";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETITERATORNEXT_PREF_V8_V8:
        os << "deprecated.getiteratornext";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_NEWLEXENV_PREF_IMM16:
        os << "wide.newlexenv";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::THROW_PATTERNNONCOERCIBLE_PREF_NONE:
        os << "throw.patternnoncoercible";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CREATEARRAYWITHBUFFER_PREF_IMM16:
        os << "deprecated.createarraywithbuffer";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16:
        os << "wide.newlexenvwithname";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16_ID16, 0>();
        os << ", id" << inst.template GetId<BytecodeInst<Mode>::Format::PREF_IMM16_ID16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::THROW_DELETESUPERPROPERTY_PREF_NONE:
        os << "throw.deletesuperproperty";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CREATEOBJECTWITHBUFFER_PREF_IMM16:
        os << "deprecated.createobjectwithbuffer";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_CALLRANGE_PREF_IMM16_V8:
        os << "wide.callrange";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_IMM16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::THROW_CONSTASSIGNMENT_PREF_V8:
        os << "throw.constassignment";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_TONUMBER_PREF_V8:
        os << "deprecated.tonumber";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8:
        os << "wide.callthisrange";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_IMM16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::THROW_IFNOTOBJECT_PREF_V8:
        os << "throw.ifnotobject";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_TONUMERIC_PREF_V8:
        os << "deprecated.tonumeric";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
        os << "wide.supercallthisrange";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_IMM16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::THROW_UNDEFINEDIFHOLE_PREF_V8_V8:
        os << "throw.undefinedifhole";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_NEG_PREF_V8:
        os << "deprecated.neg";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8:
        os << "wide.supercallarrowrange";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_IMM16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM8:
        os << "throw.ifsupernotcorrectcall";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_NOT_PREF_V8:
        os << "deprecated.not";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_LDOBJBYINDEX_PREF_IMM32:
        os << "wide.ldobjbyindex";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM32, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16:
        os << "throw.ifsupernotcorrectcall";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_INC_PREF_V8:
        os << "deprecated.inc";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_STOBJBYINDEX_PREF_V8_IMM32:
        os << "wide.stobjbyindex";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_IMM32, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_V8_IMM32, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DEC_PREF_V8:
        os << "deprecated.dec";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_STOWNBYINDEX_PREF_V8_IMM32:
        os << "wide.stownbyindex";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_IMM32, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_V8_IMM32, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARG0_PREF_V8:
        os << "deprecated.callarg0";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_COPYRESTARGS_PREF_IMM16:
        os << "wide.copyrestargs";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARG1_PREF_V8_V8:
        os << "deprecated.callarg1";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_LDLEXVAR_PREF_IMM16_IMM16:
        os << "wide.ldlexvar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16_IMM16, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16_IMM16, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARGS2_PREF_V8_V8_V8:
        os << "deprecated.callargs2";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8_V8, 2>();
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_STLEXVAR_PREF_IMM16_IMM16:
        os << "wide.stlexvar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16_IMM16, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16_IMM16, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARGS3_PREF_V8_V8_V8_V8:
        os << "deprecated.callargs3";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8_V8_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8_V8_V8, 2>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8_V8_V8, 3>();
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_GETMODULENAMESPACE_PREF_IMM16:
        os << "wide.getmodulenamespace";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLRANGE_PREF_IMM16_V8:
        os << "deprecated.callrange";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_IMM16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_STMODULEVAR_PREF_IMM16:
        os << "wide.stmodulevar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLSPREAD_PREF_V8_V8_V8:
        os << "deprecated.callspread";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8_V8, 2>();
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_LDLOCALMODULEVAR_PREF_IMM16:
        os << "wide.ldlocalmodulevar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLTHISRANGE_PREF_IMM16_V8:
        os << "deprecated.callthisrange";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_IMM16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_LDEXTERNALMODULEVAR_PREF_IMM16:
        os << "wide.ldexternalmodulevar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8:
        os << "deprecated.defineclasswithbuffer";
        os << " id" << inst.template GetId<BytecodeInst<Mode>::Format::PREF_ID16_IMM16_IMM16_V8_V8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_ID16_IMM16_IMM16_V8_V8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_ID16_IMM16_IMM16_V8_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_ID16_IMM16_IMM16_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_ID16_IMM16_IMM16_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_LDPATCHVAR_PREF_IMM16:
        os << "wide.ldpatchvar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_RESUMEGENERATOR_PREF_V8:
        os << "deprecated.resumegenerator";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_STPATCHVAR_PREF_IMM16:
        os << "wide.stpatchvar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETRESUMEMODE_PREF_V8:
        os << "deprecated.getresumemode";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETTEMPLATEOBJECT_PREF_V8:
        os << "deprecated.gettemplateobject";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DELOBJPROP_PREF_V8_V8:
        os << "deprecated.delobjprop";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_SUSPENDGENERATOR_PREF_V8_V8:
        os << "deprecated.suspendgenerator";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8:
        os << "deprecated.asyncfunctionawaituncaught";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_COPYDATAPROPERTIES_PREF_V8_V8:
        os << "deprecated.copydataproperties";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_SETOBJECTWITHPROTO_PREF_V8_V8:
        os << "deprecated.setobjectwithproto";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDOBJBYVALUE_PREF_V8_V8:
        os << "deprecated.ldobjbyvalue";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDSUPERBYVALUE_PREF_V8_V8:
        os << "deprecated.ldsuperbyvalue";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8, 1>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDOBJBYINDEX_PREF_V8_IMM32:
        os << "deprecated.ldobjbyindex";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_IMM32, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_V8_IMM32, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8:
        os << "deprecated.asyncfunctionresolve";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8_V8, 2>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_ASYNCFUNCTIONREJECT_PREF_V8_V8_V8:
        os << "deprecated.asyncfunctionreject";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8_V8_V8, 2>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLEXVAR_PREF_IMM4_IMM4_V8:
        os << "deprecated.stlexvar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM4_IMM4_V8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM4_IMM4_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_IMM4_IMM4_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLEXVAR_PREF_IMM8_IMM8_V8:
        os << "deprecated.stlexvar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM8_IMM8_V8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM8_IMM8_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_IMM8_IMM8_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLEXVAR_PREF_IMM16_IMM16_V8:
        os << "deprecated.stlexvar";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16_IMM16_V8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16_IMM16_V8, 1>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_IMM16_IMM16_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETMODULENAMESPACE_PREF_ID32:
        os << "deprecated.getmodulenamespace";
        os << " id" << inst.template GetId<BytecodeInst<Mode>::Format::PREF_ID32, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STMODULEVAR_PREF_ID32:
        os << "deprecated.stmodulevar";
        os << " id" << inst.template GetId<BytecodeInst<Mode>::Format::PREF_ID32, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDOBJBYNAME_PREF_ID32_V8:
        os << "deprecated.ldobjbyname";
        os << " id" << inst.template GetId<BytecodeInst<Mode>::Format::PREF_ID32_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_ID32_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDSUPERBYNAME_PREF_ID32_V8:
        os << "deprecated.ldsuperbyname";
        os << " id" << inst.template GetId<BytecodeInst<Mode>::Format::PREF_ID32_V8, 0>();
        os << ", v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_ID32_V8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDMODULEVAR_PREF_ID32_IMM8:
        os << "deprecated.ldmodulevar";
        os << " id" << inst.template GetId<BytecodeInst<Mode>::Format::PREF_ID32_IMM8, 0>();
        os << ", " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_ID32_IMM8, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STCONSTTOGLOBALRECORD_PREF_ID32:
        os << "deprecated.stconsttoglobalrecord";
        os << " id" << inst.template GetId<BytecodeInst<Mode>::Format::PREF_ID32, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLETTOGLOBALRECORD_PREF_ID32:
        os << "deprecated.stlettoglobalrecord";
        os << " id" << inst.template GetId<BytecodeInst<Mode>::Format::PREF_ID32, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STCLASSTOGLOBALRECORD_PREF_ID32:
        os << "deprecated.stclasstoglobalrecord";
        os << " id" << inst.template GetId<BytecodeInst<Mode>::Format::PREF_ID32, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDHOMEOBJECT_PREF_NONE:
        os << "deprecated.ldhomeobject";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CREATEOBJECTHAVINGMETHOD_PREF_IMM16:
        os << "deprecated.createobjecthavingmethod";
        os << " " << inst.template GetImm<BytecodeInst<Mode>::Format::PREF_IMM16, 0>();
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DYNAMICIMPORT_PREF_V8:
        os << "deprecated.dynamicimport";
        os << " v" << inst.template GetVReg<BytecodeInst<Mode>::Format::PREF_V8, 0>();
        break;
    }
    return os;
}

template<const BytecodeInstMode Mode> // NOLINTNEXTLINE(readability-function-size)
std::ostream& operator<<(std::ostream& os, const typename BytecodeInst<Mode>::Opcode& op)
{
    switch(op) {
    case BytecodeInst<Mode>::Opcode::LDNAN:
        os << "LDNAN";
        break;
    case BytecodeInst<Mode>::Opcode::LDINFINITY:
        os << "LDINFINITY";
        break;
    case BytecodeInst<Mode>::Opcode::LDNEWTARGET:
        os << "LDNEWTARGET";
        break;
    case BytecodeInst<Mode>::Opcode::LDUNDEFINED:
        os << "LDUNDEFINED";
        break;
    case BytecodeInst<Mode>::Opcode::LDNULL:
        os << "LDNULL";
        break;
    case BytecodeInst<Mode>::Opcode::LDSYMBOL:
        os << "LDSYMBOL";
        break;
    case BytecodeInst<Mode>::Opcode::LDGLOBAL:
        os << "LDGLOBAL";
        break;
    case BytecodeInst<Mode>::Opcode::LDTRUE:
        os << "LDTRUE";
        break;
    case BytecodeInst<Mode>::Opcode::LDFALSE:
        os << "LDFALSE";
        break;
    case BytecodeInst<Mode>::Opcode::LDHOLE:
        os << "LDHOLE";
        break;
    case BytecodeInst<Mode>::Opcode::LDTHIS:
        os << "LDTHIS";
        break;
    case BytecodeInst<Mode>::Opcode::POPLEXENV:
        os << "POPLEXENV";
        break;
    case BytecodeInst<Mode>::Opcode::GETUNMAPPEDARGS:
        os << "GETUNMAPPEDARGS";
        break;
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONENTER:
        os << "ASYNCFUNCTIONENTER";
        break;
    case BytecodeInst<Mode>::Opcode::CREATEASYNCGENERATOROBJ_V8:
        os << "CREATEASYNCGENERATOROBJ_V8";
        break;
    case BytecodeInst<Mode>::Opcode::LDFUNCTION:
        os << "LDFUNCTION";
        break;
    case BytecodeInst<Mode>::Opcode::DEBUGGER:
        os << "DEBUGGER";
        break;
    case BytecodeInst<Mode>::Opcode::GETPROPITERATOR:
        os << "GETPROPITERATOR";
        break;
    case BytecodeInst<Mode>::Opcode::GETITERATOR_IMM8:
        os << "GETITERATOR_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::GETITERATOR_IMM16:
        os << "GETITERATOR_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::CLOSEITERATOR_IMM8_V8:
        os << "CLOSEITERATOR_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::CLOSEITERATOR_IMM16_V8:
        os << "CLOSEITERATOR_IMM16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::ASYNCGENERATORRESOLVE_V8_V8_V8:
        os << "ASYNCGENERATORRESOLVE_V8_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::CREATEEMPTYOBJECT:
        os << "CREATEEMPTYOBJECT";
        break;
    case BytecodeInst<Mode>::Opcode::CREATEEMPTYARRAY_IMM8:
        os << "CREATEEMPTYARRAY_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::CREATEEMPTYARRAY_IMM16:
        os << "CREATEEMPTYARRAY_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::CREATEGENERATOROBJ_V8:
        os << "CREATEGENERATOROBJ_V8";
        break;
    case BytecodeInst<Mode>::Opcode::CREATEITERRESULTOBJ_V8_V8:
        os << "CREATEITERRESULTOBJ_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8:
        os << "CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::CALLTHIS0_IMM8_V8:
        os << "CALLTHIS0_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
        os << "CREATEARRAYWITHBUFFER_IMM8_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
        os << "CREATEARRAYWITHBUFFER_IMM16_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::CALLTHIS1_IMM8_V8_V8:
        os << "CALLTHIS1_IMM8_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::CALLTHIS2_IMM8_V8_V8_V8:
        os << "CALLTHIS2_IMM8_V8_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
        os << "CREATEOBJECTWITHBUFFER_IMM8_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::CREATEOBJECTWITHBUFFER_IMM16_ID16:
        os << "CREATEOBJECTWITHBUFFER_IMM16_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8:
        os << "CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::CREATEREGEXPWITHLITERAL_IMM16_ID16_IMM8:
        os << "CREATEREGEXPWITHLITERAL_IMM16_ID16_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::NEWOBJAPPLY_IMM8_V8:
        os << "NEWOBJAPPLY_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::NEWOBJAPPLY_IMM16_V8:
        os << "NEWOBJAPPLY_IMM16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::NEWOBJRANGE_IMM8_IMM8_V8:
        os << "NEWOBJRANGE_IMM8_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::NEWOBJRANGE_IMM16_IMM8_V8:
        os << "NEWOBJRANGE_IMM16_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::NEWLEXENV_IMM8:
        os << "NEWLEXENV_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::NEWLEXENVWITHNAME_IMM8_ID16:
        os << "NEWLEXENVWITHNAME_IMM8_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::ADD2_IMM8_V8:
        os << "ADD2_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::SUB2_IMM8_V8:
        os << "SUB2_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::MUL2_IMM8_V8:
        os << "MUL2_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DIV2_IMM8_V8:
        os << "DIV2_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::MOD2_IMM8_V8:
        os << "MOD2_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::EQ_IMM8_V8:
        os << "EQ_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::NOTEQ_IMM8_V8:
        os << "NOTEQ_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::LESS_IMM8_V8:
        os << "LESS_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::LESSEQ_IMM8_V8:
        os << "LESSEQ_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::GREATER_IMM8_V8:
        os << "GREATER_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::GREATEREQ_IMM8_V8:
        os << "GREATEREQ_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::SHL2_IMM8_V8:
        os << "SHL2_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::SHR2_IMM8_V8:
        os << "SHR2_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::ASHR2_IMM8_V8:
        os << "ASHR2_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::AND2_IMM8_V8:
        os << "AND2_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::OR2_IMM8_V8:
        os << "OR2_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::XOR2_IMM8_V8:
        os << "XOR2_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::EXP_IMM8_V8:
        os << "EXP_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::TYPEOF_IMM8:
        os << "TYPEOF_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::TYPEOF_IMM16:
        os << "TYPEOF_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::TONUMBER_IMM8:
        os << "TONUMBER_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::TONUMERIC_IMM8:
        os << "TONUMERIC_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::NEG_IMM8:
        os << "NEG_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::NOT_IMM8:
        os << "NOT_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::INC_IMM8:
        os << "INC_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::DEC_IMM8:
        os << "DEC_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::ISIN_IMM8_V8:
        os << "ISIN_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::INSTANCEOF_IMM8_V8:
        os << "INSTANCEOF_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::STRICTNOTEQ_IMM8_V8:
        os << "STRICTNOTEQ_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::STRICTEQ_IMM8_V8:
        os << "STRICTEQ_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::ISTRUE:
        os << "ISTRUE";
        break;
    case BytecodeInst<Mode>::Opcode::ISFALSE:
        os << "ISFALSE";
        break;
    case BytecodeInst<Mode>::Opcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
        os << "CALLTHIS3_IMM8_V8_V8_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::CALLTHISRANGE_IMM8_IMM8_V8:
        os << "CALLTHISRANGE_IMM8_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
        os << "SUPERCALLTHISRANGE_IMM8_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::SUPERCALLARROWRANGE_IMM8_IMM8_V8:
        os << "SUPERCALLARROWRANGE_IMM8_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEFINEFUNC_IMM8_ID16_IMM8:
        os << "DEFINEFUNC_IMM8_ID16_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::DEFINEFUNC_IMM16_ID16_IMM8:
        os << "DEFINEFUNC_IMM16_ID16_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::DEFINEMETHOD_IMM8_ID16_IMM8:
        os << "DEFINEMETHOD_IMM8_ID16_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::DEFINEMETHOD_IMM16_ID16_IMM8:
        os << "DEFINEMETHOD_IMM16_ID16_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::CALLARG0_IMM8:
        os << "CALLARG0_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::SUPERCALLSPREAD_IMM8_V8:
        os << "SUPERCALLSPREAD_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::APPLY_IMM8_V8_V8:
        os << "APPLY_IMM8_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::CALLARGS2_IMM8_V8_V8:
        os << "CALLARGS2_IMM8_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::CALLARGS3_IMM8_V8_V8_V8:
        os << "CALLARGS3_IMM8_V8_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::CALLRANGE_IMM8_IMM8_V8:
        os << "CALLRANGE_IMM8_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::LDEXTERNALMODULEVAR_IMM8:
        os << "LDEXTERNALMODULEVAR_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::LDTHISBYNAME_IMM8_ID16:
        os << "LDTHISBYNAME_IMM8_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8:
        os << "DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::LDTHISBYNAME_IMM16_ID16:
        os << "LDTHISBYNAME_IMM16_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::STTHISBYNAME_IMM8_ID16:
        os << "STTHISBYNAME_IMM8_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::STTHISBYNAME_IMM16_ID16:
        os << "STTHISBYNAME_IMM16_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::LDTHISBYVALUE_IMM8:
        os << "LDTHISBYVALUE_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::LDTHISBYVALUE_IMM16:
        os << "LDTHISBYVALUE_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::STTHISBYVALUE_IMM8_V8:
        os << "STTHISBYVALUE_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::STTHISBYVALUE_IMM16_V8:
        os << "STTHISBYVALUE_IMM16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::LDPATCHVAR_IMM8:
        os << "LDPATCHVAR_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::STPATCHVAR_IMM8_V8:
        os << "STPATCHVAR_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DYNAMICIMPORT:
        os << "DYNAMICIMPORT";
        break;
    case BytecodeInst<Mode>::Opcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8:
        os << "DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8:
        os << "DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::RESUMEGENERATOR:
        os << "RESUMEGENERATOR";
        break;
    case BytecodeInst<Mode>::Opcode::GETRESUMEMODE:
        os << "GETRESUMEMODE";
        break;
    case BytecodeInst<Mode>::Opcode::GETTEMPLATEOBJECT_IMM8:
        os << "GETTEMPLATEOBJECT_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::GETTEMPLATEOBJECT_IMM16:
        os << "GETTEMPLATEOBJECT_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::GETNEXTPROPNAME_V8:
        os << "GETNEXTPROPNAME_V8";
        break;
    case BytecodeInst<Mode>::Opcode::JEQZ_IMM8:
        os << "JEQZ_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::JEQZ_IMM16:
        os << "JEQZ_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::SETOBJECTWITHPROTO_IMM8_V8:
        os << "SETOBJECTWITHPROTO_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DELOBJPROP_V8:
        os << "DELOBJPROP_V8";
        break;
    case BytecodeInst<Mode>::Opcode::SUSPENDGENERATOR_V8:
        os << "SUSPENDGENERATOR_V8";
        break;
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONAWAITUNCAUGHT_V8:
        os << "ASYNCFUNCTIONAWAITUNCAUGHT_V8";
        break;
    case BytecodeInst<Mode>::Opcode::COPYDATAPROPERTIES_V8:
        os << "COPYDATAPROPERTIES_V8";
        break;
    case BytecodeInst<Mode>::Opcode::STARRAYSPREAD_V8_V8:
        os << "STARRAYSPREAD_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::SETOBJECTWITHPROTO_IMM16_V8:
        os << "SETOBJECTWITHPROTO_IMM16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::LDOBJBYVALUE_IMM8_V8:
        os << "LDOBJBYVALUE_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::LDOBJBYVALUE_IMM16_V8:
        os << "LDOBJBYVALUE_IMM16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::STOBJBYVALUE_IMM8_V8_V8:
        os << "STOBJBYVALUE_IMM8_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::STOBJBYVALUE_IMM16_V8_V8:
        os << "STOBJBYVALUE_IMM16_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUE_IMM8_V8_V8:
        os << "STOWNBYVALUE_IMM8_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUE_IMM16_V8_V8:
        os << "STOWNBYVALUE_IMM16_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::LDSUPERBYVALUE_IMM8_V8:
        os << "LDSUPERBYVALUE_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::LDSUPERBYVALUE_IMM16_V8:
        os << "LDSUPERBYVALUE_IMM16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::STSUPERBYVALUE_IMM8_V8_V8:
        os << "STSUPERBYVALUE_IMM8_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::STSUPERBYVALUE_IMM16_V8_V8:
        os << "STSUPERBYVALUE_IMM16_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::LDOBJBYINDEX_IMM8_IMM16:
        os << "LDOBJBYINDEX_IMM8_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::LDOBJBYINDEX_IMM16_IMM16:
        os << "LDOBJBYINDEX_IMM16_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::STOBJBYINDEX_IMM8_V8_IMM16:
        os << "STOBJBYINDEX_IMM8_V8_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::STOBJBYINDEX_IMM16_V8_IMM16:
        os << "STOBJBYINDEX_IMM16_V8_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::STOWNBYINDEX_IMM8_V8_IMM16:
        os << "STOWNBYINDEX_IMM8_V8_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::STOWNBYINDEX_IMM16_V8_IMM16:
        os << "STOWNBYINDEX_IMM16_V8_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONRESOLVE_V8:
        os << "ASYNCFUNCTIONRESOLVE_V8";
        break;
    case BytecodeInst<Mode>::Opcode::ASYNCFUNCTIONREJECT_V8:
        os << "ASYNCFUNCTIONREJECT_V8";
        break;
    case BytecodeInst<Mode>::Opcode::COPYRESTARGS_IMM8:
        os << "COPYRESTARGS_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::LDLEXVAR_IMM4_IMM4:
        os << "LDLEXVAR_IMM4_IMM4";
        break;
    case BytecodeInst<Mode>::Opcode::STLEXVAR_IMM4_IMM4:
        os << "STLEXVAR_IMM4_IMM4";
        break;
    case BytecodeInst<Mode>::Opcode::GETMODULENAMESPACE_IMM8:
        os << "GETMODULENAMESPACE_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::STMODULEVAR_IMM8:
        os << "STMODULEVAR_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::TRYLDGLOBALBYNAME_IMM8_ID16:
        os << "TRYLDGLOBALBYNAME_IMM8_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::TRYLDGLOBALBYNAME_IMM16_ID16:
        os << "TRYLDGLOBALBYNAME_IMM16_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::TRYSTGLOBALBYNAME_IMM8_ID16:
        os << "TRYSTGLOBALBYNAME_IMM8_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::TRYSTGLOBALBYNAME_IMM16_ID16:
        os << "TRYSTGLOBALBYNAME_IMM16_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::LDGLOBALVAR_IMM16_ID16:
        os << "LDGLOBALVAR_IMM16_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::STGLOBALVAR_IMM16_ID16:
        os << "STGLOBALVAR_IMM16_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::LDOBJBYNAME_IMM8_ID16:
        os << "LDOBJBYNAME_IMM8_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::LDOBJBYNAME_IMM16_ID16:
        os << "LDOBJBYNAME_IMM16_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::STOBJBYNAME_IMM8_ID16_V8:
        os << "STOBJBYNAME_IMM8_ID16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::STOBJBYNAME_IMM16_ID16_V8:
        os << "STOBJBYNAME_IMM16_ID16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::STOWNBYNAME_IMM8_ID16_V8:
        os << "STOWNBYNAME_IMM8_ID16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::STOWNBYNAME_IMM16_ID16_V8:
        os << "STOWNBYNAME_IMM16_ID16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::LDSUPERBYNAME_IMM8_ID16:
        os << "LDSUPERBYNAME_IMM8_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::LDSUPERBYNAME_IMM16_ID16:
        os << "LDSUPERBYNAME_IMM16_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::STSUPERBYNAME_IMM8_ID16_V8:
        os << "STSUPERBYNAME_IMM8_ID16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::STSUPERBYNAME_IMM16_ID16_V8:
        os << "STSUPERBYNAME_IMM16_ID16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::LDLOCALMODULEVAR_IMM8:
        os << "LDLOCALMODULEVAR_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::STCONSTTOGLOBALRECORD_IMM16_ID16:
        os << "STCONSTTOGLOBALRECORD_IMM16_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::STTOGLOBALRECORD_IMM16_ID16:
        os << "STTOGLOBALRECORD_IMM16_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::JEQZ_IMM32:
        os << "JEQZ_IMM32";
        break;
    case BytecodeInst<Mode>::Opcode::JNEZ_IMM8:
        os << "JNEZ_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::JNEZ_IMM16:
        os << "JNEZ_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::JNEZ_IMM32:
        os << "JNEZ_IMM32";
        break;
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUEWITHNAMESET_IMM8_V8_V8:
        os << "STOWNBYVALUEWITHNAMESET_IMM8_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::STOWNBYVALUEWITHNAMESET_IMM16_V8_V8:
        os << "STOWNBYVALUEWITHNAMESET_IMM16_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8:
        os << "STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8:
        os << "STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::LDBIGINT_ID16:
        os << "LDBIGINT_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::LDA_STR_ID16:
        os << "LDA_STR_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::JMP_IMM8:
        os << "JMP_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::JMP_IMM16:
        os << "JMP_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::JMP_IMM32:
        os << "JMP_IMM32";
        break;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQZ_IMM8:
        os << "JSTRICTEQZ_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQZ_IMM16:
        os << "JSTRICTEQZ_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQZ_IMM8:
        os << "JNSTRICTEQZ_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQZ_IMM16:
        os << "JNSTRICTEQZ_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::JEQNULL_IMM8:
        os << "JEQNULL_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::JEQNULL_IMM16:
        os << "JEQNULL_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::LDA_V8:
        os << "LDA_V8";
        break;
    case BytecodeInst<Mode>::Opcode::STA_V8:
        os << "STA_V8";
        break;
    case BytecodeInst<Mode>::Opcode::LDAI_IMM32:
        os << "LDAI_IMM32";
        break;
    case BytecodeInst<Mode>::Opcode::FLDAI_IMM64:
        os << "FLDAI_IMM64";
        break;
    case BytecodeInst<Mode>::Opcode::RETURN:
        os << "RETURN";
        break;
    case BytecodeInst<Mode>::Opcode::RETURNUNDEFINED:
        os << "RETURNUNDEFINED";
        break;
    case BytecodeInst<Mode>::Opcode::LDLEXVAR_IMM8_IMM8:
        os << "LDLEXVAR_IMM8_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::JNENULL_IMM8:
        os << "JNENULL_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::STLEXVAR_IMM8_IMM8:
        os << "STLEXVAR_IMM8_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::JNENULL_IMM16:
        os << "JNENULL_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::CALLARG1_IMM8_V8:
        os << "CALLARG1_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQNULL_IMM8:
        os << "JSTRICTEQNULL_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQNULL_IMM16:
        os << "JSTRICTEQNULL_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQNULL_IMM8:
        os << "JNSTRICTEQNULL_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQNULL_IMM16:
        os << "JNSTRICTEQNULL_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::JEQUNDEFINED_IMM8:
        os << "JEQUNDEFINED_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::JEQUNDEFINED_IMM16:
        os << "JEQUNDEFINED_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::JNEUNDEFINED_IMM8:
        os << "JNEUNDEFINED_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::JNEUNDEFINED_IMM16:
        os << "JNEUNDEFINED_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQUNDEFINED_IMM8:
        os << "JSTRICTEQUNDEFINED_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQUNDEFINED_IMM16:
        os << "JSTRICTEQUNDEFINED_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQUNDEFINED_IMM8:
        os << "JNSTRICTEQUNDEFINED_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQUNDEFINED_IMM16:
        os << "JNSTRICTEQUNDEFINED_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::JEQ_V8_IMM8:
        os << "JEQ_V8_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::JEQ_V8_IMM16:
        os << "JEQ_V8_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::JNE_V8_IMM8:
        os << "JNE_V8_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::JNE_V8_IMM16:
        os << "JNE_V8_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQ_V8_IMM8:
        os << "JSTRICTEQ_V8_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::JSTRICTEQ_V8_IMM16:
        os << "JSTRICTEQ_V8_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQ_V8_IMM8:
        os << "JNSTRICTEQ_V8_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::JNSTRICTEQ_V8_IMM16:
        os << "JNSTRICTEQ_V8_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::MOV_V4_V4:
        os << "MOV_V4_V4";
        break;
    case BytecodeInst<Mode>::Opcode::MOV_V8_V8:
        os << "MOV_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::MOV_V16_V16:
        os << "MOV_V16_V16";
        break;
    case BytecodeInst<Mode>::Opcode::ASYNCGENERATORREJECT_V8_V8:
        os << "ASYNCGENERATORREJECT_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::NOP:
        os << "NOP";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDLEXENV_PREF_NONE:
        os << "DEPRECATED_LDLEXENV_PREF_NONE";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8:
        os << "WIDE_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::THROW_PREF_NONE:
        os << "THROW_PREF_NONE";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_POPLEXENV_PREF_NONE:
        os << "DEPRECATED_POPLEXENV_PREF_NONE";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8:
        os << "WIDE_NEWOBJRANGE_PREF_IMM16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::THROW_NOTEXISTS_PREF_NONE:
        os << "THROW_NOTEXISTS_PREF_NONE";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETITERATORNEXT_PREF_V8_V8:
        os << "DEPRECATED_GETITERATORNEXT_PREF_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_NEWLEXENV_PREF_IMM16:
        os << "WIDE_NEWLEXENV_PREF_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::THROW_PATTERNNONCOERCIBLE_PREF_NONE:
        os << "THROW_PATTERNNONCOERCIBLE_PREF_NONE";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CREATEARRAYWITHBUFFER_PREF_IMM16:
        os << "DEPRECATED_CREATEARRAYWITHBUFFER_PREF_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16:
        os << "WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16";
        break;
    case BytecodeInst<Mode>::Opcode::THROW_DELETESUPERPROPERTY_PREF_NONE:
        os << "THROW_DELETESUPERPROPERTY_PREF_NONE";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CREATEOBJECTWITHBUFFER_PREF_IMM16:
        os << "DEPRECATED_CREATEOBJECTWITHBUFFER_PREF_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_CALLRANGE_PREF_IMM16_V8:
        os << "WIDE_CALLRANGE_PREF_IMM16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::THROW_CONSTASSIGNMENT_PREF_V8:
        os << "THROW_CONSTASSIGNMENT_PREF_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_TONUMBER_PREF_V8:
        os << "DEPRECATED_TONUMBER_PREF_V8";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8:
        os << "WIDE_CALLTHISRANGE_PREF_IMM16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::THROW_IFNOTOBJECT_PREF_V8:
        os << "THROW_IFNOTOBJECT_PREF_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_TONUMERIC_PREF_V8:
        os << "DEPRECATED_TONUMERIC_PREF_V8";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
        os << "WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::THROW_UNDEFINEDIFHOLE_PREF_V8_V8:
        os << "THROW_UNDEFINEDIFHOLE_PREF_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_NEG_PREF_V8:
        os << "DEPRECATED_NEG_PREF_V8";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8:
        os << "WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM8:
        os << "THROW_IFSUPERNOTCORRECTCALL_PREF_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_NOT_PREF_V8:
        os << "DEPRECATED_NOT_PREF_V8";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_LDOBJBYINDEX_PREF_IMM32:
        os << "WIDE_LDOBJBYINDEX_PREF_IMM32";
        break;
    case BytecodeInst<Mode>::Opcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16:
        os << "THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_INC_PREF_V8:
        os << "DEPRECATED_INC_PREF_V8";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_STOBJBYINDEX_PREF_V8_IMM32:
        os << "WIDE_STOBJBYINDEX_PREF_V8_IMM32";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DEC_PREF_V8:
        os << "DEPRECATED_DEC_PREF_V8";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_STOWNBYINDEX_PREF_V8_IMM32:
        os << "WIDE_STOWNBYINDEX_PREF_V8_IMM32";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARG0_PREF_V8:
        os << "DEPRECATED_CALLARG0_PREF_V8";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_COPYRESTARGS_PREF_IMM16:
        os << "WIDE_COPYRESTARGS_PREF_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARG1_PREF_V8_V8:
        os << "DEPRECATED_CALLARG1_PREF_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_LDLEXVAR_PREF_IMM16_IMM16:
        os << "WIDE_LDLEXVAR_PREF_IMM16_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARGS2_PREF_V8_V8_V8:
        os << "DEPRECATED_CALLARGS2_PREF_V8_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_STLEXVAR_PREF_IMM16_IMM16:
        os << "WIDE_STLEXVAR_PREF_IMM16_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLARGS3_PREF_V8_V8_V8_V8:
        os << "DEPRECATED_CALLARGS3_PREF_V8_V8_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_GETMODULENAMESPACE_PREF_IMM16:
        os << "WIDE_GETMODULENAMESPACE_PREF_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLRANGE_PREF_IMM16_V8:
        os << "DEPRECATED_CALLRANGE_PREF_IMM16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_STMODULEVAR_PREF_IMM16:
        os << "WIDE_STMODULEVAR_PREF_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLSPREAD_PREF_V8_V8_V8:
        os << "DEPRECATED_CALLSPREAD_PREF_V8_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_LDLOCALMODULEVAR_PREF_IMM16:
        os << "WIDE_LDLOCALMODULEVAR_PREF_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CALLTHISRANGE_PREF_IMM16_V8:
        os << "DEPRECATED_CALLTHISRANGE_PREF_IMM16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_LDEXTERNALMODULEVAR_PREF_IMM16:
        os << "WIDE_LDEXTERNALMODULEVAR_PREF_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8:
        os << "DEPRECATED_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_LDPATCHVAR_PREF_IMM16:
        os << "WIDE_LDPATCHVAR_PREF_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_RESUMEGENERATOR_PREF_V8:
        os << "DEPRECATED_RESUMEGENERATOR_PREF_V8";
        break;
    case BytecodeInst<Mode>::Opcode::WIDE_STPATCHVAR_PREF_IMM16:
        os << "WIDE_STPATCHVAR_PREF_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETRESUMEMODE_PREF_V8:
        os << "DEPRECATED_GETRESUMEMODE_PREF_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETTEMPLATEOBJECT_PREF_V8:
        os << "DEPRECATED_GETTEMPLATEOBJECT_PREF_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DELOBJPROP_PREF_V8_V8:
        os << "DEPRECATED_DELOBJPROP_PREF_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_SUSPENDGENERATOR_PREF_V8_V8:
        os << "DEPRECATED_SUSPENDGENERATOR_PREF_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8:
        os << "DEPRECATED_ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_COPYDATAPROPERTIES_PREF_V8_V8:
        os << "DEPRECATED_COPYDATAPROPERTIES_PREF_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_SETOBJECTWITHPROTO_PREF_V8_V8:
        os << "DEPRECATED_SETOBJECTWITHPROTO_PREF_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDOBJBYVALUE_PREF_V8_V8:
        os << "DEPRECATED_LDOBJBYVALUE_PREF_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDSUPERBYVALUE_PREF_V8_V8:
        os << "DEPRECATED_LDSUPERBYVALUE_PREF_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDOBJBYINDEX_PREF_V8_IMM32:
        os << "DEPRECATED_LDOBJBYINDEX_PREF_V8_IMM32";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8:
        os << "DEPRECATED_ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_ASYNCFUNCTIONREJECT_PREF_V8_V8_V8:
        os << "DEPRECATED_ASYNCFUNCTIONREJECT_PREF_V8_V8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLEXVAR_PREF_IMM4_IMM4_V8:
        os << "DEPRECATED_STLEXVAR_PREF_IMM4_IMM4_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLEXVAR_PREF_IMM8_IMM8_V8:
        os << "DEPRECATED_STLEXVAR_PREF_IMM8_IMM8_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLEXVAR_PREF_IMM16_IMM16_V8:
        os << "DEPRECATED_STLEXVAR_PREF_IMM16_IMM16_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_GETMODULENAMESPACE_PREF_ID32:
        os << "DEPRECATED_GETMODULENAMESPACE_PREF_ID32";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STMODULEVAR_PREF_ID32:
        os << "DEPRECATED_STMODULEVAR_PREF_ID32";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDOBJBYNAME_PREF_ID32_V8:
        os << "DEPRECATED_LDOBJBYNAME_PREF_ID32_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDSUPERBYNAME_PREF_ID32_V8:
        os << "DEPRECATED_LDSUPERBYNAME_PREF_ID32_V8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDMODULEVAR_PREF_ID32_IMM8:
        os << "DEPRECATED_LDMODULEVAR_PREF_ID32_IMM8";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STCONSTTOGLOBALRECORD_PREF_ID32:
        os << "DEPRECATED_STCONSTTOGLOBALRECORD_PREF_ID32";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STLETTOGLOBALRECORD_PREF_ID32:
        os << "DEPRECATED_STLETTOGLOBALRECORD_PREF_ID32";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_STCLASSTOGLOBALRECORD_PREF_ID32:
        os << "DEPRECATED_STCLASSTOGLOBALRECORD_PREF_ID32";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_LDHOMEOBJECT_PREF_NONE:
        os << "DEPRECATED_LDHOMEOBJECT_PREF_NONE";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_CREATEOBJECTHAVINGMETHOD_PREF_IMM16:
        os << "DEPRECATED_CREATEOBJECTHAVINGMETHOD_PREF_IMM16";
        break;
    case BytecodeInst<Mode>::Opcode::DEPRECATED_DYNAMICIMPORT_PREF_V8:
        os << "DEPRECATED_DYNAMICIMPORT_PREF_V8";
        break;
    default:
        os << "(unknown opcode:) " << static_cast<uint16_t>(op);
        break;

    }
    return os;
}

template <const BytecodeInstMode Mode>
inline bool BytecodeInst<Mode>::IsPrimaryOpcodeValid() const
{
    auto opcode = GetPrimaryOpcode();
    // NOLINTNEXTLINE(readability-magic-numbers)
    if (((opcode >= 216) &&
        // NOLINTNEXTLINE(readability-magic-numbers)
        (opcode <= 251)) ||
        // NOLINTNEXTLINE(readability-magic-numbers)
        ((opcode >= 255) &&
        // NOLINTNEXTLINE(readability-magic-numbers)
        (opcode <= 255))) {
        // NOLINTNEXTLINE(readability-simplify-boolean-expr)
        return false;
    }
    return true;
}

}  // namespace panda::ecmascript

#endif  // LIBPANDAFILE_BYTECODE_INSTRUCTION_INL_H
