/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DEBUGGER_JS_PT_LOCATION_H
#define ECMASCRIPT_DEBUGGER_JS_PT_LOCATION_H

#include <cstring>

#include "ecmascript/jspandafile/js_pandafile.h"
#include "libpandafile/file.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::tooling {
class JSPtLocation {
public:
    using EntityId = panda_file::File::EntityId;

    JSPtLocation(const JSPandaFile *jsPandaFile, EntityId methodId, uint32_t bytecodeOffset,
        const std::string &sourceFile = "") : jsPandaFile_(jsPandaFile), methodId_(methodId),
        bytecodeOffset_(bytecodeOffset), sourceFile_(sourceFile)
    {
    }

    JSPtLocation(const JSPandaFile *jsPandaFile, EntityId methodId, uint32_t bytecodeOffset,
        Global<FunctionRef> &condFuncRef, int32_t line = -1,
        int32_t column = -1, const std::string &sourceFile = "",
        bool needResetBreakpoint = false, CString recordName = "") : jsPandaFile_(jsPandaFile),
        methodId_(methodId), bytecodeOffset_(bytecodeOffset),
        condFuncRef_(condFuncRef), line_(line), column_(column),
        sourceFile_(sourceFile), needResetBreakpoint_(needResetBreakpoint), recordName_(recordName)
    {
    }

    JSPtLocation(const std::string &url, int32_t line, int32_t column) : line_(line), column_(column), sourceFile_(url)
    {
    }

    const JSPandaFile *GetJsPandaFile() const
    {
        return jsPandaFile_;
    }

    const std::string &GetSourceFile() const
    {
        return sourceFile_;
    }

    EntityId GetMethodId() const
    {
        return methodId_;
    }

    uint32_t GetBytecodeOffset() const
    {
        return bytecodeOffset_;
    }

    const Global<FunctionRef> &GetCondFuncRef() const
    {
        return condFuncRef_;
    }

    int32_t GetLine() const
    {
        return line_;
    }

    int32_t GetColumn() const
    {
        return column_;
    }

    const bool &GetNeedResetBreakpoint() const
    {
        return needResetBreakpoint_;
    }

    const CString &GetRecordName() const
    {
        return recordName_;
    }

    bool operator==(const JSPtLocation &location) const
    {
        return methodId_ == location.methodId_ && bytecodeOffset_ == location.bytecodeOffset_ &&
               jsPandaFile_ == location.jsPandaFile_;
    }

    std::string ToString() const
    {
        std::stringstream location;
        location << "[";
        location << "methodId:" << methodId_ << ", ";
        location << "bytecodeOffset:" << bytecodeOffset_ << ", ";
        location << "sourceFile:" << "\""<< sourceFile_ << "\""<< ", ";
        if (jsPandaFile_ != nullptr) {
            location << "jsPandaFile:" << "\"" << jsPandaFile_->GetJSPandaFileDesc() << "\"";
        }
        location << "line: " << line_ << ", ";
        location << "column: " << column_ << ", ";
        location << "needResetBreakpoint: " << needResetBreakpoint_;
        location << "]";
        return location.str();
    }

    ~JSPtLocation() = default;

    DEFAULT_COPY_SEMANTIC(JSPtLocation);
    DEFAULT_MOVE_SEMANTIC(JSPtLocation);

private:
    const JSPandaFile *jsPandaFile_ {nullptr};
    EntityId methodId_;
    uint32_t bytecodeOffset_ {0};
    Global<FunctionRef> condFuncRef_;
    int32_t line_;
    int32_t column_;
    std::string sourceFile_; // mainly used for breakpoint
    bool needResetBreakpoint_ {false};
    CString recordName_ {};
};
}  // namespace panda::ecmascript::tooling

#endif  // ECMASCRIPT_DEBUGGER_JS_PT_LOCATION_H