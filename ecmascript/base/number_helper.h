/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_BASE_NUMBER_HELPER_H
#define ECMASCRIPT_BASE_NUMBER_HELPER_H

#include <cstdint>

#include "ecmascript/ecma_string.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::base {
constexpr double MIN_RADIX = 2;
constexpr double MAX_RADIX = 36;
constexpr double MIN_FRACTION = 0;
constexpr double MAX_FRACTION = 100;

// Coversion flags
static constexpr uint32_t NO_FLAGS = 0U;
static constexpr uint32_t ALLOW_BINARY = 1U << 0U;
static constexpr uint32_t ALLOW_OCTAL = 1U << 1U;
static constexpr uint32_t ALLOW_HEX = 1U << 2U;
static constexpr uint32_t IGNORE_TRAILING = 1U << 3U;

static constexpr char HALFCHAR = '5';
static constexpr uint32_t MAX_PRECISION = 16;
static constexpr uint8_t BINARY = 2;
static constexpr uint8_t OCTAL = 8;
static constexpr uint8_t DECIMAL = 10;
static constexpr uint8_t HEXADECIMAL = 16;
static constexpr double HALF = 0.5;
static constexpr double EPSILON = std::numeric_limits<double>::epsilon();
static constexpr int64_t MAX_SAFE_INTEGER = 9007199254740991;
static constexpr double MAX_VALUE = std::numeric_limits<double>::max();
static constexpr double MIN_VALUE = std::numeric_limits<double>::min();
static constexpr double POSITIVE_INFINITY = std::numeric_limits<double>::infinity();
static constexpr double NAN_VALUE = std::numeric_limits<double>::quiet_NaN();
static constexpr uint64_t MAX_UINT64_VALUE = std::numeric_limits<uint64_t>::max();
static constexpr int MAX_INT_VALUE = std::numeric_limits<int>::max();

// Helper defines for double
static constexpr int DOUBLE_MAX_PRECISION = 17;
static constexpr int DOUBLE_EXPONENT_BIAS = 0x3FF;
static constexpr size_t DOUBLE_SIGNIFICAND_SIZE = 52;
static constexpr uint64_t DOUBLE_SIGN_MASK = 0x8000000000000000ULL;
static constexpr uint64_t DOUBLE_EXPONENT_MASK = 0x7FFULL << DOUBLE_SIGNIFICAND_SIZE;
static constexpr uint64_t DOUBLE_SIGNIFICAND_MASK = 0x000FFFFFFFFFFFFFULL;
static constexpr uint64_t DOUBLE_HIDDEN_BIT = 1ULL << DOUBLE_SIGNIFICAND_SIZE;
static constexpr int32_t MINUS_ZERO_LOBITS = static_cast<int32_t>(0);
static constexpr int32_t MINUS_ZERO_HIBITS = static_cast<int32_t>(1) << 31;
static constexpr int64_t MINUS_ZERO_BITS = (static_cast<uint64_t>(MINUS_ZERO_HIBITS) << 32) | MINUS_ZERO_LOBITS;
static constexpr size_t INT64_BITS = 64;
static constexpr size_t INT32_BITS = 32;
static constexpr size_t INT16_BITS = 16;
static constexpr size_t INT8_BITS = 8;
static constexpr size_t JS_DTOA_BUF_SIZE = 128;

// Max number of hexadecimal digits to display an integer
static constexpr size_t INT64_HEX_DIGITS = INT64_BITS / 4;
static constexpr size_t INT32_HEX_DIGITS = INT32_BITS / 4;
static constexpr size_t INT16_HEX_DIGITS = INT16_BITS / 4;
static constexpr size_t INT8_HEX_DIGITS = INT8_BITS / 4;

static constexpr int EXPONENTBIAS =  DOUBLE_EXPONENT_BIAS + DOUBLE_SIGNIFICAND_SIZE;
static constexpr int kDENORMAL = -EXPONENTBIAS + 1;
static constexpr uint64_t kINFINITY = 0x7FF0'0000'0000'0000;

// help defines for random
static constexpr int RIGHT12 = 12;
static constexpr int SECONDS_TO_SUBTLE = 1000000;
static constexpr int RIGHT27 = 27;
static constexpr int LEFT25 = 25;
static constexpr uint64_t GET_MULTIPLY = 0x2545F4914F6CDD1D;
// Exponent bits for double value between [1.0, 2.0)
static constexpr uint64_t EXPONENTBITS_RANGE_IN_ONE_AND_TWO = 0x3FF0000000000000;

// Special Value for Hole in ElementsKind
static constexpr uint64_t SPECIAL_HOLE = 0xFFFE000000000001;

// Special Value for Hole in ElementsKind
static constexpr uint32_t PGO_POLY_INLINE_REP = 0x7FFFFFFF;

//
static constexpr int MAX_DIGITS = 21;
static constexpr int MIN_DIGITS = -6;

// NumberFormat type
static constexpr int VAR_FORMAT = 0;
static constexpr int FIXED_FORMAT = 1;
static constexpr int FRAC_FORMAT  = 2;
static constexpr int FORCE_FORMAT = 4;

// means add the point char to buf
static constexpr int POINT_INDEX = 3;
static constexpr int DECIMAL_INDEX = 2;

class NumberHelper {
public:
    static inline JSTaggedType GetNaN()
    {
        return JSTaggedValue(NAN_VALUE).GetRawData();
    }

    static inline JSTaggedType GetPositiveInfinity()
    {
        return JSTaggedValue(POSITIVE_INFINITY).GetRawData();
    }

    static bool IsFinite(JSTaggedValue number)
    {
        return number.IsInt() || (number.IsDouble() && std::isfinite(number.GetDouble()));
    }
    static bool IsNaN(JSTaggedValue number)
    {
        return number.IsDouble() && std::isnan(number.GetDouble());
    }

    static bool inline IsDenormal(uint64_t x)
    {
        return (x & kINFINITY) == 0;
    }

    static int inline Exponent(double x)
    {
        uint64_t value =  base::bit_cast<uint64_t>(x);
        if (IsDenormal(value)) {
            return kDENORMAL;
        }
        int biased = static_cast<int>((value & kINFINITY) >> DOUBLE_SIGNIFICAND_SIZE);
        return biased - EXPONENTBIAS;
    }

    static uint64_t inline Significand(double x)
    {
        uint64_t value =  base::bit_cast<uint64_t>(x);
        uint64_t significand = value & DOUBLE_SIGNIFICAND_MASK;
        if (!IsDenormal(value)) {
            return significand + DOUBLE_HIDDEN_BIT;
        } else {
            return significand;
        }
    }

    static bool inline IsSafeIntegerNumber(double d)
    {
        double number = TruncateDouble(d);
        return (number == d) && std::abs(d) <= MAX_SAFE_INTEGER;
    }

    static JSTaggedValue DoubleToString(JSThread *thread, double number, int radix);
    static bool IsEmptyString(const uint8_t *start, const uint8_t *end);
    static JSHandle<EcmaString> IntToEcmaString(const JSThread *thread, int number);
    static JSHandle<EcmaString> DoubleToEcmaString(const JSThread *thread, double d);
    static uint32_t ToCharCode(uint32_t number);
    static JSTaggedValue Int32ToString(JSThread *thread, int32_t number, uint32_t radix);
    static JSHandle<EcmaString> NumberToString(const JSThread *thread, JSTaggedValue number);
    static double PUBLIC_API TruncateDouble(double d);
    static int64_t DoubleToInt64(double d);
    static bool IsDigitalString(const uint8_t *start, const uint8_t *end);
    static int StringToInt(const uint8_t *start, const uint8_t *end);
    static std::pair<bool, JSTaggedNumber> FastStringToNumber(const uint8_t *start,
        const uint8_t *end, JSTaggedValue string);
    static double StringToDouble(const uint8_t *start, const uint8_t *end, uint8_t radix, uint32_t flags = NO_FLAGS);
    static int32_t DoubleToInt(double d, size_t bits);
    static int32_t PUBLIC_API DoubleInRangeInt32(double d);
    static JSTaggedValue StringToNumber(EcmaString *string, int32_t radix);
    static JSTaggedValue StringToDoubleWithRadix(const uint8_t *start, const uint8_t *end, int radix, bool *negative);
    static CString IntToString(int number);
    static CString IntegerToString(double number, int radix);
    static JSTaggedValue PUBLIC_API StringToBigInt(JSThread *thread, JSHandle<JSTaggedValue> strVal);
    static JSTaggedValue DoubleToExponential(JSThread *thread, double number, int digit);
    static JSTaggedValue DoubleToASCII(JSThread *thread, double valueNumber, int digits, int flags);
    static JSTaggedValue DoubleToFixedString(JSThread *thread, double valueNumber, int digits);
    static JSTaggedValue DoubleToPrecisionString(JSThread *thread, double valueNumber, int digits);
    static void DoubleToASCIIWithFlag(std::string& buf, double valueNumber, int digits, int flags);
    static void ToASCIIWithNegative(std::string& tmpbuf, int digitNumber, int n, const std::string& buf);
    static void ToASCIIWithGreatThanZero(std::string& tmpbuf, int digitNumber, int number, const std::string& buf);
private:
    static char Carry(char current, int radix);
    static double Strtod(const char *str, int exponent, uint8_t radix);
    static bool GotoNonspace(uint8_t **ptr, const uint8_t *end);
    static void GetBase(double d, int digits, int *decimalPoint, char *buf, char *bufTmp, int size);
    static int GetMinmumDigits(double d, int *decimalPoint, char *buf);
    static int CustomEcvt(double valueNumber, int digits, int *decimalPoint, std::string& buf,
                          bool isFixed, int *sign);
    static void CustomFcvt(std::string& buf, int bufSize, double valueNumber, int digits);
    static int CustomFcvtHelper(std::string& buf, int bufSize, double valueNumber, int digits, int roundingMode);
    static void GetBaseForRoundingMode(double valueNumber, int digitNumber, int *decimalPoint, std::string& buf,
                std::string& buf1, int buf1Size, int roundingMode, int *sign);
    static void CustomEcvtIsFixed(double &valueNumber, int &digits, int *decimalPoint, std::string& buf, int *sign);
};

// This class is used to generate 0~1 uniform distribution pseudo-random numbers.
// It uses a 64-bit seed which is current timestamp to generate state value.
// The value is used in xorshift64* random generator to generate result.
class RandomGenerator {
public:
    static void InitRandom(JSThread *thread);
    static double NextDouble();
    static int32_t GenerateIdentityHash();
    static int32_t Next(int bits);

private:
    static uint64_t XorShift64(uint64_t *pVal);
    static double ToDouble(uint64_t state);

private:
    static thread_local uint64_t randomState_;
};
}  // namespace panda::ecmascript::base
#endif  // ECMASCRIPT_BASE_NUMBER_HELPER_H
