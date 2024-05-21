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

#include "lexer.h"
#include <cmath>
#include <climits>
#include <cstdlib>
#include "mpl_logging.h"
#include "debug_info.h"
#include "mir_module.h"
#include "securec.h"
#include "utils.h"

namespace maple {
int32 HexCharToDigit(char c)
{
    int32 ret = utils::ToDigit<16, int32>(c);
    return (ret != INT32_MAX ? ret : 0);
}

// Read (next) line from the MIR (text) file, and return the read
// number of chars.
// if the line is empty (nothing but a newline), returns 0.
// if EOF, return -1.
// The trailing new-line character has been removed.
int MIRLexer::ReadALine()
{
    if (airFile == nullptr) {
        line = "";
        return -1;
    }

    curIdx = 0;
    if (!std::getline(*airFile, line)) {  // EOF
        line = "";
        airFile = nullptr;
        currentLineSize = 0;
        return -1;
    }

    RemoveReturnInline(line);
    currentLineSize = line.length();
    return currentLineSize;
}

int MIRLexer::ReadALineByMirQueue()
{
    if (mirQueue.empty()) {
        line = "";
        return -1;
    }
    curIdx = 0;
    line = mirQueue.front();
    RemoveReturnInline(line);
    currentLineSize = line.length();
    mirQueue.pop();
    return currentLineSize;
}

MIRLexer::MIRLexer(MIRModule &mod)
    : module(mod), seenComments(mod.GetMPAllocator().Adapter()), keywordMap(mod.GetMPAllocator().Adapter())
{
    // initialize keywordMap
    keywordMap.clear();
#define KEYWORD(STR)                \
    {                               \
        std::string str;            \
        str = #STR;                 \
        keywordMap[str] = TK_##STR; \
    }
#include "keywords.def"
#undef KEYWORD
}

void MIRLexer::PrepareForFile(const std::string &filename)
{
    // open MIR file
    airFileInternal.open(filename);
    CHECK_FATAL(airFileInternal.is_open(), "cannot open MIR file %s\n", &filename);

    airFile = &airFileInternal;
    // try to read the first line
    if (ReadALine() < 0) {
        lineNum = 0;
    } else {
        lineNum = 1;
    }
    module.GetDbgInfo()->UpdateMsg(lineNum, line.c_str());
    kind = TK_invalid;
}

void MIRLexer::PrepareForString(const std::string &src)
{
    SetMirQueue(src);
    if (ReadALineByMirQueue() < 0) {
        lineNum = 0;
    } else {
        lineNum = 1;
    }
    module.GetDbgInfo()->UpdateMsg(lineNum, line.c_str());
    kind = TK_invalid;
}

void MIRLexer::GenName()
{
    uint32 startIdx = curIdx;
    char c = GetNextCurrentCharWithUpperCheck();
    CHECK_FATAL(curIdx > 0, "must not be zero");
    char cp = GetCharAt(curIdx - 1);
    if (c == '@' && (cp == 'h' || cp == 'f')) {
        // special pattern for exception handling labels: catch or finally
        c = GetNextCurrentCharWithUpperCheck();
    }
    while (utils::IsAlnum(c) || c < 0 || c == '_' || c == '$' || c == ';' || c == '/' || c == '|' || c == '.' ||
           c == '?' || c == '@') {
        c = GetNextCurrentCharWithUpperCheck();
    }
    name = line.substr(startIdx, curIdx - startIdx);
}

TokenKind MIRLexer::GetSpecialFloatConst()
{
    constexpr uint32 lenSpecFloat = 4;
    constexpr uint32 lenSpecDouble = 3;
    if (line.compare(curIdx, lenSpecFloat, "inff") == 0 &&
        !utils::IsAlnum(GetCharAtWithUpperCheck(curIdx + lenSpecFloat))) {
        curIdx += lenSpecFloat;
        theFloatVal = -INFINITY;
        return TK_floatconst;
    }
    if (line.compare(curIdx, lenSpecDouble, "inf") == 0 &&
        !utils::IsAlnum(GetCharAtWithUpperCheck(curIdx + lenSpecDouble))) {
        curIdx += lenSpecDouble;
        theDoubleVal = -INFINITY;
        return TK_doubleconst;
    }
    if (line.compare(curIdx, lenSpecFloat, "nanf") == 0 &&
        !utils::IsAlnum(GetCharAtWithUpperCheck(curIdx + lenSpecFloat))) {
        curIdx += lenSpecFloat;
        theFloatVal = -NAN;
        return TK_floatconst;
    }
    if (line.compare(curIdx, lenSpecDouble, "nan") == 0 &&
        !utils::IsAlnum(GetCharAtWithUpperCheck(curIdx + lenSpecDouble))) {
        curIdx += lenSpecDouble;
        theDoubleVal = -NAN;
        return TK_doubleconst;
    }
    return TK_invalid;
}

TokenKind MIRLexer::GetHexConst(uint32 valStart, bool negative)
{
    char c = GetCharAtWithUpperCheck(curIdx);
    if (!isxdigit(c)) {
        name = line.substr(valStart, curIdx - valStart);
        return TK_invalid;
    }
    uint64 tmp = static_cast<uint32>(HexCharToDigit(c));
    c = GetNextCurrentCharWithUpperCheck();
    while (isxdigit(c)) {
        tmp = (tmp << k16BitShift) + static_cast<uint32>(HexCharToDigit(c));
        c = GetNextCurrentCharWithUpperCheck();
    }
    theIntVal = static_cast<uint64>(static_cast<uint64>(tmp));
    if (negative) {
        theIntVal = -theIntVal;
    }
    theFloatVal = static_cast<float>(theIntVal);
    theDoubleVal = static_cast<double>(theIntVal);
    if (negative && theIntVal == 0) {
        theFloatVal = -theFloatVal;
        theDoubleVal = -theDoubleVal;
    }
    name = line.substr(valStart, curIdx - valStart);
    return TK_intconst;
}

TokenKind MIRLexer::GetIntConst(uint32 valStart, bool negative)
{
    auto negOrSelf = [negative](uint64 val) { return negative ? ~val + 1 : val; };

    theIntVal = static_cast<uint64>(HexCharToDigit(GetCharAtWithUpperCheck(curIdx)));

    uint64 radix = theIntVal == 0 ? 8 : 10;

    char c = GetNextCurrentCharWithUpperCheck();

    for (theIntVal = negOrSelf(theIntVal); isdigit(c); c = GetNextCurrentCharWithUpperCheck()) {
        theIntVal = (theIntVal * radix) + negOrSelf(HexCharToDigit(c));
    }

    if (c == 'u' || c == 'U') {  // skip 'u' or 'U'
        c = GetNextCurrentCharWithUpperCheck();
        if (c == 'l' || c == 'L') {
            c = GetNextCurrentCharWithUpperCheck();
        }
    }

    if (c == 'l' || c == 'L') {
        c = GetNextCurrentCharWithUpperCheck();
        if (c == 'l' || c == 'L' || c == 'u' || c == 'U') {
            ++curIdx;
        }
    }

    name = line.substr(valStart, curIdx - valStart);

    if (negative) {
        theFloatVal = static_cast<float>(static_cast<int64>(theIntVal));
        theDoubleVal = static_cast<double>(static_cast<int64>(theIntVal));

        if (theIntVal == 0) {
            theFloatVal = -theFloatVal;
            theDoubleVal = -theDoubleVal;
        }
    } else {
        theFloatVal = static_cast<float>(theIntVal);
        theDoubleVal = static_cast<double>(theIntVal);
    }

    return TK_intconst;
}

TokenKind MIRLexer::GetTokenWithPrefixDollar()
{
    // token with prefix '$'
    char c = GetCharAtWithUpperCheck(curIdx);
    if (utils::IsAlpha(c) || c == '_' || c == '$') {
        GenName();
        return TK_gname;
    } else {
        // for error reporting.
        const uint32 printLength = 2;
        name = line.substr(curIdx - 1, printLength);
        return TK_invalid;
    }
}

TokenKind MIRLexer::GetTokenWithPrefixPercent()
{
    // token with prefix '%'
    char c = GetCharAtWithUpperCheck(curIdx);
    if (isdigit(c)) {
        int valStart = static_cast<int>(curIdx) - 1;
        theIntVal = static_cast<uint64>(HexCharToDigit(c));
        c = GetNextCurrentCharWithUpperCheck();
        while (isdigit(c)) {
            theIntVal = (theIntVal * 10) + static_cast<uint64>(HexCharToDigit(c)); // 10 for decimal
            DEBUG_ASSERT(theIntVal >= 0, "int value overflow");
            c = GetNextCurrentCharWithUpperCheck();
        }
        name = line.substr(valStart, curIdx - valStart);
        return TK_preg;
    }
    if (utils::IsAlpha(c) || c == '_' || c == '$') {
        GenName();
        return TK_lname;
    }
    if (c == '%' && utils::IsAlpha(GetCharAtWithUpperCheck(curIdx + 1))) {
        ++curIdx;
        GenName();
        return TK_specialreg;
    }
    return TK_invalid;
}

TokenKind MIRLexer::GetTokenWithPrefixAmpersand()
{
    // token with prefix '&'
    char c = GetCurrentCharWithUpperCheck();
    if (utils::IsAlpha(c) || c == '_') {
        GenName();
        return TK_fname;
    }
    // for error reporting.
    constexpr uint32 printLength = 2;
    CHECK_FATAL(curIdx > 0, "must not be zero");
    name = line.substr(curIdx - 1, printLength);
    return TK_invalid;
}

TokenKind MIRLexer::GetTokenWithPrefixAtOrCircumflex(char prefix)
{
    // token with prefix '@' or `^`
    char c = GetCurrentCharWithUpperCheck();
    if (utils::IsAlnum(c) || c < 0 || c == '_' || c == '@' || c == '$' || c == '|') {
        GenName();
        if (prefix == '@') {
            return TK_label;
        }
        return TK_prntfield;
    }
    return TK_invalid;
}

TokenKind MIRLexer::GetTokenWithPrefixExclamation()
{
    // token with prefix '!'
    char c = GetCurrentCharWithUpperCheck();
    if (utils::IsAlpha(c)) {
        GenName();
        return TK_typeparam;
    }
    // for error reporting.
    const uint32 printLength = 2;
    CHECK_FATAL(curIdx > 0, "must not be zero");
    name = line.substr(curIdx - 1, printLength);
    return TK_invalid;
}

TokenKind MIRLexer::GetTokenWithPrefixQuotation()
{
    if (GetCharAtWithUpperCheck(curIdx + 1) == '\'') {
        theIntVal = GetCharAtWithUpperCheck(curIdx);
        constexpr uint32 hexLength = 2;
        curIdx += hexLength;
        return TK_intconst;
    }
    return TK_invalid;
}

TokenKind MIRLexer::GetTokenSpecial()
{
    --curIdx;
    char c = GetCharAtWithLowerCheck(curIdx);
    if (utils::IsAlpha(c) || c < 0 || c == '_') {
        GenName();
        TokenKind tk = keywordMap[name];
        switch (tk) {
            case TK_nanf:
                theFloatVal = NAN;
                return TK_floatconst;
            case TK_nan:
                theDoubleVal = NAN;
                return TK_doubleconst;
            case TK_inff:
                theFloatVal = INFINITY;
                return TK_floatconst;
            case TK_inf:
                theDoubleVal = INFINITY;
                return TK_doubleconst;
            default:
                return tk;
        }
    }
    MIR_ERROR("error in input file\n");
    return TK_eof;
}

TokenKind MIRLexer::NextToken()
{
    return kind;
}
}  // namespace maple
