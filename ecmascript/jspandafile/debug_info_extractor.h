/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_JSPANDAFILE_DEBUG_INFO_EXTRACTOR_H
#define ECMASCRIPT_JSPANDAFILE_DEBUG_INFO_EXTRACTOR_H

#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/common.h"

#include "libpandafile/file.h"

namespace panda::ecmascript {
class JSPandaFile;

struct LineTableEntry {
    uint32_t offset;
    int32_t line;

    bool operator<(const LineTableEntry &other) const
    {
        return offset < other.offset;
    }
};

struct ColumnTableEntry {
    uint32_t offset;
    int32_t column;

    bool operator<(const ColumnTableEntry &other) const
    {
        return offset < other.offset;
    }
};

using LineNumberTable = CVector<LineTableEntry>;
using ColumnNumberTable = CVector<ColumnTableEntry>;

/*
 * LocalVariableInfo define in frontend, now only use name and regNumber:
 *   std::string name
 *   std::string type
 *   std::string typeSignature
 *   int32_t regNumber
 *   uint32_t startOffset
 *   uint32_t endOffset
 */
using LocalVariableTable = CUnorderedMap<std::string, int32_t>;  // name, regNumber

// public for debugger
class PUBLIC_API DebugInfoExtractor {
public:
    explicit DebugInfoExtractor(const JSPandaFile *jsPandaFile);

    ~DebugInfoExtractor() = default;

    DEFAULT_COPY_SEMANTIC(DebugInfoExtractor);
    DEFAULT_MOVE_SEMANTIC(DebugInfoExtractor);

    const LineNumberTable &GetLineNumberTable(panda_file::File::EntityId methodId) const;

    const ColumnNumberTable &GetColumnNumberTable(panda_file::File::EntityId methodId) const;

    const LocalVariableTable &GetLocalVariableTable(panda_file::File::EntityId methodId) const;

    const std::string &GetSourceFile(panda_file::File::EntityId methodId) const;

    const std::string &GetSourceCode(panda_file::File::EntityId methodId) const;

    CVector<panda_file::File::EntityId> GetMethodIdList() const;

    template<class Callback>
    bool MatchWithLocation(const Callback &cb, int32_t line, int32_t column, const std::string &url)
    {
        if (line == SPECIAL_LINE_MARK) {
            return false;
        }

        auto methods = GetMethodIdList();
        for (const auto &method : methods) {
            // the url for testcases is empty
            if (!url.empty() && url != GetSourceFile(method)) {
                continue;
            }
            const LineNumberTable &lineTable = GetLineNumberTable(method);
            const ColumnNumberTable &columnTable = GetColumnNumberTable(method);
            for (uint32_t i = 0; i < lineTable.size(); i++) {
                if (lineTable[i].line != line) {
                    continue;
                }
                uint32_t currentOffset = lineTable[i].offset;
                uint32_t nextOffset = ((i == lineTable.size() - 1) ? UINT32_MAX : lineTable[i + 1].offset);
                for (const auto &pair : columnTable) {
                    if (pair.column == column && pair.offset >= currentOffset && pair.offset < nextOffset) {
                        return cb(method, pair.offset);
                    }
                }
                return cb(method, currentOffset);
            }
        }
        return false;
    }

    template<class Callback>
    bool MatchLineWithOffset(const Callback &cb, panda_file::File::EntityId methodId, uint32_t offset)
    {
        int32_t line = 0;
        const LineNumberTable &lineTable = GetLineNumberTable(methodId);
        auto iter = std::upper_bound(lineTable.begin(), lineTable.end(), LineTableEntry {offset, 0});
        if (iter != lineTable.begin()) {
            line = (iter - 1)->line;
        }
        return cb(line);
    }

    template<class Callback>
    bool MatchColumnWithOffset(const Callback &cb, panda_file::File::EntityId methodId, uint32_t offset)
    {
        int32_t column = 0;
        const ColumnNumberTable &columnTable = GetColumnNumberTable(methodId);
        auto iter = std::upper_bound(columnTable.begin(), columnTable.end(), ColumnTableEntry {offset, 0});
        if (iter != columnTable.begin()) {
            column = (iter - 1)->column;
        }
        return cb(column);
    }

    constexpr static int32_t SPECIAL_LINE_MARK = -1;

private:
    void Extract(const panda_file::File *pf);

    struct MethodDebugInfo {
        std::string sourceFile;
        std::string sourceCode;
        LineNumberTable lineNumberTable;
        ColumnNumberTable columnNumberTable;
        LocalVariableTable localVariableTable;
    };

    CUnorderedMap<uint32_t, MethodDebugInfo> methods_;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSPANDAFILE_DEBUG_INFO_EXTRACTOR_H
