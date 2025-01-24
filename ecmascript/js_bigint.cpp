/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/js_bigint.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript {
class ObjectFactory;
constexpr char dp[] = "0123456789abcdefghijklmnopqrstuvwxyz";
static int CharToInt(char c)
{
    uint32_t res = 0;
    if (c >= '0' && c <= '9') {
        res = c - '0';
    } else if (c >= 'A' && c <= 'Z') {
        res = c - 'A' + 10; // 10:res must Greater than 10.
    } else if (c >= 'a' && c <= 'z') {
        res = c - 'a' + 10; // 10:res must Greater than 10
    }
    return static_cast<int>(res);
}

static void Division(CString &num, uint32_t conversionToRadix, uint32_t currentRadix, uint32_t &remain)
{
    ASSERT(conversionToRadix != 0);
    uint32_t temp = 0;
    remain = 0;
    for (size_t i = 0; i < num.size(); i++) {
        temp = (currentRadix * remain + static_cast<uint32_t>(CharToInt(num[i])));
        num[i] = dp[temp / conversionToRadix];
        remain = temp % conversionToRadix;
    }
    size_t count = 0;
    while (count < num.size() && num[count] == '0') {
        count++;
    }
    num = num.substr(count);
}

CString BigIntHelper::Conversion(const CString &num, uint32_t conversionToRadix, uint32_t currentRadix)
{
    ASSERT(conversionToRadix != 0);
    CString newNum = num;
    CString res;
    uint32_t remain = 0;
    while (newNum.size() != 0) {
        Division(newNum, conversionToRadix, currentRadix, remain);
        res = dp[remain] + res;
    }
    return res;
}

JSHandle<BigInt> BigInt::CreateUint64MaxBigInt(JSThread *thread)
{
    JSHandle<BigInt> bigint = CreateBigint(thread, 3); // 3:The number of digits in an object of type BigInt
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    bigint->SetDigit(0, 0);
    bigint->SetDigit(1, 0);
    bigint->SetDigit(2, 1); // 2:The number of digits in an object of type BigInt
    return bigint;
}

JSHandle<BigInt> BigInt::CreateInt64MaxBigInt(JSThread *thread)
{
    JSHandle<BigInt> bigint = CreateBigint(thread, 2); // 2:The number of digits in an object of type BigInt
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    bigint->SetDigit(0, 0);
    bigint->SetDigit(1, 0x80000000); // 0x80000000:Int MAX
    return bigint;
}

JSHandle<BigInt> BigInt::GetUint64MaxBigInt(JSThread *thread)
{
    return JSHandle<BigInt>::Cast(thread->GlobalConstants()->GetHandledUint64MaxBigInt());
}

JSHandle<BigInt> BigInt::GetInt64MaxBigInt(JSThread *thread)
{
    return JSHandle<BigInt>::Cast(thread->GlobalConstants()->GetHandledInt64MaxBigInt());
}

JSHandle<BigInt> BigIntHelper::SetBigInt(JSThread *thread, const CString &numStr, uint32_t currentRadix)
{
    int flag = 0;
    if (numStr[0] == '-') {
        flag = 1;
    }

    CString binaryStr = "";
    if (currentRadix != BigInt::BINARY) {
        binaryStr = Conversion(numStr.substr(flag), BigInt::BINARY, currentRadix);
    } else {
        binaryStr = numStr.substr(flag);
    }

    JSHandle<BigInt> bigint;
    size_t binaryStrLen = binaryStr.size();
    size_t len = binaryStrLen / BigInt::DATEBITS;
    size_t mod = binaryStrLen % BigInt::DATEBITS;
    int index = 0;
    if (mod == 0) {
        index = static_cast<int>(len - 1);
        bigint = BigInt::CreateBigint(thread, len);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    } else {
        len++;
        index = static_cast<int>(len - 1);
        bigint = BigInt::CreateBigint(thread, len);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
        uint32_t val = 0;
        for (size_t i = 0; i < mod; ++i) {
            val <<= 1;
            val |= static_cast<uint32_t>(binaryStr[i] - '0');
        }
        bigint->SetDigit(index, val);
        index--;
    }
    if (flag == 1) {
        bigint->SetSign(true);
    }
    size_t i = mod;
    while (i < binaryStrLen) {
        uint32_t val = 0;
        for (size_t j = 0; j < BigInt::DATEBITS && i < binaryStrLen; ++j, ++i) {
            val <<= 1;
            val |= static_cast<uint32_t>(binaryStr[i] - '0');
        }
        bigint->SetDigit(index, val);
        index--;
    }
    return BigIntHelper::RightTruncate(thread, bigint);
}

JSHandle<BigInt> BigIntHelper::RightTruncate(JSThread *thread, JSHandle<BigInt> x)
{
    int len = static_cast<int>(x->GetLength());
    ASSERT(len != 0);
    if (len == 1 && x->GetDigit(0) == 0) {
        x->SetSign(false);
        return x;
    }
    int index = len - 1;
    if (x->GetDigit(index) != 0) {
        return x;
    }
    while (index >= 0) {
        if (x->GetDigit(index) != 0) {
            break;
        }
        index--;
    }

    if (index == -1) {
        return BigInt::Int32ToBigInt(thread, 0);
    } else {
        ASSERT(index >= 0);
        return BigInt::Copy(thread, x, index + 1);
    }
}

CString BigIntHelper::GetBinary(const BigInt *bigint)
{
    ASSERT(bigint != nullptr);
    int index = 0;
    int len = static_cast<int>(bigint->GetLength());
    int strLen = BigInt::DATEBITS * len;
    CString res(strLen, '0');
    int strIndex = strLen - 1;
    while (index < len) {
        int bityLen = BigInt::DATEBITS;
        uint32_t val = bigint->GetDigit(index);
        while (bityLen--) {
            res[strIndex--] = (val & 1) + '0';
            val = val >> 1;
        }
        index++;
    }
    DeZero(res);
    return res;
}

JSHandle<BigInt> BigInt::CreateBigint(JSThread *thread, uint32_t length)
{
    if (length > MAXSIZE) {
        JSHandle<BigInt> bigint(thread, JSTaggedValue::Exception());
        THROW_RANGE_ERROR_AND_RETURN(thread, "Maximum BigInt size exceeded", bigint);
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<BigInt> bigint = factory->NewBigInt(length);
    return bigint;
}

// 6.1.6.2.13
bool BigInt::Equal(const JSTaggedValue &x, const JSTaggedValue &y)
{
    BigInt* xVal = BigInt::Cast(x.GetTaggedObject());
    BigInt* yVal = BigInt::Cast(y.GetTaggedObject());
    return Equal(xVal, yVal);
}

bool BigInt::Equal(const BigInt *x, const BigInt *y)
{
    ASSERT(x != nullptr);
    ASSERT(y != nullptr);
    if (x->GetSign() != y->GetSign() || x->GetLength() != y->GetLength()) {
        return false;
    }
    for (uint32_t i = 0; i < x->GetLength(); ++i) {
        if (x->GetDigit(i) != y->GetDigit(i)) {
            return false;
        }
    }
    return true;
}

// 6.1.6.2.14
bool BigInt::SameValue(const JSTaggedValue &x, const JSTaggedValue &y)
{
    return Equal(x, y);
}

// 6.1.6.2.15
bool BigInt::SameValueZero(const JSTaggedValue &x, const JSTaggedValue &y)
{
    return Equal(x, y);
}

JSHandle<BigInt> BigInt::BitwiseOp(JSThread *thread, Operate op, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    uint32_t maxLen = 0;
    uint32_t minLen = 0;
    uint32_t xlen = x->GetLength();
    uint32_t ylen = y->GetLength();
    if (xlen > ylen) {
        maxLen = xlen;
        minLen = ylen;
    } else {
        maxLen = ylen;
        minLen = xlen;
    }
    JSHandle<BigInt> bigint = BigInt::CreateBigint(thread, maxLen);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    for (size_t i = 0; i < minLen; ++i) {
        if (op == Operate::OR) {
            bigint->SetDigit(i, x->GetDigit(i) | y->GetDigit(i));
        } else if (op == Operate::AND) {
            bigint->SetDigit(i, x->GetDigit(i) & y->GetDigit(i));
        } else {
            ASSERT(op == Operate::XOR);
            bigint->SetDigit(i, x->GetDigit(i) ^ y->GetDigit(i));
        }
    }
    if (op == Operate::OR || op == Operate::XOR) {
        if (xlen > ylen) {
            for (size_t i = ylen; i < xlen; ++i) {
                bigint->SetDigit(i, x->GetDigit(i));
            }
        } else if (ylen > xlen) {
            for (size_t i = xlen; i < ylen; ++i) {
                bigint->SetDigit(i, y->GetDigit(i));
            }
        }
    }
    return BigIntHelper::RightTruncate(thread, bigint);
}

JSHandle<BigInt> OneIsNegativeAND(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    JSHandle<BigInt> yVal = BigInt::BitwiseSubOne(thread, y, y->GetLength());
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    uint32_t xLength = x->GetLength();
    uint32_t yLength = yVal->GetLength();
    uint32_t minLen = xLength;
    if (xLength > yLength) {
        minLen = yLength;
    }
    JSHandle<BigInt> newBigint = BigInt::CreateBigint(thread, xLength);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    uint32_t i = 0;
    while (i < minLen) {
        uint32_t res = x->GetDigit(i) & ~(yVal->GetDigit(i));
        newBigint->SetDigit(i, res);
        ++i;
    }
    while (i < xLength) {
        newBigint->SetDigit(i, x->GetDigit(i));
        ++i;
    }
    return BigIntHelper::RightTruncate(thread, newBigint);
}

// 6.1.6.2.20 BigInt::bitwiseAND ( x, y )
JSHandle<BigInt> BigInt::BitwiseAND(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    if (x->GetSign() && y->GetSign()) {
        // (-x) & (-y) == -(((x-1) | (y-1)) + 1)
        JSHandle<BigInt> xVal = BitwiseSubOne(thread, x, x->GetLength());
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
        JSHandle<BigInt> yVal = BitwiseSubOne(thread, y, y->GetLength());
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
        JSHandle<BigInt> temp = BitwiseOp(thread, Operate::OR, xVal, yVal);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
        JSHandle<BigInt> res = BitwiseAddOne(thread, temp);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
        return res;
    }
    if (x->GetSign() != y->GetSign()) {
        // x & (-y) == x & ~(y-1)
        if (!x->GetSign()) {
            return OneIsNegativeAND(thread, x, y);
        } else {
            return OneIsNegativeAND(thread, y, x);
        }
    }
    return BitwiseOp(thread, Operate::AND, x, y);
}

JSHandle<BigInt> OneIsNegativeXOR(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    JSHandle<BigInt> yVal = BigInt::BitwiseSubOne(thread, y, y->GetLength());
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    JSHandle<BigInt> temp = BigInt::BitwiseOp(thread, Operate::XOR, x, yVal);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    JSHandle<BigInt> res = BigInt::BitwiseAddOne(thread, temp);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    return res;
}

// 6.1.6.2.21 BigInt::bitwiseOR ( x, y )
JSHandle<BigInt> BigInt::BitwiseXOR(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    if (x->GetSign() && y->GetSign()) {
        // (-x) ^ (-y) == (x-1) ^ (y-1)
        JSHandle<BigInt> xVal = BitwiseSubOne(thread, x, x->GetLength());
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
        JSHandle<BigInt> yVal = BitwiseSubOne(thread, y, y->GetLength());
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
        return BitwiseOp(thread, Operate::XOR, xVal, yVal);
    }
    if (x->GetSign() != y->GetSign()) {
        // x ^ (-y) == -((x ^ (y-1)) + 1)
        if (!x->GetSign()) {
            return OneIsNegativeXOR(thread, x, y);
        } else {
            return OneIsNegativeXOR(thread, y, x);
        }
    }
    return BitwiseOp(thread, Operate::XOR, x, y);
}

JSHandle<BigInt> BigInt::BitwiseSubOne(JSThread *thread, JSHandle<BigInt> bigint, uint32_t maxLen)
{
    ASSERT(!bigint->IsZero());
    ASSERT(maxLen >= bigint->GetLength());

    JSHandle<BigInt> newBigint = BigInt::CreateBigint(thread, maxLen);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    uint32_t bigintLen = bigint->GetLength();
    uint32_t carry = 1;
    for (uint32_t i = 0; i < bigintLen; i++) {
        uint32_t bigintCarry = 0;
        newBigint->SetDigit(i, BigIntHelper::SubHelper(bigint->GetDigit(i), carry, bigintCarry));
        carry = bigintCarry;
    }
    ASSERT(!carry);
    return BigIntHelper::RightTruncate(thread, newBigint);
}

JSHandle<BigInt> BigInt::BitwiseAddOne(JSThread *thread, JSHandle<BigInt> bigint)
{
    uint32_t bigintLength = bigint->GetLength();

    bool needExpend = true;
    for (uint32_t i = 0; i < bigintLength; i++) {
        if (std::numeric_limits<uint32_t>::max() != bigint->GetDigit(i)) {
            needExpend = false;
            break;
        }
    }
    uint32_t newLength = bigintLength;
    if (needExpend) {
        newLength += 1;
    }
    JSHandle<BigInt> newBigint = BigInt::CreateBigint(thread, newLength);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    uint32_t carry = 1;
    for (uint32_t i = 0; i < bigintLength; i++) {
        uint32_t bigintCarry = 0;
        newBigint->SetDigit(i, BigIntHelper::AddHelper(bigint->GetDigit(i), carry, bigintCarry));
        carry = bigintCarry;
    }
    if (needExpend) {
        newBigint->SetDigit(bigintLength, carry);
    }
    newBigint->SetSign(true);
    return BigIntHelper::RightTruncate(thread, newBigint);
}

JSHandle<BigInt> OneIsNegativeOR(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    uint32_t xLength = x->GetLength();
    uint32_t maxLen = xLength;
    if (maxLen < y->GetLength()) {
        maxLen = y->GetLength();
    }
    JSHandle<BigInt> yVal = BigInt::BitwiseSubOne(thread, y, maxLen);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    uint32_t yLength = yVal->GetLength();
    uint32_t minLen = xLength;
    if (minLen > yLength) {
        minLen = yLength;
    }
    JSHandle<BigInt> newBigint = BigInt::CreateBigint(thread, yLength);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    uint32_t i = 0;
    while (i < minLen) {
        uint32_t res = ~(x->GetDigit(i)) & yVal->GetDigit(i);
        newBigint->SetDigit(i, res);
        ++i;
    }
    while (i < yLength) {
        newBigint->SetDigit(i, yVal->GetDigit(i));
        ++i;
    }
    JSHandle<BigInt> temp = BigIntHelper::RightTruncate(thread, newBigint);
    JSHandle<BigInt> res = BigInt::BitwiseAddOne(thread, temp);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    res->SetSign(true);
    return res;
}

// 6.1.6.2.22 BigInt::bitwiseOR ( x, y )
JSHandle<BigInt> BigInt::BitwiseOR(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    if (x->GetSign() && y->GetSign()) {
        // (-x) | (-y) == -(((x-1) & (y-1)) + 1)
        uint32_t maxLen = x->GetLength();
        uint32_t yLen = y->GetLength();
        maxLen < yLen ? maxLen = yLen : 0;
        JSHandle<BigInt> xVal = BitwiseSubOne(thread, x, maxLen);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
        JSHandle<BigInt> yVal = BitwiseSubOne(thread, y, yLen);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
        JSHandle<BigInt> temp = BitwiseOp(thread, Operate::AND, xVal, yVal);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
        JSHandle<BigInt> res = BitwiseAddOne(thread, temp);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
        res->SetSign(true);
        return res;
    }
    if (x->GetSign() != y->GetSign()) {
        // x | (-y) == -(((y-1) & ~x) + 1)
        if (!x->GetSign()) {
            return OneIsNegativeOR(thread, x, y);
        } else {
            return OneIsNegativeOR(thread, y, x);
        }
    }
    return BitwiseOp(thread, Operate::OR, x, y);
}

// 6.1.6.2.23 BigInt::toString ( x )
JSHandle<EcmaString> BigInt::ToString(JSThread *thread, JSHandle<BigInt> bigint, uint32_t conversionToRadix)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    CString result = bigint->ToStdString(conversionToRadix);
    return factory->NewFromASCII(result.c_str());
}

void BigInt::AppendToCString(CString &str, BigInt *bigint, uint32_t conversionToRadix)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (bigint->GetSign()) {
        str += "-";
    }
    str += BigIntHelper::Conversion(BigIntHelper::GetBinary(bigint), conversionToRadix, BINARY);
}

CString BigInt::ToStdString(uint32_t conversionToRadix) const
{
    CString result =
        BigIntHelper::Conversion(BigIntHelper::GetBinary(this), conversionToRadix, BINARY);
    if (GetSign()) {
        result = "-" + result;
    }
    return result;
}

JSTaggedValue BigInt::NumberToBigInt(JSThread *thread, JSHandle<JSTaggedValue> number)
{
    if (!number->IsInteger()) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "The number cannot be converted to a BigInt because it is not an integer",
                                     JSTaggedValue::Exception());
    }
    double num = number->GetNumber();
    if (num == 0.0) {
        return Int32ToBigInt(thread, 0).GetTaggedValue();
    }
    return DoubleToBigInt(thread, num);
}

JSTaggedValue BigInt::DoubleToBigInt(JSThread *thread, double num)
{
    // Bit operations must be of integer type
    uint64_t bits = 0;
    if (memcpy_s(&bits, sizeof(bits), &num, sizeof(num)) != EOK) {
        LOG_FULL(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
    // Take out bits 62-52 (11 bits in total) and subtract 1023
    uint64_t integerDigits = ((bits >> base::DOUBLE_SIGNIFICAND_SIZE) & 0x7FF) - base::DOUBLE_EXPONENT_BIAS;
    uint32_t mayNeedLen = integerDigits / DATEBITS + 1;

    JSHandle<BigInt> bigint = CreateBigint(thread, mayNeedLen);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bigint->SetSign(num < 0);
    uint64_t mantissa = (bits & base::DOUBLE_SIGNIFICAND_MASK) | base::DOUBLE_HIDDEN_BIT;
    int mantissaSize = base::DOUBLE_SIGNIFICAND_SIZE;

    int leftover = 0;
    bool isFirstInto = true;
    for (int index = static_cast<int>(mayNeedLen - 1); index >= 0; --index) {
        uint32_t doubleNum = 0;
        if (isFirstInto) {
            isFirstInto = false;
            leftover = mantissaSize - static_cast<int>(integerDigits % DATEBITS);
            doubleNum = static_cast<uint32_t>(mantissa >> leftover);
            mantissa = mantissa << (64 - leftover); // 64 : double bits size
            bigint->SetDigit(index, doubleNum);
        } else {
            leftover -= DATEBITS;
            doubleNum = static_cast<uint32_t>(mantissa >> DATEBITS);
            mantissa = mantissa << DATEBITS;
            bigint->SetDigit(index, doubleNum);
        }
    }
    return BigIntHelper::RightTruncate(thread, bigint).GetTaggedValue();
}

JSHandle<BigInt> BigInt::Int32ToBigInt(JSThread *thread, const int &number)
{
    JSHandle<BigInt> bigint = CreateBigint(thread, 1);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    uint32_t value = 0;
    bool sign = number < 0;
    if (sign) {
        value = static_cast<uint32_t>(-(number + 1)) + 1;
    } else {
        value = number;
    }
    bigint->SetDigit(0, value);
    bigint->SetSign(sign);
    return bigint;
}

JSHandle<BigInt> BigInt::Uint32ToBigInt(JSThread *thread, const uint32_t &number)
{
    JSHandle<BigInt> bigint = CreateBigint(thread, 1);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    bigint->SetDigit(0, number);
    return bigint;
}

JSHandle<BigInt> BigInt::Int64ToBigInt(JSThread *thread, const int64_t &number)
{
    uint64_t value = 0;
    bool sign = number < 0;
    if (sign) {
        value = static_cast<uint64_t>(-(number + 1)) + 1;
    } else {
        value = number;
    }
    JSHandle<BigInt> bigint = Uint64ToBigInt(thread, value);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    bigint->SetSign(sign);
    return BigIntHelper::RightTruncate(thread, bigint);
}

JSHandle<BigInt> BigInt::Uint64ToBigInt(JSThread *thread, const uint64_t &number)
{
    JSHandle<BigInt> bigint = CreateBigint(thread, 2); // 2 : one int64_t bits need two uint32_t bits
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    uint32_t lowBits = static_cast<uint32_t>(number & 0xffffffff);
    uint32_t highBits = static_cast<uint32_t>((number >> DATEBITS) & 0xffffffff);
    bigint->SetDigit(0, lowBits);
    bigint->SetDigit(1, highBits);
    return BigIntHelper::RightTruncate(thread, bigint);
}

uint64_t BigInt::ToUint64()
{
    uint32_t len = GetLength();
    ASSERT(len <= 2); // The maximum length of the BigInt data is less or equal 2
    uint32_t lowBits = GetDigit(0);
    uint32_t highBits = 0;
    if (len > 1) {
        highBits = GetDigit(1);
    }
    uint64_t value = static_cast<uint64_t>(lowBits);
    value |= static_cast<uint64_t>(highBits) << DATEBITS;
    if (GetSign()) {
        value = ~(value - 1);
    }
    return value;
}

int64_t BigInt::ToInt64()
{
    return static_cast<int64_t>(ToUint64());
}

void BigInt::BigIntToInt64(JSThread *thread, JSHandle<JSTaggedValue> bigint, int64_t *cValue, bool *lossless)
{
    ASSERT(cValue != nullptr);
    ASSERT(lossless != nullptr);
    if (bigint->IsBoolean()) {
        bigint = JSHandle<JSTaggedValue>(thread, JSTaggedValue::ToBigInt(thread, bigint));
        RETURN_IF_ABRUPT_COMPLETION(thread);
    } else if (!bigint->IsBigInt()) {
        JSHandle<BigInt> bigInt64(thread, JSTaggedValue::ToBigInt64(thread, bigint));
        RETURN_IF_ABRUPT_COMPLETION(thread);
        *cValue = bigInt64->ToInt64();
        return;
    }
    JSHandle<BigInt> bigInt64(thread, JSTaggedValue::ToBigInt64(thread, bigint));
    RETURN_IF_ABRUPT_COMPLETION(thread);
    if (Equal(bigInt64.GetTaggedValue(), bigint.GetTaggedValue())) {
        *lossless = true;
    } else {
        *lossless = false;
    }
    *cValue = bigInt64->ToInt64();
}

void BigInt::BigIntToUint64(JSThread *thread, JSHandle<JSTaggedValue> bigint, uint64_t *cValue, bool *lossless)
{
    ASSERT(cValue != nullptr);
    ASSERT(lossless != nullptr);
    if (bigint->IsBoolean()) {
        bigint = JSHandle<JSTaggedValue>(thread, JSTaggedValue::ToBigInt(thread, bigint));
        RETURN_IF_ABRUPT_COMPLETION(thread);
    } else if (!bigint->IsBigInt()) {
        JSHandle<BigInt> bigInt64(thread, JSTaggedValue::ToBigUint64(thread, bigint));
        RETURN_IF_ABRUPT_COMPLETION(thread);
        *cValue = bigInt64->ToInt64();
        return;
    }
    JSHandle<BigInt> bigUint64(thread, JSTaggedValue::ToBigUint64(thread, bigint));
    RETURN_IF_ABRUPT_COMPLETION(thread);
    if (Equal(bigUint64.GetTaggedValue(), bigint.GetTaggedValue())) {
        *lossless = true;
    } else {
        *lossless = false;
    }
    *cValue = bigUint64->ToUint64();
}

JSHandle<BigInt> BigInt::CreateBigWords(JSThread *thread, bool sign, uint32_t size, const uint64_t *words)
{
    ASSERT(words != nullptr);
    if (size == 0) {
        return Uint64ToBigInt(thread, 0);
    }
    const uint32_t MULTIPLE = 2;
    uint32_t needLen = size * MULTIPLE;
    if (needLen > MAXSIZE) {
        JSHandle<BigInt> bigint(thread, JSTaggedValue::Exception());
        THROW_RANGE_ERROR_AND_RETURN(thread, "Maximum BigInt size exceeded", bigint);
    }
    JSHandle<BigInt> bigint = CreateBigint(thread, needLen);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    for (uint32_t index = 0; index < size; ++index) {
        uint32_t lowBits = static_cast<uint32_t>(words[index] & 0xffffffff);
        uint32_t highBits = static_cast<uint32_t>((words[index] >> DATEBITS) & 0xffffffff);
        bigint->SetDigit(MULTIPLE * index, lowBits);
        bigint->SetDigit(MULTIPLE * index + 1, highBits);
    }
    bigint->SetSign(sign);
    return BigIntHelper::RightTruncate(thread, bigint);
}

JSHandle<BigInt> BigInt::Add(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    bool xSignFlag = x->GetSign();
    bool ySignFlag = y->GetSign();
    // x + y == x + y
    // -x + -y == -(x + y)
    if (xSignFlag == ySignFlag) {
        return BigintAdd(thread, x, y, xSignFlag);
    }
    // x + -y == x - y == -(y - x)
    // -x + y == y - x == -(x - y)
    uint32_t xLength = x->GetLength();
    uint32_t yLength = y->GetLength();
    int i = static_cast<int>(xLength) - 1;
    int subSize = static_cast<int>(xLength - yLength);
    if (subSize > 0) {
        return BigintSub(thread, x, y, xSignFlag);
    } else if (subSize == 0) {
        while (i > 0 && x->GetDigit(i) == y->GetDigit(i)) {
            i--;
        }
        if ((x->GetDigit(i) > y->GetDigit(i))) {
            return BigintSub(thread, x, y, xSignFlag);
        } else {
            return BigintSub(thread, y, x, ySignFlag);
        }
    } else {
        return BigintSub(thread, y, x, ySignFlag);
    }
}
JSHandle<BigInt> BigInt::Subtract(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    bool xSignFlag = x->GetSign();
    bool ySignFlag = y->GetSign();
    if (xSignFlag != ySignFlag) {
        // x - (-y) == x + y
        // (-x) - y == -(x + y)
        return BigintAdd(thread, x, y, xSignFlag);
    }
    // x - y == -(y - x)
    // (-x) - (-y) == y - x == -(x - y)
    uint32_t xLength = x->GetLength();
    uint32_t yLength = y->GetLength();
    ASSERT(xLength > 0);
    uint32_t i = xLength - 1;
    int subSize = static_cast<int>(xLength - yLength);
    if (subSize > 0) {
        return BigintSub(thread, x, y, xSignFlag);
    } else if (subSize == 0) {
        while (i > 0 && x->GetDigit(i) == y->GetDigit(i)) {
            i--;
        }
        if ((x->GetDigit(i) > y->GetDigit(i))) {
            return BigintSub(thread, x, y, xSignFlag);
        } else {
            return BigintSub(thread, y, x, !ySignFlag);
        }
    } else {
        return BigintSub(thread, y, x, !ySignFlag);
    }
}

JSHandle<BigInt> BigInt::BigintAdd(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y, bool resultSign)
{
    if (x->GetLength() < y->GetLength()) {
        return BigintAdd(thread, y, x, resultSign);
    }
    JSHandle<BigInt> bigint = BigInt::CreateBigint(thread, x->GetLength() + 1);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    uint32_t bigintCarry = 0;
    uint32_t i = 0;
    while (i < y->GetLength()) {
        uint32_t newBigintCarry = 0;
        uint32_t addPlus = BigIntHelper::AddHelper(x->GetDigit(i), y->GetDigit(i), newBigintCarry);
        addPlus = BigIntHelper::AddHelper(addPlus, bigintCarry, newBigintCarry);
        bigint->SetDigit(i, addPlus);
        bigintCarry = newBigintCarry;
        i++;
    }
    while (i < x->GetLength()) {
        uint32_t newBigintCarry = 0;
        uint32_t addPlus = BigIntHelper::AddHelper(x->GetDigit(i), bigintCarry, newBigintCarry);
        bigint->SetDigit(i, addPlus);
        bigintCarry = newBigintCarry;
        i++;
    }
    bigint->SetDigit(i, bigintCarry);
    bigint->SetSign(resultSign);
    return BigIntHelper::RightTruncate(thread, bigint);
}

inline uint32_t BigIntHelper::AddHelper(uint32_t x, uint32_t y, uint32_t &bigintCarry)
{
    uint32_t addPlus = x + y;
    if (addPlus < x) {
        bigintCarry += 1;
    }
    return addPlus;
}

JSHandle<BigInt> BigInt::BigintSub(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y, bool resultSign)
{
    JSHandle<BigInt> bigint = BigInt::CreateBigint(thread, x->GetLength());
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    uint32_t bigintCarry = 0;
    uint32_t i = 0;
    while (i < y->GetLength()) {
        uint32_t newBigintCarry = 0;
        uint32_t minuSub = BigIntHelper::SubHelper(x->GetDigit(i), y->GetDigit(i), newBigintCarry);
        minuSub = BigIntHelper::SubHelper(minuSub, bigintCarry, newBigintCarry);
        bigint->SetDigit(i, minuSub);
        bigintCarry = newBigintCarry;
        i++;
    }
    while (i < x->GetLength()) {
        uint32_t newBigintCarry = 0;
        uint32_t minuSub = BigIntHelper::SubHelper(x->GetDigit(i), bigintCarry, newBigintCarry);
        bigint->SetDigit(i, minuSub);
        bigintCarry = newBigintCarry;
        i++;
    }
    bigint->SetSign(resultSign);
    return BigIntHelper::RightTruncate(thread, bigint);
}

JSHandle<BigInt> BigInt::BigintAddOne(JSThread *thread, JSHandle<BigInt> x)
{
    JSHandle<BigInt> temp = Int32ToBigInt(thread, 1);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    return Add(thread, x, temp);
}

JSHandle<BigInt> BigInt::BigintSubOne(JSThread *thread, JSHandle<BigInt> x)
{
    JSHandle<BigInt> temp = Int32ToBigInt(thread, 1);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    return Subtract(thread, x, temp);
}

inline uint32_t BigIntHelper::SubHelper(uint32_t x, uint32_t y, uint32_t &bigintCarry)
{
    ASSERT(x - y >= 0);
    uint32_t minuSub = x - y;
    if (minuSub > x) {
        bigintCarry += 1;
    }
    return minuSub;
}

ComparisonResult BigInt::Compare(const JSTaggedValue &x, const JSTaggedValue &y)
{
    BigInt* xVal = BigInt::Cast(x.GetTaggedObject());
    BigInt* yVal = BigInt::Cast(y.GetTaggedObject());
    return Compare(xVal, yVal);
}

ComparisonResult BigInt::Compare(const BigInt *x, const BigInt *y)
{
    bool xSign = x->GetSign();
    bool ySign = y->GetSign();
    if (xSign != ySign) {
        return xSign ? ComparisonResult::LESS : ComparisonResult::GREAT;
    }
    ComparisonResult compar = AbsolutelyCompare(x, y);
    if (xSign && compar != ComparisonResult::EQUAL) {
        return compar == ComparisonResult::LESS ? ComparisonResult::GREAT : ComparisonResult::LESS;
    }
    return compar;
}

bool BigInt::LessThan(const JSTaggedValue &x, const JSTaggedValue &y)
{
    return Compare(x, y) == ComparisonResult::LESS;
}

bool BigInt::LessThan(const BigInt *x, const BigInt *y)
{
    ASSERT(x != nullptr);
    ASSERT(y != nullptr);
    return Compare(x, y) == ComparisonResult::LESS;
}

JSHandle<BigInt> BigInt::SignedRightShift(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    if (x->IsZero() || y->IsZero()) {
        return x;
    }
    if (y->GetSign()) {
        return LeftShiftHelper(thread, x, y);
    } else {
        return RightShiftHelper(thread, x, y);
    }
}

JSHandle<BigInt> BigInt::ReturnIfRightShiftOverMax(JSThread *thread, bool sign)
{
    if (sign) {
        return Int32ToBigInt(thread, -1);
    }
    return Int32ToBigInt(thread, 0);
}

void BigInt::RightShift(JSHandle<BigInt> bigint, JSHandle<BigInt> x, uint32_t digitMove, uint32_t bitsMove)
{
    uint32_t size = x->GetLength();
    if (bitsMove == 0) {
        for (uint32_t i = digitMove; i < size; i++) {
            bigint->SetDigit(i - digitMove, x->GetDigit(i));
        }
    } else {
        uint32_t carry = x->GetDigit(digitMove) >> bitsMove;
        ASSERT(size > digitMove);
        uint32_t last = size - digitMove - 1;
        for (uint32_t i = 0; i < last; i++) {
            uint32_t value = x->GetDigit(i + digitMove + 1);
            bigint->SetDigit(i, (value << (DATEBITS - bitsMove)) | carry);
            carry = value >> bitsMove;
        }
        bigint->SetDigit(last, carry);
    }
}

void BigInt::JudgeRoundDown(JSHandle<BigInt> x, uint32_t digitMove, uint32_t bitsMove, uint32_t &needLen,
                            bool &roundDown)
{
    uint32_t stamp = (static_cast<uint32_t>(1U) << bitsMove) - 1;
    if (x->GetDigit(digitMove) & stamp) {
        roundDown = true;
    } else {
        for (uint32_t i = 0; i < digitMove; i++) {
            if (x->GetDigit(i) != 0) {
                roundDown = true;
                break;
            }
        }
    }

    if (roundDown && bitsMove == 0) {
        ASSERT(x->GetLength() > 0);
        uint32_t highBits = x->GetDigit(x->GetLength() - 1);
        // If all the most significant bits are 1, we think that carry will cause overflow,
        // and needLen needs to be increased by 1
        if ((~highBits) == 0) {
            needLen++;
        }
    }
}

JSHandle<BigInt> BigInt::RightShiftHelper(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    bool sign = x->GetSign();
    if (y->GetLength() > 1 || y->GetDigit(0) > MAXBITS) {
        return ReturnIfRightShiftOverMax(thread, sign);
    }
    uint32_t moveNum = y->GetDigit(0);
    uint32_t digitMove = moveNum / DATEBITS;
    uint32_t bitsMove = moveNum % DATEBITS;
    if (x->GetLength() <= digitMove) {
        return ReturnIfRightShiftOverMax(thread, sign);
    }
    uint32_t needLen = x->GetLength() - digitMove;
    bool roundDown = false;
    if (sign) {
        // If it is a negative number, you need to consider whether it will carry after moving.
        // NeedLen may need to increase by 1
        JudgeRoundDown(x, digitMove, bitsMove, needLen, roundDown);
    }
    JSHandle<BigInt> bigint = CreateBigint(thread, needLen);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);

    RightShift(bigint, x, digitMove, bitsMove);
    bigint = BigIntHelper::RightTruncate(thread, bigint);
    if (sign) {
        bigint->SetSign(true);
        if (roundDown) {
            return BitwiseAddOne(thread, bigint);
        }
    }
    return bigint;
}

JSHandle<BigInt> BigInt::LeftShift(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    if (y->GetSign()) {
        return RightShiftHelper(thread, x, y);
    } else {
        return LeftShiftHelper(thread, x, y);
    }
}

JSHandle<BigInt> BigInt::LeftShiftHelper(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    if (x->IsZero()) {
        return x;
    }
    ASSERT(y->GetLength() > 0);
    uint32_t moveNum = y->GetDigit(0);
    uint32_t digitMove = moveNum / DATEBITS;
    uint32_t bitsMove = moveNum % DATEBITS;
    // If bitsMove is not zero, needLen needs to be increased by 1
    uint32_t needLen = digitMove + x->GetLength() + static_cast<uint32_t>(!!bitsMove);
    JSHandle<BigInt> bigint = CreateBigint(thread, needLen);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    if (bitsMove == 0) {
        uint32_t index = digitMove;
        while (index < needLen) {
            bigint->SetDigit(index, x->GetDigit(index - digitMove));
            ++index;
        }
    } else {
        uint32_t carry = 0;
        uint32_t index = 0;
        while (index < x->GetLength()) {
            uint32_t value = x->GetDigit(index);
            bigint->SetDigit(index + digitMove, (value << bitsMove) | carry);
            carry = value >> (DATEBITS - bitsMove);
            ++index;
        }
        if (carry != 0) {
            ASSERT(index + digitMove < needLen);
            bigint->SetDigit(index + digitMove, carry);
        }
    }
    bigint->SetSign(x->GetSign());
    return BigIntHelper::RightTruncate(thread, bigint);
}

JSTaggedValue BigInt::UnsignedRightShift(JSThread *thread)
{
    THROW_TYPE_ERROR_AND_RETURN(thread, "BigInt have no unsigned right shift, use >> instead",
                                JSTaggedValue::Exception());
}

JSHandle<BigInt> BigInt::Copy(JSThread *thread, JSHandle<BigInt> x, uint32_t len)
{
    ASSERT(x->GetLength() >= len);
    JSHandle<BigInt> newBig = CreateBigint(thread, len);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    std::copy(x->GetData(), x->GetData() + len, newBig->GetData());
    newBig->SetSign(x->GetSign());
    return newBig;
}

JSHandle<BigInt> BigInt::UnaryMinus(JSThread *thread, JSHandle<BigInt> x)
{
    if (x->IsZero()) {
        return x;
    }
    JSHandle<BigInt> y = Copy(thread, x, x->GetLength());
    y->SetSign(!y->GetSign());
    return y;
}

// 6.1.6.2.2   BigInt::bitwiseNOT ( x )
JSHandle<BigInt> BigInt::BitwiseNOT(JSThread *thread, JSHandle<BigInt> x)
{
    // ~(-x) == ~(~(x-1)) == x-1
    // ~x == -x-1 == -(x+1)
    JSHandle<BigInt> result = BigintAddOne(thread, x);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    if (x->GetSign()) {
        result->SetSign(false);
    } else {
        result->SetSign(true);
    }
    return result;
}

JSHandle<BigInt> BigInt::Exponentiate(JSThread *thread, JSHandle<BigInt> base, JSHandle<BigInt> exponent)
{
    if (exponent->GetSign()) {
        JSHandle<BigInt> bigint(thread, JSTaggedValue::Exception());
        THROW_RANGE_ERROR_AND_RETURN(thread, "Exponent must be positive", bigint);
    }
    ASSERT(exponent->GetLength() > 0);
    if (exponent->IsZero()) {
        return Int32ToBigInt(thread, 1);
    }
    if (base->IsZero()) {
        return base;
    }
    uint32_t expValue = exponent->GetDigit(0);
    if (base->GetLength() == 1 && base->GetDigit(0) == 1) {
        if (base->GetSign() && !(expValue & 1)) {
            return BigInt::UnaryMinus(thread, base);
        }
        return base;
    }
    if (exponent->GetLength() > 1) {
        // The result is at least 2n ** 2n ** 32n, which is too big.
        JSHandle<BigInt> bigint(thread, JSTaggedValue::Exception());
        THROW_RANGE_ERROR_AND_RETURN(thread, "Maximum BigInt size exceeded", bigint);
    }

    if (base->GetLength() == 1 && base->GetDigit(0) == 2) { // 2 : We use fast path processing 2 ^ n
        uint32_t needLength = expValue / DATEBITS + 1;
        JSHandle<BigInt> bigint = CreateBigint(thread, needLength);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
        uint32_t value = 1U << (expValue % DATEBITS);
        bigint->SetDigit(needLength - 1, value);
        if (base->GetSign()) {
            bigint->SetSign(static_cast<bool>(expValue & 1));
        }
        return bigint;
    }
    JSMutableHandle<BigInt> result(thread, JSTaggedValue::Null());
    JSMutableHandle<BigInt> temp(thread, base);
    if (expValue & 1) {
        result.Update(base);
    }
    expValue >>= 1;
    for (; expValue; expValue >>= 1) {
        temp.Update(BigInt::Multiply(thread, temp, temp));
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
        if (expValue & 1) {
            if (result.GetTaggedValue().IsNull()) {
                result.Update(temp);
            } else {
                result.Update(BigInt::Multiply(thread, result, temp));
                RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
            }
        }
    }
    ASSERT(result.GetTaggedValue().IsBigInt());
    return result;
}

std::tuple<uint32_t, uint32_t> BigInt::Mul(uint32_t x, uint32_t y)
{
    uint32_t lowBitX = x & HALFDATEMASK;
    uint32_t highBitX = x >> HALFDATEBITS;
    uint32_t lowBitY = y & HALFDATEMASK;
    uint32_t highBitY = y >> HALFDATEBITS;
    // {highBitX lowBitX} * {highBitY lowBitY}
    uint32_t lowRes = lowBitX * lowBitY;
    uint32_t highRes = highBitX * highBitY;
    uint32_t midRes1 = lowBitX * highBitY;
    uint32_t midRes2 = highBitX * lowBitY;

    uint32_t carry = 0;
    uint32_t low = BigIntHelper::AddHelper(
        BigIntHelper::AddHelper(lowRes, midRes1 << HALFDATEBITS, carry), midRes2 << HALFDATEBITS, carry);
    uint32_t high = (midRes1 >> HALFDATEBITS) + (midRes2 >> HALFDATEBITS) + highRes + carry;

    return std::make_tuple(high, low);
}

JSHandle<BigInt> BigInt::Multiply(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    if (x->IsZero()) {
        return x;
    }
    if (y->IsZero()) {
        return y;
    }
    uint32_t needLength = x->GetLength() + y->GetLength();
    JSHandle<BigInt> bigint = BigInt::CreateBigint(thread, needLength);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    // the algorithm here is similar to the way we use paper money to calculate multiplication.
    // Generally, we first calculate the partial product, and then add up to get the result.
    // The only difference here is that multiplication and addition are calculated synchronously
    for (uint32_t i = 0; i < x->GetLength(); i++) {
        uint32_t xVal = x->GetDigit(i);
        // If the current multiplier is 0, we will skip this round of calculation to improve performance.
        // If we do not skip, the correctness of the calculation will not be affected
        if (xVal == 0) {
            continue;
        }
        uint32_t carry = 0;
        uint32_t high = 0;
        uint32_t index = i;
        for (uint32_t j = 0; j < y->GetLength(); j++) {
            uint32_t currentCarry = 0;
            uint32_t value = bigint->GetDigit(index);
            value = BigIntHelper::AddHelper(value, high, currentCarry);
            value = BigIntHelper::AddHelper(value, carry, currentCarry);

            uint32_t low;
            std::tie(high, low) = Mul(xVal, y->GetDigit(j));
            value = BigIntHelper::AddHelper(value, low, currentCarry);
            bigint->SetDigit(index, value);
            carry = currentCarry;
            index++;
        }
        while (carry != 0 || high != 0) {
            ASSERT(index < bigint->GetLength());
            uint32_t value = bigint->GetDigit(index);
            uint32_t currentCarry = 0;
            value = BigIntHelper::AddHelper(value, high, currentCarry);
            high = 0;
            value = BigIntHelper::AddHelper(value, carry, currentCarry);
            bigint->SetDigit(index, value);
            carry = currentCarry;
            index++;
        }
    }

    bigint->SetSign(x->GetSign() != y->GetSign());
    return BigIntHelper::RightTruncate(thread, bigint);
}

void BigIntHelper::DeZero(CString &a)
{
    size_t count = 0;
    while (count < a.size() && a[count] == '0') {
        count++;
    }
    if (count == a.size()) {
        a = "0";
    } else {
        a = a.substr(count);
    }
}

ComparisonResult BigInt::AbsolutelyCompare(const BigInt *x, const BigInt *y)
{
    uint32_t xLen = x->GetLength();
    uint32_t yLen = y->GetLength();
    if (xLen > yLen) {
        return ComparisonResult::GREAT;
    } else if (xLen < yLen) {
        return ComparisonResult::LESS;
    } else {
        int index = static_cast<int>(xLen) - 1;
        for (; index >= 0; --index) {
            if (x->GetDigit(index) != y->GetDigit(index)) {
                break;
            }
        }
        if (index < 0) {
            return ComparisonResult::EQUAL;
        }
        return x->GetDigit(index) > y->GetDigit(index) ? ComparisonResult::GREAT : ComparisonResult::LESS;
    }
}

uint32_t BigInt::DivideAndRemainder(uint32_t highBit, uint32_t lowBit, uint32_t divisor, uint32_t& remainder)
{
    uint32_t leadingZeros = base::CountLeadingZeros(divisor);
    // Before calculating, we need to align the operands to the left
    divisor <<= leadingZeros;
    uint32_t lowDividend = lowBit << leadingZeros;
    uint32_t highDividend = highBit;
    if (leadingZeros != 0) {
        // highBit is the remainder of the last calculation, which must be less than or equal to the divisor,
        // so high << leadingZeros will not lose the significant bit
        highDividend = (highBit << leadingZeros) | (lowBit >> (DATEBITS - leadingZeros));
    }
    uint32_t highDivisor = divisor >> HALFDATEBITS;
    uint32_t lowDivisor = divisor & HALFDATEMASK;
    uint32_t lowDividend1 = lowDividend >> HALFDATEBITS;
    uint32_t lowDividend2 = lowDividend & HALFDATEMASK;
    uint32_t highQuotient = highDividend / highDivisor;
    uint32_t tempRemainder = highDividend - highQuotient * highDivisor;

    // Similar to the ordinary division calculation, here we use HALFUINT32VALUE as the carry unit
    // Calculate high order results first
    while (highQuotient >= HALFUINT32VALUE ||
           highQuotient * lowDivisor > tempRemainder * HALFUINT32VALUE + lowDividend1) {
        highQuotient--;
        tempRemainder += highDivisor;
        if (tempRemainder >= HALFUINT32VALUE) {
            break;
        }
    }
    uint32_t tempLowDividend = highDividend * HALFUINT32VALUE + lowDividend1 - highQuotient * divisor;
    uint32_t lowQuotient = tempLowDividend / highDivisor;
    tempRemainder = tempLowDividend - lowQuotient * highDivisor;

    // Then calculate the low order result
    while (lowQuotient >= HALFUINT32VALUE ||
           lowQuotient * lowDivisor > tempRemainder * HALFUINT32VALUE + lowDividend2) {
        lowQuotient--;
        tempRemainder += highDivisor;
        if (tempRemainder >= HALFUINT32VALUE) {
            break;
        }
    }

    // In order to facilitate the calculation, we start to make left alignment
    // At this time, we need to move right to get the correct remainder
    remainder = (tempLowDividend * HALFUINT32VALUE + lowDividend2 - lowQuotient * divisor) >> leadingZeros;
    return highQuotient * HALFUINT32VALUE + lowQuotient;
}

JSHandle<BigInt> BigInt::FormatLeftShift(JSThread *thread, uint32_t shift, JSHandle<BigInt> bigint, bool neeedAddOne)
{
    if (!neeedAddOne && shift == 0) {
        return bigint;
    }
    uint32_t len = bigint->GetLength();
    uint32_t needLen = len;
    if (neeedAddOne) {
        needLen += 1;
    }
    JSHandle<BigInt> result = CreateBigint(thread, needLen);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    if (shift == 0) {
        std::copy(bigint->GetData(), bigint->GetData() + len, result->GetData());
    } else {
        uint32_t carry = 0;
        uint32_t index = 0;
        while (index < len) {
            uint32_t value = bigint->GetDigit(index);
            result->SetDigit(index, (value << shift) | carry);
            carry = value >> (DATEBITS - shift);
            index++;
        }
        if (carry != 0) {
            ASSERT(neeedAddOne);
            result->SetDigit(index, carry);
        }
    }
    return result;
}

void BigInt::UnformattedRightShift(JSHandle<BigInt> bigint, uint32_t shift)
{
    RightShift(bigint, bigint, 0, shift);
}

bool BigInt::SpecialMultiplyAndSub(JSHandle<BigInt> u, JSHandle<BigInt> v, uint32_t q, JSHandle<BigInt> qv,
                                   uint32_t pos)
{
    uint32_t lastCarry = 0;
    uint32_t lastHigh = 0;
    uint32_t len = v->GetLength();
    // Calculate multiplication first
    for (uint32_t i = 0; i < len; ++i) {
        uint32_t value = v->GetDigit(i);
        uint32_t carry = 0;
        uint32_t high = 0;
        std::tie(high, value) = Mul(value, q);
        // The current value plus the high and carry of the last calculation
        value = BigIntHelper::AddHelper(value, lastHigh, carry);
        value = BigIntHelper::AddHelper(value, lastCarry, carry);
        qv->SetDigit(i, value);
        // Record the new high bit and carry for the next round
        lastCarry = carry;
        lastHigh = high;
    }
    qv->SetDigit(len, lastHigh + lastCarry);

    // Next, subtract
    uint32_t lastBorrow = 0;
    for (uint32_t i = 0; i < qv->GetLength(); ++i) {
        uint32_t borrow = 0;
        uint32_t value = BigIntHelper::SubHelper(u->GetDigit(pos + i), qv->GetDigit(i), borrow);
        value = BigIntHelper::SubHelper(value, lastBorrow, borrow);
        u->SetDigit(pos + i, value);
        lastBorrow = borrow;
    }

    return lastBorrow > 0;
}

uint32_t BigInt::SpecialAdd(JSHandle<BigInt> u, JSHandle<BigInt> v, uint32_t pos)
{
    uint32_t lastCarry = 0;
    for (uint32_t i = 0; i < v->GetLength(); ++i) {
        uint32_t carry = 0;
        uint32_t value = BigIntHelper::AddHelper(u->GetDigit(pos + i), v->GetDigit(i), carry);
        value = BigIntHelper::AddHelper(value, lastCarry, carry);
        u->SetDigit(pos + i, value);
        lastCarry = carry;
    }
    return lastCarry;
}

uint32_t BigInt::ImproveAccuracy(uint32_t vHighest, uint32_t vHighestNext, uint32_t UHighest,
                                 uint32_t UHighestNext, uint32_t q)
{
    uint32_t high = 0;
    uint32_t low = 0;
    std::tie(high, low) = Mul(q, vHighestNext);
    while (high > UHighest || (high == UHighest && low > UHighestNext)) {
        q--;
        UHighest += vHighest;
        // if r is less than the current base, continue the next round of inspection. Here,
        // we confirm whether r is greater than the current base by judging whether r overflows
        if (UHighest < vHighest) {
            break;
        }
        std::tie(high, low) = Mul(q, vHighestNext);
    }
    return q;
}

JSHandle<BigInt> BigInt::DivideAndRemainderWithBigintDivisor(JSThread *thread, JSHandle<BigInt> dividend,
                                                             JSHandle<BigInt> divisor,
                                                             JSMutableHandle<BigInt> &remainder)
{
    uint32_t divisorLen = divisor->GetLength();
    // the length of the quota is the length of the dividend minus the divisor
    uint32_t quotientLen = dividend->GetLength() - divisorLen;
    JSMutableHandle<BigInt> quotient(thread, JSTaggedValue::Null());
    if (remainder.GetTaggedValue().IsNull()) {
        quotient.Update(CreateBigint(thread, quotientLen + 1));
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    }
    // format the divisor and dividend so that the highest order of the divisor is
    // greater than or equal to half of uint32_t
    ASSERT(divisorLen > 0);
    uint32_t leadingZeros = base::CountLeadingZeros(divisor->GetDigit(divisorLen - 1));
    JSHandle<BigInt> v = FormatLeftShift(thread, leadingZeros, divisor, false);
    JSHandle<BigInt> u = FormatLeftShift(thread, leadingZeros, dividend, true);
    // qv is used to store the result of quotient * divisor of each round
    JSHandle<BigInt> qv = CreateBigint(thread, divisorLen + 1);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    uint32_t vHighest = v->GetDigit(divisorLen - 1);
    for (int i = static_cast<int>(quotientLen); i >= 0; --i) {
        uint32_t currentUHighest = u->GetDigit(i + divisorLen);
        uint32_t r = 0;
        uint32_t q = DivideAndRemainder(currentUHighest, u->GetDigit(i + divisorLen - 1), vHighest, r);
        // VHighest = currentUHighest means that q may be equal to the current base
        // In the current program, the current base is the maximum value of uint32 plus 1
        if (vHighest == currentUHighest) {
            q = std::numeric_limits<uint32_t>::max();
        } else {
            uint32_t vHighestNext = v->GetDigit(divisorLen - 2); // 2 : Get the second most significant bit
            uint32_t currentUHighestNext = u->GetDigit(i + divisorLen - 2); // 2 : ditto

            // The following operations will make q only 1 greater than the value we want in most cases,
            // and will not be less than it
            q = ImproveAccuracy(vHighest, vHighestNext, r, currentUHighestNext, q);
        }
        // multiplication and subtraction
        if (SpecialMultiplyAndSub(u, v, q, qv, i)) {
            q--;
            uint32_t carry = SpecialAdd(u, v, i);
            u->SetDigit(i + divisorLen, u->GetDigit(i + divisorLen) + carry);
        }
        if (remainder.GetTaggedValue().IsNull()) {
            quotient->SetDigit(i, q);
        }
    }
    if (!remainder.GetTaggedValue().IsNull()) {
        // at the beginning of this procedure, we performed the left shift operation.
        // Here, we get the correct result by shifting the same number of digits to the right
        UnformattedRightShift(u, leadingZeros);
        remainder.Update(u);
    }
    return quotient;
}

JSHandle<BigInt> BigInt::DivideAndRemainderWithUint32Divisor(JSThread *thread, JSHandle<BigInt> dividend,
                                                             uint32_t divisor, JSMutableHandle<BigInt> &remainder)
{
    uint32_t r = 0;
    JSMutableHandle<BigInt> quotient(thread, JSTaggedValue::Null());
    if (!remainder.GetTaggedValue().IsNull()) {
        for (int i = static_cast<int>(dividend->GetLength()) - 1; i >= 0; --i) {
            DivideAndRemainder(r, dividend->GetDigit(i), divisor, r);
            remainder.Update(Uint32ToBigInt(thread, r));
        }
    } else {
        quotient.Update(CreateBigint(thread, dividend->GetLength()));
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
        for (int i = static_cast<int>(dividend->GetLength()) - 1; i >= 0; --i) {
            uint32_t q = DivideAndRemainder(r, dividend->GetDigit(i), divisor, r);
            quotient->SetDigit(i, q);
        }
    }
    return quotient;
}

// The algorithm here refers to algorithm D in Volume 2 of <The Art of Computer Programming>
JSHandle<BigInt> BigInt::Divide(JSThread *thread, JSHandle<BigInt> x, JSHandle<BigInt> y)
{
    if (y->IsZero()) {
        JSHandle<BigInt> bigint(thread, JSTaggedValue::Exception());
        THROW_RANGE_ERROR_AND_RETURN(thread, "Division by zero", bigint);
    }
    // returns 0 if x is less than y
    JSMutableHandle<BigInt> quotient(thread, JSTaggedValue::Null());
    bool sign = x->GetSign() != y->GetSign();
    ComparisonResult compare = AbsolutelyCompare(*x, *y);
    if (compare == ComparisonResult::LESS) {
        return Int32ToBigInt(thread, 0);
    }
    if (compare == ComparisonResult::EQUAL) {
        quotient.Update(Int32ToBigInt(thread, 1));
        quotient->SetSign(sign);
        return quotient;
    }
    // if y is 1, return +x or -x
    if (y->IsUint32() && y->GetDigit(0) == 1) {
        if (sign == x->GetSign()) {
            return x;
        }
        return UnaryMinus(thread, x);
    }
    JSMutableHandle<BigInt> remainder(thread, JSTaggedValue::Null());
    if (y->IsUint32()) {
        // When the divisor is uint32_t, we have a faster and simpler algorithm to calculate
        quotient.Update(DivideAndRemainderWithUint32Divisor(thread, x, y->GetDigit(0), remainder));
    } else {
        ASSERT(y->GetLength() >= 1); // 1 : Entering the current branch length must be greater than 1
        JSHandle<BigInt> newBigint = DivideAndRemainderWithBigintDivisor(thread, x, y, remainder);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
        quotient.Update(newBigint);
    }
    ASSERT(quotient.GetTaggedValue().IsBigInt());
    quotient->SetSign(sign);
    return BigIntHelper::RightTruncate(thread, quotient);
}

JSHandle<BigInt> BigInt::Remainder(JSThread *thread, JSHandle<BigInt> n, JSHandle<BigInt> d)
{
    if (d->IsZero()) {
        JSHandle<BigInt> bigint(thread, JSTaggedValue::Exception());
        THROW_RANGE_ERROR_AND_RETURN(thread, "Division by zero", bigint);
    }
    ComparisonResult compare = AbsolutelyCompare(*n, *d);
    if (n->IsZero() || compare == ComparisonResult::LESS) {
        return n;
    }
    if (compare == ComparisonResult::EQUAL || (d->IsUint32() && d->GetDigit(0) == 1)) {
        return Int32ToBigInt(thread, 0);
    }
    JSMutableHandle<BigInt> remainder(thread, JSTaggedValue::Undefined());
    if (d->IsUint32()) {
        // When the divisor is uint32_t, we have a faster and simpler algorithm to calculate
        DivideAndRemainderWithUint32Divisor(thread, n, d->GetDigit(0), remainder);
    } else {
        ASSERT(d->GetLength() > 1); // 1 : Entering the current branch length must be greater than 1
        DivideAndRemainderWithBigintDivisor(thread, n, d, remainder);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    }
    ASSERT(remainder.GetTaggedValue().IsBigInt());
    remainder->SetSign(n->GetSign());
    return BigIntHelper::RightTruncate(thread, remainder);
}

JSHandle<BigInt> BigInt::FloorMod(JSThread *thread, JSHandle<BigInt> leftVal, JSHandle<BigInt> rightVal)
{
    JSHandle<BigInt> remainder = Remainder(thread, leftVal, rightVal);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(BigInt, thread);
    if (leftVal->GetSign() && !remainder->IsZero()) {
        return Add(thread, remainder, rightVal);
    }
    return remainder;
}

JSTaggedValue BigInt::AsUintN(JSThread *thread, JSTaggedNumber &bits, JSHandle<BigInt> bigint)
{
    JSTaggedNumber number = JSTaggedValue::ToNumber(thread, bits);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int64_t bit  = base::NumberHelper::DoubleToInt64(number.GetNumber());
    if (bit == 0) {
        return Int32ToBigInt(thread, 0).GetTaggedValue();
    }
    if (bigint->IsZero()) {
        return bigint.GetTaggedValue();
    }
    JSHandle<BigInt> exponent = Uint64ToBigInt(thread, bit);
    JSHandle<BigInt> base = Int64ToBigInt(thread, 2); // 2 : base value
    if (bit >= kMaxLengthBits && !bigint->GetSign()) {
        return bigint.GetTaggedValue();
    }
    JSHandle<BigInt> tValue = Exponentiate(thread, base, exponent);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return FloorMod(thread, bigint, tValue).GetTaggedValue();
}

JSTaggedValue BigInt::AsintN(JSThread *thread, JSTaggedNumber &bits, JSHandle<BigInt> bigint)
{
    JSTaggedNumber number = JSTaggedValue::ToNumber(thread, bits);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int64_t bit  = base::NumberHelper::DoubleToInt64(number.GetNumber());
    if (bit == 0) {
        return Int32ToBigInt(thread, 0).GetTaggedValue();
    }
    if (bigint->IsZero()) {
        return bigint.GetTaggedValue();
    }
    JSHandle<BigInt> exp = Int64ToBigInt(thread, bit);
    JSHandle<BigInt> exponent = Int64ToBigInt(thread, bit - 1);
    JSHandle<BigInt> base = Int64ToBigInt(thread, 2); // 2 : base value
    if (bit >= kMaxLengthBits && !bigint->GetSign()) {
        return bigint.GetTaggedValue();
    }
    JSHandle<BigInt> tValue = Exponentiate(thread, base, exp);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<BigInt> modValue = FloorMod(thread, bigint, tValue);
    JSHandle<BigInt> resValue = Exponentiate(thread, base, exponent);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // If mod ≥ 2bits - 1, return ℤ(mod - 2bits); otherwise, return (mod).
    if (Compare(*resValue, *modValue) != ComparisonResult::GREAT) {
        return Subtract(thread, modValue, tValue).GetTaggedValue();
    }
    return modValue.GetTaggedValue();
}

static JSTaggedNumber CalculateNumber(const uint64_t &sign, const uint64_t &mantissa, uint64_t &exponent)
{
    exponent = (exponent + base::DOUBLE_EXPONENT_BIAS) << base::DOUBLE_SIGNIFICAND_SIZE;
    uint64_t doubleBit = sign | exponent | mantissa;
    double res = 0;
    if (memcpy_s(&res, sizeof(res), &doubleBit, sizeof(doubleBit)) != EOK) {
        LOG_FULL(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
    return JSTaggedNumber(res);
}

static JSTaggedNumber Rounding(const uint64_t &sign, uint64_t &mantissa, uint64_t &exponent, bool needRound)
{
    if (needRound || (mantissa & 1) == 1) {
        ++mantissa;
        if ((mantissa >> base::DOUBLE_SIGNIFICAND_SIZE) != 0) {
            mantissa = 0;
            exponent++;
            if (exponent > base::DOUBLE_EXPONENT_BIAS) {
                return JSTaggedNumber(sign ? -base::POSITIVE_INFINITY : base::POSITIVE_INFINITY);
            }
        }
    }
    return CalculateNumber(sign, mantissa, exponent);
}

JSTaggedNumber BigInt::BigIntToNumber(JSHandle<BigInt> bigint)
{
    if (bigint->IsZero()) {
        return JSTaggedNumber(0);
    }
    uint32_t bigintLen = bigint->GetLength();
    ASSERT(bigintLen > 0);
    uint32_t BigintHead = bigint->GetDigit(bigintLen - 1);
    uint32_t leadingZeros = base::CountLeadingZeros(BigintHead);
    int bigintBitLen = static_cast<int>(bigintLen * BigInt::DATEBITS - leadingZeros);
    // if Significant bits greater than 1024 then double is infinity
    bool bigintSign = bigint->GetSign();
    if (bigintBitLen > (base::DOUBLE_EXPONENT_BIAS + 1)) {
        return JSTaggedNumber(bigintSign ? -base::POSITIVE_INFINITY : base::POSITIVE_INFINITY);
    }
    uint64_t sign = bigintSign ? 1ULL << 63 : 0; // 63 : Set the sign bit of sign to 1
    int needMoveBit = static_cast<int>(leadingZeros + BigInt::DATEBITS + 1);
    // Align to the most significant bit, then right shift 12 bits so that the head of the mantissa is in place
    uint64_t mantissa = (needMoveBit == 64) ? 0 :
        ((static_cast<uint64_t>(BigintHead) << needMoveBit) >> 12); // 12 mantissa// just has 52 bits
    int remainMantissaBits = needMoveBit - 12;
    ASSERT(bigintBitLen > 0);
    uint64_t exponent = static_cast<uint64_t>(bigintBitLen - 1);
    int index = static_cast<int>(bigintLen - 1);
    uint32_t digit = 0;
    if (index > 0) {
        digit = bigint->GetDigit(--index);
    } else {
        return CalculateNumber(sign, mantissa, exponent);
    }
    // pad unset mantissa
    if (static_cast<uint32_t>(remainMantissaBits) >= BigInt::DATEBITS) {
        mantissa |= (static_cast<uint64_t>(digit) << (remainMantissaBits - BigInt::DATEBITS));
        remainMantissaBits -= BigInt::DATEBITS;
        index--;
    }
    if (remainMantissaBits > 0 && index >= 0) {
        digit = bigint->GetDigit(index);
        mantissa |= (static_cast<uint64_t>(digit) >> (BigInt::DATEBITS - remainMantissaBits));
        remainMantissaBits -= BigInt::DATEBITS;
    }
    // After the mantissa is filled, if the bits of bigint have not been used up, consider the rounding problem
    // The remaining bits of the current digit
    if (remainMantissaBits > 0) {
        return CalculateNumber(sign, mantissa, exponent);
    }
    int remainDigitBits = 0;
    if (remainMantissaBits < 0) {
        remainDigitBits = -remainMantissaBits;
    } else {
        if (index <= 0) {
            return CalculateNumber(sign, mantissa, exponent);
        }
        digit = bigint->GetDigit(index--);
        remainDigitBits = BigInt::DATEBITS;
    }
    uint32_t temp = 1ULL << (remainDigitBits - 1);
    if (!(digit & temp)) {
        return CalculateNumber(sign, mantissa, exponent);
    }
    if ((digit & (temp - 1)) != 0) {
        return Rounding(sign, mantissa, exponent, true);
    }
    while (index > 0) {
        if (bigint->GetDigit(--index) != 0) {
            return Rounding(sign, mantissa, exponent, true);
        }
    }
    return Rounding(sign, mantissa, exponent, false);
}

static int CompareToBitsLen(JSHandle<BigInt> bigint, int numBitLen, int &leadingZeros)
{
    uint32_t bigintLen = bigint->GetLength();
    ASSERT(bigintLen > 0);
    uint32_t BigintHead = bigint->GetDigit(bigintLen - 1);
    leadingZeros = static_cast<int>(base::CountLeadingZeros(BigintHead));
    int bigintBitLen = static_cast<int>(bigintLen * BigInt::DATEBITS) - leadingZeros;
    bool bigintSign = bigint->GetSign();
    if (bigintBitLen > numBitLen) {
        return bigintSign ? 0 : 1;
    }

    if (bigintBitLen < numBitLen) {
        return bigintSign ? 1 : 0;
    }
    return -1;
}

ComparisonResult BigInt::CompareWithNumber(JSHandle<BigInt> bigint, JSHandle<JSTaggedValue> number)
{
    double num = number->GetNumber();
    bool numberSign = num < 0;
    if (std::isnan(num)) {
        return ComparisonResult::UNDEFINED;
    }
    if (!std::isfinite(num)) {
        return (!numberSign ?  ComparisonResult::LESS : ComparisonResult::GREAT);
    }
    // Bit operations must be of integer type
    uint64_t bits = 0;
    if (memcpy_s(&bits, sizeof(bits), &num, sizeof(num)) != EOK) {
        LOG_FULL(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
    int exponential = (bits >> base::DOUBLE_SIGNIFICAND_SIZE) & 0x7FF;

    // Take out bits 62-52 (11 bits in total) and subtract 1023
    int integerDigits = exponential - base::DOUBLE_EXPONENT_BIAS;
    uint64_t mantissa = (bits & base::DOUBLE_SIGNIFICAND_MASK) | base::DOUBLE_HIDDEN_BIT;
    bool bigintSign = bigint->GetSign();
    // Handling the opposite sign
    if (!numberSign && bigintSign) {
        return ComparisonResult::LESS;
    } else if (numberSign && !bigintSign) {
        return ComparisonResult::GREAT;
    }
    if (bigint->IsZero() && !num) {
        return ComparisonResult::EQUAL;
    }
    if (bigint->IsZero() && num > 0) {
        return ComparisonResult::LESS;
    }

    if (integerDigits < 0) {
        return bigintSign ? ComparisonResult::LESS : ComparisonResult::GREAT;
    }

    // Compare the significant bits of bigint with the significant integer bits of double
    int leadingZeros = 0;
    int res = CompareToBitsLen(bigint, integerDigits + 1, leadingZeros);
    if (res == 0) {
        return ComparisonResult::LESS;
    } else if (res == 1) {
        return ComparisonResult::GREAT;
    }
    int mantissaSize = base::DOUBLE_SIGNIFICAND_SIZE; // mantissaSize
    uint32_t bigintLen = bigint->GetLength();
    int leftover = 0;
    bool IsFirstInto = true;
    ASSERT(bigintLen > 0);
    for (int index = static_cast<int>(bigintLen - 1); index >= 0; --index) {
        uint32_t doubleNum = 0;
        uint32_t BigintNum = bigint->GetDigit(index);
        if (IsFirstInto) {
            IsFirstInto = false;
            leftover = mantissaSize - BigInt::DATEBITS + leadingZeros + 1;
            doubleNum = static_cast<uint32_t>(mantissa >> leftover);
            mantissa = mantissa << (64 - leftover); // 64 : double bits
            if (BigintNum > doubleNum) {
                return bigintSign ? ComparisonResult::LESS : ComparisonResult::GREAT;
            }
            if (BigintNum < doubleNum) {
                return bigintSign ? ComparisonResult::GREAT : ComparisonResult::LESS;
            }
        } else {
            leftover -= BigInt::DATEBITS;
            doubleNum = static_cast<uint32_t>(mantissa >> BigInt::DATEBITS);
            mantissa = mantissa << BigInt::DATEBITS;
            if (BigintNum > doubleNum) {
                return bigintSign ? ComparisonResult::LESS : ComparisonResult::GREAT;
            }
            if (BigintNum < doubleNum) {
                return bigintSign ? ComparisonResult::GREAT : ComparisonResult::LESS;
            }
            leftover -= BigInt::DATEBITS;
        }
    }

    if (mantissa != 0) {
        ASSERT(leftover > 0);
        return bigintSign ? ComparisonResult::GREAT : ComparisonResult::LESS;
    }
    return ComparisonResult::EQUAL;
}
} // namespace
