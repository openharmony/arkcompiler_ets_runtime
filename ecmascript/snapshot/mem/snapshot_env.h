/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_ENV_H
#define ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_ENV_H

#include <unordered_map>

#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/visitor.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {
class EcmaVM;

class SnapshotEnv final {
public:
    explicit SnapshotEnv(EcmaVM *vm) : vm_(vm) {}
    ~SnapshotEnv() = default;

    void Initialize();

    void ClearEnvMap()
    {
        rootObjectMap_.clear();
    }

    void Iterate(const RootVisitor &v);

    uint32_t FindEnvObjectIndex(uintptr_t objectAddr) const
    {
        if (rootObjectMap_.find(objectAddr) != rootObjectMap_.end()) {
            return rootObjectMap_.find(objectAddr)->second;
        }
        return MAX_UINT_32;
    }

    JSTaggedType RelocateRootObjectAddr(uint32_t index)
    {
        auto globalConst = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
        size_t globalConstCount = globalConst->GetConstantCount();
        if (index < globalConstCount) {
            JSTaggedValue obj = globalConst->GetGlobalConstantObject(index);
            return obj.GetRawData();
        }
        JSHandle<JSTaggedValue> value = vm_->GetGlobalEnv()->GetNoLazyEnvObjectByIndex(index - globalConstCount);
        return value->GetRawData();
    }

    static constexpr size_t MAX_UINT_32 = 0xFFFFFFFF;

private:
    NO_MOVE_SEMANTIC(SnapshotEnv);
    NO_COPY_SEMANTIC(SnapshotEnv);

    void InitGlobalConst();
    void InitGlobalEnv();

    EcmaVM *vm_;
    CUnorderedMap<uintptr_t, uint32_t> rootObjectMap_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_ENV_H
