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
        location << "jsPandaFile:" << "\"" << jsPandaFile_->GetJSPandaFileDesc() << "\"";
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
    std::string sourceFile_; // mainly used for breakpoint
};
}  // namespace panda::ecmascript::tooling

#endif  // ECMASCRIPT_DEBUGGER_JS_PT_LOCATION_H
