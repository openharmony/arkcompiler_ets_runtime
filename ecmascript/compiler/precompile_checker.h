/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_PRECOMPILE_CHECKER_H
#define ECMASCRIPT_COMPILER_PRECOMPILE_CHECKER_H

#include "ecmascript/compiler/gate_accessor.h"

namespace panda::ecmascript::kungfu {
class PassData;
class PreCompileChecker {
public:
    PreCompileChecker(PassData* data, Circuit* circuit, const std::string& methodName, bool enableLog)
        : data_(data), acc_(circuit), methodName_(methodName), enableLog_(enableLog) {}
    
    bool Run();

private:
    bool HasReturnCheck() const;
    bool HasDebuggerStmt() const;
    void PrintAbortInfo(const std::string& checkName) const;
    PassData* data_;
    GateAccessor acc_;
    const std::string methodName_;
    bool enableLog_;
};
}

#endif