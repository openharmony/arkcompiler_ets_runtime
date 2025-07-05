/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JS_BIGINT_H
#define ECMASCRIPT_JS_BIGINT_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_object.h"
#include "ecmascript/mem/mem_common.h"
#include "ecmascript/mem/space.h"

#include "securec.h"

namespace panda::ecmascript {
enum class Operate : uint32_t { AND = 0, OR, XOR };
enum class ComparisonResult;
class JSThread;

class BigInt : public TaggedObject {
public:
    static constexpr uint32_t DATEBITS = sizeof(uint32_t) * 8; // 8 : one-bit number of bytes
    static constexpr uint32_t MAXBITS = 1_MB; // 1 MB : Maximum space that can be opened up
    static constexpr uint32_t kMaxLengthBits = 1 << 30;  // ~1 billion.
    static constexpr uint32_t MAXSIZE = MAXBITS / DATEBITS; // the maximum value of size
    static constexpr uint32_t MAXOCTALVALUE = 7; // 7 : max octal value
    static constexpr uint32_t BINARY = 2; // 2 : binary

    static constexpr uint32_t OCTAL = 8; // 8 : octal
    static constexpr uint32_t DECIMAL = 10; // 10 : decimal
    static constexpr uint32_t HEXADECIMAL = 16; // 16 : hexadecimal
    static constexpr uint32_t HALFDATEBITS = DATEBITS / 2;
    static constexpr uint32_t HALFUINT32VALUE = 1U << HALFDATEBITS;
    static constexpr uint32_t HALFDATEMASK = HALFUINT32VALUE - 1;
    CAST_CHECK(BigInt, IsBigInt);
    template <MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE>
    static JSHandle<BigInt> CreateBigint(JSThread *thread, uint32_t size);

    static bool Equal(const JSTaggedValue &x, const JSTaggedValue &y);
    static PUBLIC_API bool SameValue(const JSTaggedValue &x, const JSTaggedValue &y);
    static bool SameValueZero(const JSTaggedValue &x, const JSTaggedValue &y);

    static JSHandle<BigInt> BitwiseOp(JSThread *thread, Operate op, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSHandle<BigInt> BitwiseAND(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSHandle<BigInt> BitwiseXOR(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSHandle<BigInt> BitwiseOR(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSHandle<BigInt> BitwiseSubOne(JSThread *thread, JSHandle<BigInt> bigint, uint32_t maxLen);
    static JSHandle<BigInt> BitwiseAddOne(JSThread *thread, JSHandle<BigInt> bigint);
    static JSHandle<EcmaString> ToString(JSThread *thread, JSHandle<BigInt> bigint,
                                         uint32_t conversionToRadix = BigInt::DECIMAL);
    template <typename DstType>
    static void AppendToCString(DstType &str, BigInt *bigint, uint32_t conversionToRadix = BigInt::DECIMAL);

    CString ToStdString(uint32_t conversionToRadix) const;

    static JSHandle<BigInt> UnaryMinus(JSThread *thread, JSHandle<BigInt> x);
    static JSHandle<BigInt> BitwiseNOT(JSThread *thread, JSHandle<BigInt> x);
    static JSHandle<BigInt> Exponentiate(JSThread *thread, JSHandle<BigInt> base, JSHandle<BigInt> exponent);
    static std::tuple<uint32_t, uint32_t> Mul(uint32_t x, uint32_t y);
    static JSHandle<BigInt> Multiply(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static uint32_t DivideAndRemainder(uint32_t highBit, uint32_t lowBit, uint32_t divisor, uint32_t& remainder);
    static JSHandle<BigInt> FormatLeftShift(JSThread *thread, uint32_t shift, JSHandle<BigInt> bigint,
                                            bool neeedAddOne);
    static void UnformattedRightShift(JSHandle<BigInt> bigint, uint32_t shift);
    static bool SpecialMultiplyAndSub(JSHandle<BigInt> u, JSHandle<BigInt> v, uint32_t q, JSHandle<BigInt> qv,
                                      uint32_t pos);
    static uint32_t SpecialAdd(JSHandle<BigInt> u, JSHandle<BigInt> v, uint32_t pos);
    static uint32_t ImproveAccuracy(uint32_t vHighest, uint32_t vHighestNext, uint32_t UHighest,
                                    uint32_t UHighestNext, uint32_t q);
    static JSHandle<BigInt> DivideAndRemainderWithBigintDivisor(JSThread *thread, JSHandle<BigInt> dividend,
                                                                JSHandle<BigInt> divisor,
                                                                JSMutableHandle<BigInt> &remainder);
    static JSHandle<BigInt> DivideAndRemainderWithUint32Divisor(JSThread *thread, JSHandle<BigInt> dividend,
                                                                uint32_t divisor, JSMutableHandle<BigInt> &remainder);
    static JSHandle<BigInt> Divide(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSHandle<BigInt> Remainder(JSThread *thread, JSHandle<BigInt> n, JSHandle<BigInt> d);
    static JSHandle<BigInt> BigintAddOne(JSThread *thread, JSHandle<BigInt> x);
    static JSHandle<BigInt> BigintSubOne(JSThread *thread, JSHandle<BigInt> x);
    static JSHandle<BigInt> Copy(JSThread *thread, JSHandle<BigInt> x, uint32_t len);

    static JSHandle<BigInt> Add(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSHandle<BigInt> Subtract(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static bool LessThan(const JSTaggedValue &x, const JSTaggedValue &y);
    static ComparisonResult Compare(const JSTaggedValue &x, const JSTaggedValue &y);
    static JSHandle<BigInt> SignedRightShift(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSHandle<BigInt> ReturnIfRightShiftOverMax(JSThread *thread, bool sign);
    static void RightShift(JSHandle<BigInt> bigint, JSHandle<BigInt> x, uint32_t digitMove, uint32_t bitsMove);
    static void JudgeRoundDown(JSHandle<BigInt> x, uint32_t digitMove, uint32_t bitsMove, uint32_t &needLen,
                                bool &roundDown);
    static JSHandle<BigInt> RightShiftHelper(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSTaggedValue UnsignedRightShift(JSThread *thread);
    static JSHandle<BigInt> LeftShift(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSHandle<BigInt> LeftShiftHelper(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y);
    static JSHandle<BigInt> BigintAdd(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y, bool resultSign);
    static JSHandle<BigInt> BigintSub(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y, bool resultSign);

    static JSTaggedValue NumberToBigInt(JSThread *thread, JSHandle<JSTaggedValue> number);
    static JSTaggedValue PUBLIC_API DoubleToBigInt(JSThread *thread, double value);
    static JSHandle<BigInt> PUBLIC_API Int32ToBigInt(JSThread *thread, const int &number);
    static JSHandle<BigInt> Uint32ToBigInt(JSThread *thread, const uint32_t &number);
    static JSHandle<BigInt> Int64ToBigInt(JSThread *thread, const int64_t &number);
    static JSHandle<BigInt> Uint64ToBigInt(JSThread *thread, const uint64_t &number);
    int64_t ToInt64();
    uint64_t ToUint64();
    static void BigIntToInt64(JSThread *thread, JSHandle<JSTaggedValue> bigint, int64_t *cValue, bool *lossless);
    static void BigIntToUint64(JSThread *thread, JSHandle<JSTaggedValue> bigint, uint64_t *cValue, bool *lossless);
    static JSHandle<BigInt> CreateBigWords(JSThread *thread, bool sign, uint32_t size, const uint64_t* words);
    static JSHandle<BigInt> FloorMod(JSThread *thread, JSHandle<BigInt> leftVal, JSHandle<BigInt> rightVal);
    static JSTaggedValue AsUintN(JSThread *thread, JSTaggedNumber &bits, JSHandle<BigInt> bigint);
    static JSTaggedValue AsintN(JSThread *thread, JSTaggedNumber &bits, JSHandle<BigInt> bigint);
    static JSTaggedNumber BigIntToNumber(JSHandle<BigInt> bigint);
    static ComparisonResult CompareWithNumber(JSHandle<BigInt> bigint, JSHandle<JSTaggedValue> number);
    static JSHandle<BigInt> CreateUint64MaxBigInt(JSThread *thread);
    static JSHandle<BigInt> CreateInt64MaxBigInt(JSThread *thread);
    static JSHandle<BigInt> GetUint64MaxBigInt(JSThread *thread);
    static JSHandle<BigInt> GetInt64MaxBigInt(JSThread *thread);
    static inline size_t ComputeSize(uint32_t length)
    {
        return DATA_OFFSET + sizeof(uint32_t) * length;
    }

    inline uint32_t *GetData() const
    {
        return reinterpret_cast<uint32_t *>(ToUintPtr(this) + DATA_OFFSET);
    }

    inline void InitializationZero()
    {
        uint32_t size = GetLength() * sizeof(uint32_t);
        if (memset_s(GetData(), size, 0, size) != EOK) {
            LOG_FULL(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
    }

    inline bool IsZero()
    {
        return GetLength() == 1 && !GetDigit(0);
    }

    inline uint32_t GetDigit(uint32_t index) const
    {
        ASSERT(index < GetLength());
        return Barriers::GetPrimitive<uint32_t>(GetData(), sizeof(uint32_t) * index);
    }

    inline void SetDigit(uint32_t index, uint32_t digit)
    {
        ASSERT(index < GetLength());
        Barriers::SetPrimitive<uint32_t>(GetData(), sizeof(uint32_t) * index, digit);
    }

    static constexpr size_t LENGTH_OFFSET = TaggedObjectSize();
    ACCESSORS_PRIMITIVE_FIELD(Length, uint32_t, LENGTH_OFFSET, BIT_FIELD_OFFSET)
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    static constexpr size_t DATA_OFFSET = SIZE;

    DECL_VISIT_ARRAY(DATA_OFFSET, 0, GetPointerLength());

    // define BitField
    static constexpr size_t SIGN_BITS = 1;
    FIRST_BIT_FIELD(BitField, Sign, bool, SIGN_BITS)

    DECL_DUMP()

private:
    static bool Equal(const BigInt *x, const BigInt *y);
    static bool LessThan(const BigInt *x, const BigInt *y);
    static ComparisonResult Compare(const BigInt *x, const BigInt *y);
    static ComparisonResult AbsolutelyCompare(const BigInt *x, const BigInt *y);
    inline uint32_t IsUint32() const
    {
        return GetLength() == 1;
    }

    inline size_t GetPointerLength()
    {
        size_t byteSize = DataSize(this);
        return AlignUp(byteSize, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)) / sizeof(JSTaggedType);
    }

    static inline size_t DataSize(BigInt *bigInt)
    {
        uint32_t length = bigInt->GetLength();
        return length * sizeof(uint32_t);
    }
};

class BigIntHelper {
public:
    static CString Conversion(const CString &num, uint32_t conversionToRadix, uint32_t currentRadix);
    static JSHandle<BigInt> SetBigInt(JSThread *thread, const CString &numStr,
                                      uint32_t currentRadix = BigInt::DECIMAL);
    static CString GetBinary(const BigInt *bigint);
    static JSHandle<BigInt> RightTruncate(JSThread *thread, JSHandle<BigInt> x);

    static void DeZero(CString &a);

    static uint32_t AddHelper(uint32_t x, uint32_t y, uint32_t &bigintCarry);
    static uint32_t SubHelper(uint32_t x, uint32_t y, uint32_t &bigintCarry);
};
static_assert((BigInt::DATA_OFFSET % static_cast<uint8_t>(MemAlignment::MEM_ALIGN_OBJECT)) == 0);
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TAGGED_BIGINT_H
