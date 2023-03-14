/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/compilation_driver.h"

namespace panda::ecmascript::kungfu {
void CompilationDriver::UpdatePGO()
{
    std::unordered_set<EntityId> newMethodIds;
    auto dfs = [this, &newMethodIds] (const CString &recordName,
        const std::unordered_set<EntityId> &oldIds) -> std::unordered_set<EntityId> &{
            newMethodIds.clear();
            if (!jsPandaFile_->HasTSTypes(recordName)) {
                return newMethodIds;
            }
            uint32_t mainMethodOffset = jsPandaFile_->GetMainMethodIndex(recordName);
            SearchForCompilation(oldIds, newMethodIds, mainMethodOffset, false);
            return newMethodIds;
        };
    pfLoader_.Update(dfs);
}

void CompilationDriver::InitializeCompileQueue()
{
    auto &mainMethodIndexes = bytecodeInfo_.GetMainMethodIndexes();
    for (auto mainMethodIndex : mainMethodIndexes) {
        compileQueue_.push(mainMethodIndex);
    }
}

bool CompilationDriver::FilterMethod(const CString &recordName, const MethodLiteral *methodLiteral,
                                     const MethodPcInfo &methodPCInfo) const
{
    if (methodPCInfo.methodsSize > bytecodeInfo_.GetMaxMethodSize() ||
        !pfLoader_.Match(recordName, methodLiteral->GetMethodId())) {
        return true;
    }
    return false;
}
} // namespace panda::ecmascript::kungfu
