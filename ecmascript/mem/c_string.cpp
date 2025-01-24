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

void ConvertAndAppendToString(CString &str, const EcmaString *s, StringConvertedUsage usage, bool cesu8)
{
    if (s == nullptr) {
        return;
    }
    EcmaStringAccessor(const_cast<EcmaString *>(s)).AppendToCString(str, usage, cesu8);
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
