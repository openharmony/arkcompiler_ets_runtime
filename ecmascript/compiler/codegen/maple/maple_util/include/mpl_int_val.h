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

#ifndef MAPLE_UTIL_INCLUDE_MPL_INT_VAL_H
#define MAPLE_UTIL_INCLUDE_MPL_INT_VAL_H

#include "mir_type.h"

namespace maple {

/// @brief this class provides different operations on signed and unsigned integers with arbitrary bit-width
class IntVal {
public:
    /// @brief create zero value with zero bit-width
    IntVal() : value(0), width(0), sign(false) {}

    IntVal(uint64 val, uint8 bitWidth, bool isSigned) : value(val), width(bitWidth), sign(isSigned)
    {
        DEBUG_ASSERT(width <= valBitSize && width != 0, "bit-width is too wide");
        TruncInPlace();
    }

    IntVal(uint64 val, PrimType type) : IntVal(val, GetPrimTypeActualBitSize(type), IsSignedInteger(type))
    {
        DEBUG_ASSERT(IsPrimitiveInteger(type), "Type must be integral");
    }

    IntVal(const IntVal &val) : IntVal(val.value, val.width, val.sign) {}

    IntVal(const IntVal &val, PrimType type) : IntVal(val.value, type) {}

    IntVal(const IntVal &val, uint8 bitWidth, bool isSigned) : IntVal(val.value, bitWidth, isSigned) {}

    IntVal(const IntVal &val, bool isSigned) : IntVal(val.value, val.width, isSigned) {}

    IntVal &operator=(const IntVal &other)
    {
        if (width == 0) {
            // Allow 'this' to be assigned with new bit-width and sign iff
            // its original bit-width is zero (i.e. the value was created by the default ctor)
            Assign(other);
        } else {
            // Otherwise, assign only new value, but sign and width must be the same
            DEBUG_ASSERT(width == other.width && sign == other.sign, "different bit-width or sign");
            value = other.value;
        }

        return *this;
    }

    IntVal &operator=(uint64 other)
    {
        DEBUG_ASSERT(width != 0, "can't be assigned to value with unknown size");
        value = other;
        TruncInPlace();

        return *this;
    }

    void Assign(const IntVal &other)
    {
        value = other.value;
        width = other.width;
        sign = other.sign;
    }

    /// @return bit-width of the value
    uint8 GetBitWidth() const
    {
        return width;
    }

    /// @return true if the value is signed
    bool IsSigned() const
    {
        return sign;
    }

    /// @return sign or zero extended value depending on its signedness
    int64 GetExtValue(uint8 size = 0) const
    {
        return sign ? GetSXTValue(size) : GetZXTValue(size);
    }

    /// @return zero extended value
    uint64 GetZXTValue(uint8 size = 0) const
    {
        // if size == 0, just return the value itself because it's already truncated for an appropriate width
        return size ? (value << (valBitSize - size)) >> (valBitSize - size) : value;
    }

    /// @return sign extended value
    int64 GetSXTValue(uint8 size = 0) const
    {
        uint8 bitWidth = size ? size : width;
        return static_cast<int64>(value << (valBitSize - bitWidth)) >> (valBitSize - bitWidth);
    }

    /// @return true if the (most significant bit) MSB is set
    bool GetSignBit() const
    {
        return GetBit(width - 1);
    }

    /// @return true if all bits are 1
    bool AreAllBitsOne() const
    {
        return value == (allOnes >> (valBitSize - width));
    }

    /// @return true if the value is maximum considering its signedness
    bool IsMaxValue() const
    {
        return sign ? value == ((uint64(1) << (width - 1)) - 1) : AreAllBitsOne();
    }

    /// @return true if the value is minimum considering its signedness
    bool IsMinValue() const
    {
        return sign ? value == (uint64(1) << (width - 1)) : value == 0;
    }

    // Comparison operators that manipulate on values with the same sign and bit-width
    bool operator==(const IntVal &rhs) const
    {
        DEBUG_ASSERT(width == rhs.width && sign == rhs.sign, "bit-width and sign must be the same");
        return value == rhs.value;
    }

    bool operator!=(const IntVal &rhs) const
    {
        return !(*this == rhs);
    }

    bool operator<(const IntVal &rhs) const
    {
        DEBUG_ASSERT(width == rhs.width && sign == rhs.sign, "bit-width and sign must be the same");
        return sign ? GetSXTValue() < rhs.GetSXTValue() : value < rhs.value;
    }

    bool operator>(const IntVal &rhs) const
    {
        DEBUG_ASSERT(width == rhs.width && sign == rhs.sign, "bit-width and sign must be the same");
        return sign ? GetSXTValue() > rhs.GetSXTValue() : value > rhs.value;
    }

    bool operator<=(const IntVal &rhs) const
    {
        return !(*this > rhs);
    }

    bool operator>=(const IntVal &rhs) const
    {
        return !(*this < rhs);
    }

    // Arithmetic and bitwise operators that manipulate on values with the same sign and bit-width
    IntVal operator+(const IntVal &val) const
    {
        DEBUG_ASSERT(width == val.width && sign == val.sign, "bit-width and sign must be the same");
        return IntVal(value + val.value, width, sign);
    }

    IntVal operator-(const IntVal &val) const
    {
        DEBUG_ASSERT(width == val.width && sign == val.sign, "bit-width and sign must be the same");
        return IntVal(value - val.value, width, sign);
    }

    IntVal operator*(const IntVal &val) const
    {
        DEBUG_ASSERT(width == val.width && sign == val.sign, "bit-width and sign must be the same");
        return IntVal(value * val.value, width, sign);
    }

    IntVal operator/(const IntVal &divisor) const
    {
        DEBUG_ASSERT(width == divisor.width && sign == divisor.sign, "bit-width and sign must be the same");
        DEBUG_ASSERT(divisor.value != 0, "division by zero");
        DEBUG_ASSERT(!sign || (!IsMinValue() || !divisor.AreAllBitsOne()), "minValue / -1 leads to overflow");

        bool isNeg = sign && GetSignBit();
        bool isDivisorNeg = divisor.sign && divisor.GetSignBit();

        uint64 dividendVal = isNeg ? (-*this).value : value;
        uint64 divisorVal = isDivisorNeg ? (-divisor).value : divisor.value;

        return isNeg != isDivisorNeg ? -IntVal(dividendVal / divisorVal, width, sign)
                                     : IntVal(dividendVal / divisorVal, width, sign);
    }

    IntVal operator%(const IntVal &divisor) const
    {
        DEBUG_ASSERT(width == divisor.width && sign == divisor.sign, "bit-width and sign must be the same");
        DEBUG_ASSERT(divisor.value != 0, "division by zero");
        DEBUG_ASSERT(!sign || (!IsMinValue() || !divisor.AreAllBitsOne()), "minValue % -1 leads to overflow");

        bool isNeg = sign && GetSignBit();
        bool isDivisorNeg = divisor.sign && divisor.GetSignBit();

        uint64 dividendVal = isNeg ? (-*this).value : value;
        uint64 divisorVal = isDivisorNeg ? (-divisor).value : divisor.value;

        return isNeg ? -IntVal(dividendVal % divisorVal, width, sign) : IntVal(dividendVal % divisorVal, width, sign);
    }

    IntVal operator&(const IntVal &val) const
    {
        DEBUG_ASSERT(width == val.width && sign == val.sign, "bit-width and sign must be the same");
        return IntVal(value & val.value, width, sign);
    }

    IntVal operator|(const IntVal &val) const
    {
        DEBUG_ASSERT(width == val.width && sign == val.sign, "bit-width and sign must be the same");
        return IntVal(value | val.value, width, sign);
    }

    IntVal operator^(const IntVal &val) const
    {
        DEBUG_ASSERT(width == val.width && sign == val.sign, "bit-width and sign must be the same");
        return IntVal(value ^ val.value, width, sign);
    }

    /// @brief shift-left operator
    IntVal operator<<(uint64 bits) const
    {
        DEBUG_ASSERT(bits <= width, "invalid shift value");
        return IntVal(value << bits, width, sign);
    }

    IntVal operator<<(const IntVal &bits) const
    {
        return *this << bits.value;
    }

    /// @brief shift-right operator
    /// @note if value is signed this operator works as arithmetic shift-right operator,
    ///       otherwise it's logical shift-right operator
    IntVal operator>>(uint64 bits) const
    {
        DEBUG_ASSERT(bits <= width, "invalid shift value");
        return IntVal(sign ? GetSXTValue() >> bits : value >> bits, width, sign);
    }

    IntVal operator>>(const IntVal &bits) const
    {
        return *this >> bits.value;
    }

    // Comparison operators that compare values obtained from primitive type.
    /// @note Note that these functions work as follows:
    ///          1) sign or zero extend both values (*this and/or rhs) to the new bit-width
    ///             obtained from pType depending on their original signedness;
    ///             or truncate the values if their original bit-width is greater than new one
    ///          2) then perform the operation itself on new given values that have the same bit-width and sign
    /// @warning it's better to avoid using these function in favor of operator==, operator< etc
    bool Equal(const IntVal &rhs, PrimType pType) const
    {
        return TruncOrExtend(pType) == rhs.TruncOrExtend(pType);
    }

    bool Less(const IntVal &rhs, PrimType pType) const
    {
        return TruncOrExtend(pType) < rhs.TruncOrExtend(pType);
    }

    bool Greater(const IntVal &rhs, PrimType pType) const
    {
        return TruncOrExtend(pType) > rhs.TruncOrExtend(pType);
    }

    // Arithmetic and bitwise operators that allow creating a new value
    // with the bit-width and sign obtained from primitive type
    /// @note Note that these functions work as follows:
    ///          1) sign or zero extend both values (*this and rhs) to the new bit-width
    ///             obtained from pType depending on their original signedness;
    ///             or truncate the values if their original bit-width is greater than new one
    ///          2) then perform the operation itself on new given values that have the same bit-width and sign
    /// @warning it's better to avoid using these function in favor of operator+, operator- etc
    IntVal Add(const IntVal &val, PrimType pType) const
    {
        return TruncOrExtend(pType) + val.TruncOrExtend(pType);
    }

    IntVal Sub(const IntVal &val, PrimType pType) const
    {
        return TruncOrExtend(pType) - val.TruncOrExtend(pType);
    }

    IntVal Mul(const IntVal &val, PrimType pType) const
    {
        return TruncOrExtend(pType) * val.TruncOrExtend(pType);
    }

    IntVal Div(const IntVal &divisor, PrimType pType) const
    {
        return TruncOrExtend(pType) / divisor.TruncOrExtend(pType);
    }

    // sign division in terms of new bitWidth
    IntVal SDiv(const IntVal &divisor, uint8 bitWidth) const
    {
        return TruncOrExtend(bitWidth, true) / divisor.TruncOrExtend(bitWidth, true);
    }

    // unsigned division in terms of new bitWidth
    IntVal UDiv(const IntVal &divisor, uint8 bitWidth) const
    {
        return TruncOrExtend(bitWidth, false) / divisor.TruncOrExtend(bitWidth, false);
    }

    // unsigned division in terms of new bitWidth
    IntVal Rem(const IntVal &divisor, PrimType pType) const
    {
        return TruncOrExtend(pType) % divisor.TruncOrExtend(pType);
    }

    // signed modulo in terms of new bitWidth
    IntVal SRem(const IntVal &divisor, uint8 bitWidth) const
    {
        return TruncOrExtend(bitWidth, true) % divisor.TruncOrExtend(bitWidth, true);
    }

    // unsigned modulo in terms of new bitWidth
    IntVal URem(const IntVal &divisor, uint8 bitWidth) const
    {
        return TruncOrExtend(bitWidth, false) % divisor.TruncOrExtend(bitWidth, false);
    }

    IntVal And(const IntVal &val, PrimType pType) const
    {
        return TruncOrExtend(pType) & val.TruncOrExtend(pType);
    }

    IntVal Or(const IntVal &val, PrimType pType) const
    {
        return TruncOrExtend(pType) | val.TruncOrExtend(pType);
    }

    IntVal Xor(const IntVal &val, PrimType pType) const
    {
        return TruncOrExtend(pType) ^ val.TruncOrExtend(pType);
    }

    // left-shift operators
    IntVal Shl(const IntVal &shift, PrimType pType) const
    {
        return Shl(shift.value, pType);
    }

    IntVal Shl(uint64 shift, PrimType pType) const
    {
        return TruncOrExtend(pType) << shift;
    }

    // logical right-shift operators (MSB is zero extended)
    IntVal LShr(const IntVal &shift, PrimType pType) const
    {
        return LShr(shift.value, pType);
    }

    IntVal LShr(uint64 shift, PrimType pType) const
    {
        IntVal ret = TruncOrExtend(pType);

        DEBUG_ASSERT(shift <= ret.width, "invalid shift value");
        ret.value >>= shift;

        return ret;
    }

    IntVal LShr(uint64 shift) const
    {
        DEBUG_ASSERT(shift <= width, "invalid shift value");
        return IntVal(value >> shift, width, sign);
    }

    // arithmetic right-shift operators (MSB is sign extended)
    IntVal AShr(const IntVal &shift, PrimType pType) const
    {
        return AShr(shift.value, pType);
    }

    IntVal AShr(uint64 shift, PrimType pType) const
    {
        IntVal ret = TruncOrExtend(pType);

        DEBUG_ASSERT(shift <= ret.width, "invalid shift value");
        ret.value = ret.GetSXTValue() >> shift;
        ret.TruncInPlace();

        return ret;
    }

    /// @brief invert all bits of value
    IntVal operator~() const
    {
        return IntVal(~value, width, sign);
    }

    /// @return negated value
    IntVal operator-() const
    {
        return IntVal(~value + 1, width, sign);
    }

    /// @brief truncate value to the given bit-width in-place.
    /// @note if bitWidth is not passed (or zero), the original
    ///       bit-width is preserved and the value is truncated to the original bit-width
    void TruncInPlace(uint8 bitWidth = 0)
    {
        DEBUG_ASSERT(valBitSize >= bitWidth, "invalid bit-width for truncate");

        value &= allOnes >> (valBitSize - (bitWidth ? bitWidth : width));

        if (bitWidth) {
            width = bitWidth;
        }
    }

    /// @return truncated value to the given bit-width
    /// @note returned value will have bit-width and sign obtained from newType
    IntVal Trunc(PrimType newType) const
    {
        return Trunc(GetPrimTypeActualBitSize(newType), IsSignedInteger(newType));
    }

    /// @return sign or zero extended value depending on its signedness
    /// @note returned value will have bit-width and sign obtained from newType
    IntVal Extend(PrimType newType) const
    {
        return Extend(GetPrimTypeActualBitSize(newType), IsSignedInteger(newType));
    }

    /// @return sign/zero extended value or truncated value depending on bit-width
    /// @note returned value will have bit-width and sign obtained from newType
    IntVal TruncOrExtend(PrimType newType) const
    {
        return TruncOrExtend(GetPrimTypeActualBitSize(newType), IsSignedInteger(newType));
    }

    IntVal TruncOrExtend(uint8 newWidth, bool isSigned) const
    {
        return newWidth <= width ? Trunc(newWidth, isSigned) : Extend(newWidth, isSigned);
    }

private:
    bool GetBit(uint8 bit) const
    {
        DEBUG_ASSERT(bit < width, "Required bit is out of value range");
        return (value & (uint64(1) << bit)) != 0;
    }

    IntVal Trunc(uint8 newWidth, bool isSigned) const
    {
        return {value, newWidth, isSigned};
    }

    IntVal Extend(uint8 newWidth, bool isSigned) const
    {
        DEBUG_ASSERT(newWidth > width, "invalid size for extension");
        return IntVal(GetExtValue(), newWidth, isSigned);
    }

    static constexpr uint8 valBitSize = sizeof(uint64) * CHAR_BIT;
    static constexpr uint64 allOnes = uint64(~0);

    uint64 value;
    uint8 width;
    bool sign;
};

// Additional comparison operators
inline bool operator==(const IntVal &v1, int64 v2)
{
    return v1.GetExtValue() == v2;
}

inline bool operator==(int64 v1, const IntVal &v2)
{
    return v2 == v1;
}

inline bool operator!=(const IntVal &v1, int64 v2)
{
    return !(v1 == v2);
}

inline bool operator!=(int64 v1, const IntVal &v2)
{
    return !(v2 == v1);
}

/// @return the smaller of two values
/// @note bit-width and sign must be the same for both parameters
inline IntVal Min(const IntVal &a, const IntVal &b)
{
    return a < b ? a : b;
}

/// @return the smaller of two values in terms of newType
/// @note returned value will have bit-width and sign obtained from newType
inline IntVal Min(const IntVal &a, const IntVal &b, PrimType newType)
{
    return a.Less(b, newType) ? IntVal(a, newType) : IntVal(b, newType);
}

/// @return the larger of two values
/// @note bit-width and sign must be the same for both parameters
inline IntVal Max(const IntVal &a, const IntVal &b)
{
    return Min(a, b) == a ? b : a;
}

/// @return the larger of two values in terms of newType
/// @note returned value will have bit-width and sign obtained from newType
inline IntVal Max(const IntVal &a, const IntVal &b, PrimType newType)
{
    return Min(a, b, newType) == a ? b : a;
}

/// @brief dump IntVal object to the output stream
std::ostream &operator<<(std::ostream &os, const IntVal &value);

// Arithmetic operators that manipulate on scalar (uint64) value and IntVal object
// in terms of sign and bit-width of IntVal object
inline IntVal operator+(const IntVal &v1, uint64 v2)
{
    return v1 + IntVal(v2, v1.GetBitWidth(), v1.IsSigned());
}

inline IntVal operator+(uint64 v1, const IntVal &v2)
{
    return v2 + v1;
}

inline IntVal operator-(const IntVal &v1, uint64 v2)
{
    return v1 - IntVal(v2, v1.GetBitWidth(), v1.IsSigned());
}

inline IntVal operator-(uint64 v1, const IntVal &v2)
{
    return IntVal(v1, v2.GetBitWidth(), v2.IsSigned()) - v2;
}

inline IntVal operator*(const IntVal &v1, uint64 v2)
{
    return v1 * IntVal(v2, v1.GetBitWidth(), v1.IsSigned());
}

inline IntVal operator*(uint64 v1, const IntVal &v2)
{
    return v2 * v1;
}

inline IntVal operator/(const IntVal &v1, uint64 v2)
{
    return v1 / IntVal(v2, v1.GetBitWidth(), v1.IsSigned());
}

inline IntVal operator/(uint64 v1, const IntVal &v2)
{
    return IntVal(v1, v2.GetBitWidth(), v2.IsSigned()) / v2;
}

inline IntVal operator%(const IntVal &v1, uint64 v2)
{
    return v1 % IntVal(v2, v1.GetBitWidth(), v1.IsSigned());
}

inline IntVal operator%(uint64 v1, const IntVal &v2)
{
    return IntVal(v1, v2.GetBitWidth(), v2.IsSigned()) % v2;
}

inline IntVal operator&(const IntVal &v1, uint64 v2)
{
    return v1 & IntVal(v2, v1.GetBitWidth(), v1.IsSigned());
}

inline IntVal operator&(uint64 v1, const IntVal &v2)
{
    return v2 & v1;
}

}  // namespace maple

#endif  // MAPLE_UTIL_INCLUDE_MPL_INT_VAL_H
