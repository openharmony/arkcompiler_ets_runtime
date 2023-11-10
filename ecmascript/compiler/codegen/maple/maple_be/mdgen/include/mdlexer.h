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

#ifndef MAPLEBE_MDGEN_INCLUDE_MDLEXER_H
#define MAPLEBE_MDGEN_INCLUDE_MDLEXER_H

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include "stdio.h"
#include "mdtokens.h"
#include "mpl_logging.h"
#include "types_def.h"

namespace MDGen {
using namespace maple;
class MDLexer {
public:
    MDLexer()
    {
        keywords.clear();
        /* input can be improved */
        (void)keywords.insert(std::make_pair("Def", kMDDef));
        (void)keywords.insert(std::make_pair("Class", kMDClass));
        (void)keywords.insert(std::make_pair("DefType", kMDDefType));
    };
    ~MDLexer()
    {
        if (mdFileInternal.is_open()) {
            mdFileInternal.close();
        }
    };

    MDTokenKind ReturnError() const;
    MDTokenKind NextToken();
    MDTokenKind LexToken();
    MDTokenKind GetTokenIdentifier();
    MDTokenKind GetTokenConstVal();
    int ReadOneLine();
    bool SkipCComment();
    void SkipALineComment();

    void PrepareFile(const std::string &mdfileName);
    const std::string &GetStrToken() const
    {
        return strToken;
    }
    int64_t GetIntVal() const
    {
        return intVal;
    }
    const std::string &GetStrLine() const
    {
        return strLine;
    }
    size_t GetStrLineSize() const
    {
        return strLine.size();
    }
    void RemoveInValidAtBack()
    {
        if (strLine.length() == 0) {
            return;
        }
        if (strLine.back() == '\n') {
            strLine.pop_back();
        }
        if (strLine.back() == '\r') {
            strLine.pop_back();
        }
    }
    MDTokenKind GetCurKind() const
    {
        return curKind;
    }
    char GetCurChar()
    {
        return curPos < GetStrLineSize() ? strLine[curPos] : 0;
    }
    char GetNextChar()
    {
        ++curPos;
        return curPos < GetStrLineSize() ? strLine[curPos] : 0;
    }
    char ViewNextChar() const
    {
        return curPos < GetStrLineSize() ? strLine[curPos] : 0;
    }
    char GetCharAt(uint32 pos)
    {
        if (pos >= GetStrLineSize()) {
            return 0;
        }
        return strLine[pos];
    }
    int GetLineNumber() const
    {
        return lineNumber;
    }

    MDTokenKind GetHexConst(uint32 startPos, bool isNegative);
    MDTokenKind GetIntConst(uint32 digitStartPos, bool isNegative);
    MDTokenKind GetFloatConst();

private:
    static constexpr int maxNumLength = 10;
    std::ifstream *mdFile = nullptr;
    std::ifstream mdFileInternal;
    uint32 lineNumber = 0;                                 /* current Processing Line */
    uint32 curPos = 0;                                     /* Position in a line */
    std::string strLine = "";                              /* current token line */
    std::string strToken = "";                             /* store ID,keywords ... */
    int32 intVal = 0;                                      /* store integer when token */
    float floatVal = 0;                                    /* store float value when token */
    MDTokenKind curKind = kMDInvalid;                      /* current token kind */
    std::unordered_map<std::string, MDTokenKind> keywords; /* store keywords defined for md files */
};
} /* namespace MDGen */

#endif /* MAPLEBE_MDGEN_INCLUDE_MDLEXER_H */
