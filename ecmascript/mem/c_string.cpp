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

#include "ecmascript/mem/c_string.h"

#include "ecmascript/base/dtoa_helper.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/base/json_helper.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/js_symbol.h"

namespace panda::ecmascript {
long CStringToL(const CString &str)
{
    char *endPtr = nullptr;
    int64_t result = std::strtol(str.c_str(), &endPtr, DEC_BASE);
    ASSERT(!(result == 0 && str.c_str() == endPtr) && "CString argument is not long int");
    return result;
}

int64_t CStringToLL(const CString &str)
{
    char *endPtr = nullptr;
    int64_t result = std::strtoll(str.c_str(), &endPtr, DEC_BASE);
    ASSERT(!(result == 0 && str.c_str() == endPtr) && "CString argument is not long long int");
    return result;
}

uint64_t CStringToULL(const CString &str)
{
    char *endPtr = nullptr;
    uint64_t result = std::strtoull(str.c_str(), &endPtr, DEC_BASE);
    ASSERT(!(result == 0 && str.c_str() == endPtr) && "CString argument is not unsigned long long int");
    return result;
}

float CStringToF(const CString &str)
{
    char *endPtr = nullptr;
    float result = std::strtof(str.c_str(), &endPtr);
    ASSERT(result != HUGE_VALF && "CString argument is not float");
    ASSERT(!(result == 0 && str.c_str() == endPtr) && "CString argument is not float");
    return result;
}

double CStringToD(const CString &str)
{
    char *endPtr = nullptr;
    double result = std::strtod(str.c_str(), &endPtr);
    ASSERT(result != HUGE_VALF && "CString argument is not double");
    ASSERT(!(result == 0 && str.c_str() == endPtr) && "CString argument is not double");
    return result;
}

template<class T>
CString ConvertToString(T sp)
{
    CString res;
    res.reserve(sp.size());

    // Also support ascii that great than 127, so using unsigned char here
    constexpr size_t maxChar = std::numeric_limits<unsigned char>::max();

    for (const auto &c : sp) {
        if (c > maxChar) {
            return "";
        }
        res.push_back(c);
    }

    return res;
}

CString ConvertToString(JSTaggedValue key)
{
    ASSERT(key.IsStringOrSymbol());
    if (key.IsString()) {
        return ConvertToString(EcmaString::ConstCast(key.GetTaggedObject()));
    }

    ecmascript::JSTaggedValue desc = JSSymbol::Cast(key.GetTaggedObject())->GetDescription();
    if (desc.IsUndefined()) {
        return CString("Symbol()");
    }

    return ConvertToString(EcmaString::ConstCast(desc.GetTaggedObject()));
}

// NB! the following function need additional mem allocation, don't use when unnecessary!
CString ConvertToString(const std::string &str)
{
    CString res;
    res.reserve(str.size());
    for (auto c : str) {
        res.push_back(c);
    }
    return res;
}

CString ConvertToString(const EcmaString *s, StringConvertedUsage usage, bool cesu8)
{
    if (s == nullptr) {
        return CString("");
    }
    return EcmaStringAccessor(const_cast<EcmaString *>(s)).ToCString(usage, cesu8);
}

std::string ConvertToStdString(const CString &str)
{
    std::string res;
    res.reserve(str.size());
    for (auto c : str) {
        res.push_back(c);
    }
    return res;
}

#if ENABLE_NEXT_OPTIMIZATION
template <typename DstType>
void AppendIntToCString(DstType &str, int number)
{
    using CharT = typename DstType::value_type;
    int64_t n = static_cast<int64_t>(number);
    uint32_t preSize = str.size();
    bool isNeg = true;
    if (n >= 0) {
        n = -n;
        isNeg = false;
    }
    do {
        str.push_back(static_cast<CharT>('0' - (n % DEC_BASE)));
        n /= DEC_BASE;
    } while (n);
    if (isNeg) {
        str.push_back(static_cast<CharT>('-'));
    }
    std::reverse(str.begin() + preSize, str.end());
}

template void AppendIntToCString<CString>(CString &str, int number);
template void AppendIntToCString<C16String>(C16String &str, int number);

template <typename DstType>
bool AppendSpecialDouble(DstType &str, double d)
{
    if (d == 0.0) {
        AppendCString(str, base::NumberHelper::ZERO_STR);
        return true;
    }
    if (std::isnan(d)) {
        AppendCString(str, base::NumberHelper::NAN_STR);
        return true;
    }
    if (std::isinf(d)) {
        AppendCString(str, d < 0 ? base::NumberHelper::MINUS_INFINITY_STR : base::NumberHelper::INFINITY_STR);
        return true;
    }
    return false;
}
template bool AppendSpecialDouble<CString>(CString &str, double d);
template bool AppendSpecialDouble<C16String>(C16String &str, double d);

template <typename DstType>
void AppendDoubleToString(DstType &str, double d)
{
    if (AppendSpecialDouble(str, d)) {
        return;
    }
    bool isNeg = d < 0;
    if (isNeg) {
        d = -d;
    }
    ASSERT(d > 0);
    constexpr int startIdx = 8; // when (isNeg and n = -5), result moves left for 8times
    char buff[base::JS_DTOA_BUF_SIZE] = {0};
    char *result = buff + startIdx;
    ASSERT(startIdx < base::JS_DTOA_BUF_SIZE);
    int n = 0;
    int k = 0;
    int len = 0;
    base::DtoaHelper::Dtoa(d, result, &n, &k);
    if (n > 0 && n <= base::MAX_DIGITS) {
        if (k <= n) {
            std::fill(result + k, result + n, '0');
            len = n;
        } else {
            --result;
            std::copy(result + 1, result + 1 + n, result);
            result[n] = '.';
            len = k + 1;
        }
    } else if (base::MIN_DIGITS < n && n <= 0) {
        constexpr int prefixLen = 2;
        result -= (-n + prefixLen);
        ASSERT(result >= buff);
        ASSERT(result + prefixLen - n <= buff + base::JS_DTOA_BUF_SIZE);
        result[0] = '0';
        result[1] = '.';
        std::fill(result + prefixLen, result + prefixLen - n, '0');
        len = -n + prefixLen + k;
    } else {
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
        for (size_t i = 0; i < expo.length(); ++i) {
            result[pos++] = expo[i];
        }
        len = pos;
    }
    if (isNeg) {
        --result;
        result[0] = '-';
        len += 1;
    }
    AppendString(str, result, len);
}
template void AppendDoubleToString<CString>(CString &str, double d);
template void AppendDoubleToString<C16String>(C16String &str, double d);

template <typename DstType>
void ConvertNumberToCStringAndAppend(DstType &str, JSTaggedValue num)
{
    ASSERT(num.IsNumber());
    if (num.IsInt()) {
        int intVal = num.GetInt();
        AppendIntToCString(str, intVal);
    } else {
        double d = num.GetDouble();
        AppendDoubleToString(str, d);
    }
}
template void ConvertNumberToCStringAndAppend<CString>(CString &str, JSTaggedValue num);
template void ConvertNumberToCStringAndAppend<C16String>(C16String &str, JSTaggedValue num);

void ConvertQuotedAndAppendToCString(CString &str, const EcmaString *s)
{
    ASSERT(s != nullptr);

    uint32_t strLen = EcmaStringAccessor(const_cast<EcmaString *>(s)).GetLength();
    CVector<uint8_t> buf;
    const uint8_t *data = EcmaStringAccessor::GetUtf8DataFlat(s, buf);
    const Span<const uint8_t> dataSpan(data, strLen);
    base::JsonHelper::AppendValueToQuotedString(dataSpan, str);
}

void ConvertQuotedAndAppendToC16String(C16String &str, const EcmaString *s)
{
    ASSERT(s != nullptr);

    uint32_t strLen = EcmaStringAccessor(const_cast<EcmaString *>(s)).GetLength();
    if (EcmaStringAccessor(const_cast<EcmaString *>(s)).IsUtf8()) {
        CVector<uint8_t> buf;
        const uint8_t *data = EcmaStringAccessor::GetUtf8DataFlat(s, buf);
        const Span<const uint8_t> dataSpan(data, strLen);
        base::JsonHelper::AppendValueToQuotedString(dataSpan, str);
    } else {
        CVector<uint16_t> buf;
        const uint16_t *data = EcmaStringAccessor::GetUtf16DataFlat(s, buf);
        const Span<const uint16_t> dataSpan(data, strLen);
        base::JsonHelper::AppendValueToQuotedString(dataSpan, str);
    }
}
#endif

}  // namespace panda::ecmascript
