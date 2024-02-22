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

#include "ecmascript/snapshot/mem/snapshot_env.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"

namespace panda::ecmascript {

void SnapshotEnv::AddGlobalConstToMap()
{
    auto globalConst = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
    for (size_t index = 0; index < globalConst->GetConstantCount(); index++) {
        JSTaggedValue objectValue = globalConst->GetGlobalConstantObject(index);
        if (objectValue.IsHeapObject() && !objectValue.IsString()) {
            rootObjectMap_.emplace(objectValue.GetRawData(), index);
        }
    }
}

JSTaggedType SnapshotEnv::RelocateRootObjectAddr(uint32_t index)
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

void SnapshotEnv::Iterate(const RootVisitor &v)
{
    std::vector<std::pair<JSTaggedType, JSTaggedType>> updatedObjPair;
    for (auto &it : rootObjectMap_) {
        auto objectAddr = it.first;
        ObjectSlot slot(reinterpret_cast<uintptr_t>(&objectAddr));
        v(Root::ROOT_VM, slot);
        // Record updated obj addr after gc
        if (slot.GetTaggedType() != it.first) {
            updatedObjPair.emplace_back(std::make_pair(it.first, slot.GetTaggedType()));
        }
    }
    // Update hashmap, erase old objAddr, emplace updated objAddr
    while (!updatedObjPair.empty()) {
        auto pair = updatedObjPair.back();
        ASSERT(rootObjectMap_.find(pair.first) != rootObjectMap_.end());
        uint32_t index = rootObjectMap_.find(pair.first)->second;
        rootObjectMap_.erase(pair.first);
        rootObjectMap_.emplace(pair.second, index);
        updatedObjPair.pop_back();
    }
}
}  // namespace panda::ecmascript

