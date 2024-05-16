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

#ifndef MAPLE_IR_INCLUDE_SRC_POSITION_H
#define MAPLE_IR_INCLUDE_SRC_POSITION_H
#include "mpl_logging.h"

namespace maple {
// to store source position information
class SrcPosition {
public:
    SrcPosition() : lineNum(0), mplLineNum(0)
    {
        u.word0 = 0;
    }

    virtual ~SrcPosition() = default;

    uint32 RawData() const
    {
        return u.word0;
    }

    uint32 FileNum() const
    {
        return u.fileColumn.fileNum;
    }

    uint32 Column() const
    {
        return u.fileColumn.column;
    }

    uint32 LineNum() const
    {
        return lineNum;
    }

    uint32 MplLineNum() const
    {
        return mplLineNum;
    }

    void SetFileNum(uint16 n)
    {
        u.fileColumn.fileNum = n;
    }

    void SetColumn(uint16 n)
    {
        u.fileColumn.column = n;
    }

    void SetLineNum(uint32 n)
    {
        lineNum = n;
    }

    void SetRawData(uint32 n)
    {
        u.word0 = n;
    }

    void SetMplLineNum(uint32 n)
    {
        mplLineNum = n;
    }

    void CondSetLineNum(uint32 n)
    {
        lineNum = lineNum ? lineNum : n;
    }

    void CondSetFileNum(uint16 n)
    {
        uint16 i = u.fileColumn.fileNum;
        u.fileColumn.fileNum = i ? i : n;
    }

    // as you read: this->IsBfOrEq(pos)
    bool IsBfOrEq(SrcPosition pos) const
    {
        return (pos.FileNum() == FileNum() &&
                ((LineNum() < pos.LineNum()) || ((LineNum() == pos.LineNum()) && (Column() <= pos.Column()))));
    }

    bool IsSrcPostionEq(SrcPosition pos) const
    {
        return FileNum() == pos.FileNum() && LineNum() == pos.LineNum() && Column() == pos.Column();
    }

    void DumpLoc(uint32 &lastLineNum, uint16 &lastColumnNum) const
    {
        if (FileNum() != 0 && LineNum() != 0) {
            if (Column() != 0 && (LineNum() != lastLineNum || Column() != lastColumnNum)) {
                DumpLocWithCol();
                lastLineNum = LineNum();
                lastColumnNum = Column();
            } else if (LineNum() != lastLineNum) {
                DumpLocWithLine();
                lastLineNum = LineNum();
            }
        }
    }

    void DumpLocWithLine() const
    {
        LogInfo::MapleLogger() << "LOC " << FileNum() << " " << LineNum() << '\n';
    }

    void DumpLocWithCol() const
    {
        LogInfo::MapleLogger() << "LOC " << FileNum() << " " << LineNum() << " " << Column() << '\n';
    }

    std::string DumpLocWithColToString() const
    {
        std::stringstream ss;
        ss << "LOC " << FileNum() << " " << LineNum() << " " << Column();
        return ss.str();
    }

private:
    union {
        struct {
            uint16 fileNum;
            uint16 column : 12;
            uint16 stmtBegin : 1;
            uint16 bbBegin : 1;
            uint16 unused : 2;
        } fileColumn;
        uint32 word0;
    } u;
    uint32 lineNum;
    uint32 mplLineNum;  // line number of mpl file
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_SRC_POSITION_H
