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

bool AppendSpecialDouble(CString &str, double d)
{
    if (d == 0.0) {
        str.append(base::NumberHelper::ZERO_STR);
        return true;
    }
    if (std::isnan(d)) {
        str.append(base::NumberHelper::NAN_STR);
        return true;
    }
    if (std::isinf(d)) {
        str.append(d < 0 ? base::NumberHelper::MINUS_INFINITY_STR : base::NumberHelper::INFINITY_STR);
        return true;
    }
    return false;
}

void AppendDoubleToString(CString &str, double d)
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
    str.append(result, len);
}

void ConvertToCStringAndAppend(CString &str, JSTaggedValue num)
{
    ASSERT(num.IsNumber());
    if (num.IsInt()) {
        int intVal = num.GetInt();
        AppendToCString<int>(str, intVal);
    } else {
        double d = num.GetDouble();
        AppendDoubleToString(str, d);
    }
}

void ConvertQuotedAndAppendToString(CString &str, const EcmaString *s, StringConvertedUsage usage, bool cesu8)
{
    if (s == nullptr) {
        str += "\"\"";
        return;
    }
    EcmaStringAccessor(const_cast<EcmaString *>(s)).AppendQuotedStringToCString(str, usage, cesu8);
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

std::string ConvertToStdString(const CString &str)
{
    std::string res;
    res.reserve(str.size());
    for (auto c : str) {
        res.push_back(c);
    }
    return res;
}
}  // namespace panda::ecmascript
