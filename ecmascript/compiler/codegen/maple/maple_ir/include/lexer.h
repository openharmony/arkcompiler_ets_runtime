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

#ifndef MAPLE_IR_INCLUDE_LEXER_H
#define MAPLE_IR_INCLUDE_LEXER_H
#include "cstdio"
#include <fstream>
#include "types_def.h"
#include "tokens.h"
#include "mempool_allocator.h"
#include "mir_module.h"

namespace maple {
class MIRParser;  // circular dependency exists, no other choice
class MIRLexer {
    friend MIRParser;

public:
    explicit MIRLexer(MIRModule &mod);
    ~MIRLexer()
    {
        airFile = nullptr;
        if (airFileInternal.is_open()) {
            airFileInternal.close();
        }
    }

    void PrepareForFile(const std::string &filename);
    void PrepareForString(const std::string &src);
    TokenKind NextToken();
    TokenKind GetTokenKind() const
    {
        return kind;
    }

    uint32 GetLineNum() const
    {
        return lineNum;
    }

    uint32 GetCurIdx() const
    {
        return curIdx;
    }

    // get the identifier name after the % or $ prefix
    const std::string &GetName() const
    {
        return name;
    }

    uint64 GetTheIntVal() const
    {
        return theIntVal;
    }

    float GetTheFloatVal() const
    {
        return theFloatVal;
    }

    double GetTheDoubleVal() const
    {
        return theDoubleVal;
    }

private:
    MIRModule &module;
    // for storing the different types of constant values
    uint64 theIntVal = 0;  // also indicates preg number under TK_preg
    float theFloatVal = 0.0;
    double theDoubleVal = 0.0;
    MapleVector<std::string> seenComments;
    std::ifstream *airFile = nullptr;
    std::ifstream airFileInternal;
    std::string line;
    size_t lineBufSize = 0;  // the allocated size of line(buffer).
    uint32 currentLineSize = 0;
    uint32 curIdx = 0;
    uint32 lineNum = 0;
    TokenKind kind = TK_invalid;
    std::string name = "";  // store the name token without the % or $ prefix
    MapleUnorderedMap<std::string, TokenKind> keywordMap;
    std::queue<std::string> mirQueue;
    bool needFile = true;
    void RemoveReturnInline(std::string &line)
    {
        if (line.empty()) {
            return;
        }
        if (line.back() == '\n') {
            line.pop_back();
        }
        if (line.back() == '\r') {
            line.pop_back();
        }
    }

    int ReadALine();            // read a line from MIR (text) file.
    int ReadALineByMirQueue();  // read a line from MIR Queue.
    void GenName();
    TokenKind GetSpecialFloatConst();
    TokenKind GetHexConst(uint32 valStart, bool negative);
    TokenKind GetIntConst(uint32 valStart, bool negative);
    TokenKind GetSpecialTokenUsingOneCharacter(char c);
    TokenKind GetTokenWithPrefixDollar();
    TokenKind GetTokenWithPrefixPercent();
    TokenKind GetTokenWithPrefixAmpersand();
    TokenKind GetTokenWithPrefixAtOrCircumflex(char prefix);
    TokenKind GetTokenWithPrefixExclamation();
    TokenKind GetTokenWithPrefixQuotation();
    TokenKind GetTokenSpecial();

    char GetCharAt(uint32 idx) const
    {
        return line[idx];
    }

    char GetCharAtWithUpperCheck(uint32 idx) const
    {
        return idx < currentLineSize ? line[idx] : 0;
    }

    char GetCharAtWithLowerCheck(uint32 idx) const
    {
        return idx >= 0 ? line[idx] : 0;
    }

    char GetCurrentCharWithUpperCheck()
    {
        return curIdx < currentLineSize ? line[curIdx] : 0;
    }

    char GetNextCurrentCharWithUpperCheck()
    {
        ++curIdx;
        return curIdx < currentLineSize ? line[curIdx] : 0;
    }

    void SetFile(std::ifstream &file)
    {
        airFile = &file;
    }

    std::ifstream *GetFile() const
    {
        return airFile;
    }

    void SetMirQueue(const std::string &fileText)
    {
        StringUtils::Split(fileText, mirQueue, '\n');
        needFile = false;
    }
};

inline bool IsPrimitiveType(TokenKind tk)
{
    return (tk >= TK_void) && (tk < TK_unknown);
}

inline bool IsVarName(TokenKind tk)
{
    return (tk == TK_lname) || (tk == TK_gname);
}

inline bool IsExprBinary(TokenKind tk)
{
    return (tk >= TK_add) && (tk <= TK_sub);
}

inline bool IsConstValue(TokenKind tk)
{
    return (tk >= TK_intconst) && (tk <= TK_doubleconst);
}

inline bool IsConstAddrExpr(TokenKind tk)
{
    return (tk == TK_addrof) || (tk == TK_addroffunc) || (tk == TK_addroflabel) || (tk == TK_conststr) ||
           (tk == TK_conststr16);
}
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_LEXER_H
