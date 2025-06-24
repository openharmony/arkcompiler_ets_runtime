/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/global_env.h"

namespace panda::ecmascript {

void SnapshotEnv::AddGlobalConstToMap()
{
    auto globalConst = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
    for (size_t index = 0; index < globalConst->GetConstantCount(); index++) {
        JSTaggedValue objectValue = globalConst->GetGlobalConstantObject(index);
        if (objectValue.IsHeapObject() && !objectValue.IsString()) {
            globalObjectMap_.emplace(objectValue.GetRawData(), index);
        }
    }
}

JSTaggedType SnapshotEnv::RelocateRootObjectAddr(uint32_t index)
{
    auto globalConst = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
    size_t globalConstCount = globalConst->GetConstantCount();
    if (index < globalConstCount) {
        JSTaggedValue obj = globalConst->GetGlobalConstantObject(index);
        if (obj.IsUndefined()) {
            LOG_ECMA(FATAL) << "Relocate GlobalConstants Root undefined, index: " << index;
        }
        return obj.GetRawData();
    }
    JSHandle<JSTaggedValue> value = vm_->GetGlobalEnv()->GetNoLazyEnvObjectByIndex(index - globalConstCount);
    if (value->IsUndefined()) {
        LOG_ECMA(FATAL) << "Relocate GlobalEnv Root undefined, index: " << index
                        << ", globalConstCount:" << globalConstCount;
    }
    return value->GetRawData();
}

void SnapshotEnv::ProcessSnapShotEnv(const WeakRootVisitor& visitor)
{
    std::unordered_map<JSTaggedType, uint32_t> globalObjectMap(globalObjectMap_.size());
    auto it = globalObjectMap_.begin();
    while (it != globalObjectMap_.end()) {
        JSTaggedValue globalVal = JSTaggedValue(it->first);
        if (globalVal.IsHeapObject()) {
            TaggedObject *obj = globalVal.GetTaggedObject();
            auto fwd = visitor(obj);
            if (fwd == nullptr) {
                ++it;
                continue;
            }
            globalObjectMap.emplace(JSTaggedValue(fwd).GetRawData(), it->second);
        }
        ++it;
    }
    std::swap(globalObjectMap_, globalObjectMap);
    globalObjectMap.clear();
}

// for cmc-gc
void SnapshotEnv::IteratorSnapShotEnv(WeakVisitor& visitor)
{
    // const key cannot modify.
    std::unordered_map<JSTaggedType, uint32_t> globalObjectMap(globalObjectMap_.size());
    auto it = globalObjectMap_.begin();
    while (it != globalObjectMap_.end()) {
        JSTaggedValue globalVal = JSTaggedValue(it->first);
        if (globalVal.IsHeapObject()) {
            bool isAlive = visitor.VisitRoot(Root::ROOT_VM,
                ObjectSlot(reinterpret_cast<uintptr_t>(&globalVal)));
            if (!isAlive) {
                ++it;
                continue;
            }
            globalObjectMap.emplace(globalVal.GetRawData(), it->second);
        }
        ++it;
    }
    std::swap(globalObjectMap_, globalObjectMap);
    globalObjectMap.clear();
}
}  // namespace panda::ecmascript

