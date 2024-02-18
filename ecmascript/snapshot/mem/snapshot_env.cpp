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
void SnapshotEnv::Initialize()
{
    InitGlobalConst();
    InitGlobalEnv();
}

void SnapshotEnv::InitializeStringClass()
{
    auto globalConst = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
    auto lineStringClass = globalConst->GetLineStringClass();
    rootObjectMap_.emplace(ToUintPtr(lineStringClass.GetTaggedObject()), globalConst->GetLineStringClassIndex());
    auto constStringClass = globalConst->GetConstantStringClass();
    rootObjectMap_.emplace(ToUintPtr(constStringClass.GetTaggedObject()), globalConst->GetConstStringClassIndex());
}

void SnapshotEnv::InitGlobalConst()
{
    auto globalConst = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
    for (size_t index = 0; index < globalConst->GetConstantCount(); index++) {
        JSTaggedValue objectValue = globalConst->GetGlobalConstantObject(index);
        if (objectValue.IsHeapObject()) {
            rootObjectMap_.emplace(ToUintPtr(objectValue.GetTaggedObject()), index);
        }
    }
}

void SnapshotEnv::InitGlobalEnv()
{
    auto globalEnv = vm_->GetGlobalEnv();
    auto globalConst = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
    size_t globalEnvIndexStart = globalConst->GetConstantCount();
    for (size_t index = 0; index < globalEnv->GetGlobalEnvFieldSize(); index++) {
        JSHandle<JSTaggedValue> objectValue = globalEnv->GetGlobalEnvObjectByIndex(index);
        if (objectValue->IsHeapObject() && !objectValue->IsInternalAccessor()) {
            rootObjectMap_.emplace(ToUintPtr(objectValue->GetTaggedObject()), index + globalEnvIndexStart);
        }
    }
}

void SnapshotEnv::Iterate(const RootVisitor &v)
{
    for (auto &it : rootObjectMap_) {
        v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&(it.first))));
    }
}
}  // namespace panda::ecmascript

