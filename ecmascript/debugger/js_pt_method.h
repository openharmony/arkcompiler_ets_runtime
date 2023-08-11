/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DEBUGGER_JS_PT_METHOD_H
#define ECMASCRIPT_DEBUGGER_JS_PT_METHOD_H

#include "ecmascript/jspandafile/js_pandafile.h"

#include "libpandafile/code_data_accessor-inl.h"
#include "libpandafile/method_data_accessor-inl.h"

namespace panda::ecmascript::tooling {
class PtMethod {
public:
    PtMethod(const JSPandaFile *jsPandaFile, EntityId methodId, bool isNative)
        : jsPandaFile_(jsPandaFile), methodId_(methodId), isNative_(isNative)
    {
    }
    ~PtMethod() = default;

    const JSPandaFile *GetJSPandaFile() const
    {
        return jsPandaFile_;
    }

    EntityId GetMethodId() const
    {
        return methodId_;
    }

    bool IsNativeMethod() const
    {
        return isNative_;
    }

    bool operator==(const PtMethod &method) const
    {
        return methodId_ == method.methodId_ &&
               jsPandaFile_ == method.jsPandaFile_;
    }

    uint32_t GetCodeSize() const
    {
        if (jsPandaFile_ == nullptr) {
            return 0;
        }
        panda_file::MethodDataAccessor mda(*(jsPandaFile_->GetPandaFile()), methodId_);
        auto codeId = mda.GetCodeId().value();
        if (!codeId.IsValid()) {
            return 0;
        }
        panda_file::CodeDataAccessor cda(*(jsPandaFile_->GetPandaFile()), codeId);
        return cda.GetCodeSize();
    }

private:
    const JSPandaFile *jsPandaFile_ {nullptr};
    panda_file::File::EntityId methodId_ {0};
    bool isNative_ {false};
};
}  // namespace panda::ecmascript::tooling
#endif // ECMASCRIPT_DEBUGGER_JS_PT_METHOD_H
