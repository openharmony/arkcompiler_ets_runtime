/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/base/number_helper.h"

#include <cfenv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <sys/time.h>

#include "ecmascript/base/dtoa_helper.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/builtins/builtins_number.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript::base {
using NumberToStringResultCache = builtins::NumberToStringResultCache;

enum class Sign { NONE, NEG, POS };
thread_local uint64_t RandomGenerator::randomState_ {0};
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RETURN_IF_CONVERSION_END(p, end, result) \
    if ((p) == (end)) {                          \
        return (result);                         \
    }

constexpr char CHARS[] = "0123456789abcdefghijklmnopqrstuvwxyz";  // NOLINT (modernize-avoid-c-arrays)
constexpr uint64_t MAX_MANTISSA = 0x1ULL << 52U;

static const double POWERS_OF_TEN[] = {
    1.0,                      // 10^0
    10.0,
    100.0,
    1000.0,
    10000.0,
    100000.0,
    1000000.0,
    10000000.0,
    100000000.0,
    1000000000.0,
    10000000000.0,            // 10^10
    100000000000.0,
    1000000000000.0,
    10000000000000.0,
    100000000000000.0,
    1000000000000000.0,
    10000000000000000.0,
    100000000000000000.0,
    1000000000000000000.0,
    10000000000000000000.0,
    100000000000000000000.0,  // 10^20
    1000000000000000000000.0,
    10000000000000000000000.0 // 10^22
};
static const int POWERS_OF_TEN_SIZE = 23;

static inline uint8_t ToDigit(uint8_t c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + DECIMAL;
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + DECIMAL;
    }
    return '$';
}

bool NumberHelper::GotoNonspace(uint8_t **ptr, const uint8_t *end)
{
    while (*ptr < end) {
        uint16_t c = **ptr;
        size_t size = 1;
        if (c > INT8_MAX) {
            size = 0;
            uint16_t utf8Bit = INT8_MAX + 1;  // equal 0b1000'0000
            while (utf8Bit > 0 && (c & utf8Bit) == utf8Bit) {
                ++size;
                utf8Bit >>= 1UL;
            }
            if (base::utf_helper::ConvertRegionUtf8ToUtf16(*ptr, &c, end - *ptr, 1) <= 0) {
                return true;
            }
        }
        if (!StringHelper::IsNonspace(c)) {
            return true;
        }
        *ptr += size;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    return false;
}

static inline double SignedZero(Sign sign)
{
    return sign == Sign::NEG ? -0.0 : 0.0;
}

bool NumberHelper::IsEmptyString(const uint8_t *start, const uint8_t *end)
{
    auto p = const_cast<uint8_t *>(start);
    return !NumberHelper::GotoNonspace(&p, end);
}

/*
*  This Function Translate from number 0-9 to number '0'-'9'
*                               number 10-35 to number 'a'-'z'
*/
uint32_t NumberHelper::ToCharCode(uint32_t number)
{
    ASSERT(number < 36); // 36 == total number of '0'-'9' + 'a' -'z'
    return number < 10 ? (number + 48): // 48 == '0'; 10: '0' - '9';
                         (number - 10 + 97); // 97 == 'a'; 'a' - 'z'
}

JSTaggedValue NumberHelper::Int32ToString(JSThread *thread, int32_t number, uint32_t radix)
{
    bool isNegative = number < 0;
    uint32_t n = 0;
    if (!isNegative) {
        n = static_cast<uint32_t>(number);
        if (n < radix) {
            if (n == 0) {
                return thread->GlobalConstants()->GetHandledZeroString().GetTaggedValue();
            }
            JSHandle<SingleCharTable> singleCharTable(thread, thread->GetSingleCharTable());
            return singleCharTable->GetStringFromSingleCharTable(ToCharCode(n));
        }
    } else {
        n = static_cast<uint32_t>(-number);
    }
    uint32_t temp = n;
    uint32_t length = isNegative ? 1 : 0;
    // calculate length
    while (temp > 0) {
        temp = temp / radix;
        length = length + 1;
    }
    std::string buf;
    buf.resize(length);
    ASSERT(length > 0);
    uint32_t index = length - 1;
    uint32_t digit = 0;
    while (n > 0) {
        digit = n % radix;
        n /= radix;
        buf[index] = ToCharCode(digit) + 0X00;
        index--;
    }
    if (isNegative) {
        ASSERT(index == 0);
        buf[index] = '-';
    }
    return thread->GetEcmaVM()->GetFactory()->NewFromUtf8(buf).GetTaggedValue();
}

JSTaggedValue NumberHelper::DoubleToString(JSThread *thread, double number, int radix)
{
    static constexpr int BUFFER_SIZE = 2240; // 2240: The size of the character array buffer
    static constexpr int HALF_BUFFER_SIZE = BUFFER_SIZE >> 1;
    char buffer[BUFFER_SIZE];
    size_t integerCursor = HALF_BUFFER_SIZE;
    size_t fractionCursor = integerCursor;

    bool negative = number < 0.0;
    if (negative) {
        number = -number;
    }

    double integer = std::floor(number);
    double fraction = number - integer;

    auto value = bit_cast<uint64_t>(number);
    value += 1;
    double delta = HALF * (bit_cast<double>(value) - number);
    delta = std::max(delta, bit_cast<double>(static_cast<uint64_t>(1))); // 1 : The binary of the smallest double is 1
    if (fraction != 0 && fraction >= delta) {
        buffer[fractionCursor++] = '.';
        while (fraction >= delta) {
            fraction *= radix;
            delta *= radix;
            int64_t digit = std::floor(fraction);
            fraction -= digit;
            buffer[fractionCursor++] = CHARS[digit];
            bool needCarry = (fraction > HALF) && (fraction + delta > 1);
            if (needCarry) {
                size_t fractionEnd = fractionCursor - 1;
                buffer[fractionEnd] = Carry(buffer[fractionEnd], radix);
                for (; fractionEnd > HALF_BUFFER_SIZE; fractionEnd--) {
                    if (buffer[fractionEnd] == '0') {
                        buffer[fractionEnd - 1] = Carry(buffer[fractionEnd - 1], radix);
                    } else {
                        break;
                    }
                }
                if (fractionEnd == HALF_BUFFER_SIZE) {
                    ++integer;
                }
                break;
            }
        }
        // delete 0 in the end
        size_t fractionEnd = fractionCursor - 1;
        while (buffer[fractionEnd] == '0') {
            --fractionEnd;
        }
        fractionCursor = fractionEnd + 1;
    }

    ASSERT(radix >= MIN_RADIX && radix <= MAX_RADIX);
    while (Exponent(integer / radix) > 0) {
        integer /= radix;
        buffer[--integerCursor] = '0';
    }
    do {
        double remainder = std::fmod(integer, radix);
        buffer[--integerCursor] = CHARS[static_cast<int>(remainder)];
        integer = (integer - remainder) / radix;
    } while (integer > 0);

    if (negative) {
        buffer[--integerCursor] = '-';
    }
    buffer[fractionCursor++] = '\0';

    size_t size = fractionCursor - integerCursor;
    std::unique_ptr<char[]> result = std::make_unique<char[]>(size);
    if (memcpy_s(result.get(), size, buffer + integerCursor, size) != EOK) {
        LOG_FULL(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
    return BuiltinsBase::GetTaggedString(thread, result.get());
}

JSTaggedValue NumberHelper::DoubleToFixedString(JSThread *thread, double valueNumber, int digitNumber)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    bool negative = false;
    double absValue = valueNumber;
    std::string result;

    if (valueNumber < 0) {
        result += "-";
        absValue = -valueNumber;
        negative = true;
    }
    int decimalPoint;
    const int decimalRepCapacity = MAX_DIGITS + MAX_FRACTION + 1; // Add space for the '\0' byte.
    char decimalRep[decimalRepCapacity];
    int length;
    bool isFast = DtoaHelper::FixedDtoa(absValue, digitNumber,
        BufferVector<char>(decimalRep, decimalRepCapacity), &length, &decimalPoint);
    if (!isFast) {
        return DoubleToASCII(thread, valueNumber, digitNumber, base::FRAC_FORMAT); // slow
    }
    int zeroPrefixLen = 0;
    int zeroPostfixLen = 0;
    if (decimalPoint <= 0) {
        zeroPrefixLen = -decimalPoint + 1;
        decimalPoint = 1;
    }
    if (zeroPrefixLen + length < decimalPoint + digitNumber) {
        zeroPostfixLen = decimalPoint + digitNumber - length - zeroPrefixLen;
    }
    result += std::string(zeroPrefixLen, '0');
    result += decimalRep;
    result += std::string(zeroPostfixLen, '0');
    if (digitNumber > 0) {
        if (negative) {
            result.insert(decimalPoint + 1, 1, '.');
        } else {
            result.insert(decimalPoint, 1, '.');
        }
    }
    return factory->NewFromASCII(result.c_str()).GetTaggedValue();
}

JSTaggedValue NumberHelper::DoubleToASCII(JSThread *thread, double valueNumber, int digitNumber, int flags)
{
    std::string buffer(JS_DTOA_BUF_SIZE, '\0');
    DoubleToASCIIWithFlag(buffer, valueNumber, digitNumber, flags);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewFromASCII(buffer.c_str()).GetTaggedValue();
}

void NumberHelper::GetBaseForRoundingMode(double valueNumber, int digitNumber, int *decimalPoint, std::string& buf,
                                          std::string& buf1, int buf1Size, int roundingMode, int *sign)
{
    if (roundingMode != FE_TONEAREST) {
        fesetround(roundingMode);
    }
    int result = snprintf_s(&buf1[0], buf1Size, buf1Size - 1, "%+.*e", digitNumber - 1, valueNumber);
    if (result == -1) {
        LOG_FULL(FATAL) << "snprintf_s failed";
        UNREACHABLE();
    }
    if (roundingMode != FE_TONEAREST) {
        fesetround(FE_TONEAREST);
    }
    *sign = (buf1[0] == '-');
    buf[0] = buf1[1];
    if (digitNumber > 1) {
        if (memcpy_s(&buf[1], digitNumber - 1, &buf1[POINT_INDEX], digitNumber - 1) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    }
    buf[digitNumber] = '\0';
    *decimalPoint = std::atoi(&buf1[digitNumber + DECIMAL_INDEX + (digitNumber > 1)]) + 1;
}

void NumberHelper::CustomEcvtIsFixed(double &valueNumber, int &digits, int *decimalPoint, std::string& buf, int *sign)
{
    std::string buffer(JS_DTOA_BUF_SIZE, '\0');
    unsigned int digitsMin = 1;
    unsigned int digitsMax = DOUBLE_MAX_PRECISION;
    while (digitsMin < digitsMax) {
        digits = (digitsMin + digitsMax) / MIN_RADIX;
        ASSERT(buffer.size() <= JS_DTOA_BUF_SIZE);
        GetBaseForRoundingMode(valueNumber, digits, decimalPoint, buf, buffer, JS_DTOA_BUF_SIZE, FE_TONEAREST, sign);
        if (std::strtod(buffer.c_str(), NULL) == valueNumber) {
            while (digits >= MIN_RADIX && buf[digits - 1] == '0') {
                digits--;
            }
            digitsMax = static_cast<unsigned int>(digits);
        } else {
            digitsMin = static_cast<unsigned int>(digits) + 1;
        }
    }
    digits = static_cast<int>(digitsMax);
}

int NumberHelper::CustomEcvt(double valueNumber, int digits, int *decimalPoint,
                             std::string& buf, bool isFixed, int *sign)
{
    std::string buffer(JS_DTOA_BUF_SIZE, '\0');
    int roundingMode = FE_TONEAREST;
    if (!isFixed) {
        CustomEcvtIsFixed(valueNumber, digits, decimalPoint, buf, sign);
    } else {
        std::string buf1(JS_DTOA_BUF_SIZE, '\0');
        std::string buf2(JS_DTOA_BUF_SIZE, '\0');
        int decpt1 = 0;
        int decpt2 = 0;
        int sign1 = 0;
        int sign2 = 0;
        ASSERT(buffer.size() <= JS_DTOA_BUF_SIZE);
        GetBaseForRoundingMode(valueNumber, digits + 1, &decpt1, buf1, buffer, JS_DTOA_BUF_SIZE, roundingMode, &sign1);
        if (buf1[digits] == HALFCHAR) {
            ASSERT(buf1.size() <= JS_DTOA_BUF_SIZE);
            GetBaseForRoundingMode(valueNumber, digits + 1, &decpt1, buf1, buffer, JS_DTOA_BUF_SIZE,
                FE_DOWNWARD, &sign1);
            ASSERT(buf2.size() <= JS_DTOA_BUF_SIZE);
            GetBaseForRoundingMode(valueNumber, digits + 1, &decpt2, buf2, buffer, JS_DTOA_BUF_SIZE,
                FE_UPWARD, &sign2);
            if (memcmp(buf1.c_str(), buf2.c_str(), digits + 1) == 0 && decpt1 == decpt2) {
                roundingMode = sign1 ? FE_DOWNWARD : FE_UPWARD;
            }
        }
    }
    ASSERT(buffer.size() <= JS_DTOA_BUF_SIZE);
    GetBaseForRoundingMode(valueNumber, digits, decimalPoint, buf, buffer, JS_DTOA_BUF_SIZE, roundingMode, sign);
    return digits;
}

int NumberHelper::CustomFcvtHelper(std::string& buf, int bufSize, double valueNumber, int digits, int roundingMode)
{
    if (roundingMode != FE_TONEAREST) {
        std::fesetround(roundingMode);
    }
    int result = snprintf_s(&buf[0], bufSize, bufSize, "%.*f", digits, valueNumber);
    if (result == -1) {
        LOG_FULL(FATAL) << "snprintf_s failed";
        UNREACHABLE();
    }
    if (roundingMode != FE_TONEAREST) {
        std::fesetround(FE_TONEAREST);
    }
    ASSERT(result < bufSize);
    return result;
}

void NumberHelper::CustomFcvt(std::string& buf, int bufSize, double valueNumber, int digits)
{
    int number = 0;
    int tmpNumber = 0;
    std::string tmpbuf1(JS_DTOA_BUF_SIZE, '\0');
    std::string tmpbuf2(JS_DTOA_BUF_SIZE, '\0');
    int roundingMode = FE_TONEAREST;
    number = CustomFcvtHelper(tmpbuf1, JS_DTOA_BUF_SIZE, valueNumber, digits + 1, roundingMode);
    if (tmpbuf1[number - 1] == HALFCHAR) {
        number = CustomFcvtHelper(tmpbuf1, JS_DTOA_BUF_SIZE, valueNumber, digits + 1, FE_DOWNWARD);
        tmpNumber = CustomFcvtHelper(tmpbuf2, JS_DTOA_BUF_SIZE, valueNumber, digits + 1, FE_UPWARD);
        if (tmpbuf1 == tmpbuf2) {
            if (tmpbuf1[0] == '-') {
                roundingMode = FE_DOWNWARD;
            } else {
                roundingMode = FE_UPWARD;
            }
        }
    }
    CustomFcvtHelper(buf, bufSize, valueNumber, digits, roundingMode);
}

JSTaggedValue NumberHelper::DoubleToPrecisionString(JSThread *thread, double number, int digit)
{
    if (number == 0.0) {
        return DoubleToFixedString(thread, number, digit - 1);
    }
    double positiveNumber = number > 0 ? number : -number;
    int logDigit = std::floor(log10(positiveNumber));
    int radixDigit = digit - logDigit - 1;
    const int MIN_EXPONENT_DIGIT = -6;
    if ((logDigit >= MIN_EXPONENT_DIGIT && logDigit < digit)) {
        return DoubleToFixedString(thread, number, std::abs(radixDigit));
    }
    return DoubleToExponential(thread, number, digit);
}

JSTaggedValue NumberHelper::DoubleToExponential(JSThread *thread, double number, int digit)
{
    char tmpbuf[JS_DTOA_BUF_SIZE] = {0};
    // Can use std::to_chars for performance.
    if (digit == 0) {
        if (number == 0.0) {
            return BuiltinsBase::GetTaggedString(thread, "0e+0");
        }
        std::string res;
        if (number < 0) {
            res += "-";
            number = -number;
        }
        int n;
        int k;
        DtoaHelper::Dtoa(number, tmpbuf, &n, &k);
        std::string base = tmpbuf;
        base.erase(1, k - 1);
        if (k != 1) {
            base += std::string(".") + std::string(tmpbuf + 1);
        }
        base += "e" + (n >= 1 ? std::string("+") : "") + std::to_string(n - 1);
        res += base;
        return BuiltinsBase::GetTaggedString(thread, res.c_str());
    } else {
        int result = snprintf_s(tmpbuf, sizeof(tmpbuf), sizeof(tmpbuf) - 1, "%.*e", digit - 1, number);
        if (result == -1) {
            LOG_FULL(FATAL) << "snprintf_s failed";
            UNREACHABLE();
        }
    }
    std::string result = tmpbuf;
    size_t found = result.find_last_of('e');
    if (found != CString::npos && found < result.size() - 2 && result[found + 2] == '0') { // 2:offset of e
        result.erase(found + 2, 1); // 2:offset of e
    }
    if (digit < 0) {
        size_t end = found;
        while (--found > 0) {
            if (result[found] != '0') {
                break;
            }
        }
        if (result[found] == '.') {
            found--;
        }
        if (found < end - 1) {
            result.erase(found + 1, end - found - 1);
        }
    }
    return BuiltinsBase::GetTaggedString(thread, result.c_str());
}

void NumberHelper::DoubleToASCIIWithFlag(std::string& buf, double valueNumber, int digits, int flags)
{
    if (valueNumber == 0.0) {
        valueNumber = 0.0;
    }
    if (flags == FRAC_FORMAT) {
        CustomFcvt(buf, JS_DTOA_BUF_SIZE, valueNumber, digits);
    } else {
        std::string buf1(JS_DTOA_BUF_SIZE, '\0');
        int decimalPoint = 0;
        int sign = 0;
        bool fixed = ((static_cast<unsigned int>(flags) & POINT_INDEX) ==
            static_cast<unsigned int>(base::FIXED_FORMAT));
        int numberMax = fixed ? digits : MAX_DIGITS;
        int digitNumber = CustomEcvt(valueNumber, digits, &decimalPoint, buf1, fixed, &sign);
        int number = decimalPoint;
        std::string tmpbuf;
        int i = 0;
        if (sign) {
            tmpbuf += '-';
        }
        if (number > 0 && number <= numberMax) {
            ToASCIIWithGreatThanZero(tmpbuf, digitNumber, number, buf1);
        } else if (MIN_DIGITS < number && number <= 0) {
            tmpbuf += '0';
            tmpbuf += '.';
            for (i = 0; i < -number; i++) {
                tmpbuf += '0';
            }
            tmpbuf += buf1.substr(0, digitNumber);
        } else {
            ToASCIIWithNegative(tmpbuf, digitNumber, number, buf1);
        }
        buf = tmpbuf;
    }
}

void NumberHelper::ToASCIIWithGreatThanZero(std::string& tmpbuf, int digitNumber, int number, const std::string& buf)
{
    if (digitNumber <= number) {
        tmpbuf += buf.substr(0, digitNumber);
        tmpbuf += std::string(number - digitNumber, '0');
        tmpbuf += '\0';
    } else {
        tmpbuf += buf.substr(0, number);
        tmpbuf += '.';
        tmpbuf += buf.substr(number, digitNumber - number);
        tmpbuf += '\0';
    }
}

void NumberHelper::ToASCIIWithNegative(std::string& tmpbuf, int digitNumber, int n, const std::string& buf)
{
    tmpbuf += buf[0];
    if (digitNumber > 1) {
        tmpbuf += '.';
        for (int i = 1; i < digitNumber; i++) {
            tmpbuf += buf[i];
        }
    }
    tmpbuf += 'e';
    int p = n - 1;
    if (p >= 0) {
        tmpbuf += '+';
    }
    tmpbuf += std::to_string(p);
}

JSTaggedValue NumberHelper::StringToNumber(EcmaString *string, int32_t radix)
{
    bool negative = false;
    if ((radix == base::DECIMAL || radix == 0)) {
        int32_t elementIndex = 0;
        if (EcmaStringAccessor(string).ToInt(&elementIndex, &negative)) {
            if (elementIndex == 0 && negative == true) {
                return JSTaggedValue(-0.0);
            }
            return JSTaggedValue(elementIndex);
        }
    }
    CVector<uint8_t> buf;
    Span<const uint8_t> str = EcmaStringAccessor(string).ToUtf8Span(buf);

    JSTaggedValue result = NumberHelper::StringToDoubleWithRadix(str.begin(), str.end(), radix, &negative);
    if (result.GetNumber() == 0 && negative == true) {
        return JSTaggedValue(-0.0);
    }
    return JSTaggedValue::TryCastDoubleToInt32(result.GetNumber());
}

JSTaggedValue NumberHelper::StringToDoubleWithRadix(const uint8_t *start, const uint8_t *end, int radix, bool *negative)
{
    auto p = const_cast<uint8_t *>(start);
    JSTaggedValue nanResult = BuiltinsBase::GetTaggedDouble(NAN_VALUE);
    // 1. skip space and line terminal
    if (!NumberHelper::GotoNonspace(&p, end)) {
        return nanResult;
    }

    // 2. sign bit
    if (*p == '-') {
        *negative = true;
        RETURN_IF_CONVERSION_END(++p, end, nanResult);
    } else if (*p == '+') {
        RETURN_IF_CONVERSION_END(++p, end, nanResult);
    }
    // 3. 0x or 0X
    bool stripPrefix = true;
    // 4. If R  0, then
    //     a. If R < 2 or R > 36, return NaN.
    //     b. If R  16, let stripPrefix be false.
    if (radix != 0) {
        if (radix < MIN_RADIX || radix > MAX_RADIX) {
            return nanResult;
        }
        if (radix != HEXADECIMAL) {
            stripPrefix = false;
        }
    } else {
        radix = DECIMAL;
    }
    int size = 0;
    if (stripPrefix) {
        if (*p == '0') {
            if (++p != end && (*p == 'x' || *p == 'X')) {
                RETURN_IF_CONVERSION_END(++p, end, nanResult);
                radix = HEXADECIMAL;
            } else {
                size++;
            }
        }
    }

    double result = 0;
    bool isDone = false;
    do {
        double part = 0;
        uint32_t multiplier = 1;
        for (; p != end; ++p) {
            // The maximum value to ensure that uint32_t will not overflow
            const uint32_t MAX_MULTIPER = 0xffffffffU / 36;
            uint32_t m = multiplier * static_cast<uint32_t>(radix);
            if (m > MAX_MULTIPER) {
                break;
            }

            int currentBit = static_cast<int>(ToDigit(*p));
            if (currentBit >= radix) {
                isDone = true;
                break;
            }
            size++;
            part = part * radix + currentBit;
            multiplier = m;
        }
        result = result * multiplier + part;
        if (isDone) {
            break;
        }
    } while (p != end);

    if (size == 0) {
        return nanResult;
    }

    if (*negative) {
        result = -result;
    }
    return BuiltinsBase::GetTaggedDouble(result);
}

char NumberHelper::Carry(char current, int radix)
{
    int digit = static_cast<int>((current > '9') ? (current - 'a' + DECIMAL) : (current - '0'));
    digit = (digit == (radix - 1)) ? 0 : digit + 1;
    return CHARS[digit];
}

CString NumberHelper::IntegerToString(double number, int radix)
{
    ASSERT(radix >= MIN_RADIX && radix <= MAX_RADIX);
    CString result;
    while (number / radix > MAX_MANTISSA) {
        number /= radix;
        result = CString("0").append(result);
    }
    do {
        double remainder = std::fmod(number, radix);
        result = CHARS[static_cast<int>(remainder)] + result;
        number = (number - remainder) / radix;
    } while (number > 0);
    return result;
}

CString NumberHelper::IntToString(int number)
{
    return ToCString(number);
}

JSHandle<EcmaString> NumberHelper::IntToEcmaString(const JSThread *thread, int number)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewFromASCII(ToCString(number));
}

JSHandle<EcmaString> NumberHelper::DoubleToEcmaString(const JSThread *thread, double d)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (std::isnan(d)) {
        return JSHandle<EcmaString>::Cast(thread->GlobalConstants()->GetHandledNanCapitalString());
    }
    if (d == 0.0) {
        return JSHandle<EcmaString>::Cast(thread->GlobalConstants()->GetHandledZeroString());
    }
    if (d >= INT32_MIN + 1 && d <= INT32_MAX && d == static_cast<double>(static_cast<int32_t>(d))) {
        return factory->NewFromASCII(IntToString(static_cast<int32_t>(d)));
    }

    if (std::isinf(d)) {
        return d < 0 ? JSHandle<EcmaString>::Cast(thread->GlobalConstants()->GetHandledMinusInfinityCapitalString())
                     : JSHandle<EcmaString>::Cast(thread->GlobalConstants()->GetHandledInfinityCapitalString());
    }

    char buffer[JS_DTOA_BUF_SIZE] = {0};
    bool isNeg = false;

    if (d < 0) {
        isNeg = true;
        d = -d;
    }
    ASSERT(d > 0);
    constexpr int startIdx = 8;
    char *result = buffer + startIdx;
    int n; // decimal point
    int k; // length
    DtoaHelper::Dtoa(d, result, &n, &k); //Fast Double To Ascii.
    if (n > 0 && n <= MAX_DIGITS) {
        if (k <= n) {
            // 6. If k ≤ n ≤ 21
            for (int i = k; i < n; ++i) {
                result[i] = '0';
            }
        } else {
            // 7. If 0 < n ≤ 21
            --result;
            for (int i = 0; i < n; ++i) {
                result[i] = result[i + 1];
            }
            result[n] = '.';
        }
    } else if (MIN_DIGITS < n && n <= 0) {
        // 8. If −6 < n ≤ 0
        constexpr int prefixLen = 2;
        result -= (-n + prefixLen);
        result[0] = '0';
        result[1] = '.';
        for (int i = prefixLen; i < -n + prefixLen; ++i) {
            result[i] = '0';
        }
    } else {
        // 9. & 10. Otherwise
        int pos = k;
        if (k != 1) {
            --result;
            result[0] = result[1];
            result[1] = '.';
            ++pos;
        }
        result[pos++] = 'e';
        if (n >= 1) {
            result[pos++] = '+';
        }
        auto expo = std::to_string(n - 1);
        auto p = expo.c_str();
        for (size_t i = 0; i < expo.length(); ++i) {
            result[pos++] = p[i];
        }
    }
    if (isNeg) {
        --result;
        result[0] = '-';
    }
    return factory->NewFromASCIISkippingStringTable(result);
}

// 7.1.12.1 ToString Applied to the Number Type
JSHandle<EcmaString> NumberHelper::NumberToString(const JSThread *thread, JSTaggedValue number)
{
    ASSERT(number.IsNumber());
    JSHandle<NumberToStringResultCache> cacheTable(
        thread->GetCurrentEcmaContext()->GetNumberToStringResultCache());
    int entry = cacheTable->GetNumberHash(number);
    JSTaggedValue cacheResult = cacheTable->FindCachedResult(entry, number);
    if (cacheResult != JSTaggedValue::Undefined()) {
        return JSHandle<EcmaString>::Cast(JSHandle<JSTaggedValue>(thread, cacheResult));
    }

    JSHandle<EcmaString> resultJSHandle;
    if (number.IsInt()) {
        int intVal = number.GetInt();
        if (intVal == 0) {
            resultJSHandle = JSHandle<EcmaString>::Cast(thread->GlobalConstants()->GetHandledZeroString());
        } else {
            resultJSHandle = IntToEcmaString(thread, intVal);
        }
    } else {
        resultJSHandle = DoubleToEcmaString(thread, number.GetDouble());
    }

    cacheTable->SetCachedResult(thread, entry, number, resultJSHandle);
    return resultJSHandle;
}

double NumberHelper::TruncateDouble(double d)
{
    if (std::isnan(d)) {
        return 0;
    }
    if (!std::isfinite(d)) {
        return d;
    }
    // -0 to +0
    if (d == 0.0) {
        return 0;
    }
    double ret = (d >= 0) ? std::floor(d) : std::ceil(d);
    if (ret == 0.0) {
        ret = 0;
    }
    return ret;
}

int64_t NumberHelper::DoubleToInt64(double d)
{
    if (d >= static_cast<double>(std::numeric_limits<int64_t>::max())) {
        return std::numeric_limits<int64_t>::max();
    }
    if (d <= static_cast<double>(std::numeric_limits<int64_t>::min())) {
        return std::numeric_limits<int64_t>::min();
    }
    return static_cast<int64_t>(d);
}

uint64_t NumberHelper::DoubleToUInt64(double d)
{
    ASSERT(d <= static_cast<double>(std::numeric_limits<uint64_t>::max()) &&
           d >= static_cast<double>(std::numeric_limits<uint64_t>::min()));
    return static_cast<uint64_t>(d);
}

bool NumberHelper::IsDigitalString(const uint8_t *start, const uint8_t *end)
{
    int len = end - start;
    for (int i = 0; i < len; i++) {
        if (*(start + i) < '0' || *(start + i) > '9') {
            return false;
        }
    }
    return true;
}

int NumberHelper::StringToInt(const uint8_t *start, const uint8_t *end)
{
    int num = *start - '0';
    for (int i = 1; i < (end - start); i++) {
        num = 10 * num + (*(start + i) - '0'); // 10 : 10 represents the base of the decimal system
    }
    return num;
}

// only for string is ordinary string and using UTF8 encoding
// Fast path for short integer and some special value
std::pair<bool, JSTaggedNumber> NumberHelper::FastStringToNumber(const uint8_t *start,
    const uint8_t *end, JSTaggedValue string)
{
    ASSERT(start < end);
    EcmaStringAccessor strAccessor(string);
    bool minus = (start[0] == '-');
    int pos = (minus ? 1 : 0);

    if (pos == (end - start)) {
        return {true, JSTaggedNumber(NAN_VALUE)};
    } else if (*(start + pos) > '9') {
        // valid number's codes not longer than '9', except 'I' and non-breaking space.
        if (*(start + pos) != 'I' && *(start + pos) != 0xA0) {
            return {true, JSTaggedNumber(NAN_VALUE)};
        }
    } else if ((end - (start + pos)) <= MAX_ELEMENT_INDEX_LEN && IsDigitalString((start + pos), end)) {
        int num = StringToInt((start + pos), end);
        if (minus) {
            if (num == 0) {
                return {true, JSTaggedNumber(SignedZero(Sign::NEG))};
            }
            num = -num;
        } else {
            if ((num != 0) || (end - start == 1)) {
                strAccessor.TryToSetIntegerHash(num);
            }
        }
        return {true, JSTaggedNumber(num)};
    }

    return {false, JSTaggedNumber(NAN_VALUE)};
}

double NumberHelper::StringToDouble(const uint8_t *start, const uint8_t *end, uint8_t radix, uint32_t flags)
{
    auto p = const_cast<uint8_t *>(start);
    // 1. skip space and line terminal
    if (!NumberHelper::GotoNonspace(&p, end)) {
        return 0.0;
    }

    // 2. get number sign
    Sign sign = Sign::NONE;
    if (*p == '+') {
        RETURN_IF_CONVERSION_END(++p, end, NAN_VALUE);
        sign = Sign::POS;
    } else if (*p == '-') {
        RETURN_IF_CONVERSION_END(++p, end, NAN_VALUE);
        sign = Sign::NEG;
    }
    bool ignoreTrailing = (flags & IGNORE_TRAILING) != 0;

    // 3. judge Infinity
    static const char INF[] = "Infinity";  // NOLINT(modernize-avoid-c-arrays)
    if (*p == INF[0]) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        for (const char *i = &INF[1]; *i != '\0'; ++i) {
            if (++p == end || *p != *i) {
                return NAN_VALUE;
            }
        }
        ++p;
        if (!ignoreTrailing && NumberHelper::GotoNonspace(&p, end)) {
            return NAN_VALUE;
        }
        return sign == Sign::NEG ? -POSITIVE_INFINITY : POSITIVE_INFINITY;
    }

    // 4. get number radix
    bool leadingZero = false;
    bool prefixRadix = false;
    if (*p == '0' && radix == 0) {
        RETURN_IF_CONVERSION_END(++p, end, SignedZero(sign));
        if (*p == 'x' || *p == 'X') {
            if ((flags & ALLOW_HEX) == 0) {
                return ignoreTrailing ? SignedZero(sign) : NAN_VALUE;
            }
            RETURN_IF_CONVERSION_END(++p, end, NAN_VALUE);
            if (sign != Sign::NONE) {
                return NAN_VALUE;
            }
            prefixRadix = true;
            radix = HEXADECIMAL;
        } else if (*p == 'o' || *p == 'O') {
            if ((flags & ALLOW_OCTAL) == 0) {
                return ignoreTrailing ? SignedZero(sign) : NAN_VALUE;
            }
            RETURN_IF_CONVERSION_END(++p, end, NAN_VALUE);
            if (sign != Sign::NONE) {
                return NAN_VALUE;
            }
            prefixRadix = true;
            radix = OCTAL;
        } else if (*p == 'b' || *p == 'B') {
            if ((flags & ALLOW_BINARY) == 0) {
                return ignoreTrailing ? SignedZero(sign) : NAN_VALUE;
            }
            RETURN_IF_CONVERSION_END(++p, end, NAN_VALUE);
            if (sign != Sign::NONE) {
                return NAN_VALUE;
            }
            prefixRadix = true;
            radix = BINARY;
        } else {
            leadingZero = true;
        }
    }

    if (radix == 0) {
        radix = DECIMAL;
    }
    auto pStart = p;
    // 5. skip leading '0'
    while (*p == '0') {
        RETURN_IF_CONVERSION_END(++p, end, SignedZero(sign));
        leadingZero = true;
    }
    // 6. parse to number
    uint64_t intNumber = 0;
    uint64_t numberMax = (UINT64_MAX - (radix - 1)) / radix;
    int digits = 0;
    int exponent = 0;
    do {
        uint8_t c = ToDigit(*p);
        if (c >= radix) {
            if (!prefixRadix || ignoreTrailing || (pStart != p && !NumberHelper::GotoNonspace(&p, end))) {
                break;
            }
            // "0b" "0x1.2" "0b1e2" ...
            return NAN_VALUE;
        }
        ++digits;
        if (intNumber < numberMax) {
            intNumber = intNumber * radix + c;
        } else {
            ++exponent;
        }
    } while (++p != end);

    auto number = static_cast<double>(intNumber);
    if (sign == Sign::NEG) {
        if (number == 0) {
            number = -0.0;
        } else {
            number = -number;
        }
    }

    // 7. deal with other radix except DECIMAL
    if (p == end || radix != DECIMAL) {
        if ((digits == 0 && !leadingZero) || (p != end && !ignoreTrailing && NumberHelper::GotoNonspace(&p, end))) {
            // no digits there, like "0x", "0xh", or error trailing of "0x3q"
            return NAN_VALUE;
        }
        return number * std::pow(radix, exponent);
    }

    // 8. parse '.'
    exponent = 0;
    if (radix == DECIMAL && *p == '.') {
        RETURN_IF_CONVERSION_END(++p, end, (digits > 0 || (digits == 0 && leadingZero)) ?
                                           (number * std::pow(radix, exponent)) : NAN_VALUE);
        while (ToDigit(*p) < radix) {
            --exponent;
            ++digits;
            if (++p == end) {
                break;
            }
        }
    }
    if (digits == 0 && !leadingZero) {
        // no digits there, like ".", "sss", or ".e1"
        return NAN_VALUE;
    }
    auto pEnd = p;

    // 9. parse 'e/E' with '+/-'
    char exponentSign = '+';
    int additionalExponent = 0;
    constexpr int MAX_EXPONENT = INT32_MAX / 2;
    if (radix == DECIMAL && (p != end && (*p == 'e' || *p == 'E'))) {
        RETURN_IF_CONVERSION_END(++p, end, NAN_VALUE);

        // 10. parse exponent number
        if (*p == '+' || *p == '-') {
            exponentSign = static_cast<char>(*p);
            RETURN_IF_CONVERSION_END(++p, end, NAN_VALUE);
        }
        uint8_t digit;
        while ((digit = ToDigit(*p)) < radix) {
            if (additionalExponent > static_cast<int>(MAX_EXPONENT / radix)) {
                additionalExponent = MAX_EXPONENT;
            } else {
                additionalExponent = additionalExponent * static_cast<int>(radix) + static_cast<int>(digit);
            }
            if (++p == end) {
                break;
            }
        }
    }
    exponent += (exponentSign == '-' ? -additionalExponent : additionalExponent);
    if (!ignoreTrailing && NumberHelper::GotoNonspace(&p, end)) {
        return NAN_VALUE;
    }

    // 10. build StringNumericLiteral string
    CString buffer;
    if (sign == Sign::NEG) {
        buffer += "-";
    }
    for (uint8_t *i = pStart; i < pEnd; ++i) {  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (*i != static_cast<uint8_t>('.')) {
            buffer += *i;
        }
    }

    // 11. convert none-prefix radix string
    return Strtod(buffer.c_str(), exponent, radix);
}

double NumberHelper::Strtod(const char *str, int exponent, uint8_t radix)
{
    ASSERT(str != nullptr);
    ASSERT(radix >= base::MIN_RADIX && radix <= base::MAX_RADIX);
    auto p = const_cast<char *>(str);
    Sign sign = Sign::NONE;
    uint64_t number = 0;
    uint64_t numberMax = (UINT64_MAX - (radix - 1)) / radix;
    double result = 0.0;
    if (*p == '-') {
        sign = Sign::NEG;
        ++p;
    }
    while (*p == '0') {
        ++p;
    }
    while (*p != '\0') {
        uint8_t digit = ToDigit(static_cast<uint8_t>(*p));
        if (digit >= radix) {
            break;
        }
        if (number < numberMax) {
            number = number * radix + digit;
        } else {
            ++exponent;
        }
        ++p;
    }

    // cal pow
    int exponentAbs = exponent < 0 ? -exponent : exponent;
    double powVal = ((radix == DECIMAL) && (exponentAbs < POWERS_OF_TEN_SIZE)) ?
        POWERS_OF_TEN[exponentAbs] : std::pow(radix, exponentAbs);
    if (exponent < 0) {
        result = number / powVal;
    } else {
        result = number * powVal;
    }
    return sign == Sign::NEG ? -result : result;
}

int32_t NumberHelper::DoubleToInt(double d, size_t bits)
{
    int32_t ret = 0;
    auto u64 = bit_cast<uint64_t>(d);
    int exp = static_cast<int>((u64 & DOUBLE_EXPONENT_MASK) >> DOUBLE_SIGNIFICAND_SIZE) - DOUBLE_EXPONENT_BIAS;
    if (exp < static_cast<int>(bits - 1)) {
        // smaller than INT<bits>_MAX, fast conversion
        ret = static_cast<int32_t>(d);
    } else if (exp < static_cast<int>(bits + DOUBLE_SIGNIFICAND_SIZE)) {
        // Still has significand bits after mod 2^<bits>
        // Get low <bits> bits by shift left <64 - bits> and shift right <64 - bits>
        uint64_t value = (((u64 & DOUBLE_SIGNIFICAND_MASK) | DOUBLE_HIDDEN_BIT)
                          << (static_cast<uint32_t>(exp) - DOUBLE_SIGNIFICAND_SIZE + INT64_BITS - bits)) >>
                         (INT64_BITS - bits);
        ret = static_cast<int32_t>(value);
        if ((u64 & DOUBLE_SIGN_MASK) == DOUBLE_SIGN_MASK && ret != INT32_MIN) {
            ret = -ret;
        }
    } else {
        // No significand bits after mod 2^<bits>, contains NaN and INF
        ret = 0;
    }
    return ret;
}

int32_t NumberHelper::DoubleInRangeInt32(double d)
{
    if (d > INT_MAX) {
        return INT_MAX;
    }
    if (d < INT_MIN) {
        return INT_MIN;
    }
    return base::NumberHelper::DoubleToInt(d, base::INT32_BITS);
}

JSTaggedValue NumberHelper::StringToBigInt(JSThread *thread, JSHandle<JSTaggedValue> strVal)
{
    auto strObj = static_cast<EcmaString *>(strVal->GetTaggedObject());
    uint32_t strLen = EcmaStringAccessor(strObj).GetLength();
    if (strLen == 0) {
        return BigInt::Int32ToBigInt(thread, 0).GetTaggedValue();
    }
    CVector<uint8_t> buf;
    Span<const uint8_t> str = EcmaStringAccessor(strObj).ToUtf8Span(buf);

    auto p = const_cast<uint8_t *>(str.begin());
    auto end = str.end();
    // 1. skip space and line terminal
    if (!NumberHelper::GotoNonspace(&p, end)) {
        return BigInt::Int32ToBigInt(thread, 0).GetTaggedValue();
    }
    // 2. get bigint sign
    Sign sign = Sign::NONE;
    if (*p == '+') {
        RETURN_IF_CONVERSION_END(++p, end, JSTaggedValue(NAN_VALUE));
        sign = Sign::POS;
    } else if (*p == '-') {
        RETURN_IF_CONVERSION_END(++p, end, JSTaggedValue(NAN_VALUE));
        sign = Sign::NEG;
    }
    // 3. bigint not allow Infinity, decimal points, or exponents.
    if (isalpha(*p)) {
        return JSTaggedValue(NAN_VALUE);
    }
    // 4. get bigint radix
    uint8_t radix = DECIMAL;
    if (*p == '0') {
        if (++p == end) {
            return BigInt::Int32ToBigInt(thread, 0).GetTaggedValue();
        }
        if (*p == 'x' || *p == 'X') {
            RETURN_IF_CONVERSION_END(++p, end, JSTaggedValue(NAN_VALUE));
            if (sign != Sign::NONE) {
                return JSTaggedValue(NAN_VALUE);
            }
            radix = HEXADECIMAL;
        } else if (*p == 'o' || *p == 'O') {
            RETURN_IF_CONVERSION_END(++p, end, JSTaggedValue(NAN_VALUE));
            if (sign != Sign::NONE) {
                return JSTaggedValue(NAN_VALUE);
            }
            radix = OCTAL;
        } else if (*p == 'b' || *p == 'B') {
            RETURN_IF_CONVERSION_END(++p, end, JSTaggedValue(NAN_VALUE));
            if (sign != Sign::NONE) {
                return JSTaggedValue(NAN_VALUE);
            }
            radix = BINARY;
        }
    }

    // 5. skip leading '0'
    while (*p == '0') {
        if (++p == end) {
            return BigInt::Int32ToBigInt(thread, 0).GetTaggedValue();
        }
    }
    // 6. parse to bigint
    CString buffer;
    do {
        uint8_t c = ToDigit(*p);
        if (c < radix) {
            buffer += *p;
        } else if (NumberHelper::GotoNonspace(&p, end)) {
            // illegal character
            return JSTaggedValue(NAN_VALUE);
        }
        // tail of string is space
    } while (++p < end);
    if (buffer.size() == 0) {
        return BigInt::Uint32ToBigInt(thread, 0).GetTaggedValue();
    }
    if (sign == Sign::NEG) {
        return BigIntHelper::SetBigInt(thread, "-" + buffer, radix).GetTaggedValue();
    }
    return BigIntHelper::SetBigInt(thread, buffer, radix).GetTaggedValue();
}

void NumberHelper::GetBase(double d, int digits, int *decimalPoint, char *buf, char *bufTmp, int size)
{
    int result = snprintf_s(bufTmp, size, size - 1, "%+.*e", digits - 1, d);
    if (result == -1) {
        LOG_FULL(FATAL) << "snprintf_s failed";
        UNREACHABLE();
    }
    // mantissa
    buf[0] = bufTmp[1];
    if (digits > 1) {
        if (memcpy_s(buf + 1, digits, bufTmp + 2, digits) != EOK) { // 2 means add the point char to buf
            LOG_FULL(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    }
    buf[digits + 1] = '\0';
    // exponent
    *decimalPoint = atoi(bufTmp + digits + 2 + (digits > 1)) + 1; // 2 means ignore the integer and point
}

int NumberHelper::GetMinmumDigits(double d, int *decimalPoint, char *buf)
{
    int digits = 0;
    char bufTmp[JS_DTOA_BUF_SIZE] = {0};

    // find the minimum amount of digits
    int MinDigits = 1;
    int MaxDigits = DOUBLE_MAX_PRECISION;
    while (MinDigits < MaxDigits) {
        digits = (MinDigits + MaxDigits) / 2; // 2 :  Divide by 2
        GetBase(d, digits, decimalPoint, buf, bufTmp, sizeof(bufTmp));
        if (strtod(bufTmp, NULL) == d) {
            // no need to keep the trailing zeros
            while (digits >= 2 && buf[digits] == '0') { // 2 means ignore the integer and point
                digits--;
            }
            MaxDigits = digits;
        } else {
            MinDigits = digits + 1;
        }
    }
    digits = MaxDigits;
    GetBase(d, digits, decimalPoint, buf, bufTmp, sizeof(bufTmp));

    return digits;
}

bool NumberHelper::StringToInt64(const std::string& str, int64_t& value)
{
    if (str.empty()) {
        return false;
    }

    char *end;
    errno = 0;
    value = std::strtoll(str.c_str(), &end, 0); // Automatic check of the number system

    // If no number is converted
    if (end == str.c_str()) {
        return false;
    }
    // If there is a range error (too large or to small)
    if (errno == ERANGE && (value == LLONG_MAX || value == LLONG_MIN)) {
        return false;
    }
    // If the character string contains non-digit chaaracters
    if (*end != '\0') {
        return false;
    }

    return true;
}

uint64_t RandomGenerator::XorShift64(uint64_t *pVal)
{
    uint64_t x = *pVal;
    x ^= x >> RIGHT12;
    x ^= x << LEFT25;
    x ^= x >> RIGHT27;
    *pVal = x;
    return x * GET_MULTIPLY;
}

void RandomGenerator::InitRandom(JSThread *thread)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    randomState_ = static_cast<uint64_t>((tv.tv_sec * SECONDS_TO_SUBTLE) + tv.tv_usec);
    // the state must be non zero
    if (randomState_ == 0) {
        randomState_ = 1;
    }
    thread->SetRandomStatePtr(&randomState_);
}

double RandomGenerator::NextDouble()
{
    uint64_t val = XorShift64(&randomState_);
    return ToDouble(val);
}

double RandomGenerator::ToDouble(uint64_t state)
{
    uint64_t random = (state >> base::RIGHT12) | EXPONENTBITS_RANGE_IN_ONE_AND_TWO;
    return base::bit_cast<double>(random) - 1;
}

int32_t RandomGenerator::Next(int bits)
{
    uint64_t val = XorShift64(&randomState_);
    return static_cast<int32_t>(val >> (INT64_BITS - bits));
}

int32_t RandomGenerator::GenerateIdentityHash()
{
    return RandomGenerator::Next(INT32_BITS) & INT32_MAX;
}
}  // namespace panda::ecmascript::base
